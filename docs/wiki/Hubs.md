# Hubs

A **Teleporter Hub** is optional. Logistics work perfectly well on the Default channel with no hub anywhere on the map. A hub earns its 5 MW once your network outgrows what you can remember.

<!-- IMG: WIKI_HUB_WINDOW, hub window: channel list, route detail, endpoint directory -->

## What it adds

- **A named channel.** Routes grouped under a name of your choosing, separate from Default.
- **An endpoint directory.** Pick a route and the directory lists that route's endpoints with coordinates in metres, 64 per page, so you can find the building you mean. The box above it filters the page you are on.
- **Route detail.** Which endpoints are attached, what is buffered, and current throughput.
- **Rename in place.** Rename a route and every endpoint on it follows.
- **Pause and resume.** Stop traffic on a route without visiting each building.
- **Delete unused routes.** Remove a route once no endpoint is attached to it. A hub is the only building that can do this, and it works on Default routes as well as its own channel.
- **Reset an empty fluid route.** Clear the fluid type a route has adopted, once every endpoint on it is disabled and empty.

## Power

The hub draws **5 MW** and stores no cargo.

Cutting its power disables **management, not logistics**. Routes keep moving cargo while the hub is dark; you simply lose the window until power returns. This is deliberate: a brownout should not strand your factory's supply lines.

<!-- IMG: WIKI_HUB_UNPOWERED, hub with no power, logistics still running -->

## Channels

Each hub provides one named channel. Endpoints choose a channel when you set their route, so an endpoint belongs to Default or to a hub's channel, not both. A powered hub manages Default routes too, which is why it is the recovery path for a network that never built one.

Use channels to separate concerns rather than to subdivide arbitrarily: a channel per factory complex, or one for bulk feedstock and one for finished parts, keeps the pickers short.

## When you do not need one

If you have a handful of routes with obvious names, Default is simpler and free. Build a hub when searching a list would be quicker than walking to the building.
