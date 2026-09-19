# Changelog

## 0.5.12

- Buildings no longer snap themselves onto the sign areas of other machines while being placed. The rear sign pads were built on the placement hologram as well as the building, and exist for wall signs to attach to.
- Travelling between Personnel Teleporters no longer freezes the game for around two seconds. Arriving landed in an area that was not in memory, so the engine stopped the game thread until it had streamed the destination in. A powered teleporter now keeps its own surroundings loaded, as the game's own portals do by staying linked.
- A journey now takes 1.5 seconds nearby, rising to 3.5 across the map. The transit time had been supplied entirely by the engine stalling on streaming, so removing that stall left the hop instantaneous.
- A stutter whenever a teleporter came into view. Every building loaded its map marker and signal material as it spawned, and a blocking load while the world is streaming flushes every package in flight. All of it resolves once now, before anything is in motion.
- Travel no longer searches every actor in the world to find a destination, and hub power draw is no longer rewritten every frame.
- Blueprint Designer copies no longer join the live network: they were writing channels into the save, moving real cargo, and taking a real building's routes with them when the designer was cleared. Buffered cargo is no longer saved into a blueprint.
- Dismantling the last endpoint on a route now frees the route name, which previously only a powered hub could do.
- Destination icons now appear. A lookup that failed while the icon database was still replicating was cached as the placeholder for the rest of the window's life.
- Sign icon search matches item and texture names, not just the authored icon name most icons lack.
- The sign icon grid scrolls as one list instead of paging two visible rows at a time.
- The Personnel Teleporter has a console on both sides, so it reads properly in the build menu and from either approach, and its nameplate sits on the console rather than on posts above it.
- Corrected three display faults: the hub console sampled its glyph a quarter turn out, square icons were stretched onto rectangular screens, and hazard stripes flattened out on the two widest buildings.
- Customizer patterns apply to all six buildings, as the documentation already described.
- Release packaging refuses an archive older than the sources it claims to contain, so a partial rebuild cannot re-ship a previous build's binaries.

## 0.5.11

- Tier 9 costs and the Personnel Teleporter recipe use Time Crystal, replacing the discontinued Quantum Crystal at the same amounts.
- Closed the tops of the twin delivery rails on the item endpoint roofs, and the hub console frame built from the same helper.
- Building bodies use the rough metal surface at stock detail tiling, rather than the painted composite that read flat beside stock machines.

## 0.5.10: first public release

- Six buildings: `Item (In)`, `Item (Out)`, `Fluid (In)`, `Fluid (Out)`, Teleporter Hub and Personnel Teleporter, unlocked at Tier 5 Teleport Logistics and Tier 9 Teleport Personnel Transport.
- One display name per building carries the build menu, look-at panel and use prompt alike. The configure window keeps the descriptive Item Teleporter and Fluid Teleporter headings.
- Hovering an endpoint shows the channel and route it is wired to, and names the building only when you have given it a name of your own.
- Separate Teleport Logistics and Teleport Personnel map categories.

## Earlier versions

0.5.10 is the first public release. Versions 0.3.2 through 0.5.9 were internal development builds and were never published. Their dated engineering records ship in the source and publication kit archives.
