#pragma once
#include "CoreMinimal.h"
#include "TeleportLogisticsTypes.generated.h"

UENUM(BlueprintType)
enum class ETeleportLogisticsMedium : uint8
{
    Items,
    Fluid
};

USTRUCT(BlueprintType)
struct FTeleportLogisticsChannel
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FGuid Id;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FString Name;
};

USTRUCT(BlueprintType)
struct FTeleportLogisticsRoute
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FGuid Id;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FGuid Channel;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FString Name;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    ETeleportLogisticsMedium Medium = ETeleportLogisticsMedium::Items;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    bool Paused = false;
    UPROPERTY(SaveGame)
    TSubclassOf<class UFGItemDescriptor> Fluid;
    UPROPERTY(BlueprintReadOnly)
    double UnitsPerMinute = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 Inputs = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 Outputs = 0;
    UPROPERTY(BlueprintReadOnly)
    bool CanResetFluid = false;
};

USTRUCT()
struct FTeleportLogisticsEndpointView
{
    GENERATED_BODY()
    UPROPERTY()
    FGuid Id;
    UPROPERTY()
    FGuid Route;
    UPROPERTY()
    FString RoutePath;
    UPROPERTY()
    FString Name;
    UPROPERTY()
    FVector Location = FVector::ZeroVector;
    UPROPERTY()
    bool Input = true;
    UPROPERTY()
    ETeleportLogisticsMedium Medium = ETeleportLogisticsMedium::Items;
    UPROPERTY()
    bool Enabled = true;
    UPROPERTY()
    int32 Buffered = 0;
};

USTRUCT()
struct FTeleportLogisticsSnapshot
{
    GENERATED_BODY()
    // Echoed by the owning player's RCO so a delayed reply cannot update a
    // different TeleportLogistics window that was opened in the meantime.
    UPROPERTY()
    FGuid Context;
    UPROPERTY()
    TObjectPtr<AActor> ContextActor = nullptr;
    UPROPERTY()
    uint32 RequestId = 0;
    UPROPERTY()
    TArray<FTeleportLogisticsChannel> Channels;
    UPROPERTY()
    TArray<FTeleportLogisticsRoute> Routes;
    UPROPERTY()
    TArray<FTeleportLogisticsEndpointView> Endpoints;
    UPROPERTY()
    int32 TotalEndpoints = 0;
    UPROPERTY()
    int32 Page = 0;
    UPROPERTY()
    FString Message;
    UPROPERTY()
    bool ContextAvailable = false;
    UPROPERTY()
    bool HubPowered = false;
    UPROPERTY()
    bool EndpointEnabled = false;
    UPROPERTY()
    int32 Buffered = 0;
    UPROPERTY()
    int32 Capacity = 0;
    UPROPERTY()
    FGuid AssignedRoute;
    UPROPERTY()
    FString ContextLabel;
    UPROPERTY()
    uint32 MutationRevision = 0;
    UPROPERTY()
    bool MutationSucceeded = false;
};
