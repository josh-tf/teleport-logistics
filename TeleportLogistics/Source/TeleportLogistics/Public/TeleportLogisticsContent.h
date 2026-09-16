#pragma once
#include "CoreMinimal.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "Resources/FGBuildingDescriptor.h"
#include "FGRecipe.h"
#include "FGSchematic.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Module/GameInstanceModule.h"
#include "Module/GameWorldModule.h"
#include "TeleportLogisticsContent.generated.h"

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsCategory : public UFGBuildCategory
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsCategory();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsSubCategory : public UFGBuildSubCategory
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsSubCategory();
};
UCLASS(Abstract)
class TELEPORTLOGISTICS_API UTeleportLogisticsDescriptor : public UFGBuildingDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsDescriptor();

  protected:
    void Icon(const TCHAR *SmallPath, const TCHAR *BigPath);
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsItemInputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemInputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsItemOutputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemOutputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsFluidInputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidInputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsFluidOutputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidOutputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsHubDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsHubDescriptor();
};
UCLASS(Abstract)
class TELEPORTLOGISTICS_API UTeleportLogisticsRecipe : public UFGRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsItemInputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemInputRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsItemOutputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemOutputRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsFluidInputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidInputRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsFluidOutputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidOutputRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsHubRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsHubRecipe();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsUnlock : public UFGUnlockRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsUnlock();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsMilestone : public UFGSchematic
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsMilestone();
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsGameInstanceModule : public UGameInstanceModule
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsGameInstanceModule();
    UFUNCTION(BlueprintCallable) static void PrepareRewardPresentation();
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsWorldModule : public UGameWorldModule
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsWorldModule();
};

UCLASS() class TELEPORTLOGISTICS_API UTeleportLogisticsTravelDescriptor : public UTeleportLogisticsDescriptor { GENERATED_BODY() public: UTeleportLogisticsTravelDescriptor(); };
UCLASS() class TELEPORTLOGISTICS_API UTeleportLogisticsTravelRecipe : public UTeleportLogisticsRecipe { GENERATED_BODY() public: UTeleportLogisticsTravelRecipe(); };
UCLASS() class TELEPORTLOGISTICS_API UTeleportLogisticsTravelUnlock : public UFGUnlockRecipe { GENERATED_BODY() public: UTeleportLogisticsTravelUnlock(); };
UCLASS() class TELEPORTLOGISTICS_API UTeleportLogisticsTravelMilestone : public UFGSchematic { GENERATED_BODY() public: UTeleportLogisticsTravelMilestone(); };
