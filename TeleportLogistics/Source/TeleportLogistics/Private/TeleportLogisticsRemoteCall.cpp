#include "TeleportLogisticsRemoteCall.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsSubsystem.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Misc/ScopeLock.h"

void UTeleportLogisticsRemoteCall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTeleportLogisticsRemoteCall, Registered);
}
FString UTeleportLogisticsRemoteCall::ContextError(ATeleportLogisticsBuilding *Context) const
{
    if (!IsValid(Context))
        return TEXT("Waiting for the placed teleporter to reach the server…");
    const auto *Controller = GetOwnerPlayerController();
    const auto *Player = GetOwnerPlayerCharacter();
    if (!Player && Controller)
        Player = Cast<AFGCharacterPlayer>(Controller->GetPawn());
    if (!Player)
        return TEXT("Waiting for the player connection…");
    if (!Context->HasAuthority() || Context->GetWorld() != Player->GetWorld())
        return TEXT("Waiting for the server's teleporter instance…");
    if (Context->IsAboutToBeDismantled() || Context->GetIsDismantled())
        return TEXT("This teleporter is being dismantled. Close this window.");
    // Match reach to the visible building, not its placement pivot. Creative
    // camera/extended native use distance must not be mistaken for a bad RCO.
    const FBox Bounds = Context->GetComponentsBoundingBox(true);
    const FVector Position = Player->GetPawnViewLocation();
    const double DistanceSquared = Bounds.IsValid ? Bounds.ComputeSquaredDistanceToPoint(Position)
        : FVector::DistSquared(Position, Context->GetActorLocation());
    const double Reach = FMath::Max(1500.0, double(Player->GetUseDistance()) + 300.0);
    if (Position.ContainsNaN() || !FMath::IsFinite(DistanceSquared) || DistanceSquared > FMath::Square(Reach))
        return TEXT("Outside interaction range. Move closer to the teleporter building.");
    return FString();
}
bool UTeleportLogisticsRemoteCall::RateLimit(bool Mutation)
{
    double &Previous = Mutation ? LastWrite : LastRead;
    const double Now = FPlatformTime::Seconds();
    if (Now - Previous < (Mutation ? 0.15 : 0.25))
        return false;
    Previous = Now;
    return true;
}
bool UTeleportLogisticsRemoteCall::ServerSnapshot_Validate(ATeleportLogisticsBuilding *, FGuid, int32 Page, uint32)
{
    return Page >= 0 && Page <= 16384;
}
void UTeleportLogisticsRemoteCall::ServerSnapshot_Implementation(ATeleportLogisticsBuilding *Context, FGuid Route, int32 Page,
                                                     uint32 RequestId)
{
    if (!RateLimit(false))
        return;
    FTeleportLogisticsSnapshot S;
    S.Context = IsValid(Context) ? Context->TeleporterId : FGuid();
    S.RequestId = RequestId;
    S.ContextActor = Context;
    const FString Error = ContextError(Context);
    if (!Error.IsEmpty())
    {
        S.Message = Error;
        UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics snapshot rejected for %s: %s"), *GetNameSafe(Context), *Error);
        ClientSnapshot(S);
        return;
    }
    Context->EnsureNetworkRegistration();
    if (auto *Network = ATeleportLogisticsSubsystem::Get(Context->GetWorld()))
    {
        FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
        S = Network->Snapshot(Route, Page);
        S.ContextAvailable = true;
        S.ContextLabel = Context->Label;
        if (auto *E = Cast<ATeleportLogisticsEndpoint>(Context))
        {
            S.AssignedRoute = E->RouteId;
            S.EndpointEnabled = E->Enabled;
            S.Buffered = E->Buffered();
            S.Capacity = E->Capacity();
        }
        if (auto *Hub = Cast<ATeleportLogisticsHub>(Context))
        {
            S.HubPowered = Hub->ControlPowered();
            if (!S.HubPowered)
            {
                S.Endpoints.Reset();
                S.TotalEndpoints = 0;
                S.Message = TEXT("Connect power to unlock the directory and route controls. Transport "
                                 "continues without hub power.");
            }
        }
        else
        {
            S.Endpoints.Reset();
            S.TotalEndpoints = 0;
        }
        if (MessageContext == Context->TeleporterId)
        {
            S.MutationRevision = MutationRevision;
            S.MutationSucceeded = MutationSucceeded;
            if (!LastMessage.IsEmpty())
                S.Message = LastMessage;
        }
    }
    else
        S.Message = TEXT("teleporter network is still loading.");
    S.Context = IsValid(Context) ? Context->TeleporterId : FGuid();
    S.RequestId = RequestId;
    S.ContextActor = Context;
    ClientSnapshot(S);
}
bool UTeleportLogisticsRemoteCall::ServerConfigure_Validate(ATeleportLogisticsEndpoint *, FGuid, const FString &Route,
                                                const FString &Label)
{
    return Route.Len() <= 64 && Label.Len() <= 64;
}
void UTeleportLogisticsRemoteCall::ServerConfigure_Implementation(ATeleportLogisticsEndpoint *E, FGuid Channel, const FString &Route,
                                                      const FString &Label)
{
    if (!RateLimit(true) || !ValidateContext(E))
        return;
    if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
    {
        FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
        const FString Message = Network->Configure(E, Channel, Route, Label);
        Result(E, Message, Message == TeleportLogisticsResult::EndpointSaved);
    }
}
bool UTeleportLogisticsRemoteCall::ServerToggle_Validate(ATeleportLogisticsEndpoint *)
{
    return true;
}
void UTeleportLogisticsRemoteCall::ServerToggle_Implementation(ATeleportLogisticsEndpoint *E)
{
    if (!RateLimit(true) || !ValidateContext(E))
        return;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    E->FlushNetDormancy();
    E->Enabled = !E->Enabled;
    E->ForceNetUpdate();
    if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
        Network->InvalidateMap();
    Result(E, E->Enabled ? TEXT("Endpoint enabled.") : TEXT("Endpoint disabled. Contents are retained."),
           true);
}
bool UTeleportLogisticsRemoteCall::ServerFlushFluid_Validate(ATeleportLogisticsEndpoint *)
{
    return true;
}
void UTeleportLogisticsRemoteCall::ServerFlushFluid_Implementation(ATeleportLogisticsEndpoint *E)
{
    if (!RateLimit(true) || !ValidateContext(E))
        return;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    if (E->Medium != ETeleportLogisticsMedium::Fluid || E->Enabled || E->IoInFlight)
    {
        Result(E, TeleportLogisticsResult::FlushBlockedFluid, false);
        return;
    }
    E->Cargo.Reset();
    Result(E, TeleportLogisticsResult::FlushedFluid, true);
}
bool UTeleportLogisticsRemoteCall::ServerTakeBuffer_Validate(ATeleportLogisticsEndpoint *)
{
    return true;
}
void UTeleportLogisticsRemoteCall::ServerTakeBuffer_Implementation(ATeleportLogisticsEndpoint *E)
{
    if (!RateLimit(true) || !ValidateContext(E))
        return;
    auto *Player = GetOwnerPlayerCharacter();
    if (!Player)
        if (const auto *Controller = GetOwnerPlayerController())
            Player = Cast<AFGCharacterPlayer>(Controller->GetPawn());
    auto *Inventory = Player ? Player->GetInventory() : nullptr;
    if (!Inventory)
        return;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    if (E->Medium != ETeleportLogisticsMedium::Items)
    {
        Result(E, TeleportLogisticsResult::BufferNotItems, false);
        return;
    }
    if (E->IoInFlight)
    {
        Result(E, TeleportLogisticsResult::BufferBusy, false);
        return;
    }
    if (E->Cargo.IsEmpty())
    {
        Result(E, TeleportLogisticsResult::BufferEmpty, false);
        return;
    }
    // Move what fits and keep the remainder buffered, so a full inventory never
    // destroys items. The mutex makes this atomic against the transport tick.
    TArray<FInventoryStack> Remaining;
    for (const FInventoryStack &Stack : E->Cargo)
    {
        const int32 Added = Inventory->AddStack(Stack, true);
        if (Added < Stack.NumItems)
        {
            FInventoryStack Leftover = Stack;
            Leftover.NumItems = Stack.NumItems - FMath::Max(Added, 0);
            Remaining.Add(Leftover);
        }
    }
    const bool Complete = Remaining.IsEmpty();
    E->Cargo = MoveTemp(Remaining);
    Result(E, Complete ? TeleportLogisticsResult::BufferCollected
                       : TeleportLogisticsResult::BufferPartlyCollected, true);
}
bool UTeleportLogisticsRemoteCall::ServerRenameHub_Validate(ATeleportLogisticsHub *, const FString &Name)
{
    return Name.Len() <= 64;
}
void UTeleportLogisticsRemoteCall::ServerRenameHub_Implementation(ATeleportLogisticsHub *Hub, const FString &Name)
{
    if (!RateLimit(true) || !ValidateContext(Hub))
        return;
    if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
    {
        FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
        const FString Message = Network->RenameHub(Hub, Name);
        Result(Hub, Message, Message == TeleportLogisticsResult::HubRenamed);
    }
}
bool UTeleportLogisticsRemoteCall::ServerControl_Validate(ATeleportLogisticsHub *, FGuid, const FString &Action, const FString &Name)
{
    return Action.Len() <= 32 && Name.Len() <= 64 &&
           (Action == TEXT("pause") || Action == TEXT("rename") || Action == TEXT("reset-fluid") ||
            Action == TEXT("delete"));
}
void UTeleportLogisticsRemoteCall::ServerControl_Implementation(ATeleportLogisticsHub *Hub, FGuid Route, const FString &Action,
                                                    const FString &Name)
{
    if (!RateLimit(true) || !ValidateContext(Hub) || Action.Len() > 32 || Name.Len() > 64)
        return;
    if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
    {
        FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
        const FString Message = Network->Control(Hub, Route, Action, Name);
        const bool Success =
            Message == TeleportLogisticsResult::RoutePaused || Message == TeleportLogisticsResult::RouteResumed ||
            Message == TeleportLogisticsResult::RouteRenamed ||
            Message ==
                TeleportLogisticsResult::FluidReset ||
            Message == TeleportLogisticsResult::RouteRemoved;
        Result(Hub, Message, Success);
    }
}
void UTeleportLogisticsRemoteCall::ClientSnapshot_Implementation(const FTeleportLogisticsSnapshot &Snapshot)
{
    OnSnapshot.Broadcast(Snapshot);
}

void UTeleportLogisticsRemoteCall::Result(ATeleportLogisticsBuilding *Context, const FString &Message, bool Succeeded)
{
    MessageContext = Context->TeleporterId;
    LastMessage = Message;
    MutationSucceeded = Succeeded;
    ++MutationRevision;
}

bool UTeleportLogisticsRemoteCall::ServerPasteRoute_Validate(ATeleportLogisticsEndpoint *, FGuid Route)
{
    return Route.IsValid();
}
void UTeleportLogisticsRemoteCall::ServerPasteRoute_Implementation(ATeleportLogisticsEndpoint *Endpoint, FGuid Route)
{
    if (!RateLimit(true) || !ValidateContext(Endpoint))
        return;
    auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld());
    if (!Network)
        return;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    const auto *Source = Network->Routes.FindByPredicate([Route](const FTeleportLogisticsRoute &R) { return R.Id == Route; });
    if (!Source || Source->Medium != Endpoint->Medium)
    {
        Result(Endpoint, TEXT("Copied route is unavailable or uses a different transport type."), false);
        return;
    }
    const FString Error = Network->Configure(Endpoint, Source->Channel, Source->Name, Endpoint->Label);
    Result(Endpoint, Error == TeleportLogisticsResult::EndpointSaved ? TEXT("Route settings pasted.") : Error,
           Error == TeleportLogisticsResult::EndpointSaved);
}
