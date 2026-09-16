> Historical design record. For current behaviour see [architecture](architecture.md), [player guide](player-guide.md) and [release acceptance](validation.md).

> Implemented in TeleportLogistics 0.4.0 using the supplied layout from `design/reference-ui/`, SFUIKIT panels and native Satisfactory window/actions. This document preserves the design requirements; current behavior and validation limits are in `reports/0.4.0-ui-and-connectors.md`.

# TeleportLogistics native UI design handoff

Design an in-game interface that belongs beside Satisfactory's inventory, Industrial Fluid Buffer and Equipment Workshop screens. These three screenshots are the visual references. This is a redesign of TeleportLogistics's interaction windows, not a website or a change to transport mechanics.

## Deliverable format

A self-contained HTML/CSS prototype with a small amount of JavaScript for demonstrating interactions is ideal. Supply its source files and assets, not only screenshots. The runtime is Unreal Engine 5.6.1 Slate/UMG; implementation will translate the prototype into native widgets. HTML/CSS is a design specification, not an embedded browser UI. A ZIP containing index.html, styles.css, optional interaction JavaScript, assets/ and a short README is sufficient.

Include:

- Endpoint and hub layouts with example data, empty lists, long names and a long directory.
- Default, hover, pressed, focused, selected, disabled, loading, error and confirmation states. Show which changes require Apply and how success/rejection appears.
- Reference screenshots at 1920×1080, 2560×1440 and 3440×1440, with rules for panel bounds, minimum usable size, scrolling and UI scaling. Specify dimensions in logical design units; do not solve small viewports solely by shrinking all text.
- Font family/weights/sizes, line heights, spacing, colours, opacity, borders and corner radii. CSS variables or an optional tokens.json are useful.
- Separate transparent PNG exports for any custom artwork, with source SVGs where available. For resizable borders/panels, identify fixed corner sizes and stretchable regions for nine-slice rendering. Keep text, counters, selections and other dynamic content out of background images.
- A short interaction/keyboard specification. Native game controls and assets can be reused during implementation; mark such elements instead of recreating everything as custom artwork.

## Visual direction

Use the screenshots' shallow grey title bar with a small monochrome building icon, readable normal-case titles, an obvious X close control, translucent dark surfaces, grey list rows and orange selection/focus. Keep visual grouping and spacing consistent with stock interfaces. The implementation can reuse the base-game `Widget_Window_DarkMode` container described in the [SML interaction guide](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/SimpleMod/machines/SimpleInteraction.html). The fluid buffer/workshop references also establish an industrial panel treatment; use it where it improves hierarchy rather than covering every control with decorative machinery. Preserve contrast against bright sky, orange factory parts and dark interiors.

The five building identities are ITEM TELEPORTER (IN), ITEM TELEPORTER (OUT), FLUID TELEPORTER (IN), FLUID TELEPORTER (OUT), and TELEPORTER HUB. The IN/OUT distinction is the direction relative to the teleportation network. Direction and item/fluid medium belong to the placed building type; the settings window does not switch its type.

## Product rules

- A channel is an organizational group. `Default` works without a hub.
- Placing/naming a hub creates a named channel. A powered hub provides management controls; transport continues when its hub is unpowered.
- A route is a user-named connection within a channel, e.g. `small_copper_ingot`, `mega_copper_ingot`, `sink`, or `north_farm`. Separate routes can carry the same item. Do not replace route names with item selection or imply that an item determines the route.
- Multiple inputs merge onto a route; multiple outputs share its deliveries. Item routes can contain mixed items, including arbitrary sink traffic. Fluid routes maintain a compatible fluid type; items and fluids do not mix.
- An endpoint has its own editable label, selected channel, selected/existing-or-new route, enabled state and local buffer. Route reassignment can be rejected until its local contents drain.
- Mutations are validated by the server. Show pending/rejected changes clearly; never present an optimistic success before confirmation.

## Endpoint screen

Provide a clear building identity and IN/OUT direction, endpoint label, channel selector, searchable existing routes, and entry of a new route name. Selecting a route should make its channel and name unambiguous. Make Apply and Enable/Disable easy to distinguish.

Display route throughput in items/min or m³/min as appropriate, route pause state and connection/status messages. Include empty/no-search-match and connecting states. Fluid endpoints also have a local-buffer flush action, with a clear destruction confirmation and disabled/drain prerequisites. Do not imply that flushing affects the full attached pipe network.

Suggested example data: Default → sink; Small Copper → small_copper_ingot; North West Farm → north_farm. Show the same item on distinct named routes to make segregation understandable.

## Hub screen

Provide the hub/channel name, powered/unpowered state, route list and route details, plus a route-selected endpoint directory. Treat cross-page endpoint-name search as a proposed extension; the current API selects by route and page. The directory shows endpoint name, route path, IN/OUT, items/fluid, enabled state, buffered quantity and coordinates in metres. It is paginated; the backend currently returns up to 64 endpoints per page. Preserve a clear indication of current page and total endpoints.

Management actions include rename hub/channel, pause/resume route, rename route, delete unused route and reset an empty fluid route. Group destructive or restricted actions separately from routine navigation, with explicit reasons when unavailable. An unpowered hub should explain that management needs power while transport remains active.

## Interaction requirements

Escape and X must fully close the active TeleportLogistics window, including while a text field is focused. Closing returns control through the native game UI lifecycle and leaves no visible layer behind. Repeated open/close must be safe. Do not introduce multiple nested full-screen windows for ordinary endpoint setup. Supply a sensible tab/focus order, clear focus indicators and controller-friendly targets; controller behavior will need an in-game test.

Scrolling belongs inside long lists/directories. Retain the user's typed text and selection when periodic server snapshots refresh the screen. Do not jump the scroll position on every update. Long names should truncate/wrap deliberately, with a way to read the full value.

## Available data and implementation boundary

Current route snapshots contain channel/name, medium, paused state and aggregate throughput. Directory rows contain name, route path, position, direction, medium, enabled state and buffered quantity. There is also a status message and directory pagination/total. New graphs, per-item inventory grids, historical statistics, endpoint-specific throughput or map previews require additional backend work: mark them as proposals rather than assuming they already exist.

The native widgets pass literal strings to Slate, so the UI is not localized. Deliver the design without replacing production C++ files or generated Unreal assets.
