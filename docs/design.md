> Historical design record. For current behaviour see [architecture](architecture.md), [player guide](player-guide.md) and [release acceptance](validation.md).

TeleportLogistics's initial design follows the examples recorded in [ui-design-handoff.md](ui-design-handoff.md): `small_copper_ingot` and `mega_copper_ingot` are independent routes even though both carry copper ingots. A `sink` route accepts mixed items. A `north_farm` route can carry mixed freight to physical sorting machinery elsewhere. The material does not determine an item route's identity or eligibility.

Accepted requirements and implemented policy:

| Topic | First implementation |
| --- | --- |
| Organisation | Channel → named route → physical endpoints. Default is always available without a hub. |
| Optional hubs | Each placed hub creates a channel. Power unlocks naming and management controls. |
| Power failure | Transport continues. Management is unavailable and hub screens/lamps go dark until power returns. |
| Identity | GUIDs identify hubs, routes and endpoints. Names are editable labels. Default uses a zero channel GUID. |
| Naming | 1–64 characters, no control characters or outer whitespace. Route names are unique within a channel, case-insensitively. |
| Multiple inputs | Contribute to the same route with per-endpoint rate limits. |
| Multiple outputs | Share accepted material through persistent round-robin scheduling; blocked receivers are skipped. |
| Mixed items | No automatic material filter. Input and output queues retain arrival order. A blocked first item is not bypassed for a later preferred item. |
| Fluids | Distinct pipe endpoints; first successfully transferred fluid locks a route. Other fluid types cannot enter destination buffers on that route. |
| Fluid recovery | Disable an endpoint, explicitly confirm flushing its local buffer, clear attached pipes separately if necessary. A powered hub can reset an empty route's fluid lock. |
| Congestion | Local buffers fill; upstream intake stops. Nothing is automatically discarded. |
| Reassignment | Drain buffers before changing a route; disable an endpoint if it is actively transferring. |
| Dismantling | Item cargo is added to dismantle refunds. Fluid cargo must first be drained/flushed. Hubs with assigned endpoints cannot be dismantled. |
| Unlock | One Tier 5 milestone unlocks four endpoint variants and the hub. |
| Early limits | Up to 256 routes; directory pages contain 64 endpoints. |

Balance is provisional: item endpoints have a 1,200 items/min ceiling and 64-item buffers; fluid endpoints have a 600 m³/min ceiling and 50 m³ buffers. Scheduling is capped to a quarter-second burst and therefore does not try to catch up indefinitely after long stalls. A fluid scheduling quantum is one m³, so short-term allocation can differ by that amount while remaining balanced over time. Hubs consume 5 MW.

The interface exposes named channels and routes as searchable lists. Endpoints can select existing routes or create one by typing its name. The hub supplies the bonus directory, channel/route paths, media and buffer status, coordinates, live smoothed throughput, route rename/pause controls, and unused-route removal. A powered hub can also manage Default routes. The interface runs through Satisfactory's native interaction stack, follows its dark/orange visual language, handles Escape centrally, and scales down as a single surface for smaller resolutions.

The model family uses an open field chamber for items, a caged pressure vessel for fluids, and an angled dual-screen console with a communications tower for the hub. Teal input signals and orange output signals make direction readable independently of player paint. TeleportLogistics's custom Blender geometry is bevelled, triangulated without zero-area faces, UV-mapped into the native factory atlas, and exported with normals. The standard belt, pipe, and compact power connectors come from the separately attributed Satisfactory Modeling Tools reference; the rest of each machine is custom TeleportLogistics geometry. Unreal consolidates the result into four or five material sections and keeps one stable optimized LOD, avoiding the close-range transitions produced by the former LargeProp reduction chain. Belt origins are `(160, 0, 100)`, pipe origins are `(160, 0, 175)`, and the hub's native compact power origin is `(-155, 88, 285)`. Local positive X faces out from belt and pipe buildings. The hub uses the game's no-power factory material plus a dynamic screen scalar so every authored light is dark while unpowered.

Descriptor icons are transparent orthographic captures of the final 3D models at 256 and 512 pixels. The category and map use separate high-resolution UI glyphs, and the milestone retains its own 256/512 capture. This follows the split between model-capture item art and simple UI marks used by the base game.

Map markers are implemented for endpoints and hubs, using five dedicated transparent icons and cached `channel / route / medium / direction / label` text. Paused routes and disabled endpoints are called out in their labels. They are map-only, use the native Portal filter, do not reveal fog, and refresh after state or topology edits. Native game testing remains necessary to confirm filters, multiplayer replication, and marker cleanup.

Next refinement choices include animation and sound, LOD-distance tuning after native profiling, pipe output pressure policy after native testing, a handheld network configurator, priority/weighted outputs, controller focus/navigation, and refined unlock costs.
