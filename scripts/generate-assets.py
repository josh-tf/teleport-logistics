"""Generate TeleportLogistics's models, screen texture, and native-style icon family."""

from pathlib import Path
import math
import os
import shutil
import subprocess

from PIL import Image, ImageDraw


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
    """High-resolution grayscale glyph with transparent edges for the map."""
    scale = 4
    size = 128
    canvas = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    white = (238, 243, 245, 255)
    accent = (255, 142, 35, 255) if output else (28, 221, 177, 255)
    line = 5 * scale
    draw.ellipse((9 * scale, 9 * scale, 119 * scale, 119 * scale),
                 outline=white, width=4 * scale)
    if hub:
        draw.rounded_rectangle((31 * scale, 34 * scale, 97 * scale, 80 * scale),
                               radius=5 * scale, outline=white, width=line)
        draw.line((64 * scale, 80 * scale, 64 * scale, 96 * scale), fill=white, width=line)
        draw.line((45 * scale, 97 * scale, 83 * scale, 97 * scale), fill=white, width=line)
        for x in (43, 64, 85):
            draw.ellipse(((x - 5) * scale, 49 * scale, (x + 5) * scale, 59 * scale), fill=accent)
    elif fluid:
        points = [(64 * scale, 29 * scale), (39 * scale, 66 * scale),
                  (42 * scale, 85 * scale), (54 * scale, 96 * scale),
                  (74 * scale, 96 * scale), (86 * scale, 85 * scale),
                  (89 * scale, 66 * scale)]
        draw.polygon(points, fill=white)
    else:
        draw.rounded_rectangle((34 * scale, 40 * scale, 94 * scale, 88 * scale),
                               radius=6 * scale, outline=white, width=line)
    if not hub:
        # Match the physical displays: up into the symbol, down out of it.
        tail, tip = (66, 106) if output else (106, 66)
        neck = tip - (14 if output else -14)
        points = [(61.5, tail), (61.5, neck), (55, neck), (64, tip),
                  (73, neck), (66.5, neck), (66.5, tail)]
        draw.polygon([(x * scale, y * scale) for x, y in points], fill=accent)
    canvas.resize((size, size), Image.Resampling.LANCZOS).save(ICONS / f"M_Teleporter{name}.png")


def category_icon():
    """Transparent white logistics-network mark matching stock category art."""
    scale = 4
    size = 128
    canvas = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    white = (245, 247, 248, 255)
    line = 5 * scale
    draw.rounded_rectangle((42 * scale, 25 * scale, 86 * scale, 57 * scale),
                           radius=4 * scale, outline=white, width=line)
    draw.line((64 * scale, 57 * scale, 64 * scale, 72 * scale), fill=white, width=line)
    draw.line((27 * scale, 72 * scale, 101 * scale, 72 * scale), fill=white, width=line)
    for x in (27, 64, 101):
        draw.line((x * scale, 72 * scale, x * scale, 87 * scale), fill=white, width=line)
        draw.rounded_rectangle(((x - 10) * scale, 87 * scale, (x + 10) * scale, 106 * scale),
                               radius=3 * scale, outline=white, width=line)
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
