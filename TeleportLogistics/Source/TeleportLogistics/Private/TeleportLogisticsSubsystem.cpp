#include "TeleportLogisticsSubsystem.h"
#include "TeleportLogisticsBuilding.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Misc/ScopeLock.h"

FCriticalSection ATeleportLogisticsSubsystem::Mutex;
ATeleportLogisticsSubsystem::ATeleportLogisticsSubsystem()
{
    ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}
ATeleportLogisticsSubsystem *ATeleportLogisticsSubsystem::Get(UWorld *World)
{
    auto *Manager = World ? World->GetSubsystem<USubsystemActorManager>() : nullptr;
    return Manager ? Manager->GetSubsystemActor<ATeleportLogisticsSubsystem>() : nullptr;
}
bool ATeleportLogisticsSubsystem::ValidName(const FString &Name)
{
    if (Name.IsEmpty() || Name.Len() > 64 || Name != Name.TrimStartAndEnd())
        return false;
    for (TCHAR C : Name)
        if (C < 32 || C == 127)
            return false;
    return true;
}
void ATeleportLogisticsSubsystem::Register(ATeleportLogisticsBuilding *Building)
{
    // Designer templates never join the live network; see BeginPlay.
    if (Building->IsBuildableInsideBlueprintDesigner())
        return;
    FScopeLock Lock(&Mutex);
    if (const auto *Existing = Buildings.Find(Building->TeleporterId))
    {
        if (Existing->IsValid() && Existing->Get() != Building)
        {
            const FGuid Displaced = Building->TeleporterId;
            Building->TeleporterId = FGuid::NewGuid();
            // A hub displaced by anything other than another hub takes its saved
            // channel and routes with it. Left behind, they key on an id no hub
            // owns, and no hub can rename, pause or delete them again.
            if (Cast<ATeleportLogisticsHub>(Building) && !Cast<ATeleportLogisticsHub>(Existing->Get()))
            {
                for (auto &Route : Routes)
                    if (Route.Channel == Displaced)
                        Route.Channel = Building->TeleporterId;
                for (auto &Channel : Channels)
                    if (Channel.Id == Displaced)
                        Channel.Id = Building->TeleporterId;
            }
        }
    }
    Buildings.Add(Building->TeleporterId, Building);
    if (auto *Hub = Cast<ATeleportLogisticsHub>(Building))
    {
        if (!Channels.ContainsByPredicate([&](const auto &C) { return C.Id == Hub->TeleporterId; }))
        {
            FTeleportLogisticsChannel Channel;
            Channel.Id = Hub->TeleporterId;
            Channel.Name = FString::Printf(TEXT("Hub %s"), *Hub->TeleporterId.ToString().Left(8));
            Channels.Add(Channel);
        }
    }
    Dirty = true;
    MapDirty = true;
}
void ATeleportLogisticsSubsystem::Unregister(ATeleportLogisticsBuilding *Building, bool Dismantled)
{
    // Nor may one unregister the real building whose identity it was copied from.
    if (Building->IsBuildableInsideBlueprintDesigner())
        return;
    FScopeLock Lock(&Mutex);
    Buildings.Remove(Building->TeleporterId);
    // Keep saved channel records during world teardown. Only dismantling removes them.
    if (Cast<ATeleportLogisticsHub>(Building) && (Dismantled || Building->GetIsDismantled()))
    {
        Routes.RemoveAll([&](const auto &R) { return R.Channel == Building->TeleporterId; });
        Channels.RemoveAll([&](const auto &C) { return C.Id == Building->TeleporterId; });
    }
    // A route outlives its last endpoint otherwise, and only a powered hub can delete one,
    // so a hubless network dismantling and rebuilding endpoints walked towards the 256
    // route ceiling with nothing to show for it. Same rule as the hub branch above: world
    // teardown keeps the record, dismantling reclaims it.
    if (const auto *E = Cast<ATeleportLogisticsEndpoint>(Building);
        E && E->RouteId.IsValid() && (Dismantled || Building->GetIsDismantled()))
    {
        // Buildings.Remove has already run, so this cannot see the departing endpoint.
        bool Referenced = false;
        for (const auto &Pair : Buildings)
            if (const auto *Other = Cast<ATeleportLogisticsEndpoint>(Pair.Value.Get());
                Other && Other->DirectoryActive() && Other->RouteId == E->RouteId)
            {
                Referenced = true;
                break;
            }
        if (!Referenced)
            Routes.RemoveAll([&](const auto &R) { return R.Id == E->RouteId; });
    }
    Dirty = true;
    MapDirty = true;
}
void ATeleportLogisticsSubsystem::Reindex()
{
    TSet<FGuid> Live;
    Live.Reserve(Routes.Num());
    for (const auto &Route : Routes)
        Live.Add(Route.Id);
    for (auto It = Index.CreateIterator(); It; ++It)
    {
        if (!Live.Contains(It.Key()))
        {
            It.RemoveCurrent();
            continue;
        }
        It.Value().Inputs.Reset();
        It.Value().Outputs.Reset();
    }
    for (auto &Pair : Buildings)
        if (auto *E = Cast<ATeleportLogisticsEndpoint>(Pair.Value.Get()); E && E->DirectoryActive())
        {
            // A route deleted under an endpoint leaves no runtime entry behind.
            if (!E->RouteId.IsValid() || !Live.Contains(E->RouteId))
                continue;
            auto &Runtime = Index.FindOrAdd(E->RouteId);
            (E->Input ? Runtime.Inputs : Runtime.Outputs).Add(E);
        }
    for (auto &Pair : Index)
    {
        // Stable registration order is not guaranteed across save loads.
        auto Sort = [](const auto &A, const auto &B) {
            return A->TeleporterId < B->TeleporterId;
        };
        Pair.Value.Inputs.Sort(Sort);
        Pair.Value.Outputs.Sort(Sort);
        Pair.Value.InputBudgets.resize(Pair.Value.Inputs.Num());
        Pair.Value.OutputBudgets.resize(Pair.Value.Outputs.Num());
    }
    Dirty = false;
}
void ATeleportLogisticsSubsystem::RefreshMap()
{
    if (!MapDirty)
        return;
    TArray<TWeakObjectPtr<ATeleportLogisticsBuilding>> Pending;
    {
        FScopeLock Lock(&Mutex);
        MapDirty = false;
        TMap<FGuid, int32> RouteIndices, ChannelIndices;
        RouteIndices.Reserve(Routes.Num());
        for (int32 I = 0; I < Routes.Num(); ++I)
            RouteIndices.Add(Routes[I].Id, I);
        ChannelIndices.Reserve(Channels.Num());
        for (int32 I = 0; I < Channels.Num(); ++I)
            ChannelIndices.Add(Channels[I].Id, I);
        for (const auto &Pair : Buildings)
        {
            auto *Building = Pair.Value.Get();
            if (!Building)
                continue;
            FString Path = TEXT("Teleporter Hub");
            if (auto *Endpoint = Cast<ATeleportLogisticsEndpoint>(Building))
            {
                Path = TEXT("Unassigned");
                Endpoint->RoutePath.Reset();
                if (const int32 *RouteIndex = RouteIndices.Find(Endpoint->RouteId))
                {
                    const auto &Route = Routes[*RouteIndex];
                    const int32 *ChannelIndex = ChannelIndices.Find(Route.Channel);
                    Path = (ChannelIndex ? Channels[*ChannelIndex].Name : TEXT("Default")) + TEXT(" / ") +
                           Route.Name;
                    if (Route.Paused)
                        Path += TEXT(" [PAUSED]");
                    // The look-at panel shows this without the medium and direction suffixes,
                    // which the building's own name already carries.
                    Endpoint->RoutePath = Path;
                }
                Path += Endpoint->Medium == ETeleportLogisticsMedium::Items ? TEXT(" / Items") : TEXT(" / Fluid");
                Path += Endpoint->Input ? TEXT(" Input") : TEXT(" Output");
            }
            else if (const int32 *ChannelIndex = ChannelIndices.Find(Building->TeleporterId))
                Path += TEXT(" / ") + Channels[*ChannelIndex].Name;
            if (const auto *Endpoint = Cast<ATeleportLogisticsEndpoint>(Building); Endpoint && !Endpoint->Enabled)
                Path += TEXT(" [DISABLED]");
            Building->MapLabel = Cast<ATeleportLogisticsEndpoint>(Building) ? Path + TEXT(" / ") + Building->Label : Path;
            Pending.Add(Building);
        }
    }
    // Manager callbacks must never run inside the transport lock. Retry if world setup is incomplete.
    for (const auto &Weak : Pending)
        if (auto *Building = Weak.Get())
            if (!Building->UpdateRepresentation())
                MapDirty = true;
}
void ATeleportLogisticsSubsystem::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!HasAuthority())
        return;
    RefreshMap();
    FScopeLock Lock(&Mutex);
    if (Dirty)
        Reindex();
    for (auto &Route : Routes)
    {
        auto *Runtime = Index.Find(Route.Id);
        if (Route.Paused || !Runtime)
        {
            Route.UnitsPerMinute = 0;
            continue;
        }
        // A stalled port is offered an empty budget so the scheduler skips it, and is
        // excluded from the write-back below so that empty budget never lands on it.
        TArray<bool, TInlineAllocator<16>> InputActive, OutputActive;
        for (int32 I = 0; I < Runtime->Inputs.Num(); ++I)
        {
            auto *E = Runtime->Inputs[I].Get();
            if (E && E->Enabled)
                E->TransportBudget.accrue(Dt, E->Rate());
            const bool Active = E && E->Enabled && !E->Cargo.IsEmpty();
            InputActive.Add(Active);
            Runtime->InputBudgets[I] = Active ? E->TransportBudget : teleport_logistics::Budget{};
        }
        for (int32 I = 0; I < Runtime->Outputs.Num(); ++I)
        {
            auto *E = Runtime->Outputs[I].Get();
            if (E && E->Enabled)
                E->TransportBudget.accrue(Dt, E->Rate());
            const bool Active = E && E->Enabled && E->Buffered() < E->Capacity();
            OutputActive.Add(Active);
            Runtime->OutputBudgets[I] = Active ? E->TransportBudget : teleport_logistics::Budget{};
        }
        const int64 Transferred = teleport_logistics::distribute(
            Runtime->InputBudgets, Runtime->OutputBudgets, Runtime->Cursor,
            Route.Medium == ETeleportLogisticsMedium::Items ? 1 : 1000,
            [&](std::size_t I, std::size_t O, int64 Requested) -> int64 {
                auto *Source = Runtime->Inputs[I].Get();
                auto *Dest = Runtime->Outputs[O].Get();
                if (!Source || !Dest || Source == Dest || Source->IsAboutToBeDismantled() ||
                    Dest->IsAboutToBeDismantled() || Source->Cargo.IsEmpty())
                    return 0;
                auto &Head = Source->Cargo[0];
                const auto Type = Head.Item.GetItemClass();
                if (Route.Medium == ETeleportLogisticsMedium::Fluid)
                {
                    if ((Route.Fluid && Route.Fluid != Type) ||
                        (!Dest->Cargo.IsEmpty() && Dest->Cargo[0].Item.GetItemClass() != Type))
                        return 0;
                }
                const int32 Amount = FMath::Min3(static_cast<int32>(Requested), Head.NumItems,
                                                 Dest->Capacity() - Dest->Buffered());
                if (Amount <= 0)
                    return 0;
                FInventoryStack Moved = Head;
                Moved.NumItems = Amount;
                if (Route.Medium == ETeleportLogisticsMedium::Fluid)
                {
                    Route.Fluid = Type;
                    if (Dest->Cargo.IsEmpty())
                        Dest->Cargo.Add(Moved);
                    else
                        Dest->Cargo[0].NumItems += Amount;
                }
                else
                    Dest->Cargo.Add(Moved);
                Head.NumItems -= Amount;
                if (Head.NumItems == 0)
                    Source->Cargo.RemoveAt(0);
                return Amount;
            });
        const double InstantRate = Dt > 0 ? Transferred * 60.0 / Dt : 0;
        Route.UnitsPerMinute = FMath::Lerp(Route.UnitsPerMinute, InstantRate, 1.0 - FMath::Exp(-Dt / 2.0));
        for (int32 I = 0; I < Runtime->Inputs.Num(); ++I)
            if (auto *E = Runtime->Inputs[I].Get(); E && InputActive[I])
                E->TransportBudget = Runtime->InputBudgets[I];
        for (int32 I = 0; I < Runtime->Outputs.Num(); ++I)
            if (auto *E = Runtime->Outputs[I].Get(); E && OutputActive[I])
                E->TransportBudget = Runtime->OutputBudgets[I];
    }
}
bool ATeleportLogisticsSubsystem::Managed(FGuid Channel) const
{
    const auto *Ref = Buildings.Find(Channel);
    const auto *Hub = Ref ? Cast<ATeleportLogisticsHub>(Ref->Get()) : nullptr;
    return Hub && Hub->ControlPowered();
}
TSubclassOf<UFGItemDescriptor> ATeleportLogisticsSubsystem::LockedFluid(FGuid Route) const
{
    if (const auto *R = Routes.FindByPredicate([&](const auto &Candidate) { return Candidate.Id == Route; }))
        return R->Fluid;
    return nullptr;
}
bool ATeleportLogisticsSubsystem::ChannelEmpty(FGuid Channel) const
{
    TSet<FGuid> ChannelRoutes;
    for (const auto &Route : Routes)
        if (Route.Channel == Channel)
            ChannelRoutes.Add(Route.Id);
    if (ChannelRoutes.IsEmpty())
        return true;
    for (const auto &Pair : Buildings)
        if (const auto *E = Cast<ATeleportLogisticsEndpoint>(Pair.Value.Get()); E && E->DirectoryActive())
            if (ChannelRoutes.Contains(E->RouteId))
                return false;
    return true;
}
FString ATeleportLogisticsSubsystem::Configure(ATeleportLogisticsEndpoint *E, FGuid Channel, const FString &Name,
                                   const FString &Label)
{
    if (!E || !ValidName(Name) || !ValidName(Label))
        return TEXT("Use names of 1–64 characters without leading/trailing spaces.");
    if (E->IoInFlight)
        return TEXT("Endpoint is transferring. Disable it, then change its route.");
    if (Channel.IsValid() && !Channels.ContainsByPredicate([&](const auto &C) { return C.Id == Channel; }))
        return TEXT("Channel no longer exists.");
    FTeleportLogisticsRoute *Route = Routes.FindByPredicate(
        [&](const auto &R) { return R.Channel == Channel && R.Name.Equals(Name, ESearchCase::IgnoreCase); });
    if (Route && Route->Medium != E->Medium)
        return TEXT("That name belongs to a different transport type. Choose another route.");
    if ((!Route || Route->Id != E->RouteId) && !E->Cargo.IsEmpty())
        return TEXT("Drain the endpoint before changing its route.");
    if (!Route)
    {
        if (Routes.Num() >= 256)
            return TEXT("Up to 256 routes are supported. Remove unused routes at a hub.");
        FTeleportLogisticsRoute New;
        New.Id = FGuid::NewGuid();
        New.Channel = Channel;
        New.Name = Name;
        New.Medium = E->Medium;
        Route = &Routes.Add_GetRef(New);
    }
    E->FlushNetDormancy();
    E->RouteId = Route->Id;
    E->Label = Label;
    E->ForceNetUpdate();
    Dirty = true;
    MapDirty = true;
    return TeleportLogisticsResult::EndpointSaved;
}
FString ATeleportLogisticsSubsystem::RenameHub(ATeleportLogisticsHub *Hub, const FString &Name)
{
    if (!Hub || !Hub->ControlPowered())
        return TEXT("Power the hub to use its management controls.");
    if (!ValidName(Name) || Name.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
        return TEXT("Choose a unique hub name (1–64 characters).");
    if (Channels.ContainsByPredicate([&](const auto &C) {
            return C.Id != Hub->TeleporterId && C.Name.Equals(Name, ESearchCase::IgnoreCase);
        }))
        return TEXT("That hub name is already used.");
    auto *Channel = Channels.FindByPredicate([&](const auto &C) { return C.Id == Hub->TeleporterId; });
    if (!Channel)
        return TEXT("Hub is still registering. Try again.");
    Channel->Name = Name;
    MapDirty = true;
    Hub->FlushNetDormancy();
    Hub->Label = Name;
    Hub->ForceNetUpdate();
    return TeleportLogisticsResult::HubRenamed;
}
FString ATeleportLogisticsSubsystem::Control(ATeleportLogisticsHub *Hub, FGuid Id, const FString &Action, const FString &Name)
{
    if (!Hub || !Hub->ControlPowered())
        return TEXT("Power the hub to manage routes.");
    auto *R = Routes.FindByPredicate([&](const auto &Route) {
        return Route.Id == Id && (Route.Channel == Hub->TeleporterId || !Route.Channel.IsValid());
    });
    if (!R)
        return TEXT("Select a route belonging to this hub or Default.");
    if (Action == TEXT("pause"))
    {
        R->Paused = !R->Paused;
        MapDirty = true;
        return R->Paused ? TEXT("Route paused; output buffers can drain.") : TEXT("Route resumed.");
    }
    if (Action == TEXT("rename"))
    {
        if (!ValidName(Name))
            return TEXT("Enter a valid route name.");
        if (Routes.ContainsByPredicate([&](const auto &Other) {
                return Other.Id != R->Id && Other.Channel == R->Channel &&
                       Other.Name.Equals(Name, ESearchCase::IgnoreCase);
            }))
            return TEXT("Route name is already used.");
        R->Name = Name;
        MapDirty = true;
        return TeleportLogisticsResult::RouteRenamed;
    }
    if (Action == TEXT("reset-fluid"))
    {
        if (R->Medium != ETeleportLogisticsMedium::Fluid)
            return TEXT("Only fluid routes can reset their fluid type.");
        for (const auto &P : Buildings)
            if (auto *E = Cast<ATeleportLogisticsEndpoint>(P.Value.Get()))
                if (E->RouteId == Id && (E->Enabled || !E->Cargo.IsEmpty() || E->IoInFlight))
                    return TEXT("Disable and drain every endpoint before resetting the fluid type.");
        R->Fluid = nullptr;
        return TeleportLogisticsResult::FluidReset;
    }
    if (Action == TEXT("delete"))
    {
        for (const auto &P : Buildings)
            if (auto *E = Cast<ATeleportLogisticsEndpoint>(P.Value.Get()))
                if (E->RouteId == Id)
                    return TEXT("Move or dismantle attached endpoints before deleting a route.");
        Routes.RemoveAll([&](const auto &Other) { return Other.Id == Id; });
        Dirty = true;
        MapDirty = true;
        return TeleportLogisticsResult::RouteRemoved;
    }
    return TEXT("Unknown action.");
}
FTeleportLogisticsSnapshot ATeleportLogisticsSubsystem::Snapshot(FGuid Route, int32 Page) const
{
    FTeleportLogisticsSnapshot S;
    S.Channels = Channels;
    S.Channels.RemoveAll([this](const auto& C){const auto* B=Buildings.Find(C.Id);return !B || !B->IsValid() || !B->Get()->DirectoryActive();});
    S.Routes = Routes;
    TMap<FGuid, int32> RouteIndices;
    for (int32 I = 0; I < S.Routes.Num(); ++I)
    {
        auto &R = S.Routes[I];
        R.Inputs = R.Outputs = 0;
        R.CanResetFluid = R.Medium == ETeleportLogisticsMedium::Fluid;
        RouteIndices.Add(R.Id, I);
    }
    for (const auto &Pair : Buildings)
        if (const auto *E = Cast<ATeleportLogisticsEndpoint>(Pair.Value.Get()); E && E->DirectoryActive())
            if (const auto *I = RouteIndices.Find(E->RouteId))
            {
                auto &R = S.Routes[*I];
                if (E->Input)
                    ++R.Inputs;
                else
                    ++R.Outputs;
                R.CanResetFluid &= !E->Enabled && E->Cargo.IsEmpty() && !E->IoInFlight;
            }
    FTeleportLogisticsChannel Default;
    Default.Name = TEXT("Default");
    S.Channels.Insert(Default, 0);
    // Only a chosen route's endpoints are ever shown, and only a hub window shows them.
    // Building the whole directory regardless meant every endpoint panel paid for a pass
    // over every endpoint on the map, a linear channel search and a route-path string
    // each, and a sort that allocated two strings per comparison, all of it discarded by
    // the caller.
    TArray<FTeleportLogisticsEndpointView> All;
    if (Route.IsValid())
        for (const auto &Pair : Buildings)
            if (auto *E = Cast<ATeleportLogisticsEndpoint>(Pair.Value.Get());
                E && E->DirectoryActive() && E->RouteId == Route)
            {
                FTeleportLogisticsEndpointView V;
                V.Id = E->TeleporterId;
                V.Route = E->RouteId;
                if (const int32 *RouteIndex = RouteIndices.Find(E->RouteId))
                {
                    const auto &EndpointRoute = S.Routes[*RouteIndex];
                    const auto *Channel = Channels.FindByPredicate(
                        [&](const auto &Candidate) { return Candidate.Id == EndpointRoute.Channel; });
                    V.RoutePath = (Channel ? Channel->Name : TEXT("Default")) + TEXT(" / ") + EndpointRoute.Name;
                }
                else
                    V.RoutePath = TEXT("Unassigned");
                V.Name = E->Label;
                V.Location = E->GetActorLocation();
                V.Input = E->Input;
                V.Medium = E->Medium;
                V.Enabled = E->Enabled;
                V.Buffered = E->Buffered();
                All.Add(V);
            }
    All.Sort([](const auto &A, const auto &B) { return A.Id < B.Id; });
    S.TotalEndpoints = All.Num();
    S.Page = FMath::Clamp(Page, 0, FMath::Max(0, (All.Num() - 1) / 64));
    for (int32 I = S.Page * 64; I < FMath::Min(All.Num(), (S.Page + 1) * 64); ++I)
        S.Endpoints.Add(All[I]);
    // Default is synthetic, so the count reports the hubs that exist.
    S.Message = FString::Printf(TEXT("Network online · %d channels · %d routes"), S.Channels.Num() - 1,
                                S.Routes.Num());
    return S;
}
