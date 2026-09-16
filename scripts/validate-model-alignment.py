"""Validate TeleportLogistics scale, axis convention, and exact native connector placement."""

from pathlib import Path
import math


ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "assets" / "models"
VENDOR = ROOT / "assets" / "vendor" / "Satisfactory_ModelingTools"
BUILDING_SOURCE = ROOT / "TeleportLogistics" / "Source" / "TeleportLogistics" / "Private" / "TeleportLogisticsBuilding.cpp"


def vertices(path):
    return [tuple(map(float, line.split()[1:4])) for line in path.read_text().splitlines()
            if line.startswith("v ")]


def key(point):
    return tuple(round(value, 2) for value in point)


def require_closed_item_housing(model_name):
    """Check the side gap from the reported camera angle, not just the far back."""
    path = MODELS / f"SM_Teleporter{model_name}.obj"
    points = vertices(path)
    faces = [tuple(int(token.split('/')[0])-1 for token in line.split()[1:])
             for line in path.read_text().splitlines() if line.startswith('f ')]
    triangles = [tuple(points[i] for i in face) for face in faces]

    def intersections(axis, first, second):
        axes = [i for i in range(3) if i != axis]
        j, k = axes
        for a, b, c in triangles:
            denominator = (b[k]-c[k])*(a[j]-c[j]) + (c[j]-b[j])*(a[k]-c[k])
            if abs(denominator) < 1e-8:
                continue
            u = ((b[k]-c[k])*(first-c[j]) + (c[j]-b[j])*(second-c[k]))/denominator
            v = ((c[k]-a[k])*(first-c[j]) + (a[j]-c[j])*(second-c[k]))/denominator
            if min(u, v, 1-u-v) >= -1e-6:
                yield u*a[axis] + v*b[axis] + (1-u-v)*c[axis]

    for x in (-60, -30, 0, 30, 60, 90, 110):
        for z in (60, 100, 150, 200, 250):
            hits = list(intersections(1, x, z))
            for side in (-1, 1):
                assert any(96 < side*y < 110 for y in hits), (
                    model_name, 'open connector side', side, x, z)
    for y in (-80, -40, 0, 40, 80):
        for z in (65, 125, 185, 245):
            assert any(-83 < x < -66 for x in intersections(0, y, z)), (
                model_name, 'open rear bulkhead', y, z)
    # Preserve the field-facing mouth: no new wall ahead of its central disc.
    for y in (-20, 0, 20):
        assert not any(0 < x < 260 for x in intersections(0, y, 140)), (
            model_name, 'belt mouth obstructed', y)
    # Every raised vent vertex must hug the actual narrow orange panel.
    details = [p for p in points if -90 < p[0] < 0 and 124.6 < p[1] < 145
               and 160 < p[2] < 190]
    assert details, (model_name, 'vent geometry missing')
    assert all(-57 < x < -11 and y < 125.4 for x, y, z in details), (
        model_name, 'unsupported side vent details')


def require_closed_pipe_rear(model_name):
    """Cast axial rays through the former annular gap, behind the native rim."""
    path = MODELS / f"SM_Teleporter{model_name}.obj"
    points = vertices(path)
    faces = [tuple(int(token.split('/')[0])-1 for token in line.split()[1:])
             for line in path.read_text().splitlines() if line.startswith('f ')]
    triangles = [tuple(points[i] for i in face) for face in faces
                 if min(points[i][0] for i in face) < 107 and
                 max(points[i][0] for i in face) > 45]

    def hits(y, z):
        for a, b, c in triangles:
            denominator = (b[2]-c[2])*(a[1]-c[1]) + (c[1]-b[1])*(a[2]-c[2])
            if abs(denominator) < 1e-8:
                continue
            u = ((b[2]-c[2])*(y-c[1]) + (c[1]-b[1])*(z-c[2]))/denominator
            v = ((c[2]-a[2])*(y-c[1]) + (a[1]-c[1])*(z-c[2]))/denominator
            x = u*a[0] + v*b[0] + (1-u-v)*c[0]
            if min(u, v, 1-u-v) >= -1e-6 and 45 < x < 107:
                return True
        return False

    for radius in (60, 80, 88):
        for index in range(32):
            angle = (index + 0.37)*math.tau/32
            assert hits(radius*math.cos(angle), 175+radius*math.sin(angle)), (
                model_name, 'open rear housing', radius, index)


def require_vendor(model_name, vendor_name, location, yaw=0.0):
    model = {key(point) for point in vertices(MODELS / f"SM_Teleporter{model_name}.obj")}
    cosine, sine = math.cos(yaw), math.sin(yaw)
    expected = []
    for x, y, z in vertices(VENDOR / f"{vendor_name}.obj"):
        transformed = (x * cosine - y * sine + location[0],
                       x * sine + y * cosine + location[1],
                       z + location[2])
        expected.append(key(transformed))
    present = sum(point in model for point in expected)
    ratio = present / len(expected)
    assert ratio > 0.92, (model_name, vendor_name, ratio)
    return ratio


ratios = []
ratios.append(require_vendor("ItemInput", "FactoryBelt_In", (160, 0, 100)))
ratios.append(require_vendor("ItemOutput", "FactoryBelt_Out", (160, 0, 100)))
ratios.append(require_vendor("FluidInput", "FactoryPipe_In", (160, 0, 175)))
ratios.append(require_vendor("FluidOutput", "FactoryPipe_Out", (160, 0, 175)))

# The hub fits a crafting-bench envelope; circular mast hardware stays round.
hub_vertices = vertices(MODELS / "SM_TeleporterHub.obj")
cap_vertices = [p for p in hub_vertices
                if 4.39 < math.hypot(p[0]+83.64, p[1]+52.5) < 5.84 and p[2] > 298]
assert len(cap_vertices) >= 8, "Custom mast top cap is missing"
assert abs(max(p[2] for p in cap_vertices) - 298.8) < 1
sizes = [max(p[i] for p in hub_vertices)-min(p[i] for p in hub_vertices) for i in range(3)]
assert 290 < sizes[0] < 310 and 599 < sizes[1] < 601 and 298 < sizes[2] < 302, sizes

for name in ("ItemInput", "ItemOutput"):
    require_closed_item_housing(name)
    points = vertices(MODELS / f"SM_Teleporter{name}.obj")
    low = tuple(min(point[i] for point in points) for i in range(3))
    high = tuple(max(point[i] for point in points) for i in range(3))
    # Standard belt assembly translated from its documented 1.00 m origin.
    assert high[0] >= 252.9 and low[2] <= 0.01 and high[2] >= 298.5, (name, low, high)
for name in ("FluidInput", "FluidOutput"):
    require_closed_pipe_rear(name)
    points = vertices(MODELS / f"SM_Teleporter{name}.obj")
    low = tuple(min(point[i] for point in points) for i in range(3))
    high = tuple(max(point[i] for point in points) for i in range(3))
    # The final vessel envelope is 3.186 m tall; pipe hardware still reaches
    # the documented snap face at +X and the base remains floor-aligned.
    assert high[0] >= 244.0 and low[2] <= 0.01 and high[2] >= 318.5, (name, low, high)

source = BUILDING_SOURCE.read_text()
for token in (
    "Belt->SetRelativeLocation(FVector(160, 0, 100));",
    "Belt->SetRelativeRotation(FRotator::ZeroRotator);",
    "Pipe->SetRelativeLocation(FVector(160, 0, 175));",
    "Pipe->SetRelativeRotation(FRotator::ZeroRotator);",
    "Power->SetRelativeLocation(FVector(-83.64, 52.5, 298.8));",
    "Power->SetRelativeRotation(FRotator(0, 180, 0));",
):
    assert token in source, token

generator = (ROOT / "scripts" / "generate-models-blender.py").read_text()
assert 'forward_axis="Y", up_axis="Z"' in generator
assert 'forward_axis="NEGATIVE_Y"' not in generator
print("PASS: native belt and pipe geometry retained at >92%; component "
      "belt/pipe transforms match the model; the hub cable meets the custom mast cap")
print("PASS: 192 axial rays hit the closed fluid connector rear transitions")
print("PASS: item housings close both side gaps and rear bulkheads; belt mouths "
      "stay open and vent details are seated on their panels")

# Native connector chevrons must follow the same connection-controlled material
# as the custom lamps, rather than remain in the always-visible factory slot.
for model in MODELS.glob('SM_Teleporter*.obj'):
    uv, active_material, signal_faces = [], '', 0
    for line in model.read_text().splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == 'vt':
            uv.append(tuple(map(float, parts[1:3])))
        elif parts[0] == 'usemtl':
            active_material = parts[1]
        elif parts[0] == 'f':
            if active_material == 'signal':
                signal_faces += 1
            if active_material == 'factory':
                coords = [uv[int(v.split('/')[1]) - 1] for v in parts[1:]]
                assert not all(u >= 2/3 - .001 and 1/6 - .001 <= v <= 1/3 + .001
                               for u, v in coords), f'{model.name}: unswitched emitter face'
    assert signal_faces > 0, f'{model.name}: missing switchable lamps'
print('PASS: all connector emissive faces use connection-controlled signal slots')
