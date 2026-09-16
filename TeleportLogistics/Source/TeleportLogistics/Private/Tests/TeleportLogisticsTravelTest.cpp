#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsTravel.h"
#include "TeleportLogisticsContent.h"
#include "UObject/UnrealType.h"
#include "FGPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Curves/CurveFloat.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsTravelTest, "TeleportLogistics.Travel.ContractAndClose",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsTravelTest::RunTest(const FString &)
{
    auto *Hub = GetMutableDefault<ATeleportLogisticsTravelHub>();
    auto *Hologram = GetDefault<ATeleportLogisticsTravelHologram>();
    TestNotNull(TEXT("Placement arrow shaft retained for cooking"), Hologram->ShaftMesh.Get());
    TestNotNull(TEXT("Placement arrow head retained for cooking"), Hologram->TipMesh.Get());
    Hub->Removed = true;
    TestFalse(TEXT("Dismantled Personnel hub is excluded immediately"), Hub->DirectoryActive());
    Hub->Removed = false;
    TestTrue(TEXT("Personnel teleporter uses the native portal base"), Hub->IsA<AFGBuildablePortalBase>());
    TestNotNull(TEXT("Native travel has a duration curve"), Hub->mPortalTravelTimeOverDistance.Get());
    TestTrue(TEXT("Streaming timeout exceeds visual duration"),
             Hub->mMaxPortalTravelTime > Hub->mPortalTravelTimeOverDistance->GetFloatValue(10));
    TestFalse(TEXT("Unconnected teleporter cannot travel"), Hub->Available());
    TestEqual(TEXT("Map marker is opaque"), Hub->GetActorRepresentationColor(), FLinearColor::White);
    TestEqual(TEXT("Portal maximum power has a concrete value"), Hub->GetMaximumPowerConsumption(), 50.f);
    TestEqual(TEXT("Portal base power matches the circuit target"),
              Hub->GetProducingPowerConsumptionBase_ForPortal(), 50.f);
    FTransform Surface;
    Hub->GetPortalSurfaceTransform_Implementation(Surface);
    TestEqual(TEXT("Portal plane matches model ring centre"), Surface.GetLocation().Z, 220.0);
    Hub->UpdateVisual();
    const int32 ScreenSlot = Hub->Mesh->GetMaterialIndex(TEXT("screen"));
    auto *DynamicScreen = Cast<UMaterialInstanceDynamic>(Hub->Mesh->GetMaterial(ScreenSlot));
    if (!TestNotNull(TEXT("Power-controlled dynamic screen exists"), DynamicScreen))
        return false;
    Hub->Mesh->SetMaterial(
        ScreenSlot, LoadObject<UMaterialInterface>(
                        nullptr, TEXT("/TeleportLogistics/Models/M_TeleporterScreen.M_TeleporterScreen")));
    Hub->UpdateVisual();
    TestTrue(TEXT("Customizer reset restores the power-controlled screen"),
             Hub->Mesh->GetMaterial(ScreenSlot) == DynamicScreen);
    TestEqual(TEXT("Unpowered screen remains dark"),
              DynamicScreen->K2_GetScalarParameterValue(TEXT("TeleporterPower")), 0.f);
    TestEqual(TEXT("Travel is late game"),
              UFGSchematic::GetTechTier(UTeleportLogisticsTravelMilestone::StaticClass()), 9);
    auto *R = NewObject<UTeleportLogisticsTravelRemote>(GetMutableDefault<AFGPlayerController>());
    TestFalse(TEXT("Invalid destination GUID rejected"), R->ServerTravel_Validate(Hub, FGuid()));
    TestFalse(TEXT("Negative directory page rejected"), R->ServerDirectory_Validate(Hub, TEXT(""), -1, 1));
    TestFalse(TEXT("Oversize directory search rejected"),
              R->ServerDirectory_Validate(Hub, FString::ChrN(65, 'x'), 0, 1));
    TestTrue(TEXT("Valid directory request accepted"), R->ServerDirectory_Validate(Hub, TEXT("base"), 0, 1));
    TestFalse(TEXT("Malformed negative icon ID rejected"), R->ServerSetIcon_Validate(Hub, -2));
    TestTrue(TEXT("Default icon reset accepted"), R->ServerSetIcon_Validate(Hub, INDEX_NONE));
    const auto *IconProperty =
        FindFProperty<FStructProperty>(ATeleportLogisticsTravelHub::StaticClass(), TEXT("HubIcon"));
    TestTrue(TEXT("Selected icon is saved and replicated"),
             IconProperty && IconProperty->HasAllPropertyFlags(CPF_SaveGame | CPF_Net));
    TestEqual(TEXT("Stock map category labels are preserved"),
              UTeleportLogisticsMapHooks::CategoryName(ERepresentationType::RT_Train,
                                                       FText::FromString(TEXT("Trains")))
                  .ToString(),
              FString(TEXT("Trains")));
    const uint32 First = R->NextSequence();
    const uint32 Second = R->NextSequence();
    TestTrue(TEXT("Snapshot sequence advances"), First < Second);
    TStrongObjectPtr<UVerticalBox> Host(NewObject<UVerticalBox>());
    auto *Other = NewObject<UTextBlock>(Host.Get());
    Host->AddChild(Other);
    auto *Widget = NewObject<UTeleportLogisticsTravelWidget>(Host.Get());
    Widget->Initialize();
    Host->AddChild(Widget);
    const auto Slate = Widget->TakeWidget();
    Widget->mInteractObject = Hub;
    Widget->Init_Implementation();
    FTeleportLogisticsTravelDirectory Directory;
    Directory.Source = Hub;
    Directory.Sequence = 2;
    Directory.Powered = true;
    FTeleportLogisticsTravelDestination Destination;
    Destination.Id = FGuid::NewGuid();
    Destination.Name = TEXT("Northern Base");
    Destination.Powered = true;
    Destination.Available = true;
    Directory.Destinations.Add(Destination);
    Directory.Total = 1;
    Widget->DirectoryRequest = 2;
    Widget->Receive(Directory);
    TestEqual(TEXT("Directory renders destination"), Widget->Rows->GetChildren()->Num(), 1);
    Directory.Sequence = 1;
    Directory.Destinations.Empty();
    Widget->Receive(Directory);
    TestEqual(TEXT("Stale directory cannot clear current selection list"), Widget->Data.Sequence, uint32(2));
    Directory.Sequence = 3;
    Widget->DirectoryRequest = 3;
    Widget->RequestedSearch = TEXT("old query");
    Widget->Search->SetText(FText::FromString(TEXT("new query")));
    Widget->Receive(Directory);
    TestEqual(TEXT("Reply for an old query cannot replace the current list"), Widget->Data.Sequence,
              uint32(2));
    Directory.Sequence = 4;
    Widget->DirectoryEpoch = 3;
    Widget->DirectoryRequest = 5;
    Widget->RequestedSearch = TEXT("new query");
    Widget->Receive(Directory);
    TestEqual(TEXT("Late reply for the same query survives a polling retry"), Widget->Data.Sequence,
              uint32(4));
    Widget->Pending = true;
    Widget->Result(nullptr, true, TEXT("stale result"));
    TestFalse(TEXT("Unrelated source result cannot close this panel"), Widget->Closing);
    Widget->Result(Hub, false, TEXT("Destination blocked"));
    Directory.Sequence = 5;
    Directory.Message = TEXT("Directory ready");
    Widget->Receive(Directory);
    TestEqual(TEXT("Polling does not immediately erase an action error"),
              Widget->Status->GetText().ToString(), FString(TEXT("Destination blocked")));
    Widget->PickingIcons = true;
    Directory.Sequence = 6;
    Widget->Receive(Directory);
    TestEqual(TEXT("Directory refresh cannot replace the icon picker"), Widget->Data.Sequence, uint32(5));
    Widget->OnEscapePressed();
    TestEqual(TEXT("Escape removes travel widget only"), Host->GetChildrenCount(), 1);
    TestTrue(TEXT("Underlying panel preserved"), Host->GetChildAt(0) == Other);
    Widget->OnEscapePressed();
    Widget->Init_Implementation();
    TestEqual(TEXT("Late events cannot reopen or pop another panel"), Host->GetChildrenCount(), 1);
    return true;
}
#endif
