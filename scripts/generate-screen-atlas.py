"""Generate the model display atlas; UV tiles are four columns by two rows.

Top row: item input/output, fluid input/output. Bottom row: hub, power. Tiles 6 and
7 are unreferenced by every model's UVs and are left empty.
Keep the existing texture asset name to preserve saved/material references.
"""
from pathlib import Path
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import svg_glyph

ROOT = Path(__file__).resolve().parents[1]
TILE = 512
SUPERSAMPLE = 4
BG = (3, 8, 11)
FRAME = (21, 68, 80)
IN = (65, 206, 229)
OUT = (252, 165, 52)

# One tile per UV cell. None leaves the cell empty.
TILES = [("package-import", IN), ("package-export", OUT),
         ("droplet-up", IN), ("droplet-down", OUT),
         ("affiliate", IN), ("bolt", OUT), None, None]


def cell(name, accent):
    size = TILE * SUPERSAMPLE
    image = Image.new("RGB", (size, size), BG)
    d = ImageDraw.Draw(image)
    unit = SUPERSAMPLE
    d.rounded_rectangle((22 * unit, 22 * unit, size - 22 * unit, size - 22 * unit),
                        radius=12 * unit, outline=FRAME, width=3 * unit)
    for x, y, sx, sy in ((32, 32, 1, 1), (480, 32, -1, 1), (32, 480, 1, -1), (480, 480, -1, -1)):
        d.line([((x + sx * 36) * unit, y * unit), (x * unit, y * unit),
                (x * unit, (y + sy * 36) * unit)], fill=accent, width=3 * unit)
    span = size * 0.70
    svg_glyph.draw(image, name, accent, ((size - span) / 2, (size - span) / 2, span))
    return image.resize((TILE, TILE), Image.Resampling.LANCZOS)


def generate():
    atlas = Image.new("RGB", (TILE * 4, TILE * 2), BG)
    for index, entry in enumerate(TILES):
        if entry is None:
            continue
        name, accent = entry
        atlas.paste(cell(name, accent), ((index % 4) * TILE, (index // 4) * TILE))
    path = ROOT / "assets/textures/T_TeleporterScreen_Grid.png"
    path.parent.mkdir(exist_ok=True, parents=True)
    atlas.save(path, optimize=True)
    print(path)


if __name__ == "__main__":
    generate()
