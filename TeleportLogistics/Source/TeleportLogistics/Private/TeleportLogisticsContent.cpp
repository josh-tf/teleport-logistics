#include "TeleportLogisticsContent.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsTravel.h"
#include "TeleportLogisticsBuilding.h"
#include "TeleportLogisticsSubsystem.h"
#include "TeleportLogisticsRemoteCall.h"
#include "Brushes/SlateImageBrush.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
TSubclassOf<UFGItemDescriptor> Part(const TCHAR *Path)
{
    return LoadClass<UFGItemDescriptor>(nullptr, Path);
}
} // namespace
UTeleportLogisticsCategory::UTeleportLogisticsCategory()
{
    mDisplayName = NSLOCTEXT("TeleportLogistics", "Category", "Teleporter");
    mMenuPriority = 40;
    if (auto *Texture = LoadObject<UTexture2D>(nullptr,
                                               TEXT("/TeleportLogistics/Icons/T_TeleporterCategory_128."
                                                    "T_TeleporterCategory_128"),
                                               nullptr, LOAD_NoWarn))
        mCategoryIcon = FSlateImageBrush(Texture, FVector2D(128, 128));
}
UTeleportLogisticsSubCategory::UTeleportLogisticsSubCategory()
{
    mDisplayName = NSLOCTEXT("TeleportLogistics", "SubCategory", "Teleporter Network");
    mMenuPriority = 0;
}
UTeleportLogisticsDescriptor::UTeleportLogisticsDescriptor()
{
    mCategory = UTeleportLogisticsCategory::StaticClass();
    mSubCategories.Add(UTeleportLogisticsSubCategory::StaticClass());
}
void UTeleportLogisticsDescriptor::Icon(const TCHAR *SmallPath, const TCHAR *BigPath)
{
    mSmallIcon = LoadObject<UTexture2D>(nullptr, SmallPath, nullptr, LOAD_NoWarn);
    mPersistentBigIcon = LoadObject<UTexture2D>(nullptr, BigPath, nullptr, LOAD_NoWarn);
}
UTeleportLogisticsItemInputDescriptor::UTeleportLogisticsItemInputDescriptor()
{
    mBuildableClass = ATeleportLogisticsItemInput::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "ItemInputDescriptor", "Item (In)");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "ItemInputShort", "Item (In)");
    mDescription = NSLOCTEXT("TeleportLogistics", "ItemInputDescriptorDescription",
                             "Accepts any conveyor item into a named teleporter route.");
    mMenuPriority = 10;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterItemInput_256.T_TeleporterItemInput_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterItemInput_512.T_TeleporterItemInput_512"));
}
UTeleportLogisticsItemOutputDescriptor::UTeleportLogisticsItemOutputDescriptor()
{
    mBuildableClass = ATeleportLogisticsItemOutput::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "ItemOutputDescriptor", "Item (Out)");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "ItemOutputShort", "Item (Out)");
    mDescription = NSLOCTEXT("TeleportLogistics", "ItemOutputDescriptorDescription",
                             "Outputs items received by every input on the same route.");
    mMenuPriority = 20;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterItemOutput_256.T_TeleporterItemOutput_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterItemOutput_512.T_TeleporterItemOutput_512"));
}
UTeleportLogisticsFluidInputDescriptor::UTeleportLogisticsFluidInputDescriptor()
{
    mBuildableClass = ATeleportLogisticsFluidInput::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "FluidInputDescriptor", "Fluid (In)");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "FluidInputShort", "Fluid (In)");
    mDescription = NSLOCTEXT("TeleportLogistics", "FluidInputDescriptorDescription",
                             "Accepts pipe contents into a type-safe fluid route.");
    mMenuPriority = 30;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterFluidInput_256.T_TeleporterFluidInput_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterFluidInput_512.T_TeleporterFluidInput_512"));
}
UTeleportLogisticsFluidOutputDescriptor::UTeleportLogisticsFluidOutputDescriptor()
{
    mBuildableClass = ATeleportLogisticsFluidOutput::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "FluidOutputDescriptor", "Fluid (Out)");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "FluidOutputShort", "Fluid (Out)");
    mDescription = NSLOCTEXT("TeleportLogistics", "FluidOutputDescriptorDescription",
                             "Supplies fluid received by every input on the same route.");
    mMenuPriority = 40;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterFluidOutput_256.T_TeleporterFluidOutput_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterFluidOutput_512.T_TeleporterFluidOutput_512"));
}
UTeleportLogisticsHubDescriptor::UTeleportLogisticsHubDescriptor()
{
    mBuildableClass = ATeleportLogisticsHub::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "HubDescriptor", "Teleporter Hub");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "HubShort", "Hub");
    mDescription = NSLOCTEXT("TeleportLogistics", "HubDescriptorDescription",
                             "Creates a named channel and provides powered network management.");
    mMenuPriority = 50;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterHub_256.T_TeleporterHub_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterHub_512.T_TeleporterHub_512"));
}

UTeleportLogisticsRecipe::UTeleportLogisticsRecipe()
{
    mManufactoringDuration = 1;
    mProducedIn.Add(TSoftClassPtr<UObject>(
        FSoftObjectPath(TEXT("/Game/FactoryGame/Equipment/BuildGun/BP_BuildGun.BP_BuildGun_C"))));
    mIngredients.Add(FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/IronPlateReinforced/"
                                           "Desc_IronPlateReinforced.Desc_IronPlateReinforced_C")),
                                 4));
    mIngredients.Add(FItemAmount(
        Part(TEXT("/Game/FactoryGame/Resource/Parts/CircuitBoard/Desc_CircuitBoard.Desc_CircuitBoard_C")),
        4));
    mIngredients.Add(
        FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/Cable/Desc_Cable.Desc_Cable_C")), 10));
}
UTeleportLogisticsItemInputRecipe::UTeleportLogisticsItemInputRecipe()
{
    mProduct.Add(FItemAmount(UTeleportLogisticsItemInputDescriptor::StaticClass(), 1));
}
UTeleportLogisticsItemOutputRecipe::UTeleportLogisticsItemOutputRecipe()
{
    mProduct.Add(FItemAmount(UTeleportLogisticsItemOutputDescriptor::StaticClass(), 1));
}
UTeleportLogisticsFluidInputRecipe::UTeleportLogisticsFluidInputRecipe()
{
    mProduct.Add(FItemAmount(UTeleportLogisticsFluidInputDescriptor::StaticClass(), 1));
}
UTeleportLogisticsFluidOutputRecipe::UTeleportLogisticsFluidOutputRecipe()
{
    mProduct.Add(FItemAmount(UTeleportLogisticsFluidOutputDescriptor::StaticClass(), 1));
}
UTeleportLogisticsHubRecipe::UTeleportLogisticsHubRecipe()
{
    mProduct.Add(FItemAmount(UTeleportLogisticsHubDescriptor::StaticClass(), 1));
    mIngredients.Add(FItemAmount(
        Part(TEXT("/Game/FactoryGame/Resource/Parts/Computer/Desc_Computer.Desc_Computer_C")), 5));
}
UTeleportLogisticsUnlock::UTeleportLogisticsUnlock()
{
    mRecipes = {UTeleportLogisticsItemInputRecipe::StaticClass(), UTeleportLogisticsItemOutputRecipe::StaticClass(),
                UTeleportLogisticsFluidInputRecipe::StaticClass(), UTeleportLogisticsFluidOutputRecipe::StaticClass(),
                UTeleportLogisticsHubRecipe::StaticClass()};
}
UTeleportLogisticsMilestone::UTeleportLogisticsMilestone()
{
    mType = ESchematicType::EST_Milestone;
    mTechTier = 5;
    mSmallSchematicIcon = LoadObject<UTexture2D>(
        nullptr, TEXT("/TeleportLogistics/Icons/T_TeleporterMilestone_256.T_TeleporterMilestone_256"), nullptr, LOAD_NoWarn);
    if (auto *BigIcon = LoadObject<UTexture2D>(
            nullptr, TEXT("/TeleportLogistics/Icons/T_TeleporterMilestone_512.T_TeleporterMilestone_512"), nullptr, LOAD_NoWarn))
        mSchematicIcon = FSlateImageBrush(BigIcon, FVector2D(512, 512));
    mDisplayName = NSLOCTEXT("TeleportLogistics", "Milestone", "Teleport Logistics");
    mDescription = NSLOCTEXT("TeleportLogistics", "MilestoneDescription",
                             "Connect distant factories with mixed-item and fluid routes. Add powered hubs "
                             "to organise and monitor your network.");
    mMenuPriority = 50;
    mTimeToComplete = 120;
    mCost.Add(FItemAmount(
        Part(TEXT("/Game/FactoryGame/Resource/Parts/CircuitBoard/Desc_CircuitBoard.Desc_CircuitBoard_C")),
        100));
    mCost.Add(FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/IronPlateReinforced/"
                                    "Desc_IronPlateReinforced.Desc_IronPlateReinforced_C")),
                          100));
    mCost.Add(FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/Cable/Desc_Cable.Desc_Cable_C")), 200));
    mUnlocks.Add(CreateDefaultSubobject<UTeleportLogisticsUnlock>(TEXT("TeleporterRecipes")));
}
UTeleportLogisticsGameInstanceModule::UTeleportLogisticsGameInstanceModule()
{
    bRootModule = true;
    // SML registers this class before game modes create their player-owned
    // RCOs, including controllers for late-joining multiplayer clients.
    RemoteCallObjects.Add(UTeleportLogisticsRemoteCall::StaticClass());
    RemoteCallObjects.Add(UTeleportLogisticsTravelRemote::StaticClass());
}
UTeleportLogisticsWorldModule::UTeleportLogisticsWorldModule()
{
    bRootModule = true;
    ModSubsystems.Add(ATeleportLogisticsSubsystem::StaticClass());
    mSchematics.Add(UTeleportLogisticsMilestone::StaticClass());
    mSchematics.Add(UTeleportLogisticsTravelMilestone::StaticClass());
}

UTeleportLogisticsTravelDescriptor::UTeleportLogisticsTravelDescriptor()
{
    mBuildableClass = ATeleportLogisticsTravelHub::StaticClass();
    mDisplayName = NSLOCTEXT("TeleportLogistics", "TravelDescriptor", "Personnel Teleporter");
    mAbbreviatedDisplayName = NSLOCTEXT("TeleportLogistics", "TravelShort", "Personnel");
    mDescription = NSLOCTEXT("TeleportLogistics", "TravelDescriptorInfo",
                             "Select any powered Personnel Teleporter as a destination. Requires 50 MW at "
                             "both ends. Choose a destination from the directory to travel.");
    mMenuPriority = 60;
    Icon(TEXT("/TeleportLogistics/Icons/T_TeleporterTravelHub_256.T_TeleporterTravelHub_256"),
         TEXT("/TeleportLogistics/Icons/T_TeleporterTravelHub_512.T_TeleporterTravelHub_512"));
}
UTeleportLogisticsTravelRecipe::UTeleportLogisticsTravelRecipe()
{
    mIngredients.Empty();
    mIngredients.Add(FItemAmount(
        Part(TEXT(
            "/Game/FactoryGame/Resource/Parts/QuantumCrystal/Desc_QuantumCrystal.Desc_QuantumCrystal_C")),
        20));
    mIngredients.Add(FItemAmount(
        Part(TEXT("/Game/FactoryGame/Resource/Parts/ComputerSuper/Desc_ComputerSuper.Desc_ComputerSuper_C")),
        10));
    mIngredients.Add(FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/MotorLightweight/"
                                           "Desc_MotorLightweight.Desc_MotorLightweight_C")),
                                 5));
    mProduct.Add(FItemAmount(UTeleportLogisticsTravelDescriptor::StaticClass(), 1));
}
UTeleportLogisticsTravelUnlock::UTeleportLogisticsTravelUnlock()
{
    mRecipes = {UTeleportLogisticsTravelRecipe::StaticClass()};
}
UTeleportLogisticsTravelMilestone::UTeleportLogisticsTravelMilestone()
{
    mType = ESchematicType::EST_Milestone;
    mTechTier = 9;
    mMenuPriority = 60;
    mTimeToComplete = 300;
    mDisplayName = NSLOCTEXT("TeleportLogistics", "TravelMilestone", "Teleport Personnel Transport");
    mDescription =
        NSLOCTEXT("TeleportLogistics", "TravelMilestoneInfo",
                  "Unlock a network of powered Personnel Teleporters for long-distance Pioneer travel.");
    mSmallSchematicIcon = LoadObject<UTexture2D>(
        nullptr, TEXT("/TeleportLogistics/Icons/T_TeleporterPersonnelMilestone_256.T_TeleporterPersonnelMilestone_256"), nullptr,
        LOAD_NoWarn);
    if (auto *I = LoadObject<UTexture2D>(
            nullptr, TEXT("/TeleportLogistics/Icons/T_TeleporterPersonnelMilestone_512.T_TeleporterPersonnelMilestone_512"),
            nullptr, LOAD_NoWarn))
        mSchematicIcon = FSlateImageBrush(I, FVector2D(512));
    mCost.Add(FItemAmount(
        Part(TEXT(
            "/Game/FactoryGame/Resource/Parts/QuantumCrystal/Desc_QuantumCrystal.Desc_QuantumCrystal_C")),
        200));
    mCost.Add(FItemAmount(
        Part(TEXT("/Game/FactoryGame/Resource/Parts/ComputerSuper/Desc_ComputerSuper.Desc_ComputerSuper_C")),
        100));
    mCost.Add(FItemAmount(Part(TEXT("/Game/FactoryGame/Resource/Parts/MotorLightweight/"
                                    "Desc_MotorLightweight.Desc_MotorLightweight_C")),
                          50));
    mUnlocks.Add(CreateDefaultSubobject<UTeleportLogisticsTravelUnlock>(TEXT("PersonnelTravelRecipe")));
}

void UTeleportLogisticsGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    if (Phase == ELifecyclePhase::INITIALIZATION)
        PrepareRewardPresentation();
    Super::DispatchLifecycleEvent(Phase);
    if (Phase == ELifecyclePhase::INITIALIZATION)
        UTeleportLogisticsMapHooks::Register(GetGameInstance());
}

void UTeleportLogisticsGameInstanceModule::PrepareRewardPresentation()
{
    // Finish native CDO construction before loading the stock reward Blueprint:
    // its UI dependencies can lead back to equipment and construction recipes.
    // Native CDOs can live in the disregard-for-GC set. Adding a reflected
    // reference to them at runtime does not guarantee traversal by the GC.
    // Pin these two replacement unlocks for the lifetime of the module.
    static TArray<TStrongObjectPtr<UFGUnlockRecipe>> PresentationRoots;
    UFGSchematic *Schematics[] = {GetMutableDefault<UTeleportLogisticsMilestone>(),
                                  GetMutableDefault<UTeleportLogisticsTravelMilestone>()};
    auto *UnlockClass = LoadClass<UFGUnlockRecipe>(
        nullptr, TEXT("/Game/FactoryGame/Unlocks/BP_UnlockRecipe.BP_UnlockRecipe_C"));
    auto *UnlocksProperty = FindFProperty<FArrayProperty>(UFGSchematic::StaticClass(), TEXT("mUnlocks"));
    auto *RecipesProperty = FindFProperty<FArrayProperty>(UFGUnlockRecipe::StaticClass(), TEXT("mRecipes"));
    if (!UnlockClass || !UnlocksProperty || !RecipesProperty)
    {
        UE_LOG(LogTeleportLogistics, Error,
               TEXT("TeleportLogistics: reward presentation unavailable; keeping native recipe unlocks"));
        return;
    }
    for (UFGSchematic *Schematic : Schematics)
    {
        auto &Unlocks = *UnlocksProperty->ContainerPtrToValuePtr<TArray<TObjectPtr<UFGUnlock>>>(Schematic);
        for (auto &Entry : Unlocks)
        {
            auto *Native = Cast<UFGUnlockRecipe>(Entry.Get());
            if (!Native || Native->GetClass() == UnlockClass)
                continue;
            auto *Presentation = NewObject<UFGUnlockRecipe>(Schematic, UnlockClass, NAME_None, RF_Transient);
            *RecipesProperty->ContainerPtrToValuePtr<TArray<TSubclassOf<UFGRecipe>>>(Presentation) =
                Native->GetRecipesToUnlock();
            PresentationRoots.Emplace(Presentation);
            Entry = Presentation;
        }
    }
    UE_LOG(LogTeleportLogistics, Display,
           TEXT("TeleportLogistics: milestone reward presentation initialized after native class construction"));
}
