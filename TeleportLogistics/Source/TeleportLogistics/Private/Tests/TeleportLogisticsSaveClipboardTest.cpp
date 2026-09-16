#include "TeleportLogisticsBuilding.h"
#include "FGSaveInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsSaveClipboardTest, "TeleportLogistics.Persistence.SaveAndClipboard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsSaveClipboardTest::RunTest(const FString &)
{
    auto *Endpoint = GetMutableDefault<ATeleportLogisticsItemInput>();
    const auto OldId = Endpoint->TeleporterId;
    const auto OldRoute = Endpoint->RouteId;
    const auto OldLabel = Endpoint->Label;
    const auto OldCargo = Endpoint->Cargo;
    Endpoint->TeleporterId = FGuid::NewGuid();
    Endpoint->RouteId = FGuid::NewGuid();
    Endpoint->Label = TEXT("Save round trip");
    Endpoint->Cargo = {FInventoryStack(7, UFGItemDescriptor::StaticClass())};
    // SDK inventory constructors are stubs; initialise the saved count explicitly.
    Endpoint->Cargo[0].NumItems = 7;
    const auto SavedId = Endpoint->TeleporterId;
    const auto SavedRoute = Endpoint->RouteId;
    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes);
        FObjectAndNameAsStringProxyArchive Archive(Writer, false);
        Archive.ArIsSaveGame = true;
        Archive.ArNoDelta = true;
        Endpoint->Serialize(Archive);
    }
    Endpoint->TeleporterId.Invalidate();
    Endpoint->RouteId.Invalidate();
    Endpoint->Cargo.Reset();
    Endpoint->Label.Reset();
    {
        FMemoryReader Reader(Bytes);
        FObjectAndNameAsStringProxyArchive Archive(Reader, true);
        Archive.ArIsSaveGame = true;
        Archive.ArNoDelta = true;
        Endpoint->Serialize(Archive);
    }
    TestTrue(TEXT("Placed buildings opt into saving"), Endpoint->ShouldSave_Implementation());
    TestEqual(TEXT("Identity survives SaveGame archive"), Endpoint->TeleporterId, SavedId);
    TestEqual(TEXT("Route survives SaveGame archive"), Endpoint->RouteId, SavedRoute);
    TestEqual(TEXT("Label survives SaveGame archive"), Endpoint->Label, FString(TEXT("Save round trip")));
    TestEqual(TEXT("Cargo survives SaveGame archive"), Endpoint->Buffered(), 7);
    auto *Clipboard = Cast<UTeleportLogisticsClipboardSettings>(Endpoint->CopySettings_Implementation());
    TestNotNull(TEXT("Native clipboard settings"), Clipboard);
    if (Clipboard)
    {
        TestEqual(TEXT("Copies route identity"), Clipboard->RouteId, SavedRoute);
        TestFalse(TEXT("No player cannot paste"), Endpoint->PasteSettings_Implementation(Clipboard, nullptr));
        TestFalse(TEXT("Fluid cannot paste an item route"), GetMutableDefault<ATeleportLogisticsFluidInput>()->PasteSettings_Implementation(Clipboard, nullptr));
    }
    Endpoint->TeleporterId = OldId;
    Endpoint->RouteId = OldRoute;
    Endpoint->Label = OldLabel;
    Endpoint->Cargo = OldCargo;
    return true;
}
#endif
