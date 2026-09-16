# Teleport Logistics: native UI design handoff

Design specification for the Teleport Logistics interaction windows (endpoint + hub), authored to sit beside
Satisfactory's stock Inventory, Industrial Fluid Buffer and Equipment Workshop screens.

**This is a specification, not a shippable UI.** The runtime is UE 5.6.1 Slate/UMG. The prototype
exists so the implementation can read exact metrics, states and behaviour; do not embed it as a
browser layer in game.

## Files

| File | What it is |
| --- | --- |
| `Teleport Logistics Native UI.dc.html` | The prototype. Opens directly in a browser, no build step. |
| `support.js` | The prototype's runtime, loaded by the `.dc.html`. Generated output; its `dc-runtime` source is not in this repo, so edit the `.dc.html` and not this file. |
| `assets/tokens.json` | Colour, type, spacing, size, nine-slice and scaling tokens in logical design units. |
| `assets/teleporter_*.svg` | The five building icons, the only new artwork besides the panel slices. Monochrome `#E9E9E9`, tinted in engine. Export transparent PNGs at 32 and 64 px. |

## Screens in the prototype

Use the left rail (harness only, not part of the game UI):

- **Endpoint · Item IN**: default populated state: *Small Copper → Copper Loop / small_copper_ingot*.
- **Endpoint · Fluid OUT**: *North West Farm → North Grid / north_farm*, with local-buffer flush.
- **Endpoint · New & empty**: unnamed, unassigned, disabled, empty route list, connecting status.
- **Endpoint · No search match**: empty search result with the create-a-route escape hatch.
- **Hub · Powered**: routes, route detail, management actions, paginated directory (147 endpoints).
- **Hub · Unpowered**: management disabled with reasons; banner states transport continues.
- **Hub · Empty route**: long route name, zero endpoints, delete/reset preconditions met.
- **State matrix / Tokens & metrics / Interaction spec**: the reference pages.

The rail's *Server response for next Apply* switch drives the accept vs. reject path.

## What is interactive

- Editing the label, channel or route stages a change: a "Needs apply" dot appears on the field and a
  pending banner appears next to Apply. Nothing in the identity strip changes yet.
- **Apply** goes `pending → validating (1.4 s) → confirmed | refused`. Success is never shown before
  the server answers; a refusal reverts the draft to server truth and explains the buffer precondition.
- **Enable/Disable** is a live switch, deliberately styled as a lever and placed apart from Apply.
- **Flush** (fluid only) requires the endpoint disabled, then confirms destruction in an in-window
  overlay that states the pipe network is untouched.
- **Esc** and **X** close the window from any focus state, including a focused text field.
- Route search, directory page filter, route selection and pagination all work.

## Reading the spec

- **Tokens & metrics**: every colour, type step, metric, resolution rule and the nine-slice map.
- **State matrix**: default / hover / pressed / focused / selected / disabled / loading / error /
  confirmation for each interactive class, with the rule next to each swatch.
- **Interaction spec**: window lifecycle, tab and focus order, snapshot-refresh rules, the product
  rules the UI encodes, and the backend boundary.

## Stock reuse vs. new artwork

Reuse from the base game: `Widget_Window_DarkMode` container, title bar, close glyph, scrollbar,
text field, item and fluid icons, button base. New: the five building icons and the industrial plate /
well nine-slices. No dynamic content (text, counters, selection, throughput) is baked into any background.

## Marked as proposals, not existing capability

Cross-page endpoint-name search, per-endpoint throughput, graphs, historical statistics, per-item
inventory grids and map previews all need new backend work. They are labelled as proposals in the UI
and in the spec table.
