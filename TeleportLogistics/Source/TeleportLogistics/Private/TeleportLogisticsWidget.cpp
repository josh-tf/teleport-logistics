#include "TeleportLogisticsWidget.h"
#include "TeleportLogisticsAsset.h"
#include "TeleportLogisticsSettings.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsRemoteCall.h"
#include "FGCharacterPlayer.h"
#include "FGHUD.h"
#include "FGPlayerController.h"
#include "UI/FGGameUI.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
FLinearColor Hex(const TCHAR *Value)
{
    return FLinearColor(FColor::FromHex(Value));
}
const FLinearColor TextPrimary = Hex(TEXT("E8E8E8"));
const FLinearColor TextMuted = Hex(TEXT("ADADAD"));
const FLinearColor FicsitOrange = Hex(TEXT("E8963C"));
const FLinearColor Cyan = Hex(TEXT("4FC3E8"));
const FLinearColor Green = Hex(TEXT("7FC05F"));
const FLinearColor Warning = Hex(TEXT("E8C33C"));
const FLinearColor ErrorColor = Hex(TEXT("D1705E"));
const FLinearColor Panel = Hex(TEXT("202021"));
const FLinearColor PanelRaised = Hex(TEXT("333333"));
const FLinearColor PanelSelected = Hex(TEXT("3A2F22"));
FSlateFontInfo Font(float Size, bool Bold = false)
{
    Size *= .9f; // Compact typography leaves room for fields at the game UI scale.
    // FontFace assets are not Slate font providers. Use the native composite
    // font so Heebo weights and the game's language fallbacks resolve correctly.
    static TStrongObjectPtr<UFont> Family(
        LoadObject<UFont>(nullptr, TEXT("/Game/FactoryGame/Interface/Font/DescriptionText.DescriptionText")));
    return Family.IsValid() ? FSlateFontInfo(Family.Get(), Size, Bold ? TEXT("Bold") : TEXT("Regular"))
                            : FCoreStyle::GetDefaultFontStyle(Bold ? TEXT("Bold") : TEXT("Regular"), Size);
}
TSharedRef<STextBlock> Text(const FString &Value, float Size = 15, FLinearColor Color = TextPrimary,
                            bool Bold = false)
{
    return SNew(STextBlock)
        .Text(FText::FromString(Value))
        .Font(Font(Size, Bold))
        .ColorAndOpacity(Color)
        .AutoWrapText(true);
}
TSharedRef<SWidget> Sized(const TSharedRef<SWidget> &Button, float Width = 186)
{
    // Both action rows use one width so the column reads as a pair, not a stack of
    // differently sized buttons.
    return SNew(SBox).MinDesiredWidth(Width).HAlign(HAlign_Fill)[Button];
}
TSharedRef<STextBlock> Live(TFunction<FString()> Value, float Size = 15, FLinearColor Color = TextPrimary,
                            bool Bold = false)
{
    return SNew(STextBlock)
        .Text_Lambda([Value] { return FText::FromString(Value()); })
        .Font(Font(Size, Bold))
        .ColorAndOpacity(Color)
        .AutoWrapText(true);
}
TSharedRef<STextBlock> Line(const FString &Value, float Size = 15, FLinearColor Color = TextPrimary)
{
    return SNew(STextBlock)
        .Text(FText::FromString(Value))
        .ToolTipText(FText::FromString(Value))
        .Font(Font(Size))
        .ColorAndOpacity(Color)
        .OverflowPolicy(ETextOverflowPolicy::Ellipsis);
}
const FButtonStyle &ButtonStyle()
{
    static FButtonStyle Style = [] {
        FButtonStyle V;
        V.SetNormal(FSlateColorBrush(PanelRaised));
        V.SetHovered(FSlateColorBrush(Hex(TEXT("505050"))));
        V.SetPressed(FSlateColorBrush(FicsitOrange));
        V.SetDisabled(FSlateColorBrush(Hex(TEXT("242424"))));
        V.SetNormalPadding(FMargin(10, 5));
        V.SetPressedPadding(FMargin(10, 5));
        return V;
    }();
    return Style;
}
const FEditableTextBoxStyle &InputStyle()
{
    static FEditableTextBoxStyle Style = [] {
        auto V = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
        V.SetFont(Font(14))
            .SetForegroundColor(TextPrimary)
            .SetFocusedForegroundColor(TextPrimary)
            .SetBackgroundColor(FLinearColor::White)
            .SetPadding(FMargin(9, 5));
        V.SetBackgroundImageNormal(FSlateColorBrush(Hex(TEXT("141414"))));
        V.SetBackgroundImageHovered(FSlateColorBrush(Hex(TEXT("202020"))));
        // A subtle amber well makes focus visible without washing out the text.
        V.SetBackgroundImageFocused(FSlateColorBrush(Hex(TEXT("3A2F22"))));
        V.SetBackgroundImageReadOnly(FSlateColorBrush(Hex(TEXT("242424"))));
        return V;
    }();
    return Style;
}
TSharedRef<SWidget> Surface(const TSharedRef<SWidget> &Body, FLinearColor Color = Panel,
                            FMargin Padding = FMargin(12))
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
        .BorderBackgroundColor(Color)
        .Padding(Padding)[Body];
}
void Group(const TSharedRef<SVerticalBox> &Box, const FString &Label, const TSharedRef<SWidget> &Body)
{
    Box->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Text(Label, 12, TextMuted, true)];
    Box->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[Body];
}
FString Rate(const FTeleportLogisticsRoute *R)
{
    if (!R)
        return TEXT("—");
    return R->Medium == ETeleportLogisticsMedium::Items
               ? FString::Printf(TEXT("%.0f items/min"), R->UnitsPerMinute)
               : FString::Printf(TEXT("%.1f m³/min"), R->UnitsPerMinute / 1000.0);
}
} // namespace
UTeleportLogisticsWidget::UTeleportLogisticsWidget(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    mSupportsCaching = false;
    mSupportsStacking = false;
    SetIsFocusable(true);
    mUseKeyboard = true;
    mUseMouse = true;
    mCaptureInput = true;
    mDisablePlayerActions = true;
    mDisableBuildGunActions = true;
    mDisablePlayerEquipmentManagement = true;
    mFlushMouseKeysOnOpen = true;
    mDesiredHorizontalAlignment = HAlign_Fill;
    mDesiredVerticalAlignment = VAlign_Fill;
    mDesiredAlignmentSize = FSlateChildSize(ESlateSizeRule::Fill);
}
void UTeleportLogisticsWidget::Init_Implementation()
{
    Super::Init_Implementation();
    InitializeInteraction();
}
void UTeleportLogisticsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics: interaction widget constructed for %s"),
           *GetNameSafe(mInteractObject));
    InitializeInteraction();
}
void UTeleportLogisticsWidget::InitializeInteraction()
{
    auto *Building = Cast<ATeleportLogisticsBuilding>(mInteractObject);
    if (Closing || IsRunningDedicatedServer() || !IsValid(Building))
        return;
    if (Context != Building)
    {
        if (Remote)
            Remote->OnSnapshot.Remove(SnapshotHandle);
        Remote = nullptr;
        Context = Building;
        Data = FTeleportLogisticsSnapshot();
        RouteIndices.Reset();
        SelectedChannel.Invalidate();
        SelectedRoute.Invalidate();
        InitializedSelection = false;
        Dirty = Pending = LastActionFailed = false;
        Confirmation.Empty();
        LastListKey.Empty();
        Page = RoutePage = 0;
        LastPollSent = -100;
        LastReceivedRequestId = 0;
        if (ContentRoot)
            ContentRoot->SetContent(BuildPanel());
    }
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &UTeleportLogisticsWidget::Refresh,
                                               UTeleportLogisticsConfig::RefreshSeconds(this), true, 0.01f);
}
TSharedRef<SWidget> UTeleportLogisticsWidget::RebuildWidget()
{
    SAssignNew(ContentRoot, SBox);
    Context = Cast<ATeleportLogisticsBuilding>(mInteractObject);
    ContentRoot->SetContent(BuildPanel());
    return ContentRoot.ToSharedRef();
}
void UTeleportLogisticsWidget::LoadPresentation()
{
    auto LoadBrush = [](FSlateBrush &Brush, const TCHAR *Path, float Margin) {
        if (auto *Texture = TeleportLogisticsAsset<UTexture2D>(Path))
        {
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.Margin = FMargin(Margin);
        }
    };
    LoadBrush(PlateBrush, TEXT("/TeleportLogistics/UI/T_TeleporterUI_Plate.T_TeleporterUI_Plate"), .10f);
    LoadBrush(MeterBrush, TEXT("/TeleportLogistics/UI/T_TeleporterUI_Meter.T_TeleporterUI_Meter"), .12f);
    const auto *E = Cast<ATeleportLogisticsEndpoint>(Context);
    const FString Kind = !E ? TEXT("Hub")
                            : FString(E->Medium == ETeleportLogisticsMedium::Items ? TEXT("Item") : TEXT("Fluid")) +
                                  (E->Input ? TEXT("Input") : TEXT("Output"));
    LoadBrush(BuildingBrush, *FString::Printf(TEXT("/TeleportLogistics/Icons/T_Teleporter%s_256.T_Teleporter%s_256"), *Kind, *Kind),
              0);
    BuildingBrush.DrawAs = ESlateBrushDrawType::Image;
}
TSharedRef<SWidget> UTeleportLogisticsWidget::Plate(const TSharedRef<SWidget> &Body)
{
    return SNew(SBorder)
        .BorderImage(&PlateBrush)
        .BorderBackgroundColor(FLinearColor(.38f, .38f, .38f, 1.f))
        // The plate art carries a dark band across its bottom edge, drawn un-stretched
        // Margin.Bottom * ImageSize.Y deep. Content has to clear it or the bottom-most
        // row sits on the band, which is what clipped the confirmation buttons. The
        // other three edges are plain art, so they keep their original spacing.
        .Padding(FMargin(20, 20, 20, PlateBrush.Margin.Bottom * PlateBrush.ImageSize.Y + 12))[Body];
}
TSharedRef<SWidget> UTeleportLogisticsWidget::Action(const FString &Label, TFunction<FReply()> Callback,
                                         TFunction<bool()> Enabled, bool Primary, const FString &Hint)
{
    // One Slate layout path in the game and tests; Blueprint button internals
    // impose independent minimum sizes that cannot fit these compact rows.
    return SNew(SButton)
        .ButtonStyle(&ButtonStyle())
        .ContentPadding(FMargin(10, 5))
        .IsEnabled_Lambda([Enabled] { return Enabled(); })
        .ToolTipText(FText::FromString(Hint))
        .ButtonColorAndOpacity(Primary ? FicsitOrange : FLinearColor::White)
        .OnClicked_Lambda([this, Callback, Enabled] {
            return !Closing && Enabled() ? Callback() : FReply::Handled();
        })[Line(Label, 14, TextPrimary)];
}
TSharedRef<SWidget> UTeleportLogisticsWidget::Frame(const TSharedRef<SWidget> &Body, const FString &Title)
{
    // Own the frame geometry while FGInteractWidget owns focus/close lifetime.
    return SNew(SVerticalBox) +
           SVerticalBox::Slot().AutoHeight()[Surface(
               SNew(SHorizontalBox) +
                   SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(16, 0)[Text(Title, 18)] +
                   SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(54).HeightOverride(42)[Action(
                       TEXT("×"), [this] { return Close(); }, [] { return true; }, false,
                       TEXT("Close (Esc)"))]],
               Hex(TEXT("3E3E3E")), FMargin(0))] +
           SVerticalBox::Slot().FillHeight(1)[Body];
}
const FTeleportLogisticsRoute *UTeleportLogisticsWidget::FindRoute(FGuid Id) const
{
    const int32 *Index = RouteIndices.Find(Id);
    return Index && Data.Routes.IsValidIndex(*Index) ? &Data.Routes[*Index] : nullptr;
}
const FTeleportLogisticsRoute *UTeleportLogisticsWidget::Selected() const
{
    return FindRoute(SelectedRoute);
}
const FTeleportLogisticsRoute *UTeleportLogisticsWidget::PreviewRoute() const
{
    const auto *Endpoint = Cast<ATeleportLogisticsEndpoint>(Context);
    if (!Endpoint || !RouteField) return Selected();
    const FString Name = RouteField->GetText().ToString().TrimStartAndEnd();
    return Data.Routes.FindByPredicate([this, Endpoint, &Name](const FTeleportLogisticsRoute &Route) {
        return Route.Channel == SelectedChannel && Route.Medium == Endpoint->Medium &&
            Route.Name.Equals(Name, ESearchCase::IgnoreCase);
    });
}
FString UTeleportLogisticsWidget::ChannelName(FGuid Id) const
{
    const auto *C = Data.Channels.FindByPredicate([Id](const auto &V) { return V.Id == Id; });
    return C ? C->Name : (!Id.IsValid() ? TEXT("Default") : TEXT("Unavailable channel"));
}
FString UTeleportLogisticsWidget::RoutePath(FGuid Id) const
{
    const auto *R = FindRoute(Id);
    return R ? ChannelName(R->Channel) + TEXT(" / ") + R->Name : TEXT("Unassigned");
}
bool UTeleportLogisticsWidget::CanAct() const
{
    return !Closing && !Pending && Data.ContextAvailable && Remote && !NameTooLong();
}
bool UTeleportLogisticsWidget::NameTooLong() const
{
    const auto Over = [](const TSharedPtr<SEditableTextBox> &Field) {
        return Field && Field->GetText().ToString().TrimStartAndEnd().Len() > 64;
    };
    return Over(NameField) || Over(RouteField);
}
bool UTeleportLogisticsWidget::CanManage() const
{
    const auto *R = Selected();
    return CanAct() && Data.HubPowered && R && Context &&
           (R->Channel == Data.Context || !R->Channel.IsValid());
}
bool UTeleportLogisticsWidget::CanApply() const
{
    if (!CanAct() || !Dirty || !NameField || NameField->GetText().ToString().TrimStartAndEnd().IsEmpty())
        return false;
    return Cast<ATeleportLogisticsHub>(Context)
               ? Data.HubPowered
               : RouteField && !RouteField->GetText().ToString().TrimStartAndEnd().IsEmpty();
}
void UTeleportLogisticsWidget::StageChanges()
{
    if (!SyncingFields)
    {
        Dirty = true;
        if (Cast<ATeleportLogisticsEndpoint>(Context) && RouteField)
        {
            const FString Draft = RouteField->GetText().ToString().TrimStartAndEnd();
            const auto *Route = Data.Routes.FindByPredicate([this, &Draft](const auto &R) {
                return R.Channel == SelectedChannel && R.Name.Equals(Draft, ESearchCase::IgnoreCase);
            });
            SelectedRoute = Route ? Route->Id : FGuid();
        }
        Confirmation.Empty();
        UpdateConfirmation();
    }
}
void UTeleportLogisticsWidget::SelectRoute(FGuid Id)
{
    SelectedRoute = Id;
    Page = 0;
    Confirmation.Empty();
    UpdateConfirmation();
    if (const auto *R = Selected())
    {
        SelectedChannel = R->Channel;
        RouteField->SetText(FText::FromString(R->Name));
    }
    LastListKey.Empty();
    RenderLists();
    Refresh();
}
TSharedRef<SWidget> UTeleportLogisticsWidget::ChannelPicker()
{
    return SNew(SComboButton)
        .ButtonStyle(&ButtonStyle())
        .ContentPadding(FMargin(9, 4))
        .IsEnabled_Lambda([this] { return !Pending && Data.ContextAvailable; })
        .OnGetMenuContent_Lambda([this] {
            auto List = SNew(SVerticalBox);
            for (const auto &C : Data.Channels)
                List->AddSlot().AutoHeight()
                    [SNew(SButton)
                         .ButtonStyle(&ButtonStyle())
                         .ContentPadding(FMargin(10, 5))
                         .OnClicked_Lambda([this, Id = C.Id] {
                             SelectedChannel = Id;
                             RoutePage = 0;
                             SelectedRoute.Invalidate();
                             Page = 0;
                             RouteField->SetText(FText::GetEmpty());
                             StageChanges();
                             LastListKey.Empty();
                             RenderLists();
                             Refresh();
                             FSlateApplication::Get().DismissAllMenus();
                             return FReply::Handled();
                         })[Line(C.Name, 15, SelectedChannel == C.Id ? FicsitOrange : TextPrimary)]];
            if (Data.Channels.IsEmpty())
                List->AddSlot().AutoHeight()[Text(TEXT("Connecting…"))];
            return SNew(SBox).WidthOverride(310).MaxDesiredHeight(
                300)[SNew(SScrollBox) + SScrollBox::Slot()[List]];
        })
        .ButtonContent()[Live([this] { return ChannelName(SelectedChannel); })];
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildRoutes()
{
    auto Box = SNew(SVerticalBox);
    Group(Box, TEXT("CHANNEL"), ChannelPicker());
    Box->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Text(TEXT("ROUTES"), 12, TextMuted, true)];
    Box->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[SAssignNew(SearchField, SEditableTextBox)
                                                        .Style(&InputStyle())
                                                        .HintText(FText::FromString(TEXT("Search routes…")))
                                                        .OnTextChanged_Lambda([this](const FText &) {
                                                            LastListKey.Empty();
                                                            RoutePage = 0;
                                                            RenderLists();
                                                        })];
    if (Cast<ATeleportLogisticsHub>(Context))
    {
        // Slate does not clip by default, so a list taller than its slot drew over
        // the pager beneath it. Scrolling contains the overflow and still reaches
        // entries that do not fit.
        Box->AddSlot().FillHeight(1)[SNew(SScrollBox).Clipping(EWidgetClipping::ClipToBounds) +
                                     SScrollBox::Slot()[SAssignNew(RoutesBox, SVerticalBox)]];
        auto Pages = SNew(SHorizontalBox);
        Pages->AddSlot().AutoWidth()[Action(TEXT("Previous"), [this] {
            --RoutePage; LastListKey.Empty(); RenderLists(); return FReply::Handled();
        }, [this] { return RoutePage > 0; })];
        Pages->AddSlot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[Live([this] {
            return FString::Printf(TEXT("%d / %d"), RoutePage + 1, FMath::Max(1, FMath::DivideAndRoundUp(VisibleRouteCount, 4)));
        }, 12)];
        Pages->AddSlot().AutoWidth()[Action(TEXT("Next"), [this] {
            ++RoutePage; LastListKey.Empty(); RenderLists(); return FReply::Handled();
        }, [this] { return (RoutePage + 1) * 4 < VisibleRouteCount; })];
        Box->AddSlot().AutoHeight()[SNew(SBox).Visibility_Lambda([this] {
            return VisibleRouteCount > 4 ? EVisibility::Visible : EVisibility::Collapsed;
        })[Pages]];
    }
    else
        Box->AddSlot().FillHeight(1)[SNew(SScrollBox) +
            SScrollBox::Slot()[SAssignNew(RoutesBox, SVerticalBox)]];
    return Box;
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildMetrics()
{
    auto Metrics = SNew(SHorizontalBox);
    auto Metric = [this](const FString &Label, TFunction<FString()> Value) {
        auto B = SNew(SVerticalBox);
        B->AddSlot().AutoHeight()[Text(Label, 12, Hex(TEXT("5B5B5B")), true)];
        B->AddSlot().AutoHeight().Padding(0, 2)[Live(Value, 16, Hex(TEXT("323232")), true)];
        return SNew(SBox).MinDesiredHeight(76)[SNew(SBorder).BorderImage(&MeterBrush).Padding(FMargin(14, 12))[B]];
    };
    Metrics->AddSlot().FillWidth(1).Padding(
        0, 0, 4, 0)[Metric(TEXT("THROUGHPUT"), [this] { return Rate(PreviewRoute()); })];
    Metrics->AddSlot().FillWidth(1)[Metric(TEXT("STATE"), [this] {
        const auto *R = PreviewRoute();
        if (!R) return RouteField && !RouteField->GetText().ToString().TrimStartAndEnd().IsEmpty()
            ? TEXT("New route") : TEXT("Unassigned");
        return R->Paused ? TEXT("Paused") : TEXT("Active");
    })];
    return Metrics;
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildEndpoint()
{
    const auto *E = Cast<ATeleportLogisticsEndpoint>(Context);
    const bool Fluid = E && E->Medium == ETeleportLogisticsMedium::Fluid;
    auto Left = SNew(SVerticalBox);
    Group(Left, TEXT("ENDPOINT LABEL"), SAssignNew(NameField, SEditableTextBox)
        .Style(&InputStyle()).Text(FText::FromString(Context ? Context->Label : TEXT("")))
        .IsReadOnly_Lambda([this] { return Pending || !Data.ContextAvailable; })
        .OnTextChanged_Lambda([this](const FText &) { StageChanges(); }));
    Left->AddSlot().FillHeight(1)[BuildRoutes()];
    Left->AddSlot().AutoHeight().Padding(0, 8, 0, 4)[Text(TEXT("ROUTE NAME"), 12, TextMuted, true)];
    Left->AddSlot().AutoHeight()[SAssignNew(RouteField, SEditableTextBox)
        .Style(&InputStyle()).HintText(FText::FromString(TEXT("Choose a route or enter a new name")))
        .ToolTipText(FText::FromString(TEXT("Select an existing route above, or enter a new name. Apply changes to save.")))
        .IsReadOnly_Lambda([this] { return Pending || !Data.ContextAvailable; })
        .OnTextChanged_Lambda([this](const FText &) { StageChanges(); })];

    auto Right = SNew(SVerticalBox);
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Text(
        E && E->Input ? TEXT("SEND TO NETWORK") : TEXT("RECEIVE FROM NETWORK"), 12, TextMuted, true)];
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Live([this] {
        return ChannelName(SelectedChannel) + TEXT(" / ") +
            (RouteField && !RouteField->GetText().IsEmpty() ? RouteField->GetText().ToString() : TEXT("Choose a route"));
    }, 15, FicsitOrange, true)];
    Right->AddSlot().AutoHeight()[BuildMetrics()];
    Right->AddSlot().AutoHeight().Padding(0, 4, 0, 12)[Text(TEXT("Inputs merge; outputs share deliveries."), 12, TextMuted)];
    auto Buffer = SNew(SVerticalBox);
    Buffer->AddSlot().AutoHeight()[Text(TEXT("LOCAL BUFFER"), 12, TextMuted, true)];
    Buffer->AddSlot().AutoHeight().Padding(0, 2)[Live([this, Fluid] {
        if (!Data.ContextAvailable) return FString(TEXT("Waiting for server…"));
        return Fluid ? FString::Printf(TEXT("%.1f / %.1f m³"), Data.Buffered / 1000.f, Data.Capacity / 1000.f)
            : FString::Printf(TEXT("%d / %d items"), Data.Buffered, Data.Capacity);
    }, 16, Fluid ? Cyan : TextPrimary, true)];
    Buffer->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(8)[SNew(SProgressBar)
        .Percent_Lambda([this] { return TOptional<float>(Data.Capacity > 0 ? FMath::Clamp(float(Data.Buffered) / Data.Capacity, 0.f, 1.f) : 0.f); })
        .FillColorAndOpacity(Fluid ? Cyan : FicsitOrange)]];
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 4)[Surface(Buffer, Panel, FMargin(8))];
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[Text(TEXT("Drain this buffer before changing routes."), 12, TextMuted)];
    auto Toggle = SNew(SHorizontalBox);
    Toggle->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Live([this] {
        return !Data.ContextAvailable ? TEXT("Connecting…") : Data.EndpointEnabled ? TEXT("Enabled") : TEXT("Standby");
    }, 14)];
    Toggle->AddSlot().AutoWidth().VAlign(VAlign_Center)[Sized(Action(TEXT("Enable / standby"), [this] {
        BeginAction(TEXT("toggle")); return FReply::Handled();
    }, [this] { return CanAct(); }, false, TEXT("Contents are retained while disabled.")))];
    Right->AddSlot().AutoHeight()[Toggle];
    // Second row mirrors the toggle row so both buttons share a width, and the left
    // cell states why the action is unavailable instead of hiding it in a tooltip.
    auto Empty = SNew(SHorizontalBox);
    Empty->AddSlot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock)
        .Font(Font(14))
        .Text_Lambda([this, Fluid] {
            if (!Data.ContextAvailable)
                return FText::FromString(TEXT("Connecting…"));
            if (Data.Buffered <= 0)
                return FText::FromString(TEXT("Nothing buffered"));
            if (Fluid && Data.EndpointEnabled)
                return FText::FromString(TEXT("Disable to flush"));
            return FText::FromString(Fluid ? TEXT("Discards the fluid") : TEXT("Moves items to you"));
        })
        .ColorAndOpacity_Lambda([this, Fluid] {
            const bool Blocked = Fluid && Data.EndpointEnabled && Data.Buffered > 0;
            return FSlateColor(Blocked ? Warning : TextMuted);
        })];
    Empty->AddSlot().AutoWidth().VAlign(VAlign_Center)[Sized(Fluid
        ? Action(TEXT("Flush local buffer"), [this] {
              if (UTeleportLogisticsConfig::ConfirmFluidFlush(this))
                ConfirmAction(TEXT("flush"));
            else
                BeginAction(TEXT("flush"));
            return FReply::Handled();
          }, [this] { return CanAct() && !Data.EndpointEnabled && Data.Buffered > 0; }, false,
              TEXT("Disable the endpoint first. The fluid is destroyed."))
        : Action(TEXT("Take buffered items"), [this] {
              BeginAction(TEXT("take")); return FReply::Handled();
          }, [this] { return CanAct() && Data.Buffered > 0; }, false,
              TEXT("Moves the buffer into your inventory. Whatever does not fit stays here.")))];
    Right->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[Empty];
    Right->AddSlot().FillHeight(1)[SNullWidget::NullWidget];
    return SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.08f).Padding(0, 0, 8, 0)[Plate(Left)] +
        SHorizontalBox::Slot().FillWidth(1)[Plate(Right)];
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildDirectory()
{
    auto Box = SNew(SVerticalBox);
    Box->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Text(TEXT("ENDPOINT DIRECTORY"), 12, TextMuted, true)];
    Box->AddSlot().AutoHeight().Padding(
        0, 0, 0, 10)[SAssignNew(DirectorySearch, SEditableTextBox)
                         .Style(&InputStyle())
                         .HintText(FText::FromString(TEXT("Filter endpoints on this page…")))
                         .IsEnabled_Lambda([this] { return Data.HubPowered && SelectedRoute.IsValid(); })
                         .OnTextChanged_Lambda([this](const FText &) { RenderLists(); })];
    auto Head = SNew(SHorizontalBox);
    Head->AddSlot().FillWidth(.38f)[Text(TEXT("ENDPOINT"), 11, TextMuted, true)];
    Head->AddSlot().FillWidth(.15f)[Text(TEXT("TYPE"), 11, TextMuted, true)];
    Head->AddSlot().FillWidth(.24f)[Text(TEXT("BUFFER"), 11, TextMuted, true)];
    Head->AddSlot().FillWidth(.23f)[Text(TEXT("POSITION (m)"), 11, TextMuted, true)];
    Box->AddSlot().AutoHeight()[Surface(Head, PanelRaised, FMargin(10, 6))];
    Box->AddSlot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Directory, SVerticalBox)]];
    auto Pages = SNew(SHorizontalBox);
    Pages->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Live(
        [this] {
            return FString::Printf(TEXT("Page %d / %d · %d endpoints"), Page + 1,
                                   FMath::Max(1, FMath::DivideAndRoundUp(Data.TotalEndpoints, 64)),
                                   Data.TotalEndpoints);
        },
        12, TextMuted)];
    Pages->AddSlot().AutoWidth().Padding(0, 0, 4, 0)[Action(
        TEXT("Prev"),
        [this] {
            --Page;
            Refresh();
            return FReply::Handled();
        },
        [this] { return CanAct() && Data.HubPowered && Page > 0; })];
    Pages->AddSlot().AutoWidth()[Action(
        TEXT("Next"),
        [this] {
            ++Page;
            Refresh();
            return FReply::Handled();
        },
        [this] { return CanAct() && Data.HubPowered && (Page + 1) * 64 < Data.TotalEndpoints; })];
    Box->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[SNew(SBox).Visibility_Lambda([this] {
        return Data.HubPowered && SelectedRoute.IsValid() && Data.TotalEndpoints > 64
            ? EVisibility::Visible : EVisibility::Collapsed;
    })[Pages]];
    return Box;
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildHub()
{
    auto Right = SNew(SVerticalBox);
    Group(Right, TEXT("HUB / CHANNEL NAME"), SAssignNew(NameField, SEditableTextBox)
        .Style(&InputStyle()).Text(FText::FromString(Context ? Context->Label : TEXT("")))
        .IsReadOnly_Lambda([this] { return Pending || !Data.HubPowered; })
        .OnTextChanged_Lambda([this](const FText &) { StageChanges(); }));
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Live([this] {
        const auto *R = Selected();
        return R ? R->Name + TEXT(" · ") + Rate(R) + FString::Printf(TEXT(" · %d IN / %d OUT · %s"),
            R->Inputs, R->Outputs, R->Paused ? TEXT("Paused") : TEXT("Active")) : TEXT("Select a route to manage it.");
    }, 14, FicsitOrange)];
    auto Rename = SNew(SHorizontalBox);
    Rename->AddSlot().FillWidth(1).Padding(0, 0, 6, 0)[SAssignNew(RouteField, SEditableTextBox)
        .Style(&InputStyle()).HintText(FText::FromString(TEXT("Route name")))
        .IsReadOnly_Lambda([this] { return !CanManage(); })];
    Rename->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Action(TEXT("Rename"), [this] {
        BeginAction(TEXT("rename")); return FReply::Handled();
    }, [this] { return CanManage() && !RouteField->GetText().IsEmpty(); })];
    Rename->AddSlot().AutoWidth()[Action(TEXT("Pause / resume"), [this] {
        BeginAction(TEXT("pause")); return FReply::Handled();
    }, [this] { return CanManage(); })];
    Right->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[SNew(SBox).Visibility_Lambda([this] {
        return Data.HubPowered && SelectedRoute.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
    })[Rename]];
    Right->AddSlot().FillHeight(1)[BuildDirectory()];
    auto Restricted = SNew(SHorizontalBox);
    Restricted->AddSlot().AutoWidth().Padding(0, 0, 8, 0)[Action(TEXT("Reset empty fluid route"), [this] {
        ConfirmAction(TEXT("reset-fluid")); return FReply::Handled();
    }, [this] { return CanManage() && Selected()->CanResetFluid; })];
    Restricted->AddSlot().AutoWidth()[Action(TEXT("Delete unused route"), [this] {
        ConfirmAction(TEXT("delete")); return FReply::Handled();
    }, [this] { return CanManage() && Selected()->Inputs + Selected()->Outputs == 0; })];
    Right->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[SNew(SBox).Visibility_Lambda([this] {
        return CanManage() ? EVisibility::Visible : EVisibility::Collapsed;
    })[Restricted]];
    return SNew(SHorizontalBox) +
        SHorizontalBox::Slot().FillWidth(.30f).Padding(0, 0, 8, 0)[Plate(BuildRoutes())] +
        SHorizontalBox::Slot().FillWidth(.70f)[Plate(Right)];
}
TSharedRef<SWidget> UTeleportLogisticsWidget::BuildPanel()
{
    LoadPresentation();
    LastListKey.Empty();
    Directory = SNew(SVerticalBox);
    const bool IsHub = Cast<ATeleportLogisticsHub>(Context) != nullptr;
    const auto *E = Cast<ATeleportLogisticsEndpoint>(Context);
    const FString Title = IsHub                                   ? TEXT("Teleporter Hub")
                          : E && E->Medium == ETeleportLogisticsMedium::Fluid ? TEXT("Fluid Teleporter")
                                                                  : TEXT("Item Teleporter");
    auto Layout = SNew(SVerticalBox);
    auto Identity = SNew(SHorizontalBox);
    Identity->AddSlot().AutoWidth().Padding(
        0, 0, 14, 0)[SNew(SBox).WidthOverride(40).HeightOverride(40)[SNew(SImage).Image(&BuildingBrush)]];
    Identity->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Live(
        [this, IsHub, E] {
            const FString Id = IsHub ? TEXT("Teleporter Hub")
                                     : FString(E && E->Medium == ETeleportLogisticsMedium::Fluid ? TEXT("Fluid Teleporter")
                                                                                     : TEXT("Item Teleporter")) +
                                           (E && !E->Input ? TEXT(" (Output)") : TEXT(" (Input)"));
            // BeginPlay seeds a building's label from its display name, so the second
            // line repeats the first until the player renames it.
            const FString Detail = IsHub ? Data.ContextLabel : RoutePath(Data.AssignedRoute);
            return Detail.IsEmpty() || Detail == Id ? Id : Id + TEXT("\n") + Detail;
        },
        16)];
    Identity->AddSlot().AutoWidth().VAlign(VAlign_Center)[Live(
        [this, IsHub]() -> FString {
            if (!Data.ContextAvailable)
                return FString(TEXT("CONNECTING"));
            return IsHub ? (Data.HubPowered ? TEXT("POWERED") : TEXT("UNPOWERED"))
                         : (Data.EndpointEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
        },
        14, FicsitOrange, true)];
    Layout->AddSlot().AutoHeight()[Surface(Identity, PanelRaised, FMargin(16, 10))];
    if (IsHub)
        Layout->AddSlot().AutoHeight()[SNew(SBox).Visibility_Lambda([this] {
            return Data.ContextAvailable && !Data.HubPowered ? EVisibility::Visible : EVisibility::Collapsed;
        })[Surface(Text(TEXT("Hub unpowered. Connect power to manage routes; transport continues."),
                        12, Warning),
                   Hex(TEXT("393225")))]];
    const auto Body = IsHub ? BuildHub() : BuildEndpoint();
    // The only persistent scroll area is the data list. Do not nest viewport
    // scroll boxes around a fixed-height body: that clips fields and adds axes.
    Layout->AddSlot().FillHeight(1).Padding(0, 8)[Body];
    auto Footer = SNew(SHorizontalBox);
    Footer->AddSlot().AutoWidth().Padding(0, 0, 14, 0)[Action(
        IsHub ? TEXT("Apply hub name") : TEXT("Apply changes"),
        [this] {
            BeginAction(TEXT("apply"));
            return FReply::Handled();
        },
        [this] { return CanApply(); }, true)];
    Footer->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Live(
        [this] {
            return !Data.ContextAvailable ? TEXT("Connecting to server…")
                   : Pending ? TEXT("Validating with server…")
                   : NameTooLong() ? TEXT("Names can contain up to 64 characters.")
                   : Dirty ? TEXT("Unsaved changes · Apply to save")
                           : TEXT("No unsaved changes");
        },
        12, TextMuted)];
    Footer->AddSlot().AutoWidth()[Action(
        TEXT("Refresh"),
        [this] {
            Refresh();
            return FReply::Handled();
        },
        [this] { return !Pending; })];
    Layout->AddSlot().AutoHeight()[Surface(Footer, PanelRaised, FMargin(14, 10))];
    Layout->AddSlot().AutoHeight()[Surface(
        SAssignNew(Status, STextBlock)
            .Font(Font(13))
            .ColorAndOpacity_Lambda([this] { return LastActionFailed ? ErrorColor : TextMuted; })
            .AutoWrapText(true)
            .Text(FText::FromString(TEXT("Connecting to Teleporter…"))),
        Panel, FMargin(14, 8))];
    const auto Window = Frame(Surface(Layout, Panel, FMargin(8)), Title);
    Window->SetEnabled(TAttribute<bool>::CreateLambda([this] { return Confirmation.IsEmpty(); }));
    auto Overlay = SNew(SOverlay) + SOverlay::Slot()[Window] +
                   SOverlay::Slot()[SAssignNew(ConfirmationHost, SBox).Visibility(EVisibility::Collapsed)];
    RenderLists();
    return SNew(SOverlay) +
           SOverlay::Slot()[Surface(SNullWidget::NullWidget, FLinearColor(0, 0, 0, .38f), FMargin(0))] +
           SOverlay::Slot()
               .HAlign(HAlign_Center)
               .VAlign(VAlign_Center)
               .Padding(48)[SNew(SScaleBox)
                    .Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                    [SNew(SBox).WidthOverride(IsHub ? 1080 : 940).HeightOverride(IsHub ? 760 : 740)[Overlay]]];
}
void UTeleportLogisticsWidget::BeginAction(const FString &ActionName)
{
    if (!CanAct())
        return;
    FString Name = NameField ? NameField->GetText().ToString().TrimStartAndEnd() : FString();
    FString Route = RouteField ? RouteField->GetText().ToString().TrimStartAndEnd() : FString();
    if ((ActionName == TEXT("apply") && !CanApply()) ||
        (ActionName != TEXT("apply") && Cast<ATeleportLogisticsHub>(Context) && !CanManage()))
        return;
    Pending = true;
    PendingSince = FPlatformTime::Seconds();
    PendingAction = ActionName;
    LastActionFailed = false;
    Status->SetText(FText::FromString(TEXT("Waiting for server confirmation…")));
    if (auto *Hub = Cast<ATeleportLogisticsHub>(Context))
    {
        if (ActionName == TEXT("apply"))
            Remote->ServerRenameHub(Hub, Name);
        else
            Remote->ServerControl(Hub, SelectedRoute, ActionName, Route);
    }
    else if (auto *Endpoint = Cast<ATeleportLogisticsEndpoint>(Context))
    {
        if (ActionName == TEXT("apply"))
            Remote->ServerConfigure(Endpoint, SelectedChannel, Route, Name);
        else if (ActionName == TEXT("toggle"))
            Remote->ServerToggle(Endpoint);
        else if (ActionName == TEXT("flush"))
            Remote->ServerFlushFluid(Endpoint);
        else if (ActionName == TEXT("take"))
            Remote->ServerTakeBuffer(Endpoint);
    }
    Confirmation.Empty();
    UpdateConfirmation();
    Refresh();
}
void UTeleportLogisticsWidget::ConfirmAction(const FString &ActionName)
{
    Confirmation = ActionName;
    ConfirmationRoute = SelectedRoute;
    UpdateConfirmation();
}
void UTeleportLogisticsWidget::UpdateConfirmation()
{
    if (!ConfirmationHost)
        return;
    if (Confirmation.IsEmpty())
    {
        ConfirmationHost->SetVisibility(EVisibility::Collapsed);
        ConfirmationHost->SetContent(SNullWidget::NullWidget);
        return;
    }
    const FString ActionName = Confirmation;
    auto Content = SNew(SVerticalBox);
    Content->AddSlot().AutoHeight().Padding(
        0, 0, 0, 14)[Text(ActionName == TEXT("flush")    ? TEXT("Flush local fluid buffer?")
                          : ActionName == TEXT("delete") ? TEXT("Delete unused route?")
                                                         : TEXT("Reset fluid type?"),
                          23, TextPrimary, true)];
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 20)[Text(
        ActionName == TEXT("flush") ? TEXT("This permanently discards the fluid in this endpoint. Attached "
                                           "pipes are untouched. The endpoint must remain disabled.")
        : ActionName == TEXT("delete")
            ? TEXT("The selected route will be removed. No endpoints may still be attached.")
            : TEXT("Every endpoint must be disabled and empty. Connected pipes must be empty or contain the "
                   "new fluid."),
        16)];
    Content->AddSlot().AutoHeight()[SNew(SHorizontalBox) +
                                    SHorizontalBox::Slot().FillWidth(1).Padding(
                                        0, 0, 10, 0)[Action(TEXT("Cancel"),
                                                            [this] {
                                                                Confirmation.Empty();
                                                                UpdateConfirmation();
                                                                return FReply::Handled();
                                                            })] +
                                    SHorizontalBox::Slot().FillWidth(1)[Action(
                                        TEXT("Confirm"),
                                        [this, ActionName] {
                                            BeginAction(ActionName);
                                            return FReply::Handled();
                                        },
                                        [this, ActionName] {
                                            if (ActionName == TEXT("flush"))
                                                return CanAct() && !Data.EndpointEnabled && Data.Buffered > 0;
                                            return CanManage() && ConfirmationRoute == SelectedRoute &&
                                                   (ActionName == TEXT("delete")
                                                        ? Selected()->Inputs + Selected()->Outputs == 0
                                                        : Selected()->CanResetFluid);
                                        },
                                        true)]];
    ConfirmationHost->SetVisibility(EVisibility::Visible);
    ConfirmationHost->SetContent(
        SNew(SOverlay) +
        SOverlay::Slot()[Surface(SNullWidget::NullWidget, FLinearColor(0, 0, 0, .82f), FMargin(0))] +
        SOverlay::Slot()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(20)[SNew(SBox).WidthOverride(480)[Plate(Content)]]);
}
void UTeleportLogisticsWidget::Refresh()
{
    if (Closing)
        return;
    if (Pending && FPlatformTime::Seconds() - PendingSince > 8)
    {
        Pending = false;
        LastActionFailed = true;
        if (Status)
            Status->SetText(
                FText::FromString(TEXT("No server confirmation received. Refresh and try again.")));
    }
    if (!IsValid(Context))
    {
        Close();
        return;
    }
    if (!Remote)
    {
        auto *PC = Cast<AFGPlayerController>(GetOwningPlayer());
        if (PC)
            Remote = Cast<UTeleportLogisticsRemoteCall>(PC->GetRemoteCallObjectOfClass(UTeleportLogisticsRemoteCall::StaticClass()));
        if (!Remote)
        {
            if (Status)
                Status->SetText(FText::FromString(TEXT("Waiting for the teleporter connection…")));
            return;
        }
        SnapshotHandle = Remote->OnSnapshot.AddUObject(this, &UTeleportLogisticsWidget::Receive);
        UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics: interaction connection ready for %s"), *GetNameSafe(Context));
    }
    const double Now = FPlatformTime::Seconds();
    if (Now - LastPollSent < (Data.ContextAvailable && !Pending ? 0.9 : 0.30))
        return;
    LastPollSent = Now;
    NextRequestId = Remote->AllocateSnapshotRequest();
    Remote->ServerSnapshot(Context, SelectedRoute, Page, NextRequestId);
}
void UTeleportLogisticsWidget::Receive(const FTeleportLogisticsSnapshot &Snapshot)
{
    if (Closing || !IsValid(Context) || (Snapshot.ContextActor != Context && Snapshot.Context != Context->TeleporterId) ||
        Snapshot.RequestId <= LastReceivedRequestId)
        return;
    if (!Status || !NameField || !RouteField)
        return;
    LastReceivedRequestId = Snapshot.RequestId;
    const bool Completed = Pending && Snapshot.MutationRevision != Data.MutationRevision;
    Data = Snapshot;
    Page = Data.Page;
    RouteIndices.Reset();
    for (int32 I = 0; I < Data.Routes.Num(); ++I)
        RouteIndices.Add(Data.Routes[I].Id, I);
    if (Completed)
    {
        Pending = false;
        LastActionFailed = !Data.MutationSucceeded;
        if (Data.MutationSucceeded && PendingAction == TEXT("apply"))
        {
            Dirty = false;
            InitializedSelection = false;
        }
        Status->SetText(FText::FromString(Data.Message));
    }
    if (!InitializedSelection && Data.ContextAvailable)
    {
        SyncingFields = true;
        if (Cast<ATeleportLogisticsEndpoint>(Context))
        {
            SelectedRoute = Data.AssignedRoute;
            if (const auto *R = Selected())
            {
                SelectedChannel = R->Channel;
                RouteField->SetText(FText::FromString(R->Name));
            }
        }
        else
            SelectedChannel = Data.Context;
        NameField->SetText(FText::FromString(Data.ContextLabel));
        SyncingFields = false;
        InitializedSelection = true;
    }
    if (!Pending && !LastActionFailed)
        Status->SetText(FText::FromString(Data.Message));
    RenderLists();
}
void UTeleportLogisticsWidget::RenderLists()
{
    if (!RoutesBox || !Directory)
        return;
    const FString Search = SearchField ? SearchField->GetText().ToString() : FString();
    const auto *Endpoint = Cast<ATeleportLogisticsEndpoint>(Context);
    TArray<FGuid> Visible;
    VisibleRouteCount = 0;
    FString Key = SelectedChannel.ToString() + Search + (Data.ContextAvailable ? TEXT("ready") : TEXT("waiting"));
    for (const auto &R : Data.Routes)
    {
        if (R.Channel != SelectedChannel || (Endpoint && R.Medium != Endpoint->Medium) ||
            (!Search.IsEmpty() && !R.Name.Contains(Search, ESearchCase::IgnoreCase)))
            continue;
        Visible.Add(R.Id);
        Key += R.Id.ToString() + R.Name;
    }
    VisibleRouteCount = Visible.Num();
    if (Cast<ATeleportLogisticsHub>(Context))
    {
        RoutePage = FMath::Clamp(RoutePage, 0, FMath::Max(0, (Visible.Num() - 1) / 4));
        const int32 Start = RoutePage * 4;
        const int32 Count = FMath::Min(4, Visible.Num() - Start);
        TArray<FGuid> PageIds;
        for (int32 I = 0; I < Count; ++I) PageIds.Add(Visible[Start + I]);
        Visible = MoveTemp(PageIds);
        Key += FString::FromInt(RoutePage);
    }
    // Snapshot metrics update through attributes. Rebuild rows only if their
    // membership/order changes, preserving focus and scroll while polling.
    if (Key != LastListKey)
    {
        LastListKey = Key;
        RoutesBox->ClearChildren();
        for (const FGuid Id : Visible)
        {
            auto Row = SNew(SVerticalBox);
            Row->AddSlot()
                .AutoHeight()[SNew(STextBlock)
                                  .Font(Font(14))
                                  .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                                  .Text_Lambda([this, Id] {
                                      const auto *R = FindRoute(Id);
                                      return FText::FromString(R ? R->Name : TEXT(""));
                                  })
                                  .ToolTipText_Lambda([this, Id] { return FText::FromString(RoutePath(Id)); })
                                  .ColorAndOpacity_Lambda([this, Id] {
                                      return Id == SelectedRoute ? FicsitOrange : TextPrimary;
                                  })];
            Row->AddSlot().AutoHeight().Padding(0, 3, 0, 0)[Live(
                [this, Id] {
                    const auto *R = FindRoute(Id);
                    return R ? FString::Printf(TEXT("%d in / %d out · "), R->Inputs, R->Outputs) +
                        Rate(R) + (R->Paused ? TEXT(" · Paused") : TEXT("")) : TEXT("");
                },
                12, TextMuted)];
            RoutesBox->AddSlot().AutoHeight().Padding(
                0, 0, 0, 3)[SNew(SButton)
                                .ButtonStyle(&ButtonStyle())
                                .ContentPadding(FMargin(10, 5))
                                .HAlign(HAlign_Fill)
                                .IsEnabled_Lambda([this] { return !Pending && Confirmation.IsEmpty(); })
                                .OnClicked_Lambda([this, Id] {
                                    SelectRoute(Id);
                                    return FReply::Handled();
                                })[SNew(SBorder)
                                       .BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
                                       .BorderBackgroundColor_Lambda([this, Id] {
                                           return Id == SelectedRoute ? PanelSelected : PanelRaised;
                                       })
                                       .Padding(2)[Row]]];
        }
        if (Visible.IsEmpty())
            RoutesBox->AddSlot().AutoHeight().Padding(10, 24)[Text(
                Data.ContextAvailable
                    ? Search.IsEmpty()
                          ? (Cast<ATeleportLogisticsHub>(Context) ? TEXT("No routes in this channel. Assign an endpoint to create one.") : TEXT("No routes yet. Enter a route name below."))
                          : TEXT("No matching routes. Clear the search or enter a new route name.")
                    : TEXT("Connecting to the teleporter network…"),
                14, TextMuted)];
    }
    Directory->ClearChildren();
    if (!Cast<ATeleportLogisticsHub>(Context))
        return;
    if (!Data.HubPowered || !SelectedRoute.IsValid())
    {
        Directory->AddSlot().AutoHeight().Padding(16, 32)[Text(
            !Data.HubPowered
                ? TEXT("Power this hub to view its endpoint directory. Transport continues without power.")
                : TEXT("Select a route to see its endpoints and coordinates."),
            15, TextMuted)];
        return;
    }
    const FString Filter = DirectorySearch ? DirectorySearch->GetText().ToString() : FString();
    int32 Shown = 0;
    for (const auto &E : Data.Endpoints)
    {
        if (E.Route != SelectedRoute ||
            (!Filter.IsEmpty() && !E.Name.Contains(Filter, ESearchCase::IgnoreCase) &&
             !E.RoutePath.Contains(Filter, ESearchCase::IgnoreCase)))
            continue;
        auto Row = SNew(SHorizontalBox);
        auto Name = SNew(SVerticalBox);
        Name->AddSlot().AutoHeight()[Line(E.Name, 15)];
        Name->AddSlot().AutoHeight().Padding(0, 3, 0, 0)[Line(E.RoutePath, 11, TextMuted)];
        Row->AddSlot().FillWidth(.38f).VAlign(VAlign_Center).Padding(0, 0, 6, 0)[Name];
        Row->AddSlot().FillWidth(.15f).VAlign(
            VAlign_Center)[Text(FString(E.Input ? TEXT("IN") : TEXT("OUT")) + TEXT("\n") +
                                    (E.Medium == ETeleportLogisticsMedium::Fluid ? TEXT("Fluid") : TEXT("Items")),
                                12, E.Medium == ETeleportLogisticsMedium::Fluid ? Cyan : TextPrimary)];
        Row->AddSlot().FillWidth(.24f).VAlign(VAlign_Center)[Text(
            FString(E.Enabled ? TEXT("Enabled") : TEXT("Disabled")) + TEXT("\n") +
                (E.Medium == ETeleportLogisticsMedium::Fluid ? FString::Printf(TEXT("%.1f m³"), E.Buffered / 1000.f)
                                                 : FString::Printf(TEXT("%d items"), E.Buffered)),
            12, E.Enabled ? TextPrimary : TextMuted)];
        Row->AddSlot().FillWidth(.23f).VAlign(
            VAlign_Center)[Text(FString::Printf(TEXT("X %.0f  Y %.0f\nZ %.0f"), E.Location.X / 100,
                                                E.Location.Y / 100, E.Location.Z / 100),
                                11, TextMuted)];
        Directory->AddSlot().AutoHeight().Padding(
            0, 0, 0, 2)[Surface(Row, Shown++ % 2 ? Hex(TEXT("292929")) : PanelRaised, FMargin(10, 12))];
    }
    if (!Shown)
        Directory->AddSlot().AutoHeight().Padding(
            14, 24)[Text(Filter.IsEmpty() ? TEXT("No endpoints are attached to this route.")
                                          : TEXT("No endpoints on this page match your filter."),
                         14, TextMuted)];
}
FReply UTeleportLogisticsWidget::Close()
{
    if (Closing)
        return FReply::Handled();
    Closing = true;
    StopPolling();

    // PopWidget is the Blueprint presentation teardown used by the game itself.
    // RemoveInteractWidget alone only drops stack/input ownership and leaves
    // the visible window in BP_GameUI's hierarchy.
    bool Popped = false;
    if (auto *PC = GetOwningPlayer())
        if (auto *HUD = Cast<AFGHUD>(PC->GetHUD()))
            if (auto *GameUI = HUD->GetGameUI())
                Popped = GameUI->PopWidget(this);

    // Also make a retained Slate reference inert immediately. Destruct can be
    // delayed by focus paths/animations, and the owning HUD may already be gone.
    SetVisibility(ESlateVisibility::Collapsed);
    if (ContentRoot)
    {
        ContentRoot->SetVisibility(EVisibility::Collapsed);
        ContentRoot->SetContent(SNullWidget::NullWidget);
    }
    RemoveFromParent();
    UE_LOG(LogTeleportLogistics, Verbose, TEXT("TeleportLogistics: closed interaction for %s, native pop=%s"),
           *GetNameSafe(mInteractObject), Popped ? TEXT("true") : TEXT("false"));
    return FReply::Handled();
}
FReply UTeleportLogisticsWidget::NativeOnPreviewKeyDown(const FGeometry &Geometry, const FKeyEvent &Key)
{
    // Preview runs before editable fields can consume Escape to cancel editing.
    if (Key.GetKey() == EKeys::Escape)
        return Close();
    return Super::NativeOnPreviewKeyDown(Geometry, Key);
}
FReply UTeleportLogisticsWidget::NativeOnKeyDown(const FGeometry &Geometry, const FKeyEvent &Key)
{
    if (Key.GetKey() == EKeys::Escape)
        return Close();
    return Super::NativeOnKeyDown(Geometry, Key);
}
void UTeleportLogisticsWidget::OnEscapePressed_Implementation()
{
    Close();
}
void UTeleportLogisticsWidget::StopPolling()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
    if (Remote)
        Remote->OnSnapshot.Remove(SnapshotHandle);
    Remote = nullptr;
    SnapshotHandle.Reset();
}
void UTeleportLogisticsWidget::NativeDestruct()
{
    Closing = true;
    StopPolling();
    Super::NativeDestruct();
}
void UTeleportLogisticsWidget::ReleaseSlateResources(bool Children)
{
    Super::ReleaseSlateResources(Children);
    ContentRoot.Reset();
    RoutesBox.Reset();
    Directory.Reset();
    NameField.Reset();
    RouteField.Reset();
    SearchField.Reset();
    Status.Reset();
    DirectorySearch.Reset();
    ConfirmationHost.Reset();
}
