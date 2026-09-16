#include "TeleportLogisticsTravel.h"
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
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

ATeleportLogisticsTravelHub::ATeleportLogisticsTravelHub()
{
    bReplicates = true;
    mIsUseable = true;
    mInteractWidgetSoftClass = UTeleportLogisticsTravelWidget::StaticClass();
    mHologramClass = ATeleportLogisticsTravelHologram::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "PersonnelHub", "Personnel Teleporter");
    mDescription =
        NSLOCTEXT("TeleportLogistics", "PersonnelHubDesc",
                  "Travel between powered Personnel Teleporters. Open the destination directory with Use.");
    mAllowColoring = true;
    mAllowPatterning = false;
    mShouldApplyCustomizationData = true;
    mPowerConsumption = 50;
    mFactoryTickFunction.bCanEverTick = true;
    if (!RootComponent)
        SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TravelRoot")));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainMesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(
        nullptr, TEXT("/TeleportLogistics/Models/SM_TeleporterTravelHub.SM_TeleporterTravelHub"), nullptr, LOAD_NoWarn));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetMobility(EComponentMobility::Static);
    Power = CreateDefaultSubobject<UFGPowerConnectionComponent>(TEXT("PowerConnection"));
    Power->SetupAttachment(RootComponent);
    Power->SetRelativeLocation(FVector(-180, 220, 310));
    mPortalTravelTimeOverDistance = CreateDefaultSubobject<UCurveFloat>(TEXT("TravelTime"));
    mPortalTravelTimeOverDistance->FloatCurve.AddKey(0, 1.5f);
    mPortalTravelTimeOverDistance->FloatCurve.AddKey(10, 3.5f);
    mMaxPortalTravelTime = 25;
}
void ATeleportLogisticsTravelHub::BeginPlay()
{
    Super::BeginPlay();
    Power->SetPowerInfo(GetPowerInfo());
    if (HasAuthority())
    {
        // Blueprint copies must not share a destination identity with a live hub.
        if (HubId.IsValid())
            for (TActorIterator<ATeleportLogisticsTravelHub> It(GetWorld()); It; ++It)
                if (*It != this && It->HasActorBegunPlay() && It->HubId == HubId)
                {
                    HubId.Invalidate();
                    break;
                }
        if (!HubId.IsValid())
            HubId = FGuid::NewGuid();
        if (HubName.IsEmpty())
            HubName = TEXT("Personnel Teleporter ") + HubId.ToString().Left(8);
        SetPortalName(FText::FromString(HubName));
        FlushNetDormancy();
        ForceNetUpdate();
    }
    if (GetNetMode() != NM_DedicatedServer)
        GetWorldTimerManager().SetTimer(VisualTimer, this, &ATeleportLogisticsTravelHub::UpdateVisual, .2f, true, .01f);
}
void ATeleportLogisticsTravelHub::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(VisualTimer);
    Super::EndPlay(Reason);
}
void ATeleportLogisticsTravelHub::Factory_Tick(float)
{
    // No native pair linking, shared inventory or cross-grid power. Transport
    // uses the player's native portal state machine, not the paired factory loop.
    if (HasAuthority() && GetPowerInfo())
    {
        GetPowerInfo()->SetMaximumTargetConsumption(50);
        GetPowerInfo()->SetTargetConsumption(50);
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
    return LoadObject<UTexture2D>(nullptr, TEXT("/TeleportLogistics/Icons/M_TeleporterHub.M_TeleporterHub"));
}
UMaterialInterface *ATeleportLogisticsTravelHub::GetActorRepresentationCompassMaterial()
{
    return LoadObject<UMaterialInterface>(nullptr, TEXT("/TeleportLogistics/Icons/MI_TeleporterMapHub.MI_TeleporterMapHub"));
}
void ATeleportLogisticsTravelHub::UpdateVisual()
{
    if (!IsInGameThread() || GetNetMode() == NM_DedicatedServer || !Mesh)
        return;
    const bool On = Powered();
    const int32 Signal = Mesh->GetMaterialIndex(TEXT("signal"));
    if (Signal != INDEX_NONE)
    {
        auto *Material = LoadObject<UMaterialInterface>(
            nullptr, On ? TEXT("/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01.MI_Factory_Base_01")
                        : TEXT("/TeleportLogistics/Models/M_TeleporterSignalOff.M_TeleporterSignalOff"));
        if (Material && Mesh->GetMaterial(Signal) != Material)
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
    for (TActorIterator<ATeleportLogisticsTravelHub> It(Source->GetWorld()); It; ++It)
    {
        auto *Hub = *It;
        if (Hub == Source || !Hub->DirectoryActive() || !Hub->HasActorBegunPlay() || Hub->IsTemplate() ||
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
        return C == 0 ? A.Id.ToString() < B.Id.ToString() : C < 0;
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
    for (TActorIterator<ATeleportLogisticsTravelHub> It(Source->GetWorld()); It; ++It)
        if (It->DirectoryActive() && It->HubId == Id)
        {
            Destination = *It;
            break;
        }
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
            Icon.Animated || !Cast<UTexture2D>(Icon.Texture.LoadSynchronous()))
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
ATeleportLogisticsTravelHologram::ATeleportLogisticsTravelHologram()
{
    // Retained CDO references let cooking discover these dependencies.
    ShaftMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    TipMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
}
void ATeleportLogisticsTravelHologram::BeginPlay()
{
    Super::BeginPlay();
    if (IsRunningDedicatedServer())
        return;
    // Runtime-only cue, not a component copied into the built machine.
    auto Add = [this](const TCHAR *Name, UStaticMesh *Geometry, FVector At, FVector Scale,
                      FRotator Rotation) {
        auto *Part = NewObject<UStaticMeshComponent>(this, Name);
        Part->SetStaticMesh(Geometry);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(false);
        Part->SetupAttachment(GetRootComponent());
        Part->SetRelativeLocation(At);
        Part->SetRelativeScale3D(Scale);
        Part->SetRelativeRotation(Rotation);
        Part->SetMaterial(0, mValidPlacementMaterial);
        Part->RegisterComponent();
    };
    Add(TEXT("ExitArrowShaft"), ShaftMesh, FVector(225, 0, 65), FVector(1.1, .15, .15),
        FRotator::ZeroRotator);
    Add(TEXT("ExitArrowTip"), TipMesh, FVector(300, 0, 65), FVector(.5, .5, .6), FRotator(90, 0, 0));
}
