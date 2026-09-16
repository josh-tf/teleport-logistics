# Relay UI reference review — 14 September 2026

## Sources and conclusions

- [Machine Interaction GUI](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/SimpleMod/machines/SimpleInteraction.html): the tutorial demonstrates the native dark window and explicitly wiring its close event to interaction teardown. Relay retains the FG interaction lifecycle; its frame is Slate because reconstructed native Blueprint layouts previously introduced sizing and close problems. Reintroducing that wrapper is not a prerequisite for fixing the current layout.
- [Enhanced Input System](https://docs.ficsit.app/satisfactory-modding/latest/Development/Satisfactory/EnhancedInputSystem.html): native usable-widget contexts own input and must be cleaned up through the parent lifecycle. Existing close/lifecycle tests remain required alongside appearance work.
- [SFUIKIT](https://docs.ficsit.app/satisfactory-modding/latest/CommunityResources/SFUIKIT.html) and [its repository](https://github.com/deantendo/sfuikit): this is a graphical kit, not an automatic layout system. Continue using the imported panel/meter artwork and native composite font, while explicitly budgeting padding, text and list space.
- [Efficiency Checker](https://ficsit.app/mod/EfficiencyCheckerMod), [author description](https://github.com/MarcioHuser/EfficiencyCheckerMod-SML3/blob/master/EfficiencyCheckerMod%20-%20SMR%20description.html): the author describes distinct input, throughput and output figures and contextual colour states, including manual overrides. Design inference for Relay: a user's draft must not inherit misleading statistics from another route. Current throughput remains separate from buffer capacity and route state. No checker balance calculations or assets were copied.
- [Simple Fluid Teleporter](https://ficsit.app/mod/SimpleFluidTeleporter) links public source; [Simple item teleporter](https://ficsit.app/mod/teleportitem) does not provide a source link. Neither listing establishes an in-game UI layout to reproduce, so the user's native screenshots remain the primary visual reference.

## Refinement passes implemented

1. Layout: compact heading/identity, unified backing, taller content allocation, nonwrapping action labels, more metric inset and conditional management controls.
2. Information: explicit send/receive direction, route member counts, preview statistics resolved from the draft's channel/name/medium, unavailable directory filtering disabled, unnecessary pagination hidden.

## Remaining visual acceptance

Two actual Slate rendering attempts failed before capture: the VM software Vulkan device cannot satisfy the engine's device requirements, even with the supported profile-check bypass. No generated mockup is presented as an in-game screenshot. Native geometry tests cover three resolutions and three scale factors, powered and unpowered states. The shipped UI still needs in-game inspection of font appearance, panel contrast, keyboard focus, and large/localized route labels on Windows.
