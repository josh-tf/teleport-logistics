#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsBuilding.h"
#include "Async/Async.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Materials/MaterialInstanceDynamic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsHubPowerVisualWorkerTest,
                                 "TeleportLogistics.PowerVisual.RejectFactoryWorkerUpdates",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeleportLogisticsHubPowerVisualWorkerTest::RunTest(const FString &)
{
    // The CDO is rooted and loaded on the game thread before the worker starts.
    // No world is needed: a worker must return before accessing any scene state.
    auto *Hub = GetMutableDefault<ATeleportLogisticsHub>();
    auto *Main = Cast<UStaticMeshComponent>(Hub->GetDefaultSubobjectByName(TEXT("MainMesh")));
    if (!TestNotNull(TEXT("Hub mesh is available"), Main))
        return false;
    const bool WasInitialized = Hub->VisualPowerInitialized;
    const bool WasPowered = Hub->LastVisualPower;
    const auto Screen = Hub->ScreenMaterial;
    const auto PoweredSignal = Hub->PoweredSignalMaterial;
    const auto UnpoweredSignal = Hub->UnpoweredSignalMaterial;
    const auto Overrides = Main->OverrideMaterials;

    const bool RanOffGameThread = Async(EAsyncExecution::ThreadPool, [Hub] {
                                      const bool IsWorker = !IsInGameThread();
                                      for (int32 Index = 0; Index < 512; ++Index)
                                          Hub->ApplyPowerVisual((Index & 1) != 0);
                                      return IsWorker;
                                  }).Get();

    TestTrue(TEXT("Regression executes on an actual worker thread"), RanOffGameThread);
    TestTrue(TEXT("Worker leaves visual initialization unchanged"),
             Hub->VisualPowerInitialized == WasInitialized);
    TestTrue(TEXT("Worker leaves cached power unchanged"), Hub->LastVisualPower == WasPowered);
    TestTrue(TEXT("Worker does not create a screen MID"), Hub->ScreenMaterial == Screen);
    TestTrue(TEXT("Worker does not load signal materials"),
             Hub->PoweredSignalMaterial == PoweredSignal && Hub->UnpoweredSignalMaterial == UnpoweredSignal);
    TestTrue(TEXT("Worker does not change mesh materials"), Main->OverrideMaterials == Overrides);
    // Exercise actual game-thread material transitions and the case where the
    // Customizer restores the original mesh material while power stays off.
    const int32 SignalIndex = Main->GetMaterialIndex(TEXT("signal"));
    const int32 ScreenIndex = Main->GetMaterialIndex(TEXT("screen"));
    Hub->ApplyPowerVisual(false);
    TestNotNull(TEXT("Off material is available"), Hub->UnpoweredSignalMaterial.Get());
    TestNotNull(TEXT("Screen dynamic material is available"), Hub->ScreenMaterial.Get());
    TestTrue(TEXT("Off state uses the explicit zero-emission material"),
             Main->GetMaterial(SignalIndex) == Hub->UnpoweredSignalMaterial.Get());
    if (Hub->ScreenMaterial)
        TestEqual(TEXT("Unpowered screen has zero emission multiplier"),
                  Hub->ScreenMaterial->K2_GetScalarParameterValue(TEXT("TeleporterPower")), 0.f);
    Hub->ApplyPowerVisual(true);
    TestTrue(TEXT("Powered state restores the factory signal material"),
             Main->GetMaterial(SignalIndex) == Hub->PoweredSignalMaterial.Get());
    Hub->ApplyPowerVisual(false);
    Main->SetMaterial(SignalIndex, Hub->PoweredSignalMaterial);
    Main->SetMaterial(ScreenIndex, nullptr);
    Hub->ApplyPowerVisual(false);
    TestTrue(TEXT("Unchanged off state repairs a replaced material override"),
             Main->GetMaterial(SignalIndex) == Hub->UnpoweredSignalMaterial.Get());
    TestTrue(TEXT("Unchanged off state restores the screen MID"),
             Main->GetMaterial(ScreenIndex) == Hub->ScreenMaterial.Get());
    Main->OverrideMaterials = Overrides;
    Hub->ScreenMaterial = Screen;
    Hub->PoweredSignalMaterial = PoweredSignal;
    Hub->UnpoweredSignalMaterial = UnpoweredSignal;
    Hub->VisualPowerInitialized = WasInitialized;
    Hub->LastVisualPower = WasPowered;
    return true;
}
#endif
