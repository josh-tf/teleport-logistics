# Tabler Icons

Seven outline glyphs from [Tabler Icons](https://github.com/tabler/tabler-icons) by Paweł Kuna, under the
MIT License; see LICENSE. Retrieved from `main` at commit `55f87a73f45cf1d9eaf16d7da705065483a9e4f9`
(2026-09-03).

| File | Used for |
| --- | --- |
| `package-import.svg` | Item input display tile and map marker |
| `package-export.svg` | Item output display tile and map marker |
| `droplet-up.svg` | Fluid input display tile and map marker |
| `droplet-down.svg` | Fluid output display tile and map marker |
| `affiliate.svg` | Hub display tile and map marker, build category icon, Tier 5 milestone symbol |
| `bolt.svg` | Hub power display tile |
| `walk.svg` | Tier 9 Personnel milestone symbol |

The SVG sources are vendored unmodified. `scripts/svg_glyph.py` samples their path data and strokes it with
Pillow at 1.5 units on the authored 24 unit grid, which is lighter than Tabler's own 2.0, and the results are
baked into `assets/textures/T_TeleporterScreen_Grid.png` and the icon PNGs under `assets/icons/`. Those
rendered textures are derivative works of the glyph shapes and carry the same MIT terms; the mod's own MIT
licence does not extend to them, so the notice travels with the package in
`TeleportLogistics/Resources/ICONS-NOTICE.md`.

No Tabler artwork is used for the building meshes, the identification plates or the FICSIT mark.
