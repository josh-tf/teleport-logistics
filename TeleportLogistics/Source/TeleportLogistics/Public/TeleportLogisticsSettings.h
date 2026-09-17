#pragma once
#include "CoreMinimal.h"
#include "Configuration/ModConfiguration.h"
#include "TeleportLogisticsSettings.generated.h"

/**
 * Client-side preferences, shown under Mods in the pause menu.
 *
 * Everything here affects only the local interface. Transport rates, power draw,
 * the route ceiling and travel timings stay compile-time constants: SML writes
 * configuration to a per-client file, so exposing a gameplay number would let one
 * client disagree with the host. Map markers are deliberately absent for the same
 * reason, since representations are created under HasAuthority.
 */
UCLASS()
class TELEPORTLOGISTICS_API UTeleportLogisticsConfig : public UModConfiguration
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsConfig();

    /** Seconds between snapshot polls while an interaction window is open. */
    static float RefreshSeconds(const UObject *Context);
    /** Whether the look-at panel names the channel and route. */
    static bool ShowRouteInLookAt(const UObject *Context);
    /** Whether flushing a fluid endpoint asks for confirmation first. */
    static bool ConfirmFluidFlush(const UObject *Context);
};
