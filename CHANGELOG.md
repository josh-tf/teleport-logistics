# Changelog

## 0.5.12

The buildings snapped themselves onto sign areas of other machines while being placed. Their rear sign pads were built on the placement hologram as well as the finished building, so each machine carried a sign point of its own; the pads are there for the game's wall signs to attach to, and are now created on the building only.

Cross-map travel stuttered. Both the destination directory and the journey itself searched every actor in the world to find the handful of Personnel Teleporters; hubs now register themselves as they come up and both look the destination up directly. Hub power draw is a constant and is no longer written to the circuit every frame.

Travelling between Personnel Teleporters froze the game for around two seconds. The hub asked for its map and compass art on every request, and the map asks throughout a journey, so the load landed while the destination was streaming in and the engine stopped the game thread until all four hundred packages in flight had finished. The same shape was in the unpowered signal material on every building, resolved the first time one came into view. All of them are now resolved when the building is constructed.

Sign icon search matched only the authored icon name, which most icons do not have, and so found almost nothing; it now also matches the item and texture names. The icon grid no longer pages: only two of its rows fit on screen, so it scrolls instead, and each icon loads as it comes into view.

## 0.5.11

Tier 9 costs and the Personnel Teleporter recipe called for Quantum Crystal, which the game has discontinued in favour of Time Crystal; its milestone tile showed the discontinued item and its replacement notice. Both now use Time Crystal at the same amounts, matching what the README and the mod page already stated.

The twin delivery rails on the item endpoint roofs had no top. Their faces were wound inward, so the sloped upper face was culled and each rail read as an open channel; the hub console frame was built from the same helper and had the same fault.

Building bodies use the rough metal surface rather than the painted composite they were mapped to, which read flat beside the stock machines, and their surface detail tiles at the stock rate.

## 0.5.10: first public release

Plugin, module, native types, asset mount, source filenames, build scripts and viewer keys use the
`TeleportLogistics` identity. Earlier development builds used other names and were never published, so no
compatibility redirects ship.

In game the endpoints are named `Item (In)`, `Item (Out)`, `Fluid (In)` and `Fluid (Out)`, alongside Teleporter
Hub and Personnel Teleporter. The build menu clamps a tile label to two lines, and a building descriptor carries
one display name for the menu, the look-at panel and the use prompt alike, so the short forms are the names
everywhere. The configure window keeps the descriptive Item Teleporter and Fluid Teleporter headings. The Tier 5
and Tier 9 milestones read Teleport Logistics and Teleport Personnel Transport, and the two map categories read
Teleport Logistics and Teleport Personnel. Asset object names, the saved endpoint identity field, the screen
material scalar and subobject names use the `Teleporter` spelling, matching the buildings they belong to.

Hovering an endpoint shows the channel and route it is wired to, below the use prompt, and names the building
only when you have given it one of your own. Identification plates are lettered for the current building names.


## Earlier versions

0.5.10 is the first public release. Versions 0.3.2 through 0.5.9 were internal development builds and were never
published. Their dated engineering records ship in the source and publication kit archives.
