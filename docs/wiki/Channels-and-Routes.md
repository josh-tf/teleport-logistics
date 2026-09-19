# Channels and routes

A **route** is a name shared by two or more endpoints. A **channel** groups routes.

Endpoints on the same channel and the same route are connected. Nothing else wires them together: no cables, no line of sight, no distance limit.

## The Default channel

Every endpoint starts on **Default**, which needs no hub and no power. If you never build a hub, you never need to think about channels at all: give a pair the same route name and they are connected.

<!-- IMG: WIKI_ROUTES_PICKER, channel and route picker with search and the create option -->

## Naming routes

Press **E** on an endpoint, choose or create a route, and apply. The picker searches as you type, which is what makes a large network usable.

Route names are scoped to their channel, so `overflow` on Default and `overflow` on a hub channel are different routes. Pick names that say what the cargo is or where it goes. `plates-to-sink` beats `route 4` when you come back in a month.

The network supports up to **256 routes**.

## Reassigning an endpoint

Changing an endpoint's route is a two-step job, deliberately:

1. **Disable** the endpoint. It stops accepting and sending.
2. **Empty the local buffer.** Item endpoints let you take the buffered items into your inventory. Fluid endpoints have a flush that discards the buffer, which asks for confirmation.
3. Pick the new channel and route, and re-enable.

The buffer step matters. Cargo sitting in a buffer belongs to the route it arrived on; moving an endpoint without draining it would deliver the old route's cargo to the new one.

<!-- IMG: WIKI_ROUTES_REASSIGN, endpoint disabled, buffer actions available -->

## Fluid routes and fluid type

A fluid route carries one fluid type at a time. The route adopts a type from the first fluid sent and keeps it until the route is emptied.

To change what a fluid route carries, every endpoint on it must be disabled and its buffer empty. Select the route in a powered Teleporter Hub and use **Reset empty fluid route**; the button stays greyed out until every endpoint qualifies. A hub can do this for its own channel and for Default routes alike.

Without a hub, assign the endpoints to a new route name instead. A route created fresh carries no fluid type.

A connected pipe still holding the old fluid re-pins the route the moment an endpoint is re-enabled, so drain or repurpose the pipes first.

Route names persist once created, including after the last endpoint on them moves away or is dismantled while the world is being torn down. Dismantling the last endpoint of a route reclaims it. The 256-route limit counts stored routes, not only those currently carrying cargo, and a powered hub can delete any route that has no endpoints attached.

## Renaming and pausing

With a [hub](Hubs) you can rename a route in place, so every endpoint on it follows, and pause a route to stop traffic without visiting each building. Without a hub, renaming means reassigning each endpoint individually.

## What a route does not do

- No filtering. An output receives whatever the inputs on its route send.
- No storage. Buffers smooth belt timing; they are not a warehouse.
- No priority. Multiple outputs share what is available rather than ordering themselves.
