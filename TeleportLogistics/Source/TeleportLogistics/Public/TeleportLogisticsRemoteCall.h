#pragma once
#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "TeleportLogisticsTypes.h"
#include "TeleportLogisticsRemoteCall.generated.h"

class ATeleportLogisticsBuilding;
class ATeleportLogisticsEndpoint;
class ATeleportLogisticsHub;
DECLARE_MULTICAST_DELEGATE_OneParam(FTeleportLogisticsSnapshotReceived, const FTeleportLogisticsSnapshot &);

UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsRemoteCall : public UFGRemoteCallObject
{
    GENERATED_BODY()
  public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    UPROPERTY(Replicated)
    bool Registered = true;
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSnapshot(ATeleportLogisticsBuilding *Context, FGuid Route, int32 Page, uint32 RequestId);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerConfigure(ATeleportLogisticsEndpoint *Endpoint, FGuid Channel, const FString &Route, const FString &Label);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPasteRoute(ATeleportLogisticsEndpoint *Endpoint, FGuid Route);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggle(ATeleportLogisticsEndpoint *Endpoint);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerFlushFluid(ATeleportLogisticsEndpoint *Endpoint);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTakeBuffer(ATeleportLogisticsEndpoint *Endpoint);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRenameHub(ATeleportLogisticsHub *Hub, const FString &Name);
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerControl(ATeleportLogisticsHub *Hub, FGuid Route, const FString &Action, const FString &Name);
    UFUNCTION(Client, Reliable)
    void ClientSnapshot(const FTeleportLogisticsSnapshot &Snapshot);
    FTeleportLogisticsSnapshotReceived OnSnapshot;
    uint32 AllocateSnapshotRequest() { return ++SnapshotSequence; }

  private:
    FString ContextError(ATeleportLogisticsBuilding *Context) const;
    bool ValidateContext(ATeleportLogisticsBuilding *Context) const { return ContextError(Context).IsEmpty(); }
    uint32 SnapshotSequence = 0;
    bool RateLimit(bool Mutation);
    double LastRead = -100;
    double LastWrite = -100;
    FString LastMessage;
    FGuid MessageContext;
    uint32 MutationRevision = 0;
    bool MutationSucceeded = false;
    void Result(ATeleportLogisticsBuilding *Context, const FString &Message, bool Succeeded);
};
