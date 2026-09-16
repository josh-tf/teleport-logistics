"""Deterministic SFUIKIT composite exports; no labels baked into UI surfaces.
Sources/derivatives: CC BY-SA 4.0, Deantendo and Treelo. See assets/ui/NOTICE.md.
"""
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1]
source = root / 'assets/ui/sfuikit'
out = root / 'assets/ui/generated'
out.mkdir(parents=True, exist_ok=True)
# Crop only the white Photoshop canvas around the original panel composites.
for original, name, bounds in [
    ('sfuikit_panel_Layered.psd', 'T_TeleporterUI_Plate', (28, 20, 328, 326)),
    ('sfuikit_panel_dark.psd', 'T_TeleporterUI_Well', (20, 18, 321, 328)),
    ('sfuikit_panel_whitebox.psd', 'T_TeleporterUI_Meter', (20, 16, 277, 127)),
]:
    image = Image.open(source / original).convert('RGBA').crop(bounds)
    # Remove only white canvas connected to the outer edge; keep the meter's
    # enclosed white display opaque. This avoids white corners when nine-sliced.
    from collections import deque
    pixels = image.load(); width, height = image.size
    queue = deque([(x, y) for x in range(width) for y in (0, height-1)] +
                  [(x, y) for y in range(height) for x in (0, width-1)])
    seen = set()
    while queue:
        x, y = queue.popleft()
        if (x,y) in seen or not (0 <= x < width and 0 <= y < height): continue
        seen.add((x,y))
        r,g,b,a = pixels[x,y]
        if min(r,g,b) < 245: continue
        pixels[x,y] = (r,g,b,0)
        queue.extend(((x-1,y),(x+1,y),(x,y-1),(x,y+1)))
    image.save(out / (name + '.png'))
print('Exported three SFUIKIT resizable panels')
