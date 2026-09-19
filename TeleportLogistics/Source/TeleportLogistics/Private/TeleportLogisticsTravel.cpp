#include "TeleportLogisticsTravel.h"
#include "TeleportLogisticsAsset.h"
#include "TeleportLogisticsLog.h"
#include "FGPowerConnectionComponent.h"
#include "FGIconDatabaseSubsystem.h"
#include "FGPowerInfoComponent.h"
#include "FGCharacterPlayer.h"
#include "FGHealthComponent.h"
#include "FGPlayerController.h"
#include "FGUseableInterface.h"
#include "Engine/Texture2D.h"
#include "Hologram/FGFactoryHologram.h"
#include "Engine/StaticMesh.h"
#include "FGAttachmentPointComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

TMap<FGuid, TWeakObjectPtr<ATeleportLogisticsTravelHub>> &ATeleportLogisticsTravelHub::Live()
{
    // Function-local so there is no static initialisation order to reason about.
    static TMap<FGuid, TWeakObjectPtr<ATeleportLogisticsTravelHub>> Registry;
    return Registry;
}
ATeleportLogisticsTravelHub::ATeleportLogisticsTravelHub()
{
    bReplicates = true;
    mIsUseable = true;
    mInteractWidgetSoftClass = UTeleportLogisticsTravelWidget::StaticClass();
    mHologramClass = AFGFactoryHologram::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "PersonnelHub", "Personnel Teleporter");
    mDescription =
        NSLOCTEXT("TeleportLogistics", "PersonnelHubDesc",
                  "Travel between powered Personnel Teleporters. Open the destination directory with Use.");
    mAllowColoring = true;
    // Patterns render through MI_TeleporterFactory, a child of the game's own factory base,
    // so the Customizer applies to these the way it does to stock machines.
    mAllowPatterning = true;
    mShouldApplyCustomizationData = true;
    mPowerConsumption = 50;
    mFactoryTickFunction.bCanEverTick = true;
    // Matches ATeleportLogisticsBuilding::AddSignMount; this hub derives from the
    // native portal base instead, so it carries its own copy.
    {
        static ConstructorHelpers::FClassFinder<UFGAttachmentPointType> SignType(
            TEXT("/Game/FactoryGame/Buildable/-Shared/AttachmentPointTypes/Sign/APT_SignCenter"));
        auto *Point = CreateDefaultSubobject<UFGAttachmentPointComponent>(TEXT("SignMount"));
        Point->SetRelativeLocation(FVector(-59, 0, 416));
        Point->SetRelativeRotation(FRotator(0, 180, 0));
        // Buildable only; see ATeleportLogisticsBuilding::AddSignMount.
        Point->mUsage = EAttachmentPointUsage::EAPU_BuildableOnly;
        if (SignType.Succeeded())
            Point->mType = SignType.Class;
        else
            UE_LOG(LogTeleportLogistics, Error,
                   TEXT("TeleportLogistics: APT_SignCenter unavailable; signs will not snap"));
        SignMount = Point;
    }
    if (!RootComponent)
        SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TravelRoot")));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainMesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetStaticMesh(TeleportLogisticsAsset<UStaticMesh>(
        TEXT("/TeleportLogistics/Models/SM_TeleporterTravelHub.SM_TeleporterTravelHub")));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetMobility(EComponentMobility::Static);
    Power = CreateDefaultSubobject<UFGPowerConnectionComponent>(TEXT("PowerConnection"));
    Power->SetupAttachment(RootComponent);
    Power->SetRelativeLocation(FVector(-180, 220, 310));
    // All three resolved here rather than on demand. The map and compass ask for the
    // representation art mid-journey, exactly while the destination is streaming in,
    // and a blocking load at that moment flushes every package in flight: the log
    // showed one such flush hold a single frame for 2.3 seconds.
    MapIcon = TeleportLogisticsAsset<UTexture2D>(TEXT("/TeleportLogistics/Icons/M_TeleporterHub.M_TeleporterHub"));
    MapMaterial = TeleportLogisticsAsset<UMaterialInterface>(
        TEXT("/TeleportLogistics/Icons/MI_TeleporterMapHub.MI_TeleporterMapHub"));
    UnpoweredSignal = TeleportLogisticsAsset<UMaterialInterface>(
        TEXT("/TeleportLogistics/Models/M_TeleporterSignalOff.M_TeleporterSignalOff"));
    // Half the player's own streaming radius around each powered teleporter. Arriving
    // otherwise lands in cells that are not resident, the world partition declares
    // streaming critical, and the engine blocks the game thread until the whole queue
    // has drained: measured at 2.3 seconds in a single frame on a large save.
    StreamingSource = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StreamingSource"));
    StreamingSource->DisableStreamingSource();
    {
        FStreamingSourceShape Shape;
        Shape.bUseGridLoadingRange = true;
        Shape.LoadingRangeScale = .5f;
        StreamingSource->Shapes.Add(Shape);
    }
    mPortalTravelTimeOverDistance = CreateDefaultSubobject<UCurveFloat>(TEXT("TravelTime"));
    mPortalTravelTimeOverDistance->FloatCurve.AddKey(0, 1.5f);
    mPortalTravelTimeOverDistance->FloatCurve.AddKey(10, 3.5f);
    mMaxPortalTravelTime = 25;
}
void ATeleportLogisticsTravelHub::BeginPlay()
{
    Super::BeginPlay();
    if (SignMount && SignMount->GetAttachParent() == nullptr)
        SignMount->SetupAttachment(RootComponent);
    if (mAttachmentPoints.IsEmpty())
        CreateAttachmentPointsFromComponents(mAttachmentPoints, this);
    Power->SetPowerInfo(GetPowerInfo());
    if (HasAuthority())
    {
        // Blueprint copies must not share a destination identity with a live hub.
        if (HubId.IsValid())
            if (const auto *Existing = Live().Find(HubId))
                if (Existing->IsValid() && Existing->Get() != this)
                    HubId.Invalidate();
        if (!HubId.IsValid())
            HubId = FGuid::NewGuid();
        if (HubName.IsEmpty())
            HubName = TEXT("Personnel Teleporter ") + HubId.ToString().Left(8);
        SetPortalName(FText::FromString(HubName));
        // A designer copy keeps its name for its own panel but is not a destination.
        if (!IsBuildableInsideBlueprintDesigner())
            Live().Add(HubId, this);
        FlushNetDormancy();
        ForceNetUpdate();
    }
    if (GetNetMode() != NM_DedicatedServer)
        GetWorldTimerManager().SetTimer(VisualTimer, this, &ATeleportLogisticsTravelHub::UpdateVisual, .2f, true, .01f);
}
void ATeleportLogisticsTravelHub::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(VisualTimer);
    if (const auto *Registered = Live().Find(HubId); Registered && Registered->Get() == this)
        Live().Remove(HubId);
    Super::EndPlay(Reason);
}
void ATeleportLogisticsTravelHub::Factory_Tick(float)
{
    // No native pair linking, shared inventory or cross-grid power. Transport
    // uses the player's native portal state machine, not the paired factory loop.
    if (HasAuthority())
        if (auto *Info = GetPowerInfo())
        {
            // Only on change. These are constants, and writing them every frame
            // for every hub means touching the power circuit every frame for a
            // value that never moves.
            if (!FMath::IsNearlyEqual(Info->GetMaximumTargetConsumption(), 50.f))
                Info->SetMaximumTargetConsumption(50);
            if (!FMath::IsNearlyEqual(Info->GetTargetConsumption(), 50.f))
                Info->SetTargetConsumption(50);
        }
}
bool ATeleportLogisticsTravelHub::Powered() const
{
    return GetPowerInfo() && GetPowerInfo()->IsConnected() && GetPowerInfo()->HasPower() &&
           !GetPowerInfo()->IsFuseTriggered();
}
bool ATeleportLogisticsTravelHub::Available() const
{
    return Powered() && DirectoryActive() && GetWorld() && GetWorld()->GetTimeSeconds() >= BusyUntil;
}
bool ATeleportLogisticsTravelHub::CanDismantle_Implementation() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() >= BusyUntil && Super::CanDismantle_Implementation();
}
void ATeleportLogisticsTravelHub::UpdateUseState_Implementation(AFGCharacterPlayer *, const FVector &,
                                                    UPrimitiveComponent *, FUseState &State)
{
    State.SetUseState(UFGUseState_Valid::StaticClass());
}
void ATeleportLogisticsTravelHub::GetPortalSurfaceTransform_Implementation(FTransform &Out) const
{
    Out = FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(0, 0, 220)));
}
FVector ATeleportLogisticsTravelHub::StagingLocation(float HalfHeight) const
{
    return GetActorTransform().TransformPosition(FVector(140, 0, 32 + HalfHeight + 4));
}
void ATeleportLogisticsTravelHub::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATeleportLogisticsTravelHub, HubId);
    DOREPLIFETIME(ATeleportLogisticsTravelHub, HubName);
    DOREPLIFETIME(ATeleportLogisticsTravelHub, HubIcon);
}
UTexture2D *ATeleportLogisticsTravelHub::GetActorRepresentationTexture()
{
    return MapIcon;
}
UMaterialInterface *ATeleportLogisticsTravelHub::GetActorRepresentationCompassMaterial()
{
    return MapMaterial;
}
void ATeleportLogisticsTravelHub::UpdateVisual()
{
    if (!IsInGameThread() || GetNetMode() == NM_DedicatedServer || !Mesh)
        return;
    const bool On = Powered();
    const int32 Signal = Mesh->GetMaterialIndex(TEXT("signal"));
    if (Signal != INDEX_NONE)
    {
        // This runs on a .2s timer, so cache rather than loading per call, and take
        // the powered material from the mesh asset's own slot. Naming it instead
        // means it never compares equal to whatever the import bound, so the timer
        // reassigns the material five times a second and churns the render state.
        if (!PoweredSignal)
            if (const UStaticMesh *Asset = Mesh->GetStaticMesh())
                PoweredSignal = Asset->GetMaterial(Signal);
        if (auto *Material = On ? PoweredSignal.Get() : UnpoweredSignal.Get())
            if (Mesh->GetMaterial(Signal) != Material)
                Mesh->SetMaterial(Signal, Material);
    }
    const int32 ScreenIndex = Mesh->GetMaterialIndex(TEXT("screen"));
    if (ScreenIndex != INDEX_NONE)
    {
        if (!Screen)
            Screen = Mesh->CreateAndSetMaterialInstanceDynamic(ScreenIndex);
        if (Screen)
        {
            // Painting can restore the mesh's original material even when the
            // power state is unchanged. Reattach the darkened dynamic screen.
            if (Mesh->GetMaterial(ScreenIndex) != Screen)
                Mesh->SetMaterial(ScreenIndex, Screen);
            if (!HasVisualPower || LastVisualPower != On)
                Screen->SetScalarParameterValue(TEXT("TeleporterPower"), On ? 1 : 0);
        }
    }
    if (StreamingSource && StreamingSource->IsStreamingSourceEnabled() != On)
    {
        if (On)
            StreamingSource->EnableStreamingSource();
        else
            StreamingSource->DisableStreamingSource();
    }
    HasVisualPower = true;
    LastVisualPower = On;
}
void UTeleportLogisticsTravelRemote::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTeleportLogisticsTravelRemote, Registered);
}
FString UTeleportLogisticsTravelRemote::CheckSource(ATeleportLogisticsTravelHub *Source) const
{
    auto *Player = GetOwnerPlayerCharacter();
    if (!IsValid(Source) || !Player || !Source->HasAuthority() || Source->GetWorld() != Player->GetWorld())
        return TEXT("Waiting for the teleporter and player to reach the server.");
    if (Player->GetHealthComponent() && Player->GetHealthComponent()->IsDead())
        return TEXT("Cannot travel while dead.");
    if (Source->GetIsDismantled() || Source->IsAboutToBeDismantled())
        return TEXT("This teleporter is being dismantled.");
    if (Source->GetComponentsBoundingBox(true).ComputeSquaredDistanceToPoint(Player->GetActorLocation()) >
        FMath::Square(500.f))
        return TEXT("Move closer to the Personnel Teleporter.");
    return {};
}
bool UTeleportLogisticsTravelRemote::ServerDirectory_Validate(ATeleportLogisticsTravelHub *, const FString &Search, int32 Page,
                                                  uint32)
{
    return Search.Len() <= 64 && Page >= 0 && Page <= 16384;
}
void UTeleportLogisticsTravelRemote::ServerDirectory_Implementation(ATeleportLogisticsTravelHub *Source, const FString &Search,
                                                        int32 Page, uint32 Seq)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastRead < .4)
        return;
    LastRead = Now;
    FTeleportLogisticsTravelDirectory D;
    D.Source = Source;
    D.Sequence = Seq;
    D.Message = CheckSource(Source);
    if (!D.Message.IsEmpty())
    {
        ClientDirectory(D);
        return;
    }
    D.Powered = Source->Powered();
    TArray<FTeleportLogisticsTravelDestination> All;
    // Registered hubs only. This used to walk every actor in the world each time
    // the directory refreshed, which is the whole save on a large factory.
    for (const auto &Entry : ATeleportLogisticsTravelHub::Live())
    {
        auto *Hub = Entry.Value.Get();
        if (!Hub || Hub == Source || !Hub->DirectoryActive() || !Hub->HasActorBegunPlay() || Hub->IsTemplate() ||
            !Hub->HubId.IsValid() ||
            (!Search.IsEmpty() && !Hub->HubName.Contains(Search, ESearchCase::IgnoreCase)))
            continue;
        FTeleportLogisticsTravelDestination Row;
        Row.Id = Hub->HubId;
        Row.Name = Hub->HubName;
        if (auto *Icons = AFGIconDatabaseSubsystem::Get(Source->GetWorld()))
            Row.IconId = Icons->ResolvePersistentGlobalIconId(Hub->HubIcon);
        Row.Location = Hub->GetActorLocation();
        Row.Powered = Hub->Powered();
        Row.Available = Hub->Available();
        All.Add(Row);
    }
    All.Sort([](const auto &A, const auto &B) {
        const int32 C = A.Name.Compare(B.Name, ESearchCase::IgnoreCase);
        return C == 0 ? A.Id < B.Id : C < 0;
    });
    D.Total = All.Num();
    D.Page = FMath::Min(Page, FMath::Max(0, (D.Total - 1) / 32));
    for (int32 I = D.Page * 32; I < FMath::Min(D.Total, (D.Page + 1) * 32); ++I)
        D.Destinations.Add(All[I]);
    D.Message = D.Powered ? TEXT("Click a ready destination to travel. Both teleporters require 50 MW.")
                          : TEXT("Connect power to this teleporter to travel.");
    ClientDirectory(D);
}
bool UTeleportLogisticsTravelRemote::ServerRename_Validate(ATeleportLogisticsTravelHub *, const FString &Name)
{
    return Name.Len() <= 64;
}
void UTeleportLogisticsTravelRemote::ServerRename_Implementation(ATeleportLogisticsTravelHub *Source, const FString &Name)
{
    FString Error = CheckSource(Source);
    const FString Trimmed = Name.TrimStartAndEnd();
    if (!Error.IsEmpty() || Trimmed.IsEmpty())
    {
        ClientResult(Source, false, Error.IsEmpty() ? TEXT("Enter a teleporter name.") : Error);
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (Now - LastWrite < .4)
    {
        ClientResult(Source, false, TEXT("Please wait before renaming again."));
        return;
    }
    LastWrite = Now;
    if (Trimmed.Contains(TEXT("\n")) || Trimmed.Contains(TEXT("\r")))
    {
        ClientResult(Source, false, TEXT("Use a single-line name."));
        return;
    }
    Source->HubName = Trimmed;
    Source->SetPortalName(FText::FromString(Trimmed));
    Source->FlushNetDormancy();
    Source->ForceNetUpdate();
    Source->UpdateRepresentation();
    ClientResult(Source, false, TEXT("Teleporter name saved."));
}
bool UTeleportLogisticsTravelRemote::ServerTravel_Validate(ATeleportLogisticsTravelHub *, FGuid Id)
{
    return Id.IsValid();
}
void UTeleportLogisticsTravelRemote::ServerTravel_Implementation(ATeleportLogisticsTravelHub *Source, FGuid Id)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastWrite < .4)
    {
        ClientResult(Source, false, TEXT("Please wait before trying again."));
        return;
    }
    LastWrite = Now;
    FString Error = CheckSource(Source);
    if (!Error.IsEmpty())
    {
        ClientResult(Source, false, Error);
        return;
    }
    auto *Player = GetOwnerPlayerCharacter();
    ATeleportLogisticsTravelHub *Destination = nullptr;
    if (const auto *Found = ATeleportLogisticsTravelHub::Live().Find(Id))
        if (auto *Hub = Found->Get(); Hub && Hub->DirectoryActive())
            Destination = Hub;
    if (!Destination || Destination == Source || !Source->Available() || !Destination->Available())
    {
        ClientResult(Source, false, TEXT("Both teleporters must be powered, idle and intact."));
        return;
    }
    if (Player->IsDrivingVehicle() || Player->IsInPortal())
    {
        ClientResult(Source, false, TEXT("Leave your vehicle or finish the current journey first."));
        return;
    }
    const auto *Capsule = Player->GetCapsuleComponent();
    const FVector Entry = Source->StagingLocation(Capsule->GetScaledCapsuleHalfHeight());
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TeleportLogisticsTravelClearance), false, Player);
    const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),
                                                    Capsule->GetScaledCapsuleHalfHeight());
    // Check both sides because native portal traversal mirrors the entry offset.
    for (const float Side : {-1.f, 1.f})
    {
        FVector Exit = Destination->GetActorTransform().TransformPosition(
            FVector(Side * 140, 0, 36 + Capsule->GetScaledCapsuleHalfHeight()));
        if (Source->GetWorld()->OverlapBlockingTestByChannel(Exit, Destination->GetActorQuat(), ECC_Pawn,
                                                             Shape, Query))
        {
            ClientResult(Source, false, TEXT("Destination landing area is blocked."));
            return;
        }
    }
    const FTransform Before = Player->GetActorTransform();
    if (!Player->TeleportTo(Entry, Source->GetActorRotation(), false, false))
    {
        ClientResult(Source, false, TEXT("The departure platform is blocked."));
        return;
    }
    Player->GetCharacterMovement()->StopMovementImmediately();
    Source->BusyUntil = Destination->BusyUntil = Source->GetWorld()->GetTimeSeconds() + 30;
    UE_LOG(LogTeleportLogistics, Display, TEXT("TeleportLogistics Personnel: native travel requested %s -> %s"),
           *Source->HubId.ToString(), *Destination->HubId.ToString());
    Player->StartPortal(Source, Destination);
    if (!Player->IsInPortal())
    {
        UE_LOG(
            LogTeleportLogistics, Warning,
            TEXT(
                "TeleportLogistics Personnel: native portal state did not start; returning to departure position."));
        Source->BusyUntil = Destination->BusyUntil = 0;
        Player->TeleportTo(Before.GetLocation(), Before.Rotator(), false, false);
        ClientResult(Source, false, TEXT("Native portal travel did not start. Please report this message."));
        return;
    }
    // Transit time comes from a curve on the portal, and the journey also waits for the
    // destination to finish streaming. Now that a powered teleporter keeps its own
    // surroundings resident, that wait is over at once, which left the hop
    // instantaneous and showed that the curve was contributing nothing. Hold the
    // traveller for the authored minimum instead, scaled the same way: 1.5s nearby
    // rising to 3.5s at ten kilometres.
    const double Km = FVector::Distance(Source->GetActorLocation(), Destination->GetActorLocation()) / 100000.0;
    const double Hold = FMath::Clamp(1.5 + Km * .2, 1.5, 3.5);
    UE_LOG(LogTeleportLogistics, Display,
           TEXT("TeleportLogistics Personnel: %.2f km, native minimum %.2fs, holding %.2fs"), Km,
           Player->mPortalData.MinPortalTime, Hold);
    if (Player->mPortalData.MinPortalTime < Hold)
    {
        Player->mPortalData.MinPortalTime = Hold;
        Player->ForceNetUpdate();
    }
    ClientResult(Source, true, TEXT("Travelling…"));
}
void UTeleportLogisticsTravelRemote::ClientDirectory_Implementation(const FTeleportLogisticsTravelDirectory &D)
{
    OnDirectory.Broadcast(D);
}
void UTeleportLogisticsTravelRemote::ClientResult_Implementation(ATeleportLogisticsTravelHub *Source, bool Success,
                                                     const FString &Message)
{
    OnResult.Broadcast(Source, Success, Message);
}

bool UTeleportLogisticsTravelRemote::ServerSetIcon_Validate(ATeleportLogisticsTravelHub *, int32 IconId)
{
    return IconId >= INDEX_NONE;
}
void UTeleportLogisticsTravelRemote::ServerSetIcon_Implementation(ATeleportLogisticsTravelHub *Source, int32 IconId)
{
    const FString Error = CheckSource(Source);
    if (!Error.IsEmpty())
    {
        ClientResult(Source, false, Error);
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (Now - LastWrite < .4)
    {
        ClientResult(Source, false, TEXT("Please wait before changing the icon again."));
        return;
    }
    LastWrite = Now;
    FPersistentGlobalIconId Persistent;
    if (IconId != INDEX_NONE)
    {
        auto *DB = AFGIconDatabaseSubsystem::Get(Source->GetWorld());
        FIconData Icon;
        if (!DB || !DB->IsInitialized() || !DB->GetIconDataForIconID(IconId, Icon) || Icon.Hidden ||
            Icon.Animated || Icon.Texture.IsNull())
        {
            ClientResult(Source, false, TEXT("This icon is unavailable. Choose another sign icon."));
            return;
        }
        Persistent = DB->ResolveLocalIconId(IconId);
    }
    Source->HubIcon = Persistent;
    Source->FlushNetDormancy();
    Source->ForceNetUpdate();
    ClientResult(Source, false, TEXT("Destination icon saved."));
}

void ATeleportLogisticsTravelHub::Dismantle_Implementation()
{
    Removed = true;
    RemoveAsRepresentation();
    Super::Dismantle_Implementation();
}
void ATeleportLogisticsTravelHub::OnPlayerTeleportComplete_Implementation(AFGCharacterPlayer *Player,
                                                              AFGBuildablePortalBase *, float)
{
    if (!HasAuthority() || !IsValid(Player))
        return;
    // Native travel has completed destination streaming. Normalize the short
    // final landing move to the marked +X exit, with collision checking again.
    Player->TeleportTo(StagingLocation(Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
                       GetActorRotation(), false, false);
    const bool Front = GetActorTransform().InverseTransformPosition(Player->GetActorLocation()).X >= 0;
    const FRotator Facing(0, GetActorRotation().Yaw + (Front ? 0 : 180), 0);
    Player->SetActorRotation(Facing);
    Player->GetCharacterMovement()->StopMovementImmediately();
    GetWorldTimerManager().SetTimerForNextTick([Weak = TWeakObjectPtr<AFGCharacterPlayer>(Player), Facing] {
        if (auto *Arrived = Weak.Get())
            if (auto *Controller = Arrived->GetController())
            {
                Controller->SetControlRotation(Facing);
                Controller->ClientSetRotation(Facing, true);
            }
    });
}
