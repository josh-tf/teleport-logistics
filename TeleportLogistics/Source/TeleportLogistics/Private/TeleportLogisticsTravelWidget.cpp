#include "TeleportLogisticsTravel.h"
#include "TeleportLogisticsAsset.h"
#include "FGPlayerController.h"
#include "FGHUD.h"
#include "UI/FGGameUI.h"
#include "Engine/Font.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "InputCoreTypes.h"
#include "FGIconDatabaseSubsystem.h"
#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"

namespace
{
FSlateFontInfo TravelFont(int32 Size)
{
    static TStrongObjectPtr<UFont> Family(
        LoadObject<UFont>(nullptr, TEXT("/Game/FactoryGame/Interface/Font/DescriptionText.DescriptionText")));
    return Family.IsValid() ? FSlateFontInfo(Family.Get(), Size * .9f, TEXT("Regular"))
                            : FCoreStyle::GetDefaultFontStyle("Regular", Size * .9f);
}
TSharedRef<STextBlock> Caption(const FString &S, int32 Size = 14)
{
    return SNew(STextBlock)
        .Text(FText::FromString(S))
        .Font(TravelFont(Size))
        .ColorAndOpacity(FLinearColor(.85, .85, .85))
        .AutoWrapText(false)
        .OverflowPolicy(ETextOverflowPolicy::Ellipsis);
}
TSharedRef<SWidget> Back(const TSharedRef<SWidget> &W, FLinearColor C = FLinearColor(.035, .035, .035))
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(C)
        .Padding(14)[W];
}
const FButtonStyle &TravelButton()
{
    static FButtonStyle Style = [] {
        FButtonStyle V;
        V.SetNormal(FSlateColorBrush(FLinearColor(.033, .033, .033)));
        V.SetHovered(FSlateColorBrush(FLinearColor(.08, .08, .08)));
        V.SetPressed(FSlateColorBrush(FLinearColor(.5, .22, .045)));
        V.SetDisabled(FSlateColorBrush(FLinearColor(.016, .016, .016)));
        V.SetNormalPadding(FMargin(12, 7));
        V.SetPressedPadding(FMargin(12, 7));
        return V;
    }();
    return Style;
}
const FEditableTextBoxStyle &TravelInput()
{
    static FEditableTextBoxStyle Style = [] {
        auto V = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
        V.SetFont(TravelFont(14))
            .SetForegroundColor(FLinearColor(.8, .8, .8))
            .SetFocusedForegroundColor(FLinearColor::White)
            .SetPadding(FMargin(10, 7));
        V.SetBackgroundImageNormal(FSlateColorBrush(FLinearColor(.007, .007, .007)));
        V.SetBackgroundImageHovered(FSlateColorBrush(FLinearColor(.015, .015, .015)));
        V.SetBackgroundImageFocused(FSlateColorBrush(FLinearColor(.043, .028, .016)));
        return V;
    }();
    return Style;
}

} // namespace
UTeleportLogisticsTravelWidget::UTeleportLogisticsTravelWidget(const FObjectInitializer &I) : Super(I)
{
    mSupportsCaching = false;
    mSupportsStacking = false;
    SetIsFocusable(true);
    mUseKeyboard = mUseMouse = mCaptureInput = mDisablePlayerActions = mDisableBuildGunActions =
        mDisablePlayerEquipmentManagement = mFlushMouseKeysOnOpen = true;
    mDesiredHorizontalAlignment = HAlign_Fill;
    mDesiredVerticalAlignment = VAlign_Fill;
    mDesiredAlignmentSize = FSlateChildSize(ESlateSizeRule::Fill);
}
void UTeleportLogisticsTravelWidget::Init_Implementation()
{
    Super::Init_Implementation();
    Start();
}
void UTeleportLogisticsTravelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Start();
}
void UTeleportLogisticsTravelWidget::Start()
{
    Source = Cast<ATeleportLogisticsTravelHub>(mInteractObject);
    if (Name && Source)
        Name->SetText(FText::FromString(Source->HubName));
    if (!Closing && Source && GetWorld())
        GetWorld()->GetTimerManager().SetTimer(PollTimer, this, &UTeleportLogisticsTravelWidget::Poll, 1.f, true, .01f);
}
TSharedRef<SWidget> UTeleportLogisticsTravelWidget::RebuildWidget()
{
    Source = Cast<ATeleportLogisticsTravelHub>(mInteractObject);
    PlateTexture = TeleportLogisticsAsset<UTexture2D>(TEXT("/TeleportLogistics/UI/T_TeleporterUI_Plate.T_TeleporterUI_Plate"));
    FallbackTexture = TeleportLogisticsAsset<UTexture2D>(
        TEXT("/TeleportLogistics/Icons/T_TeleporterTravelHub_256.T_TeleporterTravelHub_256"));
    FallbackBrush.SetResourceObject(FallbackTexture);
    FallbackBrush.ImageSize = FallbackTexture ? FVector2D(FallbackTexture->GetSizeX(), FallbackTexture->GetSizeY())
                                              : FVector2D(40);
    FallbackBrush.DrawAs = ESlateBrushDrawType::Image;
    PlateBrush.SetResourceObject(PlateTexture);
    PlateBrush.DrawAs = ESlateBrushDrawType::Box;
    PlateBrush.Margin = FMargin(.1f);
    auto Layout = SNew(SVerticalBox);
    auto Header = SNew(SHorizontalBox);
    Header->AddSlot().FillWidth(1)[Caption(TEXT("Personnel Teleporter"), 19)];
    Header->AddSlot().AutoWidth()[SNew(SButton).ButtonStyle(&TravelButton()).OnClicked_Lambda([this] {
        return Close();
    })[Caption(TEXT("Close  [Esc]"))]];
    Layout->AddSlot().AutoHeight()[Back(Header, FLinearColor(.08, .08, .08))];
    auto Rename = SNew(SHorizontalBox);
    Rename->AddSlot().FillWidth(1).Padding(
        0, 0, 10, 0)[SAssignNew(Name, SEditableTextBox)
                         .Style(&TravelInput())
                         .Font(TravelFont(14))
                         .Text(FText::FromString(Source ? Source->HubName : TEXT("")))
                         .HintText(FText::FromString(TEXT("Name this teleporter")))];
    Rename->AddSlot().AutoWidth()[SNew(SButton)
                                      .ButtonStyle(&TravelButton())
                                      .IsEnabled_Lambda([this] { return Remote && !Pending; })
                                      .OnClicked_Lambda([this] {
                                          Remote->ServerRename(Source, Name->GetText().ToString().Left(64));
                                          return FReply::Handled();
                                      })[Caption(TEXT("Save name"))]];
    Rename->AddSlot().AutoWidth().Padding(8, 0, 0, 0)[SNew(SButton)
                                                          .ButtonStyle(&TravelButton())
                                                          .IsEnabled_Lambda([this] { return !Pending; })
                                                          .OnClicked_Lambda([this] {
                                                              ToggleIcons();
                                                              return FReply::Handled();
                                                          })[Caption(TEXT("Choose icon"))]];
    Layout->AddSlot().AutoHeight().Padding(0, 10)[Rename];
    Layout->AddSlot().AutoHeight().Padding(
        0, 0, 0, 8)[Caption(TEXT("DESTINATIONS   ·   Coordinates in metres   ·   50 MW at each end"), 12)];
    Layout->AddSlot().AutoHeight().Padding(
        0, 0, 0, 10)[SAssignNew(Search, SEditableTextBox)
                         .Style(&TravelInput())
                         .Font(TravelFont(14))
                         .HintText(FText::FromString(TEXT("Search destination names…")))
                         .OnTextChanged_Lambda([this](const FText &) {
                             if (PickingIcons)
                             {
                                 // Collapse a burst of typing into one rebuild; four
                                 // hundred cells is too much to reconstruct per key.
                                 if (auto *World = GetWorld())
                                     World->GetTimerManager().SetTimer(
                                         IconSearchTimer, this, &UTeleportLogisticsTravelWidget::ShowIcons, .2f,
                                         false);
                                 return;
                             }
                             Page = 0;
                             Selected.Invalidate();
                             LastList.Empty();
                         })];
    Layout->AddSlot().FillHeight(
        1)[SNew(SBorder)
               .BorderImage(&PlateBrush)
               .Padding(16)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Rows, SVerticalBox)]]];
    auto Footer = SNew(SHorizontalBox);
    Footer->AddSlot().AutoWidth()[SNew(SButton)
                                      .ButtonStyle(&TravelButton())
                                      .Visibility_Lambda([this] {
                                          return PickingIcons ? EVisibility::Collapsed : EVisibility::Visible;
                                      })
                                      .IsEnabled_Lambda([this] { return Page > 0 && !Pending; })
                                      .OnClicked_Lambda([this] {
                                          --Page;
                                          Selected.Invalidate();
                                          LastList.Empty();
                                          return FReply::Handled();
                                      })[Caption(TEXT("Previous"))]];
    Footer->AddSlot()
        .FillWidth(1)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)[SNew(STextBlock).Font(TravelFont(13)).Text_Lambda([this] {
            if (PickingIcons)
                return FText::FromString(
                    IconTotal > 400
                        ? FString::Printf(TEXT("First 400 of %d icons · search to narrow"), IconTotal)
                        : FString::Printf(TEXT("%d icon%s"), IconTotal, IconTotal == 1 ? TEXT("") : TEXT("s")));
            return FText::FromString(FString::Printf(
                TEXT("Page %d / %d · %d destinations"), Page + 1,
                FMath::Max(1, FMath::DivideAndRoundUp(Data.Total, 32)), Data.Total));
        })];
    Footer->AddSlot().AutoWidth()[SNew(SButton)
                                      .ButtonStyle(&TravelButton())
                                      .Visibility_Lambda([this] {
                                          return PickingIcons ? EVisibility::Collapsed : EVisibility::Visible;
                                      })
                                      .IsEnabled_Lambda(
                                          [this] { return (Page + 1) * 32 < Data.Total && !Pending; })
                                      .OnClicked_Lambda([this] {
                                          ++Page;
                                          Selected.Invalidate();
                                          LastList.Empty();
                                          return FReply::Handled();
                                      })[Caption(TEXT("Next"))]];
    Layout->AddSlot().AutoHeight().Padding(0, 12)[Footer];
    Layout->AddSlot().AutoHeight()[SAssignNew(Status, STextBlock)
                                       .Font(TravelFont(13))
                                       .AutoWrapText(true)
                                       .Text(FText::FromString(TEXT("Connecting to teleporter directory…")))];
    SAssignNew(Root, SBox);
    Root->SetContent(
        SNew(SOverlay) + SOverlay::Slot()[Back(SNullWidget::NullWidget, FLinearColor(0, 0, 0, .5))] +
        SOverlay::Slot()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(48)[SNew(SScaleBox)
                             .Stretch(EStretch::ScaleToFit)
                             .StretchDirection(EStretchDirection::DownOnly)
                                 [SNew(SBox).WidthOverride(820).HeightOverride(520)[Back(Layout)]]]);
    return Root.ToSharedRef();
}
void UTeleportLogisticsTravelWidget::Poll()
{
    if (Closing || !IsValid(Source))
    {
        Close();
        return;
    }
    if (!Status || !Search || !Rows)
        return;
    if (!Remote)
        if (auto *PC = Cast<AFGPlayerController>(GetOwningPlayer()))
        {
            Remote =
                Cast<UTeleportLogisticsTravelRemote>(PC->GetRemoteCallObjectOfClass(UTeleportLogisticsTravelRemote::StaticClass()));
            if (Remote)
            {
                DirectoryHandle = Remote->OnDirectory.AddUObject(this, &UTeleportLogisticsTravelWidget::Receive);
                ResultHandle = Remote->OnResult.AddUObject(this, &UTeleportLogisticsTravelWidget::Result);
            }
        }
    if (Pending && FPlatformTime::Seconds() - PendingSince > 10)
    {
        Pending = false;
        FeedbackUntil = FPlatformTime::Seconds() + 6;
        Status->SetText(FText::FromString(TEXT("Travel request timed out. Try again.")));
    }
    if (Remote && !Pending && !PickingIcons)
    {
        const FString Query = Search->GetText().ToString().Left(64);
        const double Now = FPlatformTime::Seconds();
        // Keep one request in flight. A slow connection must not lose every
        // response to a newer poll, and an old page/search must not reset edits.
        if (DirectoryRequest && RequestedSearch == Query && RequestedPage == Page &&
            Now - DirectoryRequestedAt < 3)
            return;
        const bool NewQuery = DirectoryEpoch == 0 || RequestedSearch != Query || RequestedPage != Page;
        RequestedSearch = Query;
        RequestedPage = Page;
        DirectoryRequestedAt = Now;
        DirectoryRequest = Remote->NextSequence();
        if (NewQuery)
            DirectoryEpoch = DirectoryRequest;
        Remote->ServerDirectory(Source, Query, Page, DirectoryRequest);
    }
}
void UTeleportLogisticsTravelWidget::Receive(const FTeleportLogisticsTravelDirectory &D)
{
    if (Closing || PickingIcons || D.Source != Source || D.Sequence <= Data.Sequence ||
        D.Sequence < DirectoryEpoch)
        return;
    if (!Status || !Search || !Rows)
        return;
    DirectoryRequest = 0;
    if (RequestedSearch != Search->GetText().ToString().Left(64) || RequestedPage != Page)
        return;
    Data = D;
    Page = D.Page;
    if (!Pending && FPlatformTime::Seconds() >= FeedbackUntil)
        Status->SetText(FText::FromString(D.Message));
    FString Key = FString::FromInt(Page) + Search->GetText().ToString();
    for (const auto &Row : D.Destinations)
        Key += Row.Id.ToString() + Row.Name + FString::FromInt(Row.IconId) + FString::FromInt(Row.Available) +
               FString::FromInt(Row.Powered) + Row.Location.ToCompactString();
    if (Key == LastList)
        return;
    LastList = Key;
    Rows->ClearChildren();
    if (D.Destinations.IsEmpty())
        Rows->AddSlot().AutoHeight().Padding(20)[Caption(
            Search->GetText().IsEmpty()
                ? TEXT("No other destinations. Build another Personnel Teleporter and connect it to power.")
                : TEXT("No matching destinations. Clear the search to see every destination."))];
    for (const auto &Row : D.Destinations)
    {
        auto Lines = SNew(SVerticalBox);
        Lines->AddSlot().AutoHeight()[Caption(Row.Name, 16)];
        Lines->AddSlot().AutoHeight().Padding(
            0, 4)[Caption(FString::Printf(TEXT("X %.0f   Y %.0f   Z %.0f  ·  %s"), Row.Location.X / 100,
                                          Row.Location.Y / 100, Row.Location.Z / 100,
                                          !Row.Powered    ? TEXT("Unpowered")
                                          : Row.Available ? TEXT("Ready · click to travel")
                                                          : TEXT("Cooldown")),
                          12)];
        auto RowBody = SNew(SHorizontalBox);
        RowBody->AddSlot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 14, 0)[SNew(SBox).WidthOverride(40).HeightOverride(40)[SNew(SScaleBox).Stretch(
                EStretch::ScaleToFit)[SNew(SImage).Image_Lambda(
                    [this, Id = Row.IconId] { return IconBrush(Id); })]]];
        RowBody->AddSlot().FillWidth(1)[Lines];
        Rows->AddSlot().AutoHeight().Padding(
            0, 0, 0, 6)[SNew(SButton)
                            .ButtonStyle(&TravelButton())
                            .ContentPadding(FMargin(14, 10))
                            .HAlign(HAlign_Fill)
                            .ButtonColorAndOpacity_Lambda([this, Id = Row.Id] {
                                return Selected == Id ? FLinearColor(.65, .32, .06)
                                                      : FLinearColor(.12, .12, .12);
                            })
                            .IsEnabled_Lambda([this, Available = Row.Available] {
                                return Remote && Data.Powered && Available && !Pending;
                            })
                            .OnClicked_Lambda([this, Id = Row.Id] {
                                Selected = Id;
                                Pending = true;
                                PendingSince = FPlatformTime::Seconds();
                                Status->SetText(FText::FromString(TEXT("Checking destination…")));
                                Remote->ServerTravel(Source, Id);
                                return FReply::Handled();
                            })[RowBody]];
    }
}
void UTeleportLogisticsTravelWidget::Result(ATeleportLogisticsTravelHub *ResultSource, bool Success, const FString &Message)
{
    if (Closing || ResultSource != Source)
        return;
    Pending = false;
    if (Success)
    {
        Close();
        return;
    }
    FeedbackUntil = FPlatformTime::Seconds() + 6;
    if (Status)
        Status->SetText(FText::FromString(Message));
}
FReply UTeleportLogisticsTravelWidget::Close()
{
    if (Closing)
        return FReply::Handled();
    Closing = true;
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(PollTimer);
        GetWorld()->GetTimerManager().ClearTimer(IconSearchTimer);
    if (Remote)
    {
        Remote->OnDirectory.Remove(DirectoryHandle);
        Remote->OnResult.Remove(ResultHandle);
    }
    if (auto *PC = GetOwningPlayer())
        if (auto *HUD = Cast<AFGHUD>(PC->GetHUD()))
            if (auto *UI = HUD->GetGameUI())
                UI->PopWidget(this);
    SetVisibility(ESlateVisibility::Collapsed);
    if (Root)
    {
        Root->SetVisibility(EVisibility::Collapsed);
        Root->SetContent(SNullWidget::NullWidget);
    }
    RemoveFromParent();
    return FReply::Handled();
}
void UTeleportLogisticsTravelWidget::OnEscapePressed_Implementation()
{
    Close();
}
FReply UTeleportLogisticsTravelWidget::NativeOnPreviewKeyDown(const FGeometry &G, const FKeyEvent &K)
{
    return K.GetKey() == EKeys::Escape ? Close() : Super::NativeOnPreviewKeyDown(G, K);
}
void UTeleportLogisticsTravelWidget::NativeDestruct()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(PollTimer);
        GetWorld()->GetTimerManager().ClearTimer(IconSearchTimer);
    if (Remote)
    {
        Remote->OnDirectory.Remove(DirectoryHandle);
        Remote->OnResult.Remove(ResultHandle);
    }
    Super::NativeDestruct();
}

const FSlateBrush *UTeleportLogisticsTravelWidget::IconBrush(int32 Id)
{
    if (auto *Existing = IconBrushes.Find(Id))
        return Existing->Get();
    UTexture2D *Texture = nullptr;
    if (auto *DB = AFGIconDatabaseSubsystem::Get(GetWorld()))
        if (DB->IsInitialized())
            Texture = Cast<UTexture2D>(DB->GetIconTextureFromIconID(Id));
    // Deliberately uncached. The database may still be replicating when a row first
    // paints, and caching the placeholder against the id meant the chosen icon never
    // appeared for the rest of the window's life.
    if (!Texture)
        return &FallbackBrush;
    auto Brush = MakeShared<FSlateBrush>();
    Brush->SetResourceObject(Texture);
    Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    Brush->DrawAs = ESlateBrushDrawType::Image;
    IconResources.AddUnique(Texture);
    IconBrushes.Add(Id, Brush);
    return &Brush.Get();
}
void UTeleportLogisticsTravelWidget::ToggleIcons()
{
    PickingIcons = !PickingIcons;
    if (!PickingIcons)
        IconsSorted.Empty();
    if (auto *World = GetWorld())
        World->GetTimerManager().ClearTimer(IconSearchTimer);
    LastList.Empty();
    Selected.Invalidate();
    Search->SetText(FText::GetEmpty());
    Search->SetHintText(
        FText::FromString(PickingIcons ? TEXT("Search sign icons…") : TEXT("Search destination names…")));
    Rows->ClearChildren();
    if (PickingIcons)
        ShowIcons();
    else
    {
        DirectoryRequest = 0;
        DirectoryEpoch = 0;
        Poll();
    }
}
void UTeleportLogisticsTravelWidget::ShowIcons()
{
    if (!Rows || !PickingIcons)
        return;
    Rows->ClearChildren();
    Rows->AddSlot().AutoHeight().Padding(
        0, 0, 0, 10)[SNew(SButton).ButtonStyle(&TravelButton()).OnClicked_Lambda([this] {
        ToggleIcons();
        return FReply::Handled();
    })[Caption(TEXT("Back to destinations"))]];
    auto *DB = AFGIconDatabaseSubsystem::Get(GetWorld());
    if (!DB || !DB->IsInitialized())
    {
        Rows->AddSlot()
            .AutoHeight()[Caption(TEXT("Sign icons are still loading. Reopen the picker shortly."))];
        return;
    }
    // Ordered once when the picker opens. The filter below preserves relative order, so
    // sorting the whole library on every keystroke produced the same list at a cost that
    // grew with the typing.
    if (IconsSorted.IsEmpty())
    {
        for (const auto &I : DB->GetAllIconData())
            if (!I.Hidden && !I.Animated)
                IconsSorted.Add(I);
        IconsSorted.Sort([](const FIconData &A, const FIconData &B) { return A.IconName.CompareTo(B.IconName) < 0; });
    }
    TArray<FIconData> Icons;
    const FString Query = Search->GetText().ToString().TrimStartAndEnd();
    // IconName is only authored for some icons; the rest take their name from the
    // descriptor they were generated from, so matching on it alone finds almost
    // nothing. The soft references carry the asset names without needing a load,
    // and those are what a player's search terms actually look like.
    const auto Matches = [&Query](const FIconData &I) {
        if (Query.IsEmpty())
            return true;
        if (I.IconName.ToString().Contains(Query))
            return true;
        if (!I.ItemDescriptor.IsNull() && I.ItemDescriptor.GetAssetName().Contains(Query))
            return true;
        return !I.Texture.IsNull() && I.Texture.GetAssetName().Contains(Query);
    };
    for (const auto &I : IconsSorted)
        if ((!I.SearchOnly || !Query.IsEmpty()) && Matches(I))
            Icons.Add(I);
    IconTotal = Icons.Num();
    // The grid already lives in a scroll box, and only two of its rows fit, so
    // paging on top of that meant scrolling a page, paging, then scrolling back.
    // Show the lot and let the scroll box do the work; the cap only bounds the
    // unfiltered case, and says so rather than silently truncating.
    constexpr int32 MaxShown = 400;
    const int32 Shown = FMath::Min(IconTotal, MaxShown);
    Rows->AddSlot().AutoHeight().Padding(0, 0, 0,
                                         8)[SNew(SButton)
                                                .ButtonStyle(&TravelButton())
                                                .IsEnabled_Lambda([this] { return Remote != nullptr; })
                                                .OnClicked_Lambda([this] {
                                                    Remote->ServerSetIcon(Source, INDEX_NONE);
                                                    ToggleIcons();
                                                    return FReply::Handled();
                                                })[Caption(TEXT("Use default Personnel Teleporter icon"))]];
    TSharedPtr<SHorizontalBox> Line;
    for (int32 N = 0; N < Shown; ++N)
    {
        if ((N % 7) == 0)
        {
            Line = SNew(SHorizontalBox);
            Rows->AddSlot().AutoHeight().Padding(0, 3)[Line.ToSharedRef()];
        }
        const auto &I = Icons[N];
        Line->AddSlot().AutoWidth().Padding(
            3, 0)[SNew(SButton)
                      .ButtonStyle(&TravelButton())
                      .ToolTipText(I.IconName)
                      .IsEnabled_Lambda([this] { return Remote != nullptr; })
                      .OnClicked_Lambda([this, Id = I.ID] {
                          Remote->ServerSetIcon(Source, Id);
                          ToggleIcons();
                          return FReply::Handled();
                      })
                          [SNew(SBox).WidthOverride(52).HeightOverride(52)[SNew(SScaleBox).Stretch(
                              EStretch::ScaleToFit)[SNew(SImage).Image_Lambda(
                              // Resolved when the cell is first painted. The scroll box
                              // culls what is off screen, so opening the picker does not
                              // pull four hundred textures off disk at once.
                              [this, Id = I.ID] { return IconBrush(Id); })]]]];
    }
    if (Icons.IsEmpty())
        Rows->AddSlot().AutoHeight()[Caption(TEXT("No matching icons."))];
}
