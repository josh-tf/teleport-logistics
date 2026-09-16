#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsBuilding.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsEndpointConnectionVisualTest,
    "TeleportLogistics.PowerVisual.EndpointConnections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeleportLogisticsEndpointConnectionVisualTest::RunTest(const FString &)
{
    auto *Property = FindFProperty<FBoolProperty>(ATeleportLogisticsEndpoint::StaticClass(), TEXT("PortConnected"));
    TestTrue(TEXT("Connection status replicates with an immediate visual notify"),
        Property && Property->HasAnyPropertyFlags(CPF_Net) &&
        Property->RepNotifyFunc == TEXT("OnRep_PortConnected"));
    ATeleportLogisticsEndpoint *Endpoints[] = {GetMutableDefault<ATeleportLogisticsItemInput>(), GetMutableDefault<ATeleportLogisticsItemOutput>(),
        GetMutableDefault<ATeleportLogisticsFluidInput>(), GetMutableDefault<ATeleportLogisticsFluidOutput>()};
    for (auto *Endpoint : Endpoints)
    {
        auto *Main = Cast<UStaticMeshComponent>(Endpoint->GetDefaultSubobjectByName(TEXT("MainMesh")));
        if (!TestNotNull(TEXT("Endpoint mesh"), Main)) continue;
        const auto Overrides = Main->OverrideMaterials;
        const auto Screen = Endpoint->ScreenMaterial;
        const auto On = Endpoint->PoweredSignalMaterial;
        const auto Off = Endpoint->UnpoweredSignalMaterial;
        const bool Initialized = Endpoint->VisualPowerInitialized;
        const bool Last = Endpoint->LastVisualPower;
        const bool Connected = Endpoint->PortConnected;
        const int32 SignalIndex = Main->GetMaterialIndex(TEXT("signal"));
        const int32 ScreenIndex = Main->GetMaterialIndex(TEXT("screen"));
        TestTrue(TEXT("Both material sections exist"), SignalIndex != INDEX_NONE && ScreenIndex != INDEX_NONE);
        Endpoint->PortConnected = false;
        Endpoint->OnRep_PortConnected();
        TestTrue(TEXT("Unconnected lamps use explicit non-emissive material"),
            Main->GetMaterial(SignalIndex) == Endpoint->UnpoweredSignalMaterial.Get());
        if (TestNotNull(TEXT("Endpoint screen MID"), Endpoint->ScreenMaterial.Get()))
        {
            TestEqual(TEXT("Unconnected screen is dark"),
                Endpoint->ScreenMaterial->K2_GetScalarParameterValue(TEXT("TeleporterPower")), 0.f);
            Endpoint->PortConnected = true;
            Endpoint->OnRep_PortConnected();
            TestEqual(TEXT("Replicated attachment lights screen"),
                Endpoint->ScreenMaterial->K2_GetScalarParameterValue(TEXT("TeleporterPower")), 1.f);
            TestTrue(TEXT("Attached lamps restore factory material"),
                Main->GetMaterial(SignalIndex) == Endpoint->PoweredSignalMaterial.Get());
            Endpoint->PortConnected = false;
            Endpoint->OnRep_PortConnected();
            Main->SetMaterial(SignalIndex, Endpoint->PoweredSignalMaterial);
            Main->SetMaterial(ScreenIndex, nullptr);
            Endpoint->RefreshConnectionVisual();
            TestTrue(TEXT("Detached state repairs Customizer material overrides"),
                Main->GetMaterial(SignalIndex) == Endpoint->UnpoweredSignalMaterial.Get() &&
                Main->GetMaterial(ScreenIndex) == Endpoint->ScreenMaterial.Get());
            TestEqual(TEXT("Detaching darkens screen again"),
                Endpoint->ScreenMaterial->K2_GetScalarParameterValue(TEXT("TeleporterPower")), 0.f);
        }
        Main->OverrideMaterials = Overrides;
        Endpoint->ScreenMaterial = Screen;
        Endpoint->PoweredSignalMaterial = On;
        Endpoint->UnpoweredSignalMaterial = Off;
        Endpoint->VisualPowerInitialized = Initialized;
        Endpoint->LastVisualPower = Last;
        Endpoint->PortConnected = Connected;
    }
    return true;
}
#endif
