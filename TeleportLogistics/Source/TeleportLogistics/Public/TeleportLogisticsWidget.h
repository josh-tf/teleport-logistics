#pragma once
#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "TeleportLogisticsTypes.h"
#include "Styling/SlateBrush.h"
#include "TeleportLogisticsWidget.generated.h"

class SVerticalBox;
class SEditableTextBox;
class STextBlock;
class SBox;
class ATeleportLogisticsBuilding;
class AFGCharacterPlayer;
class UTeleportLogisticsRemoteCall;

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsWidget : public UFGInteractWidget
{
    GENERATED_BODY()
#if WITH_DEV_AUTOMATION_TESTS
    friend class FTeleportLogisticsInteractionCloseTest;
    friend class FTeleportLogisticsUIDesignTest;
#endif
  public:
    UTeleportLogisticsWidget(const FObjectInitializer &ObjectInitializer);
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void Init_Implementation() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry &Geometry, const FKeyEvent &Key) override;
    virtual FReply NativeOnKeyDown(const FGeometry &Geometry, const FKeyEvent &Key) override;
    virtual void OnEscapePressed_Implementation() override;
    virtual void ReleaseSlateResources(bool ReleaseChildren) override;

  private:
    UPROPERTY()
    TObjectPtr<ATeleportLogisticsBuilding> Context;
    UPROPERTY()
    TObjectPtr<UTeleportLogisticsRemoteCall> Remote;
    FTeleportLogisticsSnapshot Data;
    FGuid SelectedChannel;
    TMap<FGuid, int32> RouteIndices;
    FGuid SelectedRoute;
    FTimerHandle RefreshTimer;
    FDelegateHandle SnapshotHandle;
    TSharedPtr<SVerticalBox> RoutesBox, Directory;
    TSharedPtr<SBox> ContentRoot;
    TSharedPtr<SEditableTextBox> NameField, RouteField, SearchField;
    TSharedPtr<STextBlock> Status;
    bool Closing = false;
    bool InitializedSelection = false;
    bool Pending = false;
    bool Dirty = false;
    bool SyncingFields = false;
    bool LastActionFailed = false;
    double PendingSince = 0;
    FString PendingAction;
    FString Confirmation;
    FGuid ConfirmationRoute;
    FString LastListKey;
    TSharedPtr<SEditableTextBox> DirectorySearch;
    TSharedPtr<SBox> ConfirmationHost;
    UPROPERTY(Transient)
    FSlateBrush PlateBrush;
    UPROPERTY(Transient)
    FSlateBrush MeterBrush;
    UPROPERTY(Transient)
    FSlateBrush BuildingBrush;
    void LoadPresentation();
    TSharedRef<SWidget> Frame(const TSharedRef<SWidget> &Body, const FString &Title);
    TSharedRef<SWidget> Action(
        const FString &Label, TFunction<FReply()> Callback, TFunction<bool()> Enabled = [] { return true; },
        bool Primary = false, const FString &Hint = FString());
    TSharedRef<SWidget> Plate(const TSharedRef<SWidget> &Body);
    TSharedRef<SWidget> BuildEndpoint();
    TSharedRef<SWidget> BuildHub();
    TSharedRef<SWidget> BuildRoutes();
    TSharedRef<SWidget> BuildDirectory();
    TSharedRef<SWidget> BuildMetrics();
    TSharedRef<SWidget> ChannelPicker();
    const FTeleportLogisticsRoute *FindRoute(FGuid Id) const;
    const FTeleportLogisticsRoute *Selected() const;
    const FTeleportLogisticsRoute *PreviewRoute() const;
    FString ChannelName(FGuid Id) const;
    FString RoutePath(FGuid Id) const;
    void StageChanges();
    void SelectRoute(FGuid Id);
    void BeginAction(const FString &Action);
    void ConfirmAction(const FString &Action);
    void UpdateConfirmation();
    bool CanManage() const;
    bool CanAct() const;
    bool NameTooLong() const;
    bool CanApply() const;
    int32 Page = 0;
    int32 RoutePage = 0;
    int32 VisibleRouteCount = 0;
    double LastPollSent = -100;
    uint32 NextRequestId = 0;
    uint32 LastReceivedRequestId = 0;
    TSharedRef<SWidget> BuildPanel();
    void InitializeInteraction();
    void Refresh();
    void Receive(const FTeleportLogisticsSnapshot &Snapshot);
    void RenderLists();
    void StopPolling();
    FReply Close();
};
