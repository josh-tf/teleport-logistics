"""Trace the bundled FICSIT alpha silhouette into planar, simplified polygons.

Only generates model-source geometry; does not depend on Blender or OpenCV.
"""
from pathlib import Path
from collections import defaultdict
import json
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
folder = ROOT / 'assets/vendor/FICSIT'
im = Image.open(folder / 'Ficsit-logo-reference.png').convert('RGBA')
a = im.getchannel('A')
w, h = im.size
pixels = a.load()
edges = defaultdict(list)

def solid(x, y):
    return 0 <= x < w and 0 <= y < h and pixels[x, y] >= 128

for y in range(h):
    for x in range(w):
        if not solid(x, y):
            continue
        if not solid(x, y-1): edges[x, y].append((x+1, y))
        if not solid(x+1, y): edges[x+1, y].append((x+1, y+1))
        if not solid(x, y+1): edges[x+1, y+1].append((x, y+1))
        if not solid(x-1, y): edges[x, y+1].append((x, y))

def simplify(points, tolerance=1.1):
    if len(points) <= 2: return points
    ax, ay = points[0]; bx, by = points[-1]
    dx, dy = bx-ax, by-ay
    length = (dx*dx + dy*dy)**.5
    distances = [abs(dx*(ay-y)-(ax-x)*dy)/length if length else ((x-ax)**2+(y-ay)**2)**.5 for x,y in points[1:-1]]
    maximum = max(distances, default=0)
    if maximum <= tolerance: return [points[0], points[-1]]
    split = distances.index(maximum)+1
    return simplify(points[:split+1], tolerance)[:-1] + simplify(points[split:], tolerance)

polygons = []
while edges:
    start = next(iter(edges)); point = start; loop = []
    while True:
        loop.append(point)
        targets = edges[point]; next_point = targets.pop()
        if not targets: del edges[point]
        point = next_point
        if point == start: break
    area = sum(x1*y2-x2*y1 for (x1,y1),(x2,y2) in zip(loop,loop[1:]+loop[:1]))/2
    if abs(area) < 25: continue
    half = len(loop)//2
    loop = simplify(loop[:half+1])[:-1]+simplify(loop[half:]+loop[:1])[:-1]
    polygons.append([[round(x/w-.5,7),round((h/2-y)/w,7)] for x,y in loop])
assert len(polygons) == 7, f'Unexpected FICSIT silhouette components: {len(polygons)}'
(folder/'Ficsit-outline.json').write_text(json.dumps({'width':w,'height':h,'polygons':polygons},indent=2)+'\n')
print(f'FICSIT silhouette: {len(polygons)} polygons, {sum(map(len,polygons))} vertices')
