> Historical design record. For current behaviour see [architecture](architecture.md), [player guide](player-guide.md) and [release acceptance](validation.md).

# Personnel Teleporter: implementation plan

Written against the 0.4.10 development build, before the Tier 9 work landed.

## Scope and acceptance

A separate late-game Personnel Teleporter building provides a searchable directory of other Personnel Teleporters. The player approaches either side, presses Use (E by default), clicks a ready named destination with XYZ coordinates to travel. Existing item/fluid routing and logistics hubs remain separate.

1. Inspect the native portal player API, streaming lifecycle, power and interaction interfaces.
2. Implement a server-authoritative directory, persistent hub identity/name, rename and travel requests. Reject remote use, dead players, vehicles, unavailable hubs and blocked landing pads. Bound list size and request frequency.
3. Route travel through the native player portal state machine, including its streaming acknowledgement and player presentation. Supply valid portal transforms and travel-time curve. Do not reproduce native paired-portal fuel or cross-grid power behavior.
4. Add a powered custom ring, terminal and mast with three LODs, authored collision and existing shader-safe materials. Add model-capture icons and viewer inspection from both sides.
5. Add Tier 9 milestone, resource costs and build recipe. Preserve the five Tier 5 logistics unlocks.
6. Run editor compilation, content inspection, native automation, mesh/viewer checks and Windows client/server packaging. Record evidence and distinguish runtime acceptance from SDK checks.

## Balance

| Setting | Value |
|---|---|
| Milestone | Teleport Personnel Transport, Tier 9 |
| HUB unlock cost | 200 Time Crystals; 100 Supercomputers; 50 Turbo Motors |
| Milestone dispatch time | 300 seconds |
| Build cost per hub | 20 Time Crystals; 10 Supercomputers; 5 Turbo Motors |
| Electricity | 50 MW at each end |
| Transit curve | 1.5 seconds at 0 km, 3.5 seconds at 10 km; native streaming may extend travel |
| Native maximum travel time | 25 seconds |
| Reservation | Both hubs reserved for 30 seconds after departure |
| Directory | 32 destinations per page, one-second refresh, case-insensitive search |

## Animation/transition choices

**Implemented integration:** `AFGCharacterPlayer::StartPortal`, backed by `AFGBuildablePortalBase` transforms and a travel-time curve. The SDK exposes portal player state, exit transform/velocity, multicast state and client streaming completion. This is the appropriate integration point for stock player presentation and destination loading. Exact game Blueprint effects and sounds require a live-game test.

**Considered, deferred:** bespoke fade-out/fade-in, bespoke Niagara ring activation, moving iris panels and arrival audio. Adding them before confirming the stock presentation could double-play effects or conflict with streaming timing. The model currently has static rings and powered indicator materials; it has no new skeletal animation or simulated portal surface.

**Rejected for long-distance travel:** a raw `TeleportTo(destination)` alone. Epic describes preloading destination cells with a streaming source before teleporting. A short local staging move is used only to normalize the native portal entry offset, with a return to the prior transform if native travel does not begin.

References:
- [World Partition and streaming sources](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)
- [Satisfactory industrial modeling style](https://docs.ficsit.app/satisfactory-modding/latest/Development/Modeling/style.html)
- Local SDK: `FGCharacterPlayer.h`, `Buildables/FGBuildablePortalBase.h`, `UI/FGInteractWidget.h`, `FGRemoteCallObject.h` under `.toolchains/sml-project/Source/FactoryGame/Public`.
