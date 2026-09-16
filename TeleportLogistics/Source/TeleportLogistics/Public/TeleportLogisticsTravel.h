#pragma once
#include "CoreMinimal.h"
#include "TeleportLogisticsMap.h"
#include "Buildables/FGBuildablePortalBase.h"
#include "FGRemoteCallObject.h"
#include "FGIconLibrary.h"
#include "Hologram/FGFactoryHologram.h"
#include "UI/FGInteractWidget.h"
#include "TeleportLogisticsTravel.generated.h"

UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsTravelHub : public AFGBuildablePortalBase
{
    friend class FTeleportLogisticsTravelTest;
    GENERATED_BODY()
  public:
    ATeleportLogisticsTravelHub();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Factory_Tick(float Dt) override;
    virtual float GetMaximumPowerConsumption() const override
    {
        return 50.f;
    }
    virtual float GetProducingPowerConsumptionBase_ForPortal() const override
    {
        return 50.f;
    }
    virtual bool ShouldSave_Implementation() const override
    {
        return !GetIsDismantled();
    }
    virtual bool CanDismantle_Implementation() const override;
    virtual void Dismantle_Implementation() override;
    bool DirectoryActive() const
    {
        return !Removed && !GetIsDismantled() && !IsAboutToBeDismantled() && !IsActorBeingDestroyed();
    }
    virtual bool IsUseable_Implementation() const override
    {
        return true;
    }
    virtual void UpdateUseState_Implementation(AFGCharacterPlayer *, const FVector &, UPrimitiveComponent *,
                                               FUseState &) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &Out) const override;
    virtual void GetPortalSurfaceTransform_Implementation(FTransform &Out) const override;
    virtual void OnPlayerTeleportBegin_Implementation(AFGCharacterPlayer *, AFGBuildablePortalBase *,
                                                      float) override
    {
    }
    virtual void OnPlayerTeleportComplete_Implementation(AFGCharacterPlayer *, AFGBuildablePortalBase *,
                                                         float) override;
    virtual UTexture2D *GetActorRepresentationTexture() override;
    virtual FLinearColor GetActorRepresentationColor() override
    {
        return FLinearColor::White;
    }
    virtual ERepresentationType GetActorRepresentationType() override
    {
        return TeleportLogisticsMap::Personnel();
    }
    virtual FVector GetRealActorLocation() override
    {
        return GetActorLocation();
    }
    virtual bool IsActorStatic() override
    {
        return true;
    }
    virtual UMaterialInterface *GetActorRepresentationCompassMaterial() override;
    virtual FText GetActorRepresentationText() override
    {
        return FText::FromString(HubName);
    }
    virtual bool GetActorShouldShowOnMap() override
    {
        return true;
    }
    virtual bool GetActorShouldShowInCompass() override
    {
        return false;
    }
    bool Powered() const;
    bool Available() const;
    FVector StagingLocation(float CapsuleHalfHeight) const;
    UPROPERTY(SaveGame, Replicated)
    FGuid HubId;
    UPROPERTY(SaveGame, Replicated)
    FString HubName;
    UPROPERTY(SaveGame, Replicated)
    FPersistentGlobalIconId HubIcon;
    double BusyUntil = 0;

  private:
    UPROPERTY()
    TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY()
    TObjectPtr<class UFGPowerConnectionComponent> Power;
    UPROPERTY()
    TObjectPtr<class UMaterialInstanceDynamic> Screen;
    FTimerHandle VisualTimer;
    bool Removed = false;
    bool HasVisualPower = false, LastVisualPower = false;
    void UpdateVisual();
};

USTRUCT()
struct FTeleportLogisticsTravelDestination
{
    GENERATED_BODY()
    UPROPERTY()
    FGuid Id;
    UPROPERTY()
    FString Name;
    UPROPERTY()
    int32 IconId = INDEX_NONE;
    UPROPERTY()
    FVector Location = FVector::ZeroVector;
    UPROPERTY()
    bool Powered = false;
    UPROPERTY()
    bool Available = false;
};
USTRUCT()
struct FTeleportLogisticsTravelDirectory
{
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<ATeleportLogisticsTravelHub> Source;
    UPROPERTY()
    TArray<FTeleportLogisticsTravelDestination> Destinations;
    UPROPERTY()
    uint32 Sequence = 0;
    UPROPERTY()
    int32 Total = 0;
    UPROPERTY()
    int32 Page = 0;
    UPROPERTY()
    bool Powered = false;
    UPROPERTY()
    FString Message;
};
DECLARE_MULTICAST_DELEGATE_OneParam(FTeleportLogisticsTravelDirectoryEvent, const FTeleportLogisticsTravelDirectory &);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTeleportLogisticsTravelResultEvent, ATeleportLogisticsTravelHub *, bool, const FString &);
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsTravelRemote : public UFGRemoteCallObject
{
    GENERATED_BODY()
  public:
    UPROPERTY(Replicated)
    bool Registered = true;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &Out) const override;
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerDirectory(ATeleportLogisticsTravelHub *Source, const FString &Search, int32 Page, uint32 Sequence);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTravel(ATeleportLogisticsTravelHub *Source, FGuid Destination);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRename(ATeleportLogisticsTravelHub *Source, const FString &Name);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetIcon(ATeleportLogisticsTravelHub *Source, int32 IconId);
    UFUNCTION(Client, Reliable)
    void ClientDirectory(const FTeleportLogisticsTravelDirectory &Data);
    UFUNCTION(Client, Reliable)
    void ClientResult(ATeleportLogisticsTravelHub *Source, bool Success, const FString &Message);
    FTeleportLogisticsTravelDirectoryEvent OnDirectory;
    FTeleportLogisticsTravelResultEvent OnResult;
    uint32 NextSequence()
    {
        return ++SequenceCounter;
    }

  private:
    FString CheckSource(ATeleportLogisticsTravelHub *Source) const;
    double LastRead = -100, LastWrite = -100;
    uint32 SequenceCounter = 0;
};

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsTravelWidget : public UFGInteractWidget
{
    friend class FTeleportLogisticsTravelTest;
    GENERATED_BODY()
  public:
    UTeleportLogisticsTravelWidget(const FObjectInitializer &Init);
    virtual void Init_Implementation() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void OnEscapePressed_Implementation() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry &, const FKeyEvent &) override;

  private:
    UPROPERTY()
    TObjectPtr<ATeleportLogisticsTravelHub> Source;
    UPROPERTY()
    TObjectPtr<UTeleportLogisticsTravelRemote> Remote;
    TSharedPtr<class SBox> Root;
    TSharedPtr<class SVerticalBox> Rows;
    TSharedPtr<class SEditableTextBox> Search, Name;
    TSharedPtr<class STextBlock> Status;
    FDelegateHandle DirectoryHandle, ResultHandle;
    FTimerHandle PollTimer;
    FTeleportLogisticsTravelDirectory Data;
    FGuid Selected;
    int32 Page = 0;
    bool Closing = false, Pending = false;
    double PendingSince = 0, FeedbackUntil = 0;
    FString LastList;
    uint32 DirectoryRequest = 0, DirectoryEpoch = 0;
    FString RequestedSearch;
    int32 RequestedPage = 0;
    double DirectoryRequestedAt = 0;
    bool PickingIcons = false;
    int32 IconPage = 0, IconTotal = 0;
    TMap<int32, TSharedPtr<FSlateBrush>> IconBrushes;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> IconResources;
    FSlateBrush PlateBrush;
    UPROPERTY()
    TObjectPtr<UTexture2D> PlateTexture;
    const FSlateBrush *IconBrush(int32 IconId);
    void ShowIcons();
    void ToggleIcons();
    void Start();
    void Poll();
    void Receive(const FTeleportLogisticsTravelDirectory &);
    void Result(ATeleportLogisticsTravelHub *ResultSource, bool Success, const FString &Message);
    FReply Close();
};

UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsTravelHologram : public AFGFactoryHologram
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsTravelHologram();
    UPROPERTY()
    TObjectPtr<UStaticMesh> ShaftMesh;
    UPROPERTY()
    TObjectPtr<UStaticMesh> TipMesh;
    virtual void BeginPlay() override;
};
