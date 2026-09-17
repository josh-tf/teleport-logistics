#pragma once
#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "FGSaveInterface.h"
#include "TeleportLogisticsTypes.h"
#include "Core/TeleportLogisticsScheduler.h"
#include "TeleportLogisticsSubsystem.generated.h"

class ATeleportLogisticsBuilding;
class ATeleportLogisticsEndpoint;
class ATeleportLogisticsHub;

// Success is decided by comparing these against what the subsystem returned, so both
// sides must name the same constant rather than repeat the prose.
namespace TeleportLogisticsResult
{
inline const TCHAR *BufferBusy = TEXT("The endpoint is transferring. Try again.");
inline const TCHAR *BufferEmpty = TEXT("This endpoint is empty.");
inline const TCHAR *BufferCollected = TEXT("Buffered items moved to your inventory.");
inline const TCHAR *BufferPartlyCollected = TEXT("Your inventory filled up. Some items are still buffered.");
inline const TCHAR *BufferNotItems = TEXT("Only item endpoints can be emptied by hand.");
inline const TCHAR *FlushedFluid = TEXT("Local fluid buffer flushed. Attached pipes are unchanged.");
inline const TCHAR *FlushBlockedFluid = TEXT("Disable the fluid endpoint before flushing its local buffer.");
inline const TCHAR *EndpointSaved = TEXT("Endpoint saved.");
inline const TCHAR *HubRenamed = TEXT("Hub renamed. Connections are unchanged.");
inline const TCHAR *RoutePaused = TEXT("Route paused; output buffers can drain.");
inline const TCHAR *RouteResumed = TEXT("Route resumed.");
inline const TCHAR *RouteRenamed = TEXT("Route renamed without disconnecting endpoints.");
inline const TCHAR *FluidReset = TEXT("Fluid type reset. Connected pipes must also contain the new fluid or be empty.");
inline const TCHAR *RouteRemoved = TEXT("Unused route removed.");
}

UCLASS()
class TELEPORTLOGISTICS_API ATeleportLogisticsSubsystem : public AModSubsystem, public IFGSaveInterface
{
    GENERATED_BODY()
  public:
    ATeleportLogisticsSubsystem();
    static ATeleportLogisticsSubsystem *Get(UWorld *World);
    static FCriticalSection Mutex;
    virtual void Tick(float Dt) override;
    virtual bool ShouldSave_Implementation() const override
    {
        return true;
    }
    virtual bool NeedTransform_Implementation() override
    {
        return false;
    }
    void Register(ATeleportLogisticsBuilding *Building);
    void Unregister(ATeleportLogisticsBuilding *Building, bool Dismantled = false);
    // These methods are called on the server game thread with Mutex held by RCO.
    FString Configure(ATeleportLogisticsEndpoint *Endpoint, FGuid Channel, const FString &RouteName,
                      const FString &Label);
    FString RenameHub(ATeleportLogisticsHub *Hub, const FString &Name);
    FString Control(ATeleportLogisticsHub *Hub, FGuid Route, const FString &Action, const FString &Name);
    FTeleportLogisticsSnapshot Snapshot(FGuid Route, int32 Page) const;
    // The fluid a route is pinned to, or null while the route is unpinned. Read from
    // factory worker threads, so the caller holds Mutex.
    TSubclassOf<class UFGItemDescriptor> LockedFluid(FGuid Route) const;
    bool ChannelEmpty(FGuid Channel) const;
    bool Managed(FGuid Channel) const;
    void InvalidateMap()
    {
        MapDirty = true;
    }
    static bool ValidName(const FString &Name);
    UPROPERTY(SaveGame)
    TArray<FTeleportLogisticsChannel> Channels;
    UPROPERTY(SaveGame)
    TArray<FTeleportLogisticsRoute> Routes;

  private:
    struct FRouteRuntime
    {
        TArray<TWeakObjectPtr<ATeleportLogisticsEndpoint>> Inputs;
        TArray<TWeakObjectPtr<ATeleportLogisticsEndpoint>> Outputs;
        teleport_logistics::Cursor Cursor;
        std::vector<teleport_logistics::Budget> InputBudgets, OutputBudgets;
    };
    TMap<FGuid, FRouteRuntime> Index;
    TMap<FGuid, TWeakObjectPtr<ATeleportLogisticsBuilding>> Buildings;
    bool Dirty = true;
    bool MapDirty = true;
    void RefreshMap();
    void Reindex();
};
