#pragma once
#include "CoreMinimal.h"
#include "FGActorRepresentation.h"
#include "TeleportLogisticsMap.generated.h"
namespace TeleportLogisticsMap {
    TELEPORTLOGISTICS_API ERepresentationType Logistics();
    TELEPORTLOGISTICS_API ERepresentationType Personnel();
    void RegisterTypes();
}
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsMapHooks : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION() static FText CategoryName(ERepresentationType mRepresentationType, FText OriginalValue);
    static void Register(class UGameInstance* Instance);
};
