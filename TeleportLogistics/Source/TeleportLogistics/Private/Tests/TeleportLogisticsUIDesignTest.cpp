#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsWidget.h"
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsRemoteCall.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Layout/ArrangedChildren.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "StaticMeshResources.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
void CollectLayout(const TSharedRef<SWidget> &Widget, const FGeometry &Geometry,
                   TMap<const SWidget *, FSlateRect> &Rects, int32 &ScrollCount, FSlateRect &Apply)
{
    const FVector2D Position = Geometry.GetAbsolutePosition();
    const FVector2D Size = Geometry.GetAbsoluteSize();
    Rects.Add(&Widget.Get(), FSlateRect(Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y));
    if (Widget->GetTypeAsString() == TEXT("SScrollBox")) ++ScrollCount;
    if (Widget->GetTypeAsString() == TEXT("STextBlock"))
    {
        const FString Label = StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString();
        if (Label == TEXT("Apply changes") || Label == TEXT("Apply hub name"))
            Apply = Rects[&Widget.Get()];
    }
    FArrangedChildren Children(EVisibility::Visible);
    Widget->ArrangeChildren(Geometry, Children);
    for (const FArrangedWidget &Child : Children.GetInternalArray())
        CollectLayout(Child.Widget, Child.Geometry, Rects, ScrollCount, Apply);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsUIDesignTest, "TeleportLogistics.Interaction.DesignAndSnapshots",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsUIDesignTest::RunTest(const FString &)
{
    const auto *NativeFont =
        LoadObject<UFont>(nullptr, TEXT("/Game/FactoryGame/Interface/Font/DescriptionText.DescriptionText"));
    TestNotNull(TEXT("Native composite font is available, not a raw font-face asset"), NativeFont);
    if (NativeFont)
        TestTrue(TEXT("Native composite font contains named typefaces"),
                 NativeFont->CompositeFont.DefaultTypeface.Fonts.Num() >= 3);
    auto *WindowClass = LoadClass<UUserWidget>(
        nullptr, TEXT("/Game/FactoryGame/Interface/UI/Widget_Window_DarkMode.Widget_Window_DarkMode_C"));
    TestNotNull(TEXT("Native window class is available"), WindowClass);
    if (WindowClass)
    {
        TestNotNull(TEXT("Native title property exists"),
                    FindFProperty<FTextProperty>(WindowClass, TEXT("mTitleText")));
        TestNotNull(TEXT("Native close delegate exists"),
                    FindFProperty<FProperty>(WindowClass, TEXT("OnClose")));
        TStrongObjectPtr<UUserWidget> Window(NewObject<UUserWidget>(GetTransientPackage(), WindowClass));
        Window->Initialize();
        TArray<FName> Slots;
        Window->GetSlotNames(Slots);
        TestTrue(TEXT("Native window exposes the documented body slot"), Slots.Contains(TEXT("WindowBody")));
    }
    for (const bool Hub : {false, true})
    {
        TStrongObjectPtr<UTeleportLogisticsWidget> Widget(NewObject<UTeleportLogisticsWidget>());
        Widget->Initialize();
        Widget->mInteractObject = Hub ? static_cast<ATeleportLogisticsBuilding *>(GetMutableDefault<ATeleportLogisticsHub>())
                                      : static_cast<ATeleportLogisticsBuilding *>(GetMutableDefault<ATeleportLogisticsFluidOutput>());
        auto Slate = Widget->TakeWidget();
        TestNotNull(TEXT("SFUIKIT panel is retained"), Widget->PlateBrush.GetResourceObject());
        TestNotNull(TEXT("SFUIKIT display is retained"), Widget->MeterBrush.GetResourceObject());
        Widget->Init_Implementation();
        Widget->Remote = NewObject<UTeleportLogisticsRemoteCall>(GetMutableDefault<AFGPlayerController>());
        // Same actor can arrive before its newly assigned TeleporterId replication.
        Widget->RenderLists();
        const auto WaitingRow = Widget->RoutesBox->GetChildren()->GetChildAt(0);
        FTeleportLogisticsSnapshot Ready;
        Ready.ContextActor = Widget->Context;
        Ready.Context = FGuid(91, 92, 93, 94);
        Ready.RequestId = 1;
        Ready.ContextAvailable = true;
        Widget->Receive(Ready);
        TestTrue(TEXT("First snapshot accepts matching actor before ID replication"), Widget->Data.ContextAvailable);
        TestTrue(TEXT("Empty successful response replaces the connecting placeholder"),
            Widget->RoutesBox->GetChildren()->GetChildAt(0) != WaitingRow);
        Widget->InitializedSelection = false;
        const uint32 FirstSequence = Widget->Remote->AllocateSnapshotRequest();
        TestTrue(TEXT("Request sequence belongs to the persistent player connection"),
            Widget->Remote->AllocateSnapshotRequest() > FirstSequence);
        FTeleportLogisticsSnapshot Snapshot;
        Snapshot.Context = Widget->Context->TeleporterId;
        Snapshot.RequestId = 2;
        Snapshot.ContextAvailable = true;
        Snapshot.HubPowered = true;
        Snapshot.EndpointEnabled = false;
        Snapshot.Capacity = 50000;
        Snapshot.Buffered = 18500;
        Snapshot.ContextLabel = Hub ? TEXT("North West Farm") : TEXT("Copper Feed A");
        Snapshot.Message = TEXT("Network online · 3 channels · 4 routes");
        FTeleportLogisticsChannel Default;
        Default.Name = TEXT("Default");
        Snapshot.Channels.Add(Default);
        FTeleportLogisticsChannel Copper;
        Copper.Id = FGuid(1, 2, 3, 4);
        Copper.Name = TEXT("Small Copper");
        Snapshot.Channels.Add(Copper);
        FTeleportLogisticsChannel Farm;
        Farm.Id = FGuid(1, 2, 3, 5);
        Farm.Name = TEXT("North West Farm");
        Snapshot.Channels.Add(Farm);
        for (int32 I = 0; I < 4; ++I)
        {
            FTeleportLogisticsRoute R;
            R.Id = FGuid(10, 20, 30, I + 1);
            R.Channel = FGuid();
            R.Name = I == 0   ? TEXT("north_farm")
                     : I == 1 ? TEXT("small_copper_ingot")
                     : I == 2 ? TEXT("sink")
                              : TEXT("mega_copper_ingot");
            R.Medium = I == 0 ? ETeleportLogisticsMedium::Fluid : ETeleportLogisticsMedium::Items;
            R.Inputs = I + 1;
            R.Outputs = I + 3;
            R.UnitsPerMinute = I == 0 ? 240000 : 118;
            Snapshot.Routes.Add(R);
        }
        Snapshot.AssignedRoute = Snapshot.Routes[0].Id;
        for (int32 I = 0; I < 8; ++I)
        {
            FTeleportLogisticsEndpointView E;
            E.Id = FGuid(5, 6, 7, I);
            E.Route = Snapshot.Routes[0].Id;
            E.Name = I == 0 ? TEXT("North West Farm — Coal Generators")
                            : FString::Printf(TEXT("Fluid Drop Point %d"), I + 1);
            E.RoutePath = TEXT("Default / north_farm");
            E.Medium = ETeleportLogisticsMedium::Fluid;
            E.Input = I < 2;
            E.Enabled = I != 3;
            E.Buffered = I * 800;
            E.Location = FVector(172800 + I * 1200, -94700, 18500);
            Snapshot.Endpoints.Add(E);
        }
        Snapshot.TotalEndpoints = 147;
        Widget->Receive(Snapshot);
        Widget->SelectedRoute = Snapshot.AssignedRoute;
        Widget->RenderLists();
        TestTrue(TEXT("Server snapshot initializes the selection"), Widget->InitializedSelection);
        Widget->NameField->SetText(FText::FromString(TEXT("Unsaved draft")));
        Widget->RouteField->SetText(FText::FromString(TEXT("new_route")));
        if (!Hub)
        {
            TestNull(TEXT("New route draft cannot show the old route's statistics"), Widget->PreviewRoute());
            Widget->RouteField->SetText(FText::FromString(TEXT(" NORTH_FARM ")));
            TestNotNull(TEXT("Route preview resolves trimmed case-insensitive names"), Widget->PreviewRoute());
            Widget->SelectedChannel = Copper.Id;
            TestNull(TEXT("Route preview is isolated to the selected channel"), Widget->PreviewRoute());
            Widget->SelectedChannel.Invalidate();
            Widget->RouteField->SetText(FText::FromString(TEXT("new_route")));
        }
        Widget->SearchField->SetText(FText::FromString(TEXT("north")));
        const int32 RowCount = Widget->RoutesBox->NumSlots();
        auto RowBefore = RowCount ? Widget->RoutesBox->GetChildren()->GetChildAt(0) : Slate;
        ++Snapshot.RequestId;
        Snapshot.Buffered = 24000;
        Widget->Receive(Snapshot);
        TestEqual(TEXT("Polling retains typed label"), Widget->NameField->GetText().ToString(),
                  FString(TEXT("Unsaved draft")));
        TestEqual(TEXT("Polling retains typed route"), Widget->RouteField->GetText().ToString(),
                  FString(TEXT("new_route")));
        if (RowCount)
            TestTrue(TEXT("Metric updates retain the same route row and focus target"),
                     RowBefore == Widget->RoutesBox->GetChildren()->GetChildAt(0));
        Widget->Pending = true;
        Widget->PendingAction = TEXT("apply");
        ++Snapshot.RequestId;
        ++Snapshot.MutationRevision;
        Snapshot.MutationSucceeded = false;
        Snapshot.Message = TEXT("Drain the local buffer before changing its route.");
        Widget->Receive(Snapshot);
        TestFalse(TEXT("Server rejection ends pending state"), Widget->Pending);
        TestTrue(TEXT("Server rejection is visible"), Widget->LastActionFailed);
        TestTrue(TEXT("Rejected draft remains available to correct"), Widget->Dirty);
        Widget->Pending = true;
        Widget->PendingAction = TEXT("apply");
        ++Snapshot.RequestId;
        ++Snapshot.MutationRevision;
        Snapshot.MutationSucceeded = true;
        Snapshot.Message = TEXT("Endpoint saved.");
        Widget->Receive(Snapshot);
        TestFalse(TEXT("Accepted mutation ends pending state"), Widget->Pending);
        TestFalse(TEXT("Accepted mutation clears the failure flag"), Widget->LastActionFailed);
        TestFalse(TEXT("Accepted apply clears the unsaved draft"), Widget->Dirty);
        Snapshot.HubPowered = false;
        ++Snapshot.RequestId;
        Widget->Receive(Snapshot);
        if (Hub)
            TestFalse(TEXT("Unpowered hub cannot manage routes"), Widget->CanManage());
        const uint32 Latest = Widget->LastReceivedRequestId;
        Snapshot.RequestId = Latest - 1;
        Snapshot.ContextLabel = TEXT("Stale");
        Widget->Receive(Snapshot);
        TestEqual(TEXT("Out-of-order snapshots are ignored"), Widget->LastReceivedRequestId, Latest);

        if (Hub)
        {
            Widget->SearchField->SetText(FText::GetEmpty());
            Widget->SelectedChannel.Invalidate();
            for (int32 I = 0; I < 20; ++I)
            {
                FTeleportLogisticsRoute Extra;
                Extra.Id = FGuid(500, 0, 0, I + 1);
                Extra.Name = FString::Printf(TEXT("Additional route %d"), I);
                Widget->Data.Routes.Add(Extra);
            }
            Widget->LastListKey.Empty();
            Widget->RenderLists();
            TestEqual(TEXT("Hub route pages bound the visible rows"), Widget->RoutesBox->NumSlots(), 4);
            Widget->RoutePage = 2;
            Widget->LastListKey.Empty();
            Widget->RenderLists();
            TestEqual(TEXT("Route paging does not require a server refresh"), Widget->RoutePage, 2);
        }
        for (const FVector2D View : {FVector2D(1920,1080), FVector2D(1280,720), FVector2D(960,540)})
        {
            for (const float Dpi : {1.f, 1.5f, 2.f})
            {
                for (const bool Powered : {true, false})
                {
                Widget->Data.HubPowered = Powered;
                Slate->SlatePrepass(Dpi);
                TMap<const SWidget *, FSlateRect> Rects;
                int32 ScrollCount = 0;
                FSlateRect Apply;
                CollectLayout(Slate, FGeometry::MakeRoot(View / Dpi, FSlateLayoutTransform(Dpi)), Rects, ScrollCount, Apply);
                // Every footer bound below reads Apply.Top, which stays at -1 if the label moved.
                TestTrue(TEXT("Apply footer was located"), Apply.Top > 0);
                TestEqual(TEXT("Only the active data list scrolls; no outer or nested scroll regions"), ScrollCount, 1);
                for (const auto &Pair : Rects)
                {
                    if (Pair.Key->GetTypeAsString() != TEXT("STextBlock")) continue;
                    const FString Label = static_cast<const STextBlock *>(Pair.Key)->GetText().ToString();
                    if (Label == TEXT("Flush local buffer") || Label == TEXT("Enable / standby") ||
                        Label == TEXT("Delete unused route") || Label == TEXT("Previous") || Label == TEXT("Next"))
                        TestTrue(FString::Printf(TEXT("%s bottom %.1f <= footer %.1f (hub %d, viewport %.0f)"), *Label, Pair.Value.Bottom, Apply.Top, Hub, View.X), Pair.Value.Bottom <= Apply.Top + 1);
                }
                if (!Hub)
                {
                    const auto *SearchRect = Rects.Find(Widget->SearchField.Get());
                    const auto *RouteRect = Rects.Find(Widget->RouteField.Get());
                    if (SearchRect && RouteRect)
                        TestTrue(TEXT("Route editor never overlaps route search"), SearchRect->Bottom <= RouteRect->Top);
                }
                for (const auto &Field : {Widget->NameField, Widget->RouteField})
                {
                    if (Hub && !Powered && Field == Widget->RouteField)
                    {
                        TestFalse(TEXT("Unpowered hubs hide route editing"), Rects.Contains(Field.Get()));
                        continue;
                    }
                    const auto *Rect = Rects.Find(Field.Get());
                    TestNotNull(TEXT("Editable field is in the visible layout"), Rect);
                    if (Rect)
                    {
                        TestTrue(TEXT("Editable fields fit the viewport width"), Rect->Left >= 0 && Rect->Right <= View.X + 1);
                        TestTrue(TEXT("Editable fields stay above the Apply footer"), Rect->Bottom <= Apply.Top + 1);
                        TestTrue(TEXT("Editable fields retain a nonzero height"), Rect->Bottom > Rect->Top + 8);
                    }
                }
            }

            } // DPI cases
        } // Viewport cases

        // Optional actual Slate offscreen captures, never mock HTML screenshots.
        if (FParse::Param(FCommandLine::Get(), TEXT("TeleportLogisticsUICapture")))
        {
            Snapshot.RequestId = Latest + 1;
            Snapshot.HubPowered = true;
            Snapshot.ContextLabel = Hub ? TEXT("North West Farm") : TEXT("Coal Generator Feed");
            Snapshot.Message = TEXT("Network online · 3 channels · 4 routes");
            Widget->Dirty = false;
            Widget->InitializedSelection = false;
            Widget->LastActionFailed = false;
            Widget->SearchField->SetText(FText::GetEmpty());
            Widget->Receive(Snapshot);
            Widget->SelectedRoute = Snapshot.AssignedRoute;
            Widget->RouteField->SetText(FText::FromString(TEXT("north_farm")));
            Widget->Dirty = false;
            Widget->RenderLists();
            FWidgetRenderer Renderer(true, true);
            for (const FIntPoint Size :
                 {FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3440, 1440), FIntPoint(1280, 720)})
            {
                auto *Target = Renderer.DrawWidget(Slate, FVector2D(Size));
                FlushRenderingCommands();
                // A second draw applies geometry-dependent window bounds.
                Renderer.DrawWidget(Target, Slate, FVector2D(Size), 0);
                FlushRenderingCommands();
                TArray<FColor> Pixels;
                Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
                TArray64<uint8> PNG;
                FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
                const FString File = FString::Printf(TEXT("/tmp/teleporter-ui-%s-%dx%d.png"),
                                                     Hub ? TEXT("hub") : TEXT("endpoint"), Size.X, Size.Y);
                TestTrue(TEXT("UI capture saved"), FFileHelper::SaveArrayToFile(PNG, *File));
            }
        }
        Widget->Close();
    }
    if (auto *Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/TeleportLogistics/Models/SM_TeleporterHub.SM_TeleporterHub")))
    {
        const auto &Vertices = Mesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
        FVector Sum = FVector::ZeroVector;
        int32 Count = 0;
        for (uint32 I = 0; I < Vertices.GetNumVertices(); ++I)
        {
            const FVector P(Vertices.VertexPosition(I));
            // Top cap of the resized mast, independently measured in imported space.
            if (P.Z > 298 && P.Z < 300 && FMath::Abs(P.X + 83.64) < 7 && FMath::Abs(P.Y - 52.5) < 7)
            {
                Sum += P;
                ++Count;
            }
        }
        TestTrue(TEXT("Imported custom mast cap has vertices"), Count > 0);
        if (Count)
        {
            const FVector Cap = Sum / Count;
            const auto *Power = GetDefault<ATeleportLogisticsHub>()->Power.Get();
            TestTrue(TEXT("Power socket is within 1cm of the imported mast cap"),
                     FVector::Distance(Cap, Power->GetRelativeLocation()) < 1.f);
            AddInfo(FString::Printf(TEXT("Imported mast top centroid: %s (%d vertices)"), *Cap.ToString(),
                                    Count));
        }
    }
    return true;
}
#endif
