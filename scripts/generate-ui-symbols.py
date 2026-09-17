"""Draw milestone tile symbols without modifying building reward captures."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import svg_glyph

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/icons"
WHITE = (242, 245, 247, 255)

# Tier 5 unlocks the routed network; Tier 9 unlocks moving people through it.
SYMBOLS = [("affiliate", "Milestone"), ("walk", "PersonnelMilestone")]

for glyph, asset in SYMBOLS:
    for size in (256, 512):
        svg_glyph.tile(glyph, size, WHITE, stroke=1.7, fill=0.78).save(OUT / f"T_Teleporter{asset}_{size}.png")
print("Generated Logistics and Personnel milestone symbols; building captures preserved")
