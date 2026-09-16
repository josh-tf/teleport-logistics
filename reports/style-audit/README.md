# Relay documentation and style audit

**Implementation update:** see [0.4.7 fixes](../0.4.7-style-fixes.md). The table below records the pre-fix audit; live acceptance items remain open.

Audited source and generated meshes after 0.4.6. This is a gap assessment, not certification that the models now match stock machines. The user's close-up shows a real in-game surface mismatch. The previous explanation about the simplified browser shader does not explain that screenshot.

## Design target

The [style guide](https://docs.ficsit.app/satisfactory-modding/latest/Development/Modeling/style.html) describes maintained retro-industrial machinery: angular construction, restrained bevels, readable mechanisms and shared medium-poly parts. Our interpretation for Relay is a compact industrial transfer appliance with a protected circular transfer chamber, service covers and modest status lighting. Keep the circle the user likes. Avoid turning it into a glowing portal or adding heavy rust to compensate for flat shading.

Acceptance targets specific to Relay:

- Preserve the five recognizable functions and all connector transforms.
- Neutral metal on broad bodies, controlled primary/secondary paint accents.
- Subtle roughness and wear visible at the same scale as the adjoining stock connector.
- Small supported labels and believable fasteners; remove unnecessary tiny geometry.
- A clear mechanical path from the belt mouth into the circular chamber.
- Status illumination, not large decorative neon surfaces. Disconnected endpoints stay dark.
- Review front, back, both sides and elevated views beside a stock machine, using the same swatch and lighting.

## Findings and priorities

| Priority | Area | Observed Relay implementation | Assessment and action |
|---|---|---|---|
| P1 | Surface appearance | Native `MI_Factory_Base_01` is assigned, but the user screenshot shows broad clean panels beside a visibly mottled connector. | Open. Matching the material reference is insufficient. Compare UV density and paint finish under identical conditions before adding a new shader. |
| P1 | Generated UVs | `project_surface_uv` cube-projects at 68 units, then independently applies fractional wrapping to every UV vertex. | Confirmed risky operation: a face spanning a wrap boundary can shrink or reverse its UV span. Replace with continuous island mapping and measure density against native parts. It is a plausible contributor, not a proven sole cause of the screenshot. |
| P1 | Decal distance behavior | Import explicitly keeps one LOD. Native decal surfaces persist at every distance. | Missing distance treatment. The previous removal of automatic LODs addressed a symptom. Author and inspect distant LODs that remove surface detail without reopening the housing. |
| P2 | Item entrance | No `InputFog` component or material binding exists in Relay. | Available improvement. Place a separate fog plane inside the throat, ahead of the item disappearance point. Determine its depth from belt movement; fog does not fix intake timing or throughput. |
| P2 | Custom labels | FICSIT mark and Open Sans lettering are planar glyph meshes on plates. | They have no extrusion, but do not use the native colour-decal atlas. Move labels to a shared authored decal atlas with proper surface offset and distance removal. |
| P2 | Hazard strips and bolts | Custom bumper strips are individual boxes; many bolts and vents are modeled. | Convert flat cosmetic details to decals; retain geometry where it changes silhouette. |
| P2 | Visual complexity | Each model has roughly 24k–32k triangles, one LOD and 3–5 material slots. | No claim that a documented triangle cap is exceeded. Profile many placed relays; reduce detail that cannot be seen at normal factory distances. |
| P2 | Custom collision | Auto-generated collision, `BlockAll` on the main mesh. | Not verified against the hollow belt entrance or hub silhouette. Inspect collision and placement clearance before replacing it with authored simple hulls. |
| P2 | Save acceptance | Relay fields have `SaveGame`; subsystem explicitly opts into saving. Building inherits its save decision. | Audit the actual inherited behavior before adding an unconditional override. Verify a placed actor, nonempty item/fluid buffers and route identity across save/load. Do not infer data loss from an old documentation warning alone. |
| P2 | Multiplayer acceptance | Server-only network subsystem; player-owned RCO for changes and snapshots; replicated endpoint connection flag. | Architecture follows the documented pattern. Host/client and dedicated-server acceptance still required. |
| P3 | Clipboard | No machine-settings clipboard implementation. | Native UX opportunity, not a mandatory requirement. Copy channel/route settings through validated server configuration, with existing buffer reassignment rules. |

The [material guide](https://docs.ficsit.app/satisfactory-modding/latest/Development/Modeling/MainMaterials.html) recommends shared factory surfaces and decals, describes the fog plane, and warns about decals showing through surfaces at distance. It also requires linear sampling for normal and packed surface maps. Relay's current custom texture is a colour screen atlas; future roughness/normal maps must use the appropriate import settings.

## Which decal materials are actually used?

Counts below are exported triangle counts, not separate draw calls:

| Model | Total triangles | Colour decals | Normal decals |
|---|---:|---:|---:|
| Item input | 24,278 | 212 | 0 |
| Item output | 25,155 | 212 | 0 |
| Fluid input | 28,809 | 250 | 40 |
| Fluid output | 30,228 | 250 | 20 |
| Hub | 31,520 | 0 | 0 |

These decal faces come from the shared connector pieces. The hub and custom body details do not yet use the same approach. Raw counts: [model-inventory.json](model-inventory.json).

## SDK verification, rather than relying only on documentation paths

Read-only Unreal inspection confirmed:

- `MI_Factory_Base_01` inherits `MI_Factory2D_01`, which inherits `MM_Factory_Array` under `Buildable/-Shared/Material` in this project.
- `MI_Factory2D_01` explicitly has `Scale=20`; no overlay-density override was found in the two inspected instances. This is not a full enumeration of inherited master parameters.
- `InputFog` is a Material; `InputFogPlane` is a StaticMesh. Both load successfully.
- The standalone viewer uses the plain atlas, so it remains useful for shape and paint-region review, not native roughness acceptance.

Evidence: [native-materials.txt](native-materials.txt). Reproduce with `scripts/audit-native-materials.py` inside Unreal. Reconstructed SDK assets can still differ from the running game's assets.

## General implementation checks

- RCO registration is in the game-instance module. A replicated property exists; requests use validated server RPCs, and responses use a client RPC. There are no network multicast RPCs in the RCO. A similarly named local multicast delegate is not an RPC. This matches the [multiplayer guidance](https://docs.ficsit.app/satisfactory-modding/latest/Development/Satisfactory/Multiplayer.html).
- Mutations check world, authority, interaction range, dismantling state and rate limits. The network subsystem runs on the server.
- Material changes are guarded to the game thread and skip dedicated servers. Factory processing does not directly update rendering.
- Save flags cover IDs, labels, routes and cargo; serialization behavior still needs a real round trip. See [saving guidance](https://docs.ficsit.app/satisfactory-modding/latest/Development/Satisfactory/Savegame.html).
- Reusing `AFGFactoryHologram` is consistent with [hologram guidance](https://docs.ficsit.app/satisfactory-modding/latest/Development/Satisfactory/BuildableHolograms.html); a custom hologram is not inherently required.
- Five existing native tests and conservation checks are useful evidence, but do not replace the game's save/load, interaction and multiplayer tests. See [testing resources](https://docs.ficsit.app/satisfactory-modding/latest/Development/TestingResources.html).

## Order of the next production pass

1. Establish an in-game material comparison with identical swatches/finishes. Correct continuous UV mapping and check detail density before introducing any additional grunge.
2. Add the item fog plane with collision disabled and verify belt entry/exit in motion.
3. Convert suitable labels, fasteners and hazard markings to shared decals; author distant detail removal at the same time.
4. Refine lighting and functional industrial detail while preserving the circular chamber and all snap points.
5. Capture every side, inspect collision, test distant views, then run save/load and host/client acceptance.

No speculative material overrides or new fog placement were shipped by this audit. Version 0.4.6 remains the latest packaged build; the findings above remain open until implemented and checked.
