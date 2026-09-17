# Playing with TeleportLogistics

## Routes and channels

Input means cargo enters the teleportation network; Output means cargo leaves it onto a belt or pipe. Give both ends the same route name and channel. Routes with the same name in different channels remain separate. Several inputs can feed one route, and several outputs can draw from it.

The Default channel needs no management hub. A Teleporter Hub creates its own named channel. Power it to rename routes, pause/resume distribution, inspect endpoints and delete unused routes. Its controls manage its own channel and Default; other channels are view-only. Hubs with assigned endpoints cannot be dismantled.

## Buffers and fluids

Items and fluid stay in local endpoint buffers. Drain a buffer before changing its route. A paused route stops network transfers; already buffered output cargo can still drain locally. Full receivers cause backpressure rather than discarding cargo.

A fluid route locks to one fluid type. Stop supply and drain it before resetting the empty route's fluid type. A disabled fluid endpoint can explicitly flush its local buffer after confirmation. This discards only that buffer, not the connected pipe network.

## Personal travel

Unlock Tier 9, place two Personnel Teleporters, and power each with 50 MW. Press E, name the source, optionally choose a sign icon, and select a ready destination. Coordinates use metres. Destination search is by name. The arrow in placement marks the front exit.

Unpowered, busy or dismantled destinations cannot accept a journey. Blocked landing areas are rejected. You cannot initiate travel while dead, driving a vehicle or already in a portal. Both hubs are reserved for 30 seconds. The native portal state machine handles the journey and destination streaming; on arrival the mod aligns your view with the exit.


## Emptying an endpoint

An endpoint holding cargo will not change route, so each one offers a way to empty its local buffer.

- **Item endpoints** show **Take buffered items**, which moves the buffer into your inventory. Anything that
  does not fit stays in the endpoint, so nothing is destroyed.
- **Fluid endpoints** show **Flush local buffer**, which discards the fluid. Disable the endpoint first. The
  attached pipes are untouched.

The row beside each button states why it is unavailable: nothing buffered, or the endpoint still enabled.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Nothing transfers | Matching channel and route, correct Input/Output, enabled endpoints, unpaused route, space at receiver, belt/pipe direction. |
| Dismantle refused | A transfer in flight blocks any endpoint; a fluid endpoint also needs an empty buffer and a drained attached pipe. |
| Fluid route rejects assignment | Existing fluid lock or local contents; drain before reassignment. |
| Hub controls disabled | Hub power and whether the selected channel is managed by this hub. |
| Personnel destination unavailable | Both hubs powered, cooldown elapsed and landing area clear. |
| Directory still connecting | Stay near the building and retry after replication; capture logs if it persists. |
| Milestone costs absent | Test in a normal progression save; Creative Mode can hide costs. |

For a bug report, include the mod version, game build, other mods, save/new-world reproduction, singleplayer/listen/dedicated mode, exact steps, and screenshots. For a crash, include `FactoryGame.log` and `CrashContext.runtime-xml`; include server and client logs for multiplayer issues. Support contact will be listed on the published mod page.

## Settings

Pause menu, Mods, Teleport Logistics. These are your own preferences and never change transport rates, power
draw or travel timings.

| Setting | Default | Effect |
| --- | --- | --- |
| Window refresh interval | 0.35 s | How often an open window refreshes. Raise it on a large network if the interface costs you frames. |
| Show route when looking at a building | On | Names the channel and route in the look-at panel. |
| Confirm before flushing fluid | On | Asks before discarding a fluid buffer. Taking items back is never destructive, so it is never confirmed. |
