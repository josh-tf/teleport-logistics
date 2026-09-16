# Relay 0.3.1 build validation

Historical build evidence. In-game testing subsequently found a hub material update on a factory worker thread in 0.3.1. Use the [0.3.2 crash-fix report](0.3.2-power-thread-fix.md) and replacement packages for testing; the build checks below did not catch that runtime defect.

Validated on 2026-09-13 (Australia/Melbourne) in the Linux development VM.

## Toolchain

- CSS Unreal Engine `5.6.1-83+++5.6.1-CSS`
- Satisfactory Mod Loader `3.12.0+d2162c99`, commit `d2162c999bbaa9650594ee9ad6e6e06190306009`
- FactoryGame headers/build `CL502094`
- MSVC `19.38.33145` and Windows SDK `10.0.22621` through MSVC-Wine
- Wwise SDK `2023.1.14.8770`, Unreal integration `2023.1.14.3555`

## Results

- `FactoryEditor Linux Development` compiled and linked `libUnrealEditor-Relay.so`.
- UnrealHeaderTool accepted Relay's replicated fields, six validated RPCs, context-tagged snapshots, root game-instance module, server subsystem and power-state delegate.
- UnrealBuildTool linked `FactoryGameSteam-Relay-Win64-Shipping.dll`, `FactoryGameEGS-Relay-Win64-Shipping.dll`, and `FactoryServer-Relay-Win64-Shipping.dll`.
- The Unreal commandlet imported five UV-mapped single-LOD meshes, eighteen UI/map textures and one screen texture, then created the power-aware screen material. Exactly 25 runtime packages were retained. Content inspection reported zero Relay errors and validated five recipe rewards, three milestone costs, category/subcategory assignment, milestone/model icons, each building CDO's map texture, every material slot and binding, and the native no-power material.
- Both Windows client and WindowsServer cooks retained all 25 packages. The client IoStore contains 40 chunks and compresses 16.79 MiB of raw data to 5.31 MiB. The headless server IoStore contains 26 chunks and compresses 0.09 MiB to 0.04 MiB. Alpakit produced separate platform archives successfully.
- The five custom model sources contain 127,095 valid triangles in total, exported normals and UV0, and four or five consolidated material sections. Each imports with one stable LOD to remove the former visible close-range LOD/material transition. Structural surfaces bind to `MI_Factory_Base_01` for native Customizer behaviour; the hub's signal slot can swap to `MI_FactoryLod_01_NoPowerEmissive`, and its screen exposes `RelayPower`.
- The final model contains the attributed standard connector geometry at greater than 92% exact vertex retention. Component and mesh origins match at belt `(160, 0, 100)`, pipe `(160, 0, 175)`, and compact hub power `(-155, 88, 285)` with the correct outward/rear rotations.
- `scripts/test.sh` passes seven scheduler scenario groups and 60,000 randomized conservation steps under AddressSanitizer and UndefinedBehaviorSanitizer, plus source, manifest, model, material, content, RPC, dedicated-server, map, interaction, power-state and alignment assertions.
- The release assembler checks plugin versions, required binaries, IoStore files, safe paths, archive CRCs and the combined Windows/WindowsServer SMR layout, then emits SHA-256 checksums.

SML emits upstream deprecation and Python-wrapper warnings during compilation and commandlets. The base project also reports normal notices for disabled commandlet audio, ExampleMod metadata, editor-domain metadata and optional online plugins. None stopped compilation, import, inspection, cooking or packaging.

Earlier in-game smoke testing confirmed module loading, milestone discovery, searchable recipes, placement and core transport. Relay 0.3.1's revised models, Customizer response, icons, research rewards, use prompt, map markers, power visuals, save/load and two-player behaviour still require hands-on game acceptance. Build success establishes target compatibility and packaged code paths; it does not establish visual fit, network latency or runtime replication under a live session.
