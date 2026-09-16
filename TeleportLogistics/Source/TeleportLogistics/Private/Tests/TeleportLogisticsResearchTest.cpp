#if WITH_DEV_AUTOMATION_TESTS
#include "TeleportLogisticsContent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportLogisticsResearchTest, "TeleportLogistics.Research.RewardLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportLogisticsResearchTest::RunTest(const FString &)
{
    UTeleportLogisticsGameInstanceModule::PrepareRewardPresentation();
    auto *Field = FindFProperty<FArrayProperty>(UFGSchematic::StaticClass(), TEXT("mUnlocks"));
    if (!TestNotNull(TEXT("Schematic unlock property exists"), Field))
        return false;
    UFGSchematic *Defaults[] = {GetMutableDefault<UTeleportLogisticsMilestone>(),
                                GetMutableDefault<UTeleportLogisticsTravelMilestone>()};
    TWeakObjectPtr<UFGUnlockRecipe> Weak[2];
    for (int32 I = 0; I < 2; ++I)
    {
        auto &Entries = *Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Defaults[I]);
        if (!TestEqual(TEXT("One recipe reward per milestone"), Entries.Num(), 1))
            return false;
        Weak[I] = Cast<UFGUnlockRecipe>(Entries[0].Get());
        if (!TestTrue(TEXT("Reward is a valid recipe unlock"), Weak[I].IsValid()))
            return false;
    }
    // Restore the CDO edges on every exit, including a failing one. PrepareRewardPresentation
    // only replaces entries that already exist, so an empty array leaves the milestone
    // unlockless for the rest of the editor process.
    ON_SCOPE_EXIT
    {
        for (int32 I = 0; I < 2; ++I)
        {
            auto &Entries = *Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Defaults[I]);
            if (Entries.IsEmpty() && Weak[I].IsValid())
                Entries.Add(Weak[I].Get());
        }
    };
    // Remove the reflected CDO edges entirely to reproduce their absence from
    // retail GC traversal. Only weak handles remain in this test scope.
    for (auto *Default : Defaults)
        Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Default)->Empty();
    CollectGarbage(RF_NoFlags, true);
    for (int32 I = 0; I < 2; ++I)
    {
        auto &Entries = *Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Defaults[I]);
        if (TestTrue(TEXT("Runtime reward survives without CDO GC edge"), Weak[I].IsValid()))
            Entries.Add(Weak[I].Get());
    }
    if (!Weak[0].IsValid() || !Weak[1].IsValid())
        return false;
    TestEqual(TEXT("Logistics retains five build recipes"), Weak[0]->GetRecipesToUnlock().Num(), 5);
    TestEqual(TEXT("Personnel retains one build recipe"), Weak[1]->GetRecipesToUnlock().Num(), 1);
    for (int32 Pass = 0; Pass < 3; ++Pass)
    {
        UTeleportLogisticsGameInstanceModule::PrepareRewardPresentation();
        CollectGarbage(RF_NoFlags, true);
        for (int32 I = 0; I < 2; ++I)
        {
            auto &Entries = *Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Defaults[I]);
            TestEqual(TEXT("Repeated initialization keeps one reward"), Entries.Num(), 1);
            TestTrue(TEXT("Repeated initialization preserves reward identity"),
                     Entries.Num() == 1 && Entries[0].Get() == Weak[I].Get());
        }
    }
    return true;
}
#endif
