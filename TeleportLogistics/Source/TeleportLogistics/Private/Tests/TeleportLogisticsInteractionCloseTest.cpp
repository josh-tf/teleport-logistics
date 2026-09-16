#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsRemoteCall.h"
#include "TeleportLogisticsWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "InputCoreTypes.h"
#include "FGPlayerController.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Layout/SBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsInteractionCloseTest, "TeleportLogistics.Interaction.CloseLifecycle",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsInteractionCloseTest::RunTest(const FString &)
{
    const FKeyEvent Escape(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0);
    const FKeyEvent Letter(EKeys::A, FModifierKeysState(), 0, false, 0, 0);
    for (int32 EntryPoint = 0; EntryPoint < 3; ++EntryPoint)
    {
        // Keep the Slate wrapper alive across removal, as a focus path or
        // transition can. Cleanup cannot wait for NativeDestruct to run.
        TStrongObjectPtr<UVerticalBox> Host(NewObject<UVerticalBox>());
        auto *OtherPanel = NewObject<UTextBlock>(Host.Get());
        OtherPanel->SetText(FText::FromString(TEXT("Another interaction")));
        Host->AddChild(OtherPanel);
        auto *Widget = NewObject<UTeleportLogisticsWidget>(Host.Get());
        Widget->Initialize();
        Widget->mInteractObject = GetMutableDefault<ATeleportLogisticsItemInput>();
        Host->AddChild(Widget);
        const auto HostSlate = Host->TakeWidget();
        const auto RetainedSlate = Widget->GetCachedWidget().ToSharedRef();
        const auto RetainedContent = Widget->ContentRoot.ToSharedRef();
        Widget->Init_Implementation();
        // RCOs require an FGPlayerController outer, even in a worldless test.
        auto *Remote = NewObject<UTeleportLogisticsRemoteCall>(GetMutableDefault<AFGPlayerController>());
        Widget->Remote = Remote;
        Widget->SnapshotHandle = Remote->OnSnapshot.AddUObject(Widget, &UTeleportLogisticsWidget::Receive);

        TestEqual(TEXT("Both panels are attached before close"), Host->GetChildrenCount(), 2);
        TestTrue(TEXT("The open panel has a snapshot subscription"), Remote->OnSnapshot.IsBound());
        TestFalse(TEXT("Ordinary typing is not consumed by the Escape preview handler"),
                  Widget->NativeOnPreviewKeyDown(FGeometry(), Letter).IsEventHandled());

        if (EntryPoint == 0)
            TestTrue(TEXT("Escape is consumed before an editable child"),
                     Widget->NativeOnPreviewKeyDown(FGeometry(), Escape).IsEventHandled());
        else if (EntryPoint == 1)
            Widget->OnEscapePressed(); // Native game input dispatch.
        else
            Widget->Close(); // Same callback used by the Close button.

        TestTrue(TEXT("Closed panel is marked closed"), Widget->Closing);
        TestTrue(TEXT("Closed UMG widget is collapsed"), Widget->GetVisibility() == ESlateVisibility::Collapsed);
        TestTrue(TEXT("Retained Slate wrapper is collapsed"), RetainedSlate->GetVisibility() == EVisibility::Collapsed);
        TestTrue(TEXT("Retained panel content is collapsed"), RetainedContent->GetVisibility() == EVisibility::Collapsed);
        TestEqual(TEXT("Only the other panel remains in UMG"), Host->GetChildrenCount(), 1);
        TestEqual(TEXT("Only the other panel remains in Slate"), HostSlate->GetChildren()->Num(), 1);
        TestTrue(TEXT("The other panel is preserved"), Host->GetChildAt(0) == OtherPanel);
        TestFalse(TEXT("Snapshot subscription is removed before destruction"), Remote->OnSnapshot.IsBound());
        TestNull(TEXT("Remote reference is released"), Widget->Remote.Get());

        Widget->OnEscapePressed();
        Widget->Init_Implementation(); // A delayed Init cannot resurrect a closed panel.
        TestTrue(TEXT("Late Init leaves the panel closed"), Widget->Closing);
        TestEqual(TEXT("Repeated close cannot remove the next panel"), Host->GetChildrenCount(), 1);
        TestTrue(TEXT("Repeated close leaves the next panel visible"), OtherPanel->IsVisible());

        // Reopening must create a fresh, interactive panel, not reuse dead state.
        auto *Reopened = NewObject<UTeleportLogisticsWidget>(Host.Get());
        Reopened->Initialize();
        Reopened->mInteractObject = GetMutableDefault<ATeleportLogisticsHub>();
        Host->AddChild(Reopened);
        const auto ReopenedSlate = Reopened->TakeWidget();
        TestFalse(TEXT("A fresh panel is not closing"), Reopened->Closing);
        TestTrue(TEXT("A fresh panel is visible"), Reopened->IsVisible());
        Reopened->Close();
        TestEqual(TEXT("Reopened panel closes without stranding another layer"), Host->GetChildrenCount(), 1);
    }
    return true;
}
#endif
