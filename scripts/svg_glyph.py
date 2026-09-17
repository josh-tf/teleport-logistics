"""Render vendored Tabler icon glyphs with Pillow.

No SVG rasteriser is installed and adding one would put a native dependency in the
asset pipeline, so path data is sampled into polylines and stroked with Pillow's
round joins plus explicit round caps. The vendored sets are stroke-based line art,
which this subset of the path grammar covers exactly.

Callers draw at a multiple of the final size and downsample; Pillow does not
antialias strokes.
"""
from pathlib import Path
import math
import re

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
GLYPHS = ROOT / "assets/vendor/TablerIcons"
NUM = re.compile(r"-?\d*\.?\d+(?:e-?\d+)?")
STROKE = 1.5  # Tabler authors at 2.0 on a 24 unit grid; 1.5 reads lighter in game.
# Pillow stamps a pixel-rounded circle at every vertex of a joined polyline, so a
# densely sampled curve beads visibly. Twelve steps per curve stays smooth at 512 px
# while keeping those joins sparse.
CURVE_STEPS = 12


def _commands(data):
    for letter, chunk in re.findall(r"([MmLlHhVvCcSsQqTtAaZz])([^MmLlHhVvCcSsQqTtAaZz]*)", data):
        yield letter, [float(v) for v in NUM.findall(chunk)]


def _bezier(p0, p1, p2, p3, steps=CURVE_STEPS):
    for i in range(1, steps + 1):
        t = i / steps
        u = 1 - t
        yield (u ** 3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t ** 3 * p3[0],
               u ** 3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t ** 3 * p3[1])


def _arc(p0, p1, rx, ry, phi, large, sweep, steps=CURVE_STEPS):
    if rx == 0 or ry == 0:
        return [p1]
    cos_p, sin_p = math.cos(phi), math.sin(phi)
    dx2, dy2 = (p0[0] - p1[0]) / 2, (p0[1] - p1[1]) / 2
    x1 = cos_p * dx2 + sin_p * dy2
    y1 = -sin_p * dx2 + cos_p * dy2
    oversize = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry)
    if oversize > 1:
        rx, ry = rx * math.sqrt(oversize), ry * math.sqrt(oversize)
    num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1
    den = rx * rx * y1 * y1 + ry * ry * x1 * x1
    factor = math.sqrt(max(num / den, 0)) * (-1 if large == sweep else 1)
    cx1, cy1 = factor * rx * y1 / ry, -factor * ry * x1 / rx
    cx = cos_p * cx1 - sin_p * cy1 + (p0[0] + p1[0]) / 2
    cy = sin_p * cx1 + cos_p * cy1 + (p0[1] + p1[1]) / 2
    start = math.atan2((y1 - cy1) / ry, (x1 - cx1) / rx)
    finish = math.atan2((-y1 - cy1) / ry, (-x1 - cx1) / rx)
    delta = finish - start
    if not sweep and delta > 0:
        delta -= 2 * math.pi
    elif sweep and delta < 0:
        delta += 2 * math.pi
    out = []
    for i in range(1, steps + 1):
        angle = start + delta * i / steps
        px, py = rx * math.cos(angle), ry * math.sin(angle)
        out.append((cos_p * px - sin_p * py + cx, sin_p * px + cos_p * py + cy))
    return out


def polylines(data):
    """Sample one SVG path into polylines, in viewBox units."""
    out, current, point, start, ctrl = [], [], (0.0, 0.0), (0.0, 0.0), None
    for letter, args in _commands(data):
        rel = letter.islower()
        up = letter.upper()

        def at(x, y):
            return (point[0] + x, point[1] + y) if rel else (x, y)

        if up == "M":
            if len(current) > 1:
                out.append(current)
            point = at(args[0], args[1])
            start, current, ctrl = point, [point], None
            for i in range(2, len(args), 2):
                point = at(args[i], args[i + 1])
                current.append(point)
        elif up in "LT":
            for i in range(0, len(args), 2):
                point = at(args[i], args[i + 1])
                current.append(point)
            ctrl = None
        elif up == "H":
            for value in args:
                point = (point[0] + value, point[1]) if rel else (value, point[1])
                current.append(point)
            ctrl = None
        elif up == "V":
            for value in args:
                point = (point[0], point[1] + value) if rel else (point[0], value)
                current.append(point)
            ctrl = None
        elif up == "C":
            for i in range(0, len(args), 6):
                c1, c2 = at(args[i], args[i + 1]), at(args[i + 2], args[i + 3])
                end = at(args[i + 4], args[i + 5])
                current.extend(_bezier(point, c1, c2, end))
                point, ctrl = end, c2
        elif up == "S":
            for i in range(0, len(args), 4):
                c1 = (2 * point[0] - ctrl[0], 2 * point[1] - ctrl[1]) if ctrl else point
                c2 = at(args[i], args[i + 1])
                end = at(args[i + 2], args[i + 3])
                current.extend(_bezier(point, c1, c2, end))
                point, ctrl = end, c2
        elif up == "Q":
            for i in range(0, len(args), 4):
                c = at(args[i], args[i + 1])
                end = at(args[i + 2], args[i + 3])
                c1 = (point[0] + 2 / 3 * (c[0] - point[0]), point[1] + 2 / 3 * (c[1] - point[1]))
                c2 = (end[0] + 2 / 3 * (c[0] - end[0]), end[1] + 2 / 3 * (c[1] - end[1]))
                current.extend(_bezier(point, c1, c2, end))
                point, ctrl = end, c
        elif up == "A":
            for i in range(0, len(args), 7):
                rx, ry, rot, large, sweep, x, y = args[i:i + 7]
                end = at(x, y)
                current.extend(_arc(point, end, rx, ry, math.radians(rot), int(large), int(sweep)))
                point, ctrl = end, None
        elif up == "Z":
            if current:
                current.append(start)
                out.append(current)
                current, point = [start], start
    if len(current) > 1:
        out.append(current)
    return out


def glyph(name):
    """Vendored SVG source, by icon name."""
    return (GLYPHS / f"{name}.svg").read_text()


def draw(image, name, colour, box, stroke=STROKE):
    """Stroke one glyph, fitted to box=(left, top, size) in image pixels."""
    text = glyph(name)
    view = [float(v) for v in NUM.findall(re.search(r'viewBox="([^"]+)"', text).group(1))]
    left, top, size = box
    scale = size / max(view[2], view[3])
    width = max(int(round(stroke * scale)), 1)
    d = ImageDraw.Draw(image)
    for path in re.findall(r'<path[^>]*\sd="([^"]+)"', text):
        for line in polylines(path):
            points = [(left + x * scale, top + y * scale) for x, y in line]
            if len(points) > 1:
                d.line(points, fill=colour, width=width, joint="curve")
            # Pillow strokes butt caps; round them so corners match Tabler's look.
            for cap in (points[0], points[-1]):
                d.ellipse((cap[0] - width / 2, cap[1] - width / 2,
                           cap[0] + width / 2, cap[1] + width / 2), fill=colour)
    return image


def tile(name, size, colour, background=None, stroke=STROKE, fill=0.72, supersample=4):
    """One square glyph tile, antialiased by drawing large and downsampling."""
    big = size * supersample
    image = Image.new("RGBA", (big, big), background or (0, 0, 0, 0))
    span = big * fill
    draw(image, name, colour, ((big - span) / 2, (big - span) / 2, span), stroke)
    return image.resize((size, size), Image.Resampling.LANCZOS)
