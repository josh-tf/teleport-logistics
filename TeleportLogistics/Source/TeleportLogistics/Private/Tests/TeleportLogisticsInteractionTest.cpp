#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsRemoteCall.h"
#include "TeleportLogisticsWidget.h"
#include "FGPlayerController.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
bool ContainsText(const TSharedRef<SWidget> &Widget, const FString &Expected)
{
    if (Widget->GetTypeAsString() == TEXT("STextBlock") &&
        StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Expected)
        return true;
    FChildren *Children = Widget->GetChildren();
    for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
        if (ContainsText(Children->GetChildAt(Index), Expected))
            return true;
    return false;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsInteractionTest, "TeleportLogistics.Interaction.NativeWidgetLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsInteractionTest::RunTest(const FString &)
{
    const TArray<ATeleportLogisticsBuilding *> Buildings = {
        GetMutableDefault<ATeleportLogisticsItemInput>(), GetMutableDefault<ATeleportLogisticsItemOutput>(),
        GetMutableDefault<ATeleportLogisticsFluidInput>(), GetMutableDefault<ATeleportLogisticsFluidOutput>(),
        GetMutableDefault<ATeleportLogisticsHub>()};
    for (auto *Building : Buildings)
    {
        struct FWidgetClassResult
        {
            TSubclassOf<UFGInteractWidget> ReturnValue;
        } Result;
        UFunction *Getter = Building->FindFunction(TEXT("GetInteractWidgetClass"));
        if (!TestNotNull(TEXT("Native interaction getter is reflected"), Getter))
            return false;
        Building->ProcessEvent(Getter, &Result);
        TestTrue(TEXT("Every building registers the native interaction widget"),
                 Result.ReturnValue == UTeleportLogisticsWidget::StaticClass());
    }

    // The native UI pool can create Slate before assigning mInteractObject.
    // Exercise that ordering without a player connection or game world.
    auto *Widget = NewObject<UTeleportLogisticsWidget>();
    const auto Root = Widget->RebuildWidget();
    Widget->mInteractObject = GetMutableDefault<ATeleportLogisticsHub>();
    Widget->Init_Implementation();
    TestTrue(TEXT("Late hub context rebuilds the hub controls"), ContainsText(Root, TEXT("Teleporter Hub")));
    TestTrue(TEXT("Hub management actions exist before the RCO arrives"),
             ContainsText(Root, TEXT("Apply hub name")));
    Widget->mInteractObject = GetMutableDefault<ATeleportLogisticsFluidOutput>();
    Widget->Init_Implementation();
    TestTrue(TEXT("Changing context rebuilds the endpoint controls"),
             ContainsText(Root, TEXT("Fluid Teleporter")));
    TestTrue(TEXT("Fluid endpoints expose their buffer action"),
             ContainsText(Root, TEXT("Flush local buffer")));
    TestFalse(TEXT("The previous hub panel is removed"), ContainsText(Root, TEXT("Apply hub name")));

    // Every mutation reaches the server through one of these validators, so each bound
    // is checked here. RCOs require an FGPlayerController outer, even without a world.
    auto *Remote = NewObject<UTeleportLogisticsRemoteCall>(GetMutableDefault<AFGPlayerController>());
    auto *Endpoint = GetMutableDefault<ATeleportLogisticsFluidOutput>();
    auto *Hub = GetMutableDefault<ATeleportLogisticsHub>();
    const FString TooLong = FString::ChrN(65, 'x');
    TestFalse(TEXT("Negative snapshot page rejected"), Remote->ServerSnapshot_Validate(Hub, FGuid(), -1, 1));
    TestFalse(TEXT("Out-of-range snapshot page rejected"),
              Remote->ServerSnapshot_Validate(Hub, FGuid(), 16385, 1));
    TestTrue(TEXT("First snapshot page accepted"), Remote->ServerSnapshot_Validate(Hub, FGuid(), 0, 1));
    TestFalse(TEXT("Oversize route name rejected"),
              Remote->ServerConfigure_Validate(Endpoint, FGuid(), TooLong, TEXT("Copper Feed A")));
    TestFalse(TEXT("Oversize endpoint label rejected"),
              Remote->ServerConfigure_Validate(Endpoint, FGuid(), TEXT("north_farm"), TooLong));
    TestTrue(TEXT("Route and label within the field limits accepted"),
             Remote->ServerConfigure_Validate(Endpoint, FGuid(), TEXT("north_farm"), TEXT("Copper Feed A")));
    TestFalse(TEXT("Invalid pasted route rejected"), Remote->ServerPasteRoute_Validate(Endpoint, FGuid()));
    TestTrue(TEXT("Pasted route GUID accepted"),
             Remote->ServerPasteRoute_Validate(Endpoint, FGuid::NewGuid()));
    TestFalse(TEXT("Oversize hub name rejected"), Remote->ServerRenameHub_Validate(Hub, TooLong));
    TestTrue(TEXT("Hub name within the field limit accepted"),
             Remote->ServerRenameHub_Validate(Hub, TEXT("North West Farm")));
    TestFalse(TEXT("Unlisted route action rejected"),
              Remote->ServerControl_Validate(Hub, FGuid(), TEXT("drop"), TEXT("north_farm")));
    TestFalse(TEXT("Oversize route rename rejected"),
              Remote->ServerControl_Validate(Hub, FGuid(), TEXT("rename"), TooLong));
    const TCHAR *const Actions[] = {TEXT("pause"), TEXT("rename"), TEXT("reset-fluid"), TEXT("delete")};
    for (const TCHAR *Action : Actions)
        TestTrue(FString::Printf(TEXT("Route action %s accepted"), Action),
                 Remote->ServerControl_Validate(Hub, FGuid(), Action, TEXT("north_farm")));
    return true;
}
#endif
