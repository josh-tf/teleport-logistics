# Your first route

You need Tier 5 and about five minutes.

## 1. Unlock

Unlock **Teleport Logistics** at Tier 5 in the HUB terminal.

100 Circuit Boards · 100 Reinforced Iron Plates · 200 Cable

That unlocks the four item and fluid endpoints and the management hub, all in the **Teleporter**
build category under the subheading **Teleporter Network**.

<!-- IMG: WIKI_FIRST_UNLOCK — Tier 5 milestone tile in the HUB terminal, 3D rewards visible -->

## 2. Build the pair

Place an **Item (In)** where your cargo comes from and an **Item (Out)** where you want it. They can
be any distance apart, on opposite sides of the map if you like. Neither needs power.

The build-menu tiles read `Item (In)` and `Item (Out)`; the same short names appear when you look at
a placed building. Each one carries a glyph on both side panels so you can tell input from output
without opening anything.

<!-- IMG: WIKI_FIRST_PLACED — an Item (In) and an Item (Out) placed, unconnected -->

## 3. Name the route

Press **E** on the input. Leave the channel as **Default**, type a route name, and apply. Then press
**E** on the output and give it the **same** route name.

The name is the whole mechanism. Two endpoints on the same channel and route are connected; that is
all there is to it. `iron-plates` or `sink-feed` works well. Route names are per channel, so you can
reuse a name on a different channel later.

<!-- IMG: WIKI_FIRST_ROUTE_ENTRY — endpoint window with the route name being entered -->

## 4. Connect belts

Feed the input with a belt and run a belt from the output into your factory. Cargo starts moving as
soon as both ends share a route and the input has something to take.

<!-- IMG: WIKI_FIRST_WORKING — belts running into the input and out of the output, cargo flowing -->

## 5. Watch it work

Both windows show a local buffer. The input fills as it accepts cargo and drains as it sends; the
output does the reverse. A buffer sitting full at the output means your receiving belt cannot keep
up, not that the route is broken.

## Fluids

The fluid pair works the same way with pipes, with one rule: a fluid route carries one fluid type at
a time. Send only Water on a route, or only Fuel. To change what a route carries, empty it first —
see [Channels and routes](Channels-and-Routes).

## What next

- More than a handful of routes? A [hub](Hubs) gives you named channels and one window for the lot.
- Wondering what it can actually push? [Throughput and power](Throughput-and-Power).
- Something not working? [FAQ](FAQ).
