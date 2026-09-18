# Buildings

Six buildings, all in the **Teleporter** build category under **Teleporter Network**.

| Build menu | Power | Local buffer | Connection |
| --- | --- | --- | --- |
| Item (In) | none | 64 items | one conveyor input |
| Item (Out) | none | 64 items | one conveyor output |
| Fluid (In) | none | 50 m³ | one pipe input |
| Fluid (Out) | none | 50 m³ | one pipe output |
| Teleporter Hub | 5 MW | none | none |
| Personnel Teleporter | 50 MW | none | none |

The build menu clamps a tile label to two lines, so the endpoints use short names. The same name appears when you look at a placed building and in the use prompt. The configure window keeps the fuller Item Teleporter and Fluid Teleporter headings.

<!-- IMG: WIKI_BUILDINGS_LINEUP, all six buildings placed side by side, powered -->

## Item endpoints

**Item (In)** takes anything a conveyor can carry and sends it into its route. **Item (Out)** takes whatever arrives on that route and puts it on a belt.

Mixed cargo is fine. Items keep their order and their state, so partially-used equipment and anything with durability arrives as it left. There is no filtering and no shared warehouse: an output receives whatever the inputs on its route send.

Several inputs on one route merge their cargo. Several outputs on one route share what is available and skip any that are backed up, so one full receiver does not stall the others.

Each endpoint holds 64 individual items locally. That buffer exists to smooth belt timing, not to store anything. Treat a persistently full buffer as a sign the far end cannot keep up.

<!-- IMG: WIKI_BUILDINGS_ITEM, Item (In) and Item (Out) with belts attached, window open -->

## Fluid endpoints

**Fluid (In)** and **Fluid (Out)** do the same job for liquids and gases through pipes, with one extra rule: a fluid route carries a single fluid type at a time. This prevents the silent mixing that a shared pipe network would otherwise allow.

Each endpoint holds 50 m³. The window shows what is buffered and which fluid the route has adopted.

To repurpose a fluid route you must empty it. The endpoint window has a flush action that discards **only the local buffer**. Your pipe network is never touched. Flushing asks for confirmation, and requires the endpoint to be disabled first so nothing refills it mid-operation.

Item endpoints have no flush. Instead you can **take the buffered items** straight into your own inventory, so nothing is ever destroyed.

<!-- IMG: WIKI_BUILDINGS_FLUID, Fluid (In) and Fluid (Out) on pipes, buffer visible -->

## Teleporter Hub

Optional. See [Hubs](Hubs). Draws 5 MW and stores no cargo. Cutting its power stops the management interface, not the logistics. Routes carry on moving cargo regardless.

## Personnel Teleporter

Tier 9, 50 MW at each end. See [Personnel Teleporters](Personnel-Teleporters).

## Appearance

All six support the Customizer, so paint and pattern apply as with any other building. Each carries directional indicators, an identification plate and a lit display glyph on both side panels.

Logistics endpoints and Personnel Teleporters appear in two separate map sections, so a large network stays readable.
