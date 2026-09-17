"""Generate TeleportLogistics's models, screen texture, and native-style icon family."""

from pathlib import Path
import sys
import math
import os
import shutil
import subprocess

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import svg_glyph


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
MODELS = ASSETS / "models"
ICONS = ASSETS / "icons"
TEXTURES = ASSETS / "textures"
RESOURCES = ROOT / "TeleportLogistics" / "Resources"
MODEL_NAMES = ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub", "TravelHub")
for directory in (MODELS, ICONS, TEXTURES, RESOURCES):
    directory.mkdir(parents=True, exist_ok=True)


def screen_texture():
    # generate-detail-atlas.py letters the identification plates and is run by hand,
    # because relettering changes every plate on every placed building.
    import runpy
    runpy.run_path(str(ROOT / "scripts/generate-screen-atlas.py"))["generate"]()


def map_icon(name, fluid=False, output=False, hub=False):
    """High-resolution map marker: white ring around a coloured direction glyph."""
    scale = 4
    size = 128
    canvas = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    white = (238, 243, 245, 255)
    accent = (255, 142, 35, 255) if output else (28, 221, 177, 255)
    draw.ellipse((9 * scale, 9 * scale, 119 * scale, 119 * scale), outline=white, width=4 * scale)
    if hub:
        glyph = "affiliate"
    elif fluid:
        glyph = "droplet-down" if output else "droplet-up"
    else:
        glyph = "package-export" if output else "package-import"
    span = 74 * scale
    svg_glyph.draw(canvas, glyph, accent if not hub else white,
                   ((size * scale - span) / 2, (size * scale - span) / 2, span), stroke=1.7)
    canvas.resize((size, size), Image.Resampling.LANCZOS).save(ICONS / f"M_Teleporter{name}.png")


def category_icon():
    """Transparent white network mark, matching the stock category art weight."""
    scale = 4
    size = 128
    canvas = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    span = 96 * scale
    svg_glyph.draw(canvas, "affiliate", (245, 247, 248, 255),
                   ((size * scale - span) / 2, (size * scale - span) / 2, span), stroke=1.7)
    result = canvas.resize((size, size), Image.Resampling.LANCZOS)
    result.save(ICONS / "T_TeleporterCategory_128.png")
    result.save(RESOURCES / "Icon128.png")


def require_models():
    missing = [str(MODELS / f"SM_Teleporter{name}{suffix}") for name in MODEL_NAMES
               for suffix in (".obj", ".mtl")
               if not (MODELS / f"SM_Teleporter{name}{suffix}").is_file()]
    if missing:
        raise RuntimeError("Missing generated model sources: " + ", ".join(missing))


def require_captures():
    missing = [str(ICONS / f"T_Teleporter{name}_512.png") for name in MODEL_NAMES
               if not (ICONS / f"T_Teleporter{name}_512.png").is_file()]
    if missing:
        raise RuntimeError("Missing descriptor model captures: " + ", ".join(missing))


# Remove old flat descriptor icons and old custom surface textures so they can
# never leak into a staged build or source package.
for path in ICONS.glob("*.png"):
    # Retain source captures until a replacement has rendered successfully.
    if path.name not in {f"T_Teleporter{name}_512.png" for name in MODEL_NAMES}:
        path.unlink()
for name in ("T_TeleporterSurface_Detail.png", "T_TeleporterSurface_Normal.png", "T_TeleporterSurface_ORM.png"):
    path = TEXTURES / name
    if path.exists():
        path.unlink()

screen_texture()
for fluid in (False, True):
    for output in (False, True):
        name = ("Fluid" if fluid else "Item") + ("Output" if output else "Input")
        map_icon(name, fluid=fluid, output=output)
map_icon("Hub", hub=True)
category_icon()

blender = os.environ.get("BLENDER") or shutil.which("blender")
if blender and not os.environ.get("TELEPORTLOGISTICS_REUSE_CAPTURES"):
    threads = os.environ.get("TELEPORTLOGISTICS_BLENDER_THREADS", "1")
    subprocess.run([blender, "--background", "--threads", threads, "--factory-startup", "--python-exit-code", "1", "--python",
                    str(ROOT / "scripts" / "generate-models-blender.py")], check=True)
    subprocess.run([blender, "--background", "--threads", threads, "--factory-startup", "--python-exit-code", "1", "--python",
                    str(ROOT / "scripts" / "render-icons-blender.py")], check=True)

require_models()
require_captures()
for name in MODEL_NAMES:
    source = Image.open(ICONS / f"T_Teleporter{name}_512.png").convert("RGBA")
    source.resize((256, 256), Image.Resampling.LANCZOS).save(
        ICONS / f"T_Teleporter{name}_256.png", optimize=True)

# Only milestone tiles use flat symbols; individual rewards retain model captures.
import runpy
runpy.run_path(str(ROOT / "scripts/generate-ui-symbols.py"))

expected = ({f"M_Teleporter{name}.png" for name in MODEL_NAMES if name != "TravelHub"} |
            {f"T_Teleporter{name}_{size}.png" for name in MODEL_NAMES for size in (256, 512)} |
            {"T_TeleporterCategory_128.png", "T_TeleporterMilestone_256.png", "T_TeleporterMilestone_512.png", "T_TeleporterPersonnelMilestone_256.png", "T_TeleporterPersonnelMilestone_512.png"})
actual = {path.name for path in ICONS.glob("*.png")}
if actual != expected:
    raise RuntimeError(f"Icon family mismatch: extra={sorted(actual - expected)}, "
                       f"missing={sorted(expected - actual)}")
print("Generated six stock-scale models, six model-capture icon pairs, five map markers, "
      "a native-style category glyph, a milestone symbol, and one screen texture.")
