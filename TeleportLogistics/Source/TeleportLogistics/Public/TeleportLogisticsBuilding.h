#pragma once
#include "CoreMinimal.h"
#include "TeleportLogisticsMap.h"
#include "Buildables/FGBuildableFactory.h"
#include "FGInventoryComponent.h"
#include "FGActorRepresentationInterface.h"
#include "TeleportLogisticsTypes.h"
#include "Core/TeleportLogisticsScheduler.h"
#include "TimerManager.h"
#include "TeleportLogisticsBuilding.generated.h"

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsActorRepresentation : public UFGActorRepresentation
{
    GENERATED_BODY()
  public:
    virtual bool GetScaleWithMap() const override
    {
        return false;
    }
    virtual float GetScaleOnMap() const override
    {
        return 0.72f;
    }
};

UCLASS(Abstract)
class TELEPORTLOGISTICS_API ATeleportLogisticsBuilding : public AFGBuildableFactory, public IFGActorRepresentationInterface
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsBuilding();
    virtual bool ShouldSave_Implementation() const override { return !GetIsDismantled(); }
    virtual void BeginPlay() override;
    void EnsureNetworkRegistration();
    virtual void Dismantle_Implementation() override;
    bool DirectoryActive() const { return !RemovedFromNetwork && !GetIsDismantled() && !IsAboutToBeDismantled() && !IsActorBeingDestroyed(); }
    bool RemovedFromNetwork = false;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void UpdateUseState_Implementation(AFGCharacterPlayer *Player, const FVector &At,
                                               UPrimitiveComponent *Hit, FUseState &State) override;
    virtual void OnUse_Implementation(AFGCharacterPlayer *Player, const FUseState &State) override;
    virtual bool IsUseable_Implementation() const override
    {
        return true;
    }
    virtual FText GetLookAtDecription_Implementation(AFGCharacterPlayer *Player,
                                                     const FUseState &State) const override;
    // Appended above the stock use prompt in the look-at panel.
    virtual FString LookAtDetail() const;
    virtual bool AddAsRepresentation() override;
    virtual bool UpdateRepresentation() override;
    virtual bool RemoveAsRepresentation() override;
    virtual FVector GetRealActorLocation() override
    {
        return GetActorLocation();
    }
    virtual UTexture2D *GetActorRepresentationTexture() override
    {
        return MapIcon;
    }
    virtual UMaterialInterface *GetActorRepresentationCompassMaterial() override
    {
        return MapMaterial;
    }
    virtual FText GetActorRepresentationText() override
    {
        return FText::FromString(MapLabel.IsEmpty() ? Label : MapLabel);
    }
    virtual FLinearColor GetActorRepresentationColor() override
    {
        return FLinearColor::White;
    }
    virtual ERepresentationType GetActorRepresentationType() override
    {
        return TeleportLogisticsMap::Logistics();
    }
    virtual bool GetActorShouldShowInCompass() override
    {
        return false;
    }
    virtual bool GetActorShouldShowOnMap() override
    {
        return true;
    }
    // Server cache; the representation manager replicates text independently of actor relevancy.
    UPROPERTY(Transient)
    FString MapLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> MapIcon;
    // Persistent asset references replicate with the native representation, even
    // when the building actor is outside client relevancy. Never replicate MIDs.
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> MapMaterial;
    UPROPERTY(SaveGame, Replicated)
    FGuid TeleporterId;
    UPROPERTY(SaveGame, Replicated)
    FString Label;

  private:
    FTimerHandle RegistrationTimer;
    bool NetworkRegistered = false;
  protected:
    void ApplyPowerVisual(bool HasPower);
    UPROPERTY(Transient)
    /** This building's map marker, held so refreshes need not search the world for it. */
    TWeakObjectPtr<class UFGActorRepresentation> Representation;
    TObjectPtr<class UMaterialInterface> PoweredSignalMaterial;
    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInterface> UnpoweredSignalMaterial;
    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> ScreenMaterial;
    bool VisualPowerInitialized = false;
    bool LastVisualPower = false;
    bool UseModel(const TCHAR *AssetPath);
    /** Make the rear pad a snap target for the game's own wall signs. */
    void AddSignMount(const FVector &At);
    void Part(const TCHAR *Name, const TCHAR *Mesh, FVector Position, FVector Scale,
              FRotator Rotation = FRotator::ZeroRotator);
};

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsClipboardSettings : public UFGFactoryClipboardSettings
{
    GENERATED_BODY()
  public:
    UPROPERTY()
    FGuid RouteId;
    UPROPERTY()
    ETeleportLogisticsMedium Medium = ETeleportLogisticsMedium::Items;
};

UCLASS(Abstract)
class TELEPORTLOGISTICS_API ATeleportLogisticsEndpoint : public ATeleportLogisticsBuilding
{
    GENERATED_BODY()
#if WITH_DEV_AUTOMATION_TESTS
    friend class FTeleportLogisticsEndpointConnectionVisualTest;
#endif
  public:
    ATeleportLogisticsEndpoint();
    virtual bool CanUseFactoryClipboard_Implementation() override { return true; }
    virtual TSubclassOf<UObject> GetClipboardMappingClass_Implementation() override;
    virtual UFGFactoryClipboardSettings *CopySettings_Implementation() override;
    virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings *Settings,
                                             AFGPlayerController *Player) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Factory_Tick(float Dt) override;
    /** Buffered cargo is not part of a design. Without these a blueprint saved over a
     *  full endpoint carries its contents and mints them again at every placement. */
    virtual void PreSerializedToBlueprint() override;
    virtual void PostSerializedToBlueprint() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void GetDismantleRefund_Implementation(TArray<FInventoryStack> &Refund,
                                                   bool NoBuildCost) const override;
    virtual bool CanDismantle_Implementation() const override;
    virtual FString LookAtDetail() const override;
    UPROPERTY(SaveGame, Replicated)
    FGuid RouteId;
    UPROPERTY(SaveGame, Replicated)
    bool Enabled = true;
    // Channel and route names resolved server-side in RefreshMap. Replicated because the
    // look-at panel is built on the client and cannot wait for a snapshot round trip.
    UPROPERTY(Replicated)
    FString RoutePath;
    UPROPERTY(EditDefaultsOnly)
    ETeleportLogisticsMedium Medium = ETeleportLogisticsMedium::Items;
    UPROPERTY(EditDefaultsOnly)
    bool Input = true;
    UPROPERTY(SaveGame)
    TArray<FInventoryStack> Cargo;
    /** Held across blueprint serialisation only; never saved and never replicated. */
    TArray<FInventoryStack> StashedCargo;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UFGFactoryConnectionComponent> Belt;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UFGPipeConnectionFactory> Pipe;
    // All Cargo reads/writes and topology changes use ATeleportLogisticsSubsystem::Mutex.
    int32 Buffered() const;
    int32 Capacity() const
    {
        return Medium == ETeleportLogisticsMedium::Items ? 64 : 50000;
    }
    double Rate() const
    {
        return Medium == ETeleportLogisticsMedium::Items ? 20.0 : 10000.0;
    }
    teleport_logistics::Budget TransportBudget;
    teleport_logistics::Budget IntakeBudget;
    bool IoInFlight = false;

  protected:
    UPROPERTY(ReplicatedUsing = OnRep_PortConnected)
    bool PortConnected = false;
    UFUNCTION()
    void OnRep_PortConnected();
    void RefreshConnectionVisual();
    FTimerHandle ConnectionVisualTimer;
    void ItemPort(bool IsInput);
    void FluidPort(bool IsInput);
    virtual bool Factory_PeekOutput_Implementation(const UFGFactoryConnectionComponent *Connection,
                                                   TArray<FInventoryItem> &Items,
                                                   TSubclassOf<UFGItemDescriptor> Type) const override;
    virtual bool Factory_GrabOutput_Implementation(UFGFactoryConnectionComponent *Connection,
                                                   FInventoryItem &Item, float &Offset,
                                                   TSubclassOf<UFGItemDescriptor> Type) override;
};

UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsItemInput : public ATeleportLogisticsEndpoint
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsItemInput();
};
UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsItemOutput : public ATeleportLogisticsEndpoint
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsItemOutput();
};
UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsFluidInput : public ATeleportLogisticsEndpoint
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsFluidInput();
};
UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsFluidOutput : public ATeleportLogisticsEndpoint
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsFluidOutput();
};
UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsHub : public ATeleportLogisticsBuilding
{
    GENERATED_BODY()
#if WITH_DEV_AUTOMATION_TESTS
    friend class FTeleportLogisticsHubPowerVisualWorkerTest;
#endif
  public:
    ATeleportLogisticsHub();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool CanDismantle_Implementation() const override;
    virtual void Factory_Tick(float Dt) override;
    bool ControlPowered() const;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UFGPowerConnectionComponent> Power;

  protected:
    // Only the world timer manager (game thread) updates rendering state.
    void RefreshPowerVisual();
    void RefreshWireAnchors();
    FTimerHandle WireAnchorTimer;
    FTimerHandle PowerVisualTimer;


};
