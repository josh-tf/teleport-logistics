# Development and release builds

The internal module is `TeleportLogistics`. Native types and content packages were migrated in 0.5.10. The tested SDK is pinned in `sdk-lock.json`.

## Fast checks

```sh
bash scripts/test.sh
npm ci --prefix tools/model-viewer
npm run --prefix tools/model-viewer build
node tools/model-viewer/check.mjs
```

Use a configured local Chromium via `PLAYWRIGHT_CHROMIUM_EXECUTABLE` when needed. Viewer checks write to a versioned report directory and must not overwrite historical screenshots.

## Unreal

Follow the [official SDK setup](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/project_setup.html). Build using the pinned CSS engine, SML project and Audiokinetic integration. Never commit SDK/toolchain binaries or credentials.

```sh
TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS=2 UE_WINE_MSVC=/path/to/msvc \
  bash scripts/build-linux.sh /path/to/SatisfactoryModLoader /path/to/CSSUnreal WindowsAll
```

This full pipeline clears the generated staging plugin, compiles the editor, regenerates/imports content, inspects it, then packages. It is expensive. For source-only work, copy the changed Source tree and manifest into the existing staging plugin and build there, preserving already validated content. Disable core dumps (`ulimit -c 0`) before editor/Wine commands. Limit parallel actions on memory-limited hosts. Do not add access transformers without understanding their SDK-wide rebuild cost.

Windows alternative:

```powershell
.\scripts\build.ps1 -ProjectRoot D:\SatisfactoryModLoader -EngineRoot 'C:\Program Files\Unreal Engine - CSS' -Target WindowsAll -MaxParallelActions 2
```

The PowerShell workflow is supplied but has not been exercised on this Linux host. LinuxServer/All require the matching server toolchain and separate testing; current packaged artifacts cover Windows only.

Run `scripts/inspect-content.py` in an Unreal Python commandlet after editor compilation. Run native `TeleportLogistics.` automation separately with `Automation RunTests TeleportLogistics.;Quit` and export its report. Native tests exercise SDK code and reconstructed game APIs; a passing report does not establish retail-game acceptance.

## Assets

The model generator and texture import scripts retain six meshes with three LODs and native material bindings. `generate-ui-symbols.py` touches only the two milestone symbol pairs. Individual building rewards must keep 3D captures. `import-ui-symbols.py` performs a texture-only refresh when no model/material change is needed.

Regenerate the models, the screen atlas and the descriptor icon family with:

```sh
python3 scripts/generate-assets.py
```

That script shells out to Blender twice, for `generate-models-blender.py` and `render-icons-blender.py`. Its environment knobs:

| Variable | Effect |
|---|---|
| `BLENDER` | Blender executable to use. Defaults to the first `blender` on `PATH`. |
| `TELEPORTLOGISTICS_REUSE_CAPTURES` | Skip both Blender passes. The committed model sources and 512 px captures must already be present. |
| `TELEPORTLOGISTICS_BLENDER_THREADS` | Blender `--threads` value. Default 1. |
| `TELEPORTLOGISTICS_MODEL_ONLY` | Build one mesh by full name, for example `SM_TeleporterHub`, instead of all six. |
| `TELEPORTLOGISTICS_ICON_ONLY` | Render one descriptor capture by short name, for example `TravelHub`. |
| `TELEPORTLOGISTICS_RENDER_CLAY`, `_RENDER_TARGET`, `_RENDER_SCALE`, `_RENDER_SIZE` | Review renders. Any of them redirects output to `reports/icon-debug/`, so a clay or zoomed frame cannot overwrite a shipped icon. |
| `TELEPORTLOGISTICS_RENDER_SAMPLES` | Cycles sample count. Default 128. |
| `TELEPORTLOGISTICS_RENDER_DIRECTION` | Camera direction vector, `x,y,z`. Default `1.55,-1.75,1.22`. |
| `TELEPORTLOGISTICS_RENDER_OUTPUT` | Output directory override. |
| `TELEPORTLOGISTICS_ICON_CPU` | Force CPU rendering when no GPU device is usable. |
| `TELEPORTLOGISTICS_SECONDARY_PAINT` | Secondary paint colour, `r,g,b`. Defaults to the graphite baked into every shipped icon. |

`TeleportLogisticsSettings.h` declares the SML mod configuration as a C++ `UModConfiguration` subclass, so it
needs no editor-authored asset; `UTeleportLogisticsGameInstanceModule` registers it through
`ModConfigurations` and values are read back through `UConfigManager::GetConfigurationRootSection`. Only
client-local preferences belong there. SML writes configuration per client, so a gameplay number exposed this
way would let a client disagree with its host, and map-marker visibility cannot work either because
representations are created under `HasAuthority`.

`scripts/svg_glyph.py` renders the vendored Tabler glyphs with Pillow: it samples SVG path data into
polylines and strokes them, because no SVG rasteriser is installed and adding one would put a native
dependency in the asset pipeline. Curves sample at twelve steps per segment, since Pillow stamps a
pixel-rounded circle at every vertex of a joined polyline and a denser curve beads visibly. Callers draw at a
multiple of the target size and downsample. `generate-screen-atlas.py`, `generate-ui-symbols.py` and the map
and category icons in `generate-assets.py` all go through it, so the glyph language stays consistent from the
build menu to the map to the screens on the models.

Descriptor captures come from `render-icons-blender.py`, which follows Coffee Stain's documented icon setup:
three-point lighting and a perspective camera standing in for their CineCameraActor, at 85 mm on a 36 mm
sensor. The distance is solved by projecting the mesh's eight bounding-box corners and bisecting until they
just fill the frame; a width-only solve clips the corners and a bounding-sphere solve wastes the frame. Depth
of field is deliberately omitted, since at building scale the whole mesh sits inside the focal depth.

`scripts/generate-detail-atlas.py` letters the six in-world identification plates. It is run by hand rather than
from `generate-assets.py`, because relettering changes the plates on every placed building. It picks the largest
type size whose widest label clears a 20 px margin inside each 512 px tile, so a longer label shrinks the
lettering instead of bleeding into the neighbouring plate's UV rectangle.

Preserve the opaque `M_TeleporterDetails` crash workaround and the explicit GC roots for research reward presentation. Runtime rendering belongs on the game thread. Keep custom destination icons strongly referenced while Slate uses their brushes.

## Packaging

```sh
python3 scripts/package-source.py
python3 scripts/package-release.py
```

The second command verifies manifests, expected binaries/cooked containers, unsafe/duplicate paths and CRCs, then writes combined Windows/client-server archives and SHA256SUMS. `SHA256SUMS` lists only the archives produced by that run, so run `package-source.py` first for the source zip to appear in it, and move superseded archives out of `dist/` before a release build. The source handoff needs a `LICENSE` at the repo root and excludes toolchains, binaries, intermediate files, node_modules and `reports/model-renders`. The combined archive is the SMR upload artifact, not the source archive.

Read the publication kit, which ships in the kit archive rather than in this tree, and [release validation](validation.md) before upload.
