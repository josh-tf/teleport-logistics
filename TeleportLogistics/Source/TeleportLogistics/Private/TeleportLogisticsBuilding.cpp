#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsRemoteCall.h"
#include "FGPlayerController.h"
#include "TeleportLogisticsSubsystem.h"
#include "FGActorRepresentationManager.h"
#include "Engine/Texture2D.h"
#include "TeleportLogisticsWidget.h"
#include "FGCharacterPlayer.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPipeConnectionFactory.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "Buildables/FGBuildableWire.h"
#include "UObject/UnrealType.h"
#include "Hologram/FGFactoryHologram.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ScopeLock.h"
#include "Net/UnrealNetwork.h"

ATeleportLogisticsBuilding::ATeleportLogisticsBuilding()
{
    bReplicates = true;
    mIsUseable = true;
    mInteractWidgetSoftClass = UTeleportLogisticsWidget::StaticClass();
    mHologramClass = AFGFactoryHologram::StaticClass();
    mPowerConsumption = 0;
    // TeleportLogistics's primary/secondary material slots read FactoryGame's native
    // customization primitive data, including paint-finish roughness/metallic.
    mAllowColoring = true;
    mAllowPatterning = false;
    mShouldApplyCustomizationData = true;
    if (!RootComponent)
        SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TeleporterRoot")));
    Part(TEXT("MainMesh"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 0, 12), FVector(1.8, 1.8, 0.24));
}

void ATeleportLogisticsBuilding::Part(const TCHAR *Name, const TCHAR *Mesh, FVector Position, FVector Scale,
                          FRotator Rotation)
{
    auto *Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Component->SetupAttachment(RootComponent);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Mesh));
    Component->SetRelativeLocation(Position);
    Component->SetRelativeScale3D(Scale);
    Component->SetRelativeRotation(Rotation);
    Component->SetCollisionProfileName(TEXT("BlockAll"));
    Component->SetMobility(EComponentMobility::Static);
}

bool ATeleportLogisticsBuilding::UseModel(const TCHAR *AssetPath)
{
    auto *Mesh = LoadObject<UStaticMesh>(nullptr, AssetPath, nullptr, LOAD_NoWarn);
    auto *Main = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("MainMesh")));
    if (!Mesh || !Main)
        return false;
    Main->SetStaticMesh(Mesh);
    Main->SetRelativeLocation(FVector::ZeroVector);
    Main->SetRelativeScale3D(FVector::OneVector);
    return true;
}
void ATeleportLogisticsBuilding::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (!TeleporterId.IsValid())
            TeleporterId = FGuid::NewGuid();
        if (Label.IsEmpty())
            Label = mDisplayName.ToString();
        EnsureNetworkRegistration();
        if (!NetworkRegistered)
            GetWorldTimerManager().SetTimer(RegistrationTimer, this,
                &ATeleportLogisticsBuilding::EnsureNetworkRegistration, 0.35f, true);
        FlushNetDormancy();
        ForceNetUpdate();
    }
}
void ATeleportLogisticsBuilding::EnsureNetworkRegistration()
{
    if (NetworkRegistered || !HasAuthority() || !DirectoryActive())
        return;
    if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
    {
        if (!TeleporterId.IsValid()) TeleporterId = FGuid::NewGuid();
        Network->Register(this);
        NetworkRegistered = true;
        GetWorldTimerManager().ClearTimer(RegistrationTimer);
        FlushNetDormancy();
        ForceNetUpdate();
    }
}
void ATeleportLogisticsBuilding::EndPlay(const EEndPlayReason::Type Reason)
{
    if (HasAuthority())
        if (auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
            Network->Unregister(this);
    GetWorldTimerManager().ClearTimer(RegistrationTimer);
    RemoveAsRepresentation();
    Super::EndPlay(Reason);
}
bool ATeleportLogisticsBuilding::AddAsRepresentation()
{
    if (!HasAuthority())
        return false;
    if (auto *Manager = AFGActorRepresentationManager::Get(GetWorld()))
        return Manager->FindActorRepresentation(this) != nullptr ||
               Manager->CreateAndAddNewRepresentation(this, false,
                                                      UTeleportLogisticsActorRepresentation::StaticClass()) != nullptr;
    return false;
}
bool ATeleportLogisticsBuilding::UpdateRepresentation()
{
    if (!AddAsRepresentation())
        return false;
    return AFGActorRepresentationManager::Get(GetWorld())->UpdateRepresentationOfActor(this);
}
bool ATeleportLogisticsBuilding::RemoveAsRepresentation()
{
    if (!HasAuthority())
        return false;
    if (auto *Manager = AFGActorRepresentationManager::Get(GetWorld()))
        return Manager->RemoveRepresentationOfActor(this);
    return false;
}
void ATeleportLogisticsBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATeleportLogisticsBuilding, TeleporterId);
    DOREPLIFETIME(ATeleportLogisticsBuilding, Label);
}
void ATeleportLogisticsBuilding::UpdateUseState_Implementation(AFGCharacterPlayer *, const FVector &,
                                                   UPrimitiveComponent *, FUseState &State)
{
    State.SetUseState(UFGUseState_Valid::StaticClass());
}
void ATeleportLogisticsBuilding::OnUse_Implementation(AFGCharacterPlayer *Player, const FUseState &State)
{
    // Native use routes the registered widget through HUD::OpenInteractUI,
    // including the game's Blueprint UI stack and initialization lifecycle.
    UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics: use %s, widget %s"), *GetName(),
           *GetNameSafe(GetInteractWidgetClass()));
    Super::OnUse_Implementation(Player, State);
}
FString ATeleportLogisticsBuilding::LookAtDetail() const
{
    return Label;
}
FText ATeleportLogisticsBuilding::GetLookAtDecription_Implementation(AFGCharacterPlayer *Player,
                                                         const FUseState &State) const
{
    // The stock implementation injects the player's currently bound Use key.
    const FText Prompt = Super::GetLookAtDecription_Implementation(Player, State);
    const FString Detail = LookAtDetail();
    if (Detail.IsEmpty())
        return Prompt;
    return FText::FromString(Detail + LINE_TERMINATOR + Prompt.ToString());
}

ATeleportLogisticsEndpoint::ATeleportLogisticsEndpoint()
{
    // Transport must accept belt items every factory tick, even when the
    // manufacturer's idle/significance heuristic considers us non-producing.
    mIsTickRateManaged = false;
    mFactoryTickFunction.TickInterval = 0;
    mFactoryTickFunction.bCanEverTick = true;
}
void ATeleportLogisticsEndpoint::ItemPort(bool IsInput)
{
    Input = IsInput;
    Medium = ETeleportLogisticsMedium::Items;
    MapIcon =
        LoadObject<UTexture2D>(nullptr, IsInput ? TEXT("/TeleportLogistics/Icons/M_TeleporterItemInput.M_TeleporterItemInput")
                                                : TEXT("/TeleportLogistics/Icons/M_TeleporterItemOutput.M_TeleporterItemOutput"));
    MapMaterial = LoadObject<UMaterialInterface>(
        nullptr, IsInput ? TEXT("/TeleportLogistics/Icons/MI_TeleporterMapItemInput.MI_TeleporterMapItemInput")
                         : TEXT("/TeleportLogistics/Icons/MI_TeleporterMapItemOutput.MI_TeleporterMapItemOutput"));
    Belt = CreateDefaultSubobject<UFGFactoryConnectionComponent>(TEXT("ConveyorAny0"));
    Belt->SetupAttachment(RootComponent);
    Belt->SetDirection(IsInput ? EFactoryConnectionDirection::FCD_INPUT
                               : EFactoryConnectionDirection::FCD_OUTPUT);
    Belt->SetRelativeLocation(FVector(160, 0, 100));
    Belt->SetRelativeRotation(FRotator::ZeroRotator);
    Belt->SetConnectorClearance(20);
    Belt->SetForwardPeekAndGrabToBuildable(true);
    // Fade the last 30cm of visible belt travel before the connection consumes
    // an item. Purely visual; never participates in collision or transport.
    auto *Fog = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InputFog"));
    Fog->SetupAttachment(RootComponent);
    Fog->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    Fog->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/-Shared/Material/InputFog.InputFog")));
    Fog->SetRelativeLocation(FVector(190, 0, 140));
    Fog->SetRelativeRotation(FRotator(90, 0, 0));
    Fog->SetRelativeScale3D(FVector(1.5, 1.35, 1));
    Fog->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fog->SetGenerateOverlapEvents(false);
    Fog->SetCastShadow(false);
    mHasFactory_GrabOutput = true;
    mHasFactory_PeekOutput = true;
    if (UseModel(IsInput ? TEXT("/TeleportLogistics/Models/SM_TeleporterItemInput.SM_TeleporterItemInput")
                         : TEXT("/TeleportLogistics/Models/SM_TeleporterItemOutput.SM_TeleporterItemOutput")))
        return;
    Part(TEXT("LeftPier"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, -68, 92),
         FVector(1.1, 0.25, 1.6));
    Part(TEXT("RightPier"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 68, 92),
         FVector(1.1, 0.25, 1.6));
    Part(TEXT("PortalCrown"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 0, 172),
         FVector(1.1, 1.6, 0.22));
    Part(TEXT("BeltBed"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 0, 63), FVector(1.8, 1.1, 0.14));
    // Raised direction marker: input points into the portal, output out of it.
    Part(TEXT("DirectionMarker"), TEXT("/Engine/BasicShapes/Cone.Cone"), FVector(0, 0, 180),
         FVector(0.24, 0.24, 0.45), FRotator(IsInput ? -90 : 90, 0, 0));
}
void ATeleportLogisticsEndpoint::FluidPort(bool IsInput)
{
    Input = IsInput;
    Medium = ETeleportLogisticsMedium::Fluid;
    MapIcon =
        LoadObject<UTexture2D>(nullptr, IsInput ? TEXT("/TeleportLogistics/Icons/M_TeleporterFluidInput.M_TeleporterFluidInput")
                                                : TEXT("/TeleportLogistics/Icons/M_TeleporterFluidOutput.M_TeleporterFluidOutput"));
    MapMaterial = LoadObject<UMaterialInterface>(
        nullptr, IsInput ? TEXT("/TeleportLogistics/Icons/MI_TeleporterMapFluidInput.MI_TeleporterMapFluidInput")
                         : TEXT("/TeleportLogistics/Icons/MI_TeleporterMapFluidOutput.MI_TeleporterMapFluidOutput"));
    Pipe = CreateDefaultSubobject<UFGPipeConnectionFactory>(TEXT("PipeConnection0"));
    Pipe->SetupAttachment(RootComponent);
    Pipe->SetPipeConnectionType(IsInput ? EPipeConnectionType::PCT_CONSUMER
                                        : EPipeConnectionType::PCT_PRODUCER);
    Pipe->SetRelativeLocation(FVector(160, 0, 175));
    Pipe->SetRelativeRotation(FRotator::ZeroRotator);
    Pipe->SetConnectorClearance(20);
    Pipe->SetInventoryAccessIndex(0);
    if (UseModel(IsInput ? TEXT("/TeleportLogistics/Models/SM_TeleporterFluidInput.SM_TeleporterFluidInput")
                         : TEXT("/TeleportLogistics/Models/SM_TeleporterFluidOutput.SM_TeleporterFluidOutput")))
        return;
    Part(TEXT("PressureVessel"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0, 0, 110),
         FVector(1.15, 1.15, 1.7));
    Part(TEXT("LowerCollar"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0, 0, 40),
         FVector(1.4, 1.4, 0.12));
    Part(TEXT("UpperCollar"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0, 0, 185),
         FVector(1.4, 1.4, 0.12));
    Part(TEXT("PipeMouth"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(68, 0, 100),
         FVector(0.55, 0.55, 0.45), FRotator(90, 0, 0));
    Part(TEXT("DirectionMarker"), TEXT("/Engine/BasicShapes/Cone.Cone"), FVector(0, 0, 212),
         FVector(0.3, 0.3, 0.45), FRotator(IsInput ? -90 : 90, 0, 0));
}
ATeleportLogisticsItemInput::ATeleportLogisticsItemInput()
{
    ItemPort(true);
    mDisplayName = NSLOCTEXT("TeleportLogistics", "ItemInput", "Item Teleporter (Input)");
    mDescription =
        NSLOCTEXT("TeleportLogistics", "ItemInputDescription", "Accepts any conveyor item into a named teleporter route.");
}
ATeleportLogisticsItemOutput::ATeleportLogisticsItemOutput()
{
    ItemPort(false);
    mDisplayName = NSLOCTEXT("TeleportLogistics", "ItemOutput", "Item Teleporter (Output)");
    mDescription = NSLOCTEXT("TeleportLogistics", "ItemOutputDescription",
                             "Outputs items received by every input on the same route.");
}
ATeleportLogisticsFluidInput::ATeleportLogisticsFluidInput()
{
    FluidPort(true);
    mDisplayName = NSLOCTEXT("TeleportLogistics", "FluidInput", "Fluid Teleporter (Input)");
    mDescription =
        NSLOCTEXT("TeleportLogistics", "FluidInputDescription", "Accepts pipe contents into a type-safe fluid route.");
}
ATeleportLogisticsFluidOutput::ATeleportLogisticsFluidOutput()
{
    FluidPort(false);
    mDisplayName = NSLOCTEXT("TeleportLogistics", "FluidOutput", "Fluid Teleporter (Output)");
    mDescription = NSLOCTEXT("TeleportLogistics", "FluidOutputDescription",
                             "Supplies fluid received by every input on the same route.");
}
void ATeleportLogisticsEndpoint::BeginPlay()
{
    Super::BeginPlay();
    // Native events need forwarding even when no Blueprint graph overrides them.
    mHasFactory_GrabOutput = mHasFactory_PeekOutput = true;
    RefreshConnectionVisual();
    // Sample on the game thread. Clients use the replicated boolean rather than
    // requiring the attached belt/pipe actor to have resolved locally.
    GetWorldTimerManager().SetTimer(ConnectionVisualTimer, this,
        &ATeleportLogisticsEndpoint::RefreshConnectionVisual, 0.2f, true);
}
void ATeleportLogisticsEndpoint::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ConnectionVisualTimer);
    Super::EndPlay(Reason);
}
void ATeleportLogisticsEndpoint::OnRep_PortConnected()
{
    ApplyPowerVisual(PortConnected);
}
void ATeleportLogisticsEndpoint::RefreshConnectionVisual()
{
    if (!IsInGameThread())
        return;
    if (HasAuthority())
    {
        const bool Connected = Medium == ETeleportLogisticsMedium::Items
            ? Belt && Belt->IsConnected() : Pipe && Pipe->IsConnected();
        if (PortConnected != Connected)
        {
            PortConnected = Connected;
            FlushNetDormancy();
            ForceNetUpdate();
        }
    }
    // Also repairs material overrides after painting while connection is unchanged.
    ApplyPowerVisual(PortConnected);
}
int32 ATeleportLogisticsEndpoint::Buffered() const
{
    int32 Total = 0;
    for (const auto &Stack : Cargo)
        Total += Stack.NumItems;
    return Total;
}
FString ATeleportLogisticsEndpoint::LookAtDetail() const
{
    FString Detail = Label.IsEmpty() ? FString() : Label + LINE_TERMINATOR;
    Detail += TEXT("Route: ") + (RoutePath.IsEmpty() ? FString(TEXT("unassigned")) : RoutePath);
    if (!Enabled)
        Detail += TEXT(" (disabled)");
    return Detail;
}
void ATeleportLogisticsEndpoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATeleportLogisticsEndpoint, RouteId);
    DOREPLIFETIME(ATeleportLogisticsEndpoint, RoutePath);
    DOREPLIFETIME(ATeleportLogisticsEndpoint, PortConnected);
    DOREPLIFETIME(ATeleportLogisticsEndpoint, Enabled);
}

void ATeleportLogisticsEndpoint::Factory_Tick(float Dt)
{
    // Do not run manufacturer's production/input logic: these are transport endpoints.
    if (!HasAuthority())
        return;
    int32 Space;
    {
        FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
        if (!Enabled || !RouteId.IsValid() || IsAboutToBeDismantled())
            return;
        IntakeBudget.accrue(Dt, Rate());
        Space = FMath::Min(Capacity() - Buffered(), static_cast<int32>(IntakeBudget.available()));
        IoInFlight = true;
    }
    struct FFinishIo
    {
        ATeleportLogisticsEndpoint *Endpoint;
        ~FFinishIo()
        {
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            Endpoint->IoInFlight = false;
        }
    } FinishIo{this};
    // Never hold the network mutex while calling into another building/pipe network.
    if (Input && Medium == ETeleportLogisticsMedium::Items && Belt && Belt->IsConnected())
    {
        const int32 MaxGrab = FMath::Min(Space, static_cast<int32>(Belt->MaxNumGrab(Dt)));
        for (int32 Count = 0; Count < MaxGrab; ++Count)
        {
            FInventoryItem Item;
            float Offset = 0;
            if (!Belt->Factory_GrabOutput(Item, Offset))
                break;
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            Cargo.Add(FInventoryStack(Item)); // Preserve full item state and arrival order.
            IntakeBudget.spend(1);
        }
    }
    else if (Input && Medium == ETeleportLogisticsMedium::Fluid && Pipe && Pipe->IsConnected() && Space > 0)
    {
        TSubclassOf<UFGItemDescriptor> Type;
        {
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            Type = Cargo.IsEmpty() ? Pipe->GetFluidDescriptor() : Cargo[0].Item.GetItemClass();
            TSubclassOf<UFGItemDescriptor> Locked;
            if (const auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld()))
                Locked = Network->LockedFluid(RouteId);
            // The route refuses a fluid it is not pinned to, so taking it in would
            // strand the buffer and block dismantle, reroute and reset. Let the pipe
            // back up instead.
            if (Locked && Locked != Type)
                return;
        }
        if (!Type)
            return;
        FInventoryStack Stack;
        if (Pipe->Factory_PullPipeInput(Dt, Stack, Type, Space) && Stack.HasItems())
        {
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            if (Cargo.IsEmpty())
                Cargo.Add(Stack);
            else
                Cargo[0].NumItems += Stack.NumItems;
            IntakeBudget.spend(Stack.NumItems);
        }
    }
    else if (!Input && Medium == ETeleportLogisticsMedium::Fluid && Pipe && Pipe->IsConnected())
    {
        FInventoryStack Stack;
        {
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            if (Cargo.IsEmpty())
                return;
            Stack = Cargo[0];
            Stack.NumItems = FMath::Min(Stack.NumItems, static_cast<int32>(IntakeBudget.available()));
        }
        if (Stack.NumItems <= 0)
            return;
        // Native pipe simulation determines capacity, compatibility, and pressure.
        const int32 Sent = Pipe->Factory_PushPipeOutput(Dt, Stack);
        if (Sent > 0)
        {
            FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
            Cargo[0].NumItems -= Sent;
            if (!Cargo[0].NumItems)
                Cargo.RemoveAt(0);
            IntakeBudget.spend(Sent);
        }
    }
}
bool ATeleportLogisticsEndpoint::Factory_PeekOutput_Implementation(const UFGFactoryConnectionComponent *,
                                                       TArray<FInventoryItem> &Items,
                                                       TSubclassOf<UFGItemDescriptor> Type) const
{
    if (!HasAuthority())
        return false;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    if (Input || !Enabled || Cargo.IsEmpty() || (Type && Cargo[0].Item.GetItemClass() != Type))
        return false;
    Items.Add(Cargo[0].Item);
    return true;
}
bool ATeleportLogisticsEndpoint::Factory_GrabOutput_Implementation(UFGFactoryConnectionComponent *, FInventoryItem &Item,
                                                       float &Offset, TSubclassOf<UFGItemDescriptor> Type)
{
    if (!HasAuthority())
        return false;
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    if (Input || !Enabled || Cargo.IsEmpty() || (Type && Cargo[0].Item.GetItemClass() != Type))
        return false;
    Item = Cargo[0].Item;
    Offset = 0;
    if (--Cargo[0].NumItems == 0)
        Cargo.RemoveAt(0);
    return true;
}
void ATeleportLogisticsEndpoint::GetDismantleRefund_Implementation(TArray<FInventoryStack> &Refund,
                                                       bool NoBuildCost) const
{
    Super::GetDismantleRefund_Implementation(Refund, NoBuildCost);
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    if (Medium == ETeleportLogisticsMedium::Items)
        Refund.Append(Cargo);
}
bool ATeleportLogisticsEndpoint::CanDismantle_Implementation() const
{
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    const FFluidBox *FluidBox = Pipe ? Pipe->GetFluidBox() : nullptr;
    const bool HasConnectionFluid = FluidBox && FluidBox->Content > KINDA_SMALL_NUMBER;
    return !IoInFlight && (Medium != ETeleportLogisticsMedium::Fluid || (Cargo.IsEmpty() && !HasConnectionFluid)) &&
           Super::CanDismantle_Implementation();
}

ATeleportLogisticsHub::ATeleportLogisticsHub()
{
    MapIcon = LoadObject<UTexture2D>(nullptr, TEXT("/TeleportLogistics/Icons/M_TeleporterHub.M_TeleporterHub"));
    MapMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/TeleportLogistics/Icons/MI_TeleporterMapHub.MI_TeleporterMapHub"));
    mDisplayName = NSLOCTEXT("TeleportLogistics", "Hub", "Teleporter Hub");
    mDescription = NSLOCTEXT("TeleportLogistics", "HubDescription",
                             "Creates a named channel and provides powered network management.");
    mPowerConsumption = 5;
    mFactoryTickFunction.bCanEverTick = true;
    Power = CreateDefaultSubobject<UFGPowerConnectionComponent>(TEXT("PowerConnection"));
    Power->SetupAttachment(RootComponent);
    // Workbench-sized mesh; OBJ import mirrors Blender Y at the mast cap.
    Power->SetRelativeLocation(FVector(-83.64, 52.5, 298.8));
    Power->SetRelativeRotation(FRotator(0, 180, 0));
    if (UseModel(TEXT("/TeleportLogistics/Models/SM_TeleporterHub.SM_TeleporterHub")))
        return;
    Part(TEXT("ConsoleStand"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 0, 70),
         FVector(0.65, 1.1, 1.1));
    Part(TEXT("Console"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0, 0, 140), FVector(0.3, 1.6, 0.85),
         FRotator(-20, 0, 0));
    Part(TEXT("Antenna"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(-55, 60, 190),
         FVector(0.08, 0.08, 1.5));
}
void ATeleportLogisticsHub::BeginPlay()
{
    Super::BeginPlay();
    // Saved component transforms must not restore the old socket below the roof.
    Power->SetRelativeTransform(GetDefault<ATeleportLogisticsHub>()->Power->GetRelativeTransform());
    Power->SetPowerInfo(GetPowerInfo());
    // Wires serialize their own endpoints; moving the component alone leaves
    // existing saves and replicated clients drawing the old floating cable.
    GetWorldTimerManager().SetTimer(WireAnchorTimer, this, &ATeleportLogisticsHub::RefreshWireAnchors, 1.0f, true);
    if (HasAuthority() && GetPowerInfo())
        GetPowerInfo()->SetTargetConsumption(5);
    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshPowerVisual();
        GetWorldTimerManager().SetTimer(PowerVisualTimer, this, &ATeleportLogisticsHub::RefreshPowerVisual, 0.1f, true);
    }
}
void ATeleportLogisticsHub::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(PowerVisualTimer);
    GetWorldTimerManager().ClearTimer(WireAnchorTimer);
    Super::EndPlay(Reason);
}
bool ATeleportLogisticsHub::CanDismantle_Implementation() const
{
    FScopeLock Lock(&ATeleportLogisticsSubsystem::Mutex);
    auto *Network = ATeleportLogisticsSubsystem::Get(GetWorld());
    return (!Network || Network->ChannelEmpty(TeleporterId)) && Super::CanDismantle_Implementation();
}

void ATeleportLogisticsHub::Factory_Tick(float)
{
    // Factory ticks run on worker threads. Keep all material/scene access in
    // RefreshPowerVisual, which is driven by the game-thread timer manager.
    if (HasAuthority() && GetPowerInfo())
        GetPowerInfo()->SetTargetConsumption(5);
}
bool ATeleportLogisticsHub::ControlPowered() const
{
    return GetPowerInfo() && GetPowerInfo()->IsConnected() && GetPowerInfo()->HasPower() &&
           !GetPowerInfo()->IsFuseTriggered();
}
void ATeleportLogisticsHub::RefreshPowerVisual()
{
    ApplyPowerVisual(ControlPowered());
}
void ATeleportLogisticsBuilding::ApplyPowerVisual(bool HasPower)
{
    // A guard remains in Shipping, where check() may be compiled out. Neither
    // the cached state nor any UObject/material work is safe on factory workers.
    if (!IsInGameThread() || GetNetMode() == NM_DedicatedServer)
        return;
    auto *Main = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("MainMesh")));
    if (!Main)
        return;

    const int32 SignalIndex = Main->GetMaterialIndex(TEXT("signal"));
    if (SignalIndex != INDEX_NONE)
    {
        if (!PoweredSignalMaterial)
            PoweredSignalMaterial = LoadObject<UMaterialInterface>(
                nullptr, TEXT("/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01."
                              "MI_Factory_Base_01"));
        if (!UnpoweredSignalMaterial)
            UnpoweredSignalMaterial = LoadObject<UMaterialInterface>(
                nullptr, TEXT("/TeleportLogistics/Models/M_TeleporterSignalOff.M_TeleporterSignalOff"));
        if (auto *Signal = HasPower ? PoweredSignalMaterial.Get() : UnpoweredSignalMaterial.Get())
            if (Main->GetMaterial(SignalIndex) != Signal)
                Main->SetMaterial(SignalIndex, Signal);
    }

    const int32 ScreenIndex = Main->GetMaterialIndex(TEXT("screen"));
    if (ScreenIndex != INDEX_NONE)
    {
        if (!ScreenMaterial)
            ScreenMaterial = Main->CreateAndSetMaterialInstanceDynamic(ScreenIndex);
        if (ScreenMaterial)
        {
            // Customizer can restore the mesh material without changing power.
            if (Main->GetMaterial(ScreenIndex) != ScreenMaterial)
                Main->SetMaterial(ScreenIndex, ScreenMaterial);
            if (!VisualPowerInitialized || LastVisualPower != HasPower)
                ScreenMaterial->SetScalarParameterValue(TEXT("TeleporterPower"), HasPower ? 1.0f : 0.0f);
        }
    }
    LastVisualPower = HasPower;
    VisualPowerInitialized = true;
}

void ATeleportLogisticsHub::RefreshWireAnchors()
{
    if (!IsInGameThread() || !IsValid(Power))
        return;
    TArray<AFGBuildableWire *> Wires;
    Power->GetWires(Wires);
    const FVector Target = Power->GetComponentLocation();
    for (auto *Wire : Wires)
    {
        if (!IsValid(Wire))
            continue;
        const int32 End = Wire->GetConnection(0) == Power   ? 0
                          : Wire->GetConnection(1) == Power ? 1
                                                            : INDEX_NONE;
        if (End == INDEX_NONE)
            continue;
        auto *InstancesProperty = FindFProperty<FArrayProperty>(Wire->GetClass(), TEXT("mWireInstances"));
        auto *LocationsProperty =
            FindFProperty<FStructProperty>(Wire->GetClass(), TEXT("mConnectionLocations"));
        auto *InstanceType =
            InstancesProperty ? CastField<FStructProperty>(InstancesProperty->Inner) : nullptr;
        if (!InstanceType || InstanceType->Struct != FWireInstance::StaticStruct() || !LocationsProperty ||
            LocationsProperty->ArrayDim != 2)
            continue;
        FScriptArrayHelper Instances(InstancesProperty,
                                     InstancesProperty->ContainerPtrToValuePtr<void>(Wire));
        bool Changed = false;
        for (int32 I = 0; I < Instances.Num(); ++I)
        {
            auto &Instance = *reinterpret_cast<FWireInstance *>(Instances.GetRawPtr(I));
            if (Instance.Locations[End].Equals(Target, .25f))
                continue;
            Instance.Locations[End] = Target;
            Instance.CachedRelativeLocations[End] =
                Wire->GetActorTransform().InverseTransformPosition(Target);
            if (GetNetMode() != NM_DedicatedServer && IsValid(Instance.WireMesh))
                AFGBuildableWire::UpdateWireInstanceMesh(Instance);
            Changed = true;
        }
        if (!Wire->GetConnectionLocation(End).Equals(Target, .25f))
        {
            *LocationsProperty->ContainerPtrToValuePtr<FVector>(Wire, End) = Target;
            Changed = true;
        }
        if (Changed)
        {
            if (auto *LengthProperty = FindFProperty<FFloatProperty>(Wire->GetClass(), TEXT("mCachedLength")))
                LengthProperty->SetPropertyValue_InContainer(
                    Wire, FVector::Distance(Wire->GetConnectionLocation(0), Wire->GetConnectionLocation(1)));
            if (Wire->HasAuthority())
            {
                Wire->FlushNetDormancy();
                Wire->ForceNetUpdate();
            }
            UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics: aligned saved wire %s to hub mast %s"), *Wire->GetName(),
                   *Target.ToString());
        }
    }
}

TSubclassOf<UObject> ATeleportLogisticsEndpoint::GetClipboardMappingClass_Implementation()
{
    return UTeleportLogisticsClipboardSettings::StaticClass();
}
UFGFactoryClipboardSettings *ATeleportLogisticsEndpoint::CopySettings_Implementation()
{
    auto *Settings = NewObject<UTeleportLogisticsClipboardSettings>();
    Settings->RouteId = RouteId;
    Settings->Medium = Medium;
    return Settings;
}
bool ATeleportLogisticsEndpoint::PasteSettings_Implementation(UFGFactoryClipboardSettings *Settings,
                                                 AFGPlayerController *Player)
{
    const auto *TeleportLogisticsSettings = Cast<UTeleportLogisticsClipboardSettings>(Settings);
    if (!TeleportLogisticsSettings || TeleportLogisticsSettings->Medium != Medium || !TeleportLogisticsSettings->RouteId.IsValid() || !Player)
        return false;
    auto *Remote = Cast<UTeleportLogisticsRemoteCall>(Player->GetRemoteCallObjectOfClass(UTeleportLogisticsRemoteCall::StaticClass()));
    if (!Remote)
        return false;
    Remote->ServerPasteRoute(this, TeleportLogisticsSettings->RouteId);
    return true; // Accepted for server validation; not an optimistic local mutation.
}

void ATeleportLogisticsBuilding::Dismantle_Implementation()
{
    RemovedFromNetwork=true;
    GetWorldTimerManager().ClearTimer(RegistrationTimer);
    if(HasAuthority())if(auto* Network=ATeleportLogisticsSubsystem::Get(GetWorld()))Network->Unregister(this,true);
    RemoveAsRepresentation();Super::Dismantle_Implementation();
}
