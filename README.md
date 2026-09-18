# Teleport Logistics

Teleport items, fluids and Pioneers between distant parts of your factory. Use named routes for logistics and a destination directory for personal travel.

Current candidate: **0.5.10**, built for Satisfactory **CL502094**, SML **3.12.0** and CSS Unreal **5.6.1**. Windows Steam/Epic client and Windows dedicated-server packages are the release targets. See the [release audit](reports/0.5.10-release-audit.md) for verified checks and remaining retail-game acceptance.

## Get started

1. Unlock **Teleport Logistics** at Tier 5 in the HUB.
2. Build an **Item (In)** and an **Item (Out)** from the **Teleporter** build category.
3. Press **E** at each building. Leave the channel as **Default**, enter the same route name, and apply.
4. Connect a supplying belt to the In and a receiving belt to the Out. In sends cargo into the network; Out delivers it back into your factory.
5. Use the fluid pair the same way with pipes. A fluid route accepts one fluid type at a time.

Default works without a hub. A powered **Teleporter Hub** adds a named channel, endpoint directory, route renaming, pause/resume and throughput information. Cutting its power disables management, not logistics transport.

At Tier 9, unlock **Teleport Personnel Transport**. Build and power two **Personnel Teleporters**, name them, then press E and click a ready destination. The directory shows coordinates in metres and supports native sign icons. Both buildings need 50 MW. The placement arrow marks the intended exit direction.

## Buildings and balance

| Building | Role | Power | Local buffer |
| --- | --- | --- | --- |
| Item (In) / Item (Out) | Belt cargo into/out of a route | No electrical connection | 64 individual items |
| Fluid (In) / Fluid (Out) | Pipe cargo into/out of a route | No electrical connection | 50 m³ |
| Teleporter Hub | Optional channel management | 5 MW | None |
| Personnel Teleporter | Pioneer travel | 50 MW at each end | None |

Logistics ceilings are **1,200 items/min** and **600 m³/min per endpoint**, limited by connected belts/pipes, supply and receiver capacity. Multiple inputs merge; multiple outputs share available supply and skip full receivers. Mixed items keep FIFO order and item state. There is no shared warehouse or item filter. The current route limit is 256.

| HUB milestone | Tier | Unlock cost |
| --- | --- | --- |
| Teleport Logistics | 5 | 100 Circuit Boards, 100 Reinforced Iron Plates, 200 Cable |
| Teleport Personnel Transport | 9 | 200 Time Crystals, 100 Supercomputers, 50 Turbo Motors |

Each logistics endpoint costs 4 Reinforced Iron Plates, 4 Circuit Boards and 10 Cable. The hub adds 5 Computers to that cost. A Personnel Teleporter costs 20 Time Crystals, 10 Supercomputers and 5 Turbo Motors. Personnel travel reserves both hubs for 30 seconds; a busy hub cannot be dismantled.

## Installation and compatibility

Use matching versions on host and clients. For direct testing, use `dist/TeleportLogistics-0.5.10-Windows.zip`; the Windows server archive is separate. `dist/TeleportLogistics-0.5.10.zip` combines both platforms for SMR. Source archives and the model viewer are not installable mods.

Keep a backup of any save you test with. Linux dedicated servers, controller navigation and large multiplayer factories are not certified by the current release checks.

[Player guide and troubleshooting](docs/player-guide.md) · [Acceptance checklist](docs/validation.md) · [Changelog](CHANGELOG.md)

## Development

```sh
bash scripts/test.sh
```

Runs the production scheduler with address/undefined-behaviour sanitizers, randomized conservation scenarios, source/content checks, connector/LOD validation and archive regression tests. Leak detection is off by default; run `ASAN_OPTIONS=detect_leaks=1 bash scripts/test.sh` to enable it. Native SDK tests and actual game tests are separate.

[Build instructions](docs/development.md) · [Architecture](docs/architecture.md) · [Publication kit](docs/publishing/README.md)

The offline [model viewer](tools/model-viewer/README.md) contains all six authored models. Its factory materials approximate the game shader; it is not evidence of in-game rendering.

## Credits

SFUIKIT artwork by Deantendo and Treelo, adapted under CC BY-SA 4.0. Standard connector references come from David “AngryBeaver” Gillen’s Satisfactory Modeling Tools. Coffee Stain owns Satisfactory's game assets and FICSIT branding. Open Sans and viewer dependencies retain their notices. See [third-party credits](THIRD-PARTY-NOTICES.md).
