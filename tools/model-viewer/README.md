# TeleportLogistics model workshop

`dist/TeleportLogistics-model-viewer.html` is a standalone, offline viewer containing all six
current OBJ meshes, the factory atlas, display texture and bundled Three.js.
Open it directly in a modern browser, or run `node tools/model-viewer/serve.mjs`
and visit http://127.0.0.1:8765. The server only exposes the viewer on loopback.

Controls: orbit, pan, zoom, eight camera presets, wireframe/clay/factory surfaces,
primary/secondary paint, light preview, native connection locations, 1 m grid,
and a lineup at the same scale. Keyboard arrows orbit; +/- zoom; R fits.

Rebuild after model changes:

```sh
npm ci --prefix tools/model-viewer
npm run --prefix tools/model-viewer build
```

The viewer ignores loose OBJ construction edges so OBJLoader preserves the actual
triangle meshes. It does not simplify or replace the source geometry. Material
appearance is a preview; the native factory shader, decal textures and gameplay
power state require checking in Satisfactory. Models use Blender Z-up centimetres
converted to metres. Runtime Unreal Y coordinates are mirrored for port markers.

`check.mjs` verifies all six models, surface modes, camera controls, offline
loading, keyboard operation, and small-screen layout using local Chromium.

Set `PLAYWRIGHT_CHROMIUM_EXECUTABLE` to use an existing Chromium installation.

Generate the listing banner and six transparent renders with `node tools/model-viewer/publication-media.mjs`. Output is `docs/publishing/media/`, which is kept out of the tracked tree; the banner is explicitly a model preview.
