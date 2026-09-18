# Changelog

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
