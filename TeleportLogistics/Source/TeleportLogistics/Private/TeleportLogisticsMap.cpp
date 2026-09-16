#include "TeleportLogisticsMap.h"
#include "TeleportLogisticsLog.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Patching/BlueprintHookManager.h"
#include "Patching/BlueprintHookBlueprint.h"
#include "Patching/BlueprintHookTargetSpecifiers.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
ERepresentationType LogisticsType = ERepresentationType::RT_Portal;
ERepresentationType PersonnelType = ERepresentationType::RT_Portal;
} // namespace
ERepresentationType TeleportLogisticsMap::Logistics()
{
    return LogisticsType;
}
ERepresentationType TeleportLogisticsMap::Personnel()
{
    return PersonnelType;
}
void TeleportLogisticsMap::RegisterTypes()
{
    // Runtime-only additions must never be serialized into cooked editor assets.
    if (GIsEditor || IsRunningCommandlet())
        return;
    UEnum *Types = StaticEnum<ERepresentationType>();
    const FName L(TEXT("ERepresentationType::RT_TeleporterLogistics")),
        P(TEXT("ERepresentationType::RT_TeleporterPersonnel"));
    if (Types->GetValueByName(L) != INDEX_NONE)
    {
        LogisticsType = static_cast<ERepresentationType>(Types->GetValueByName(L));
        PersonnelType = static_cast<ERepresentationType>(Types->GetValueByName(P));
        return;
    }
    TArray<TPair<FName, int64>> Values;
    int64 Last = -1;
    const bool HasMax = Types->ContainsExistingMax();
    for (int32 I = 0; I < Types->NumEnums() - (HasMax ? 1 : 0); ++I)
    {
        const int64 V = Types->GetValueByIndex(I);
        Values.Emplace(Types->GetNameByIndex(I), V);
        Last = FMath::Max(Last, V);
    }
    if (Last > 251)
    {
        UE_LOG(LogTeleportLogistics, Error, TEXT("TeleportLogistics map categories: representation enum is full"));
        return;
    }
    LogisticsType = static_cast<ERepresentationType>(++Last);
    Values.Emplace(L, Last);
    PersonnelType = static_cast<ERepresentationType>(++Last);
    Values.Emplace(P, Last);
    Types->SetEnums(Values, Types->GetCppForm(), EEnumFlags::None, HasMax);
}
FText UTeleportLogisticsMapHooks::CategoryName(ERepresentationType mRepresentationType, FText OriginalValue)
{
    if (LogisticsType != PersonnelType)
    {
        if (mRepresentationType == LogisticsType)
            return NSLOCTEXT("TeleportLogistics", "MapLogistics", "Teleport Logistics");
        if (mRepresentationType == PersonnelType)
            return NSLOCTEXT("TeleportLogistics", "MapPersonnel", "Teleport Personnel");
    }
    return OriginalValue;
}
void UTeleportLogisticsMapHooks::Register(UGameInstance *Instance)
{
    if (GIsEditor || IsRunningCommandlet() || IsRunningDedicatedServer() || !Instance || !GEngine)
        return;
    static TStrongObjectPtr<UHookBlueprintGeneratedClass> Carrier;
    if (Carrier.IsValid())
        return;
    auto *Widget = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/Minimap/MapFilters/"
                                                        "BPW_MapFilterCategories.BPW_MapFilterCategories_C"));
    auto *Target = Widget ? Widget->FindFunctionByName(TEXT("GetCategoryName")) : nullptr;
    auto *Manager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
    if (!Target || !Manager)
    {
        UE_LOG(LogTeleportLogistics, Warning, TEXT("TeleportLogistics map category labels: native widget unavailable"));
        return;
    }
    Carrier.Reset(NewObject<UHookBlueprintGeneratedClass>(GetTransientPackage()));
    FBlueprintHookDefinition Hook;
    Hook.TargetFunction = Target;
    Hook.HookFunction = StaticClass()->FindFunctionByName(TEXT("CategoryName"));
    Hook.Type = EBlueprintFunctionHookType::RedirectHook;
    Hook.InsertLocation = EBlueprintFunctionHookInsertLocation::ReplaceTarget;
    Hook.TargetSelectionMode = EBlueprintFunctionHookTargetSelectionMode::All;
    Hook.TargetSpecifier = NewObject<UBlueprintHookTargetSpecifier_ReturnValue>(Carrier.Get());
    Carrier->HookDescriptors.Add(Hook);
    Manager->RegisterBlueprintHook(Instance, Carrier.Get());
}
