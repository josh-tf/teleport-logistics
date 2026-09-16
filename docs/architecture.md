# TeleportLogistics architecture

The mod is published as Teleport Logistics; the native module and asset mount use `TeleportLogistics`. Serialized names use the Teleporter spelling, matching the building names. SML is the only gameplay-mod dependency.

| Source | Responsibility |
| --- | --- |
| `Core/TeleportLogisticsScheduler.h` | Engine-independent rate budgets and fair source/receiver selection; shared with standalone tests. |
| `TeleportLogisticsBuilding` | Five logistics buildings, belt/pipe adapters, local buffers, saved identity and rendering. |
| `TeleportLogisticsSubsystem` | Server-owned channels/routes, membership cache, transport commits, fluid locks and directory snapshots. |
| `TeleportLogisticsRemoteCall` | Player-owned logistics RPCs, context validation, throttling and feedback. |
| `TeleportLogisticsWidget` | Logistics interaction UI and draft/server-state handling. |
| `TeleportLogisticsTravel` | Sixth building, portal traversal, landing validation, direction hologram and Personnel RPCs. |
| `TeleportLogisticsTravelWidget` | Destination paging, sign-icon selection and travel interaction lifecycle. |
| `TeleportLogisticsMap` | Separate runtime logistics/personnel representation types and native map category labels. |
| `TeleportLogisticsContent` | Tier 5/9 costs, recipes, descriptors, SML modules and reward presentation. |
| `TeleportLogisticsLog` | Dedicated log category; normal UI lifecycle details are Verbose. |

## Transport and persistence

The server schedules logistics at 20 Hz. Route membership is cached until topology changes. Per-endpoint budgets bound sending and receiving; empty sources and full receivers are skipped. Items retain their full native item state and FIFO order. Cargo lives in endpoint buffers, not a pooled warehouse.

A common mutex protects topology and cargo commits. Calls into adjacent native belts/pipes happen outside that lock; in-flight flags prevent reconfiguration, flushing or dismantling during local I/O. Native pipes remain responsible for pressure and acceptance. The standalone scheduler tests do not model native pipe hydraulics.

Saved state includes GUIDs, cargo, channel/route records, pause state and fluid locks. Transient actor indices and fairness cursors rebuild on load. Duplicate copied identities get fresh GUIDs. Personnel names and persistent icon IDs are saved and replicated. Dismantled Personnel hubs are immediately excluded from the directory.

## Lifecycle and networking

The root game-instance module registers player-owned remote-call objects before player controllers are created. The world module registers the subsystem and both milestones. The subsystem spawns on the server. Logistics snapshots contain bounded scalar directory data rather than depending on far-away building actor relevancy.

Server RPCs check live actor/world/range/authority and payload limits. Operations re-resolve current state; clients cannot directly mutate cargo. Logistics reads and writes are throttled independently. Context and sequence IDs prevent an old panel or query from replacing the active UI. Personnel directories page 32 rows; icon picking pages 42 local database entries. Cooperative sessions share channels; no private ownership system is implemented.

Personnel travel validates power, busy state, health, vehicle state and landing clearance, then invokes native portal traversal. Both hubs are reserved for 30 seconds. Completion attempts a collision-checked landing at the marked front and aligns character/controller yaw. Actual streaming and multiplayer arrival must be exercised in the retail game.

## Crash-sensitive boundaries

Native milestone CDOs create native unlock subobjects only. After native construction, game-instance initialization replaces the presentation with stock `BP_UnlockRecipe` objects. Explicit `TStrongObjectPtr` roots pin those two runtime objects because native CDOs may be outside normal GC traversal. Do not move Blueprint loading back into constructors or remove those roots. The regression test forces GC without the CDO array references and checks reward survival and idempotence.

Rendering uses game-thread timers, cleared on EndPlay; factory ticks only maintain power consumption. Dedicated servers skip visual timers. Power-off material overrides are restored after Customizer resets. The custom detail material remains opaque following the earlier masked-shader crash. The temporary global BuildGun construction hook has been removed from the release candidate.

Map category extension happens only at runtime; do not serialize its extra enum values into editor assets. Its Blueprint hook carrier is strongly rooted. Failure to resolve native map widgets leaves fallback behaviour and a log message. The existing access-transformer file grants narrowly scoped friend access to logistics buildings and the Personnel portal entry point. This pass adds no transformers; changing them can rebuild the SDK.

Widgets use the native interaction stack, close only their own panel and clear delegates/timers on teardown. Textures used by Slate brushes are retained, and sign images fit their native aspect ratio into square cells.
