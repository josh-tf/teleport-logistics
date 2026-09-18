"""Generate TeleportLogistics's game-ready static mesh sources with Blender.

The five models use Unreal centimetres and Satisfactory's connector convention:
local +X points away from the building. Stock-scale connector meshes are
vendored from Satisfactory Modeling Tools (see assets/vendor attribution).

Procedural surfaces are UV packed into the game's factory material atlas. At
import time they are consolidated to the native MI_Factory_Base_01 material,
so Customizer colours and paint finishes behave like stock machines. Actual
lamps share a separate ``signal`` slot, allowing the hub to disable every
emissive surface when it loses power.
"""

from pathlib import Path
import json
import math
import os

import bpy
import bmesh
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "models"
VENDOR = ROOT / "assets" / "vendor" / "Satisfactory_ModelingTools"
OUTPUT.mkdir(parents=True, exist_ok=True)

# Blender preview values only. Unreal binds native materials by slot name.
PALETTE = {
    "paint_primary": ((0.95, 0.35, 0.025), 0.12, 0.34),
    "paint_secondary": ((0.15, 0.17, 0.19), 0.18, 0.39),
    "shell": ((0.72, 0.72, 0.72), 0.82, 0.26),
    "steel": ((0.22, 0.24, 0.25), 0.80, 0.28),
    "frame": ((0.035, 0.040, 0.045), 0.58, 0.40),
    "rubber": ((0.008, 0.010, 0.012), 0.0, 0.78),
    "warning": ((1.0, 0.64, 0.02), 0.05, 0.40),
    "input": ((0.02, 0.90, 0.67), 0.05, 0.24),
    "output": ((1.0, 0.31, 0.035), 0.05, 0.24),
    "glow": ((0.05, 0.78, 1.0), 0.0, 0.18),
    "screen": ((0.006, 0.055, 0.070), 0.02, 0.20),
    "factory": ((0.58, 0.59, 0.60), 0.48, 0.36),
    "signal": ((0.04, 0.82, 0.67), 0.02, 0.20),
    "decal_custom": ((0.3, 0.3, 0.3), 0.0, 0.65),
    "decal_color": ((0.04, 0.05, 0.06), 0.05, 0.40),
    "decal_normal": ((0.50, 0.50, 0.50), 0.0, 0.45),
}

# UV origin is bottom-left. Regions match the vendored Factory_Base_UVSheet.png,
# whose cells the upstream kit labels as follows; our key names predate reading it,
# so the documented name is given for each:
#   Primary Metal   Composite Color   Dark Rubber/Plastic
#   Secondary Metal Grey Rough Metal  Plastic
#   Chrome          Dark Steel        Lights / Pure Unlit Black
# Composite Color is a painted composite, not a metal, which is why body panels
# mapped to it read as flat grey next to the stock machines.
# A small inset prevents mip bleeding across atlas cells.
ATLAS_RECTS = {
    "paint_primary": (0.012, 0.678, 0.322, 0.988),      # Primary Metal
    "paint_secondary": (0.012, 0.345, 0.322, 0.655),    # Secondary Metal
    # The body panels read as flat grey in game: that cell is painted metal, not
    # bare. Point them at the same cell the plate bezels use, which is the one
    # that reads as metal in the stock assets. Kept as its own key so the two
    # can diverge again without touching 41 call sites.
    "shell": (0.012, 0.012, 0.322, 0.322),              # Chrome, was Composite Color
    "steel": (0.012, 0.012, 0.322, 0.322),              # Chrome
    "frame": (0.345, 0.012, 0.655, 0.322),              # Dark Steel
    "rubber": (0.678, 0.678, 0.988, 0.988),             # Dark Rubber/Plastic
    "warning": (0.678, 0.012, 0.822, 0.155),
    "glow": (0.678, 0.178, 0.822, 0.322),
    # The atlas preview's teal and orange cells are input and output.
    "input": (0.927, 0.178, 0.988, 0.322),
    "output": (0.844, 0.178, 0.905, 0.322),
}

SIGNAL_SURFACES = {"input", "output", "glow"}
MODEL_OBJECTS = []


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    MODEL_OBJECTS.clear()


def material(name):
    value = bpy.data.materials.get(name)
    if value is None:
        colour, metallic, roughness = PALETTE[name]
        value = bpy.data.materials.new(name)
        value.diffuse_color = (*colour, 1.0)
        value.use_nodes = True
        shader = value.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = (*colour, 1.0)
        shader.inputs["Metallic"].default_value = metallic
        shader.inputs["Roughness"].default_value = roughness
        if name in SIGNAL_SURFACES or name == "signal":
            shader.inputs["Emission Color"].default_value = (*colour, 1.0)
            shader.inputs["Emission Strength"].default_value = 1.6
    return value


def register(obj, surface, smooth=False):
    obj.data.materials.append(material(surface))
    obj["teleport_logistics_surface"] = surface
    if smooth:
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
    MODEL_OBJECTS.append(obj)
    return obj


def apply_bevel(obj, amount, segments=2):
    if amount <= 0:
        return
    bpy.context.view_layer.objects.active = obj
    modifier = obj.modifiers.new("Edge radii", "BEVEL")
    modifier.width = amount
    modifier.segments = min(segments, 2)
    modifier.limit_method = "ANGLE"
    modifier.angle_limit = math.radians(25)
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def box(name, location, dimensions, surface="shell", bevel=2.0, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    apply_bevel(obj, min(bevel, min(dimensions) * 0.20))
    return register(obj, surface)


def cylinder(name, location, radius, depth, surface="steel", axis="z", vertices=20, bevel=1.0):
    rotation = (0, 0, 0)
    if axis == "x":
        rotation = (0, math.pi / 2, 0)
    elif axis == "y":
        rotation = (math.pi / 2, 0, 0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
                                       location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    apply_bevel(obj, bevel)
    return register(obj, surface, smooth=True)


def sphere(name, location, scale, surface="shell", segments=20, rings=10):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return register(obj, surface, smooth=True)


def reducer(name, start_x, end_x, start_radius, end_radius, z, surface="shell"):
    """Closed pressure-housing transition, along +X toward the native socket."""
    bpy.ops.mesh.primitive_cone_add(vertices=48, radius1=start_radius, radius2=end_radius,
                                    depth=end_x-start_x, end_fill_type="NGON",
                                    location=((start_x+end_x)/2, 0, z),
                                    rotation=(0, math.pi/2, 0))
    obj = bpy.context.object
    obj.name = name
    apply_bevel(obj, 1.2)
    return register(obj, surface, smooth=True)


def torus(name, location, major, minor, surface="frame", axis="z",
          major_segments=28, minor_segments=8):
    rotation = (0, 0, 0)
    if axis == "x":
        rotation = (0, math.pi / 2, 0)
    elif axis == "y":
        rotation = (math.pi / 2, 0, 0)
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                    major_segments=major_segments,
                                    minor_segments=minor_segments,
                                    location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    return register(obj, surface, smooth=True)


def strut(name, start, end, width, surface="frame", bevel=1.0):
    start_v, end_v = Vector(start), Vector(end)
    direction = end_v - start_v
    obj = box(name, (start_v + end_v) * 0.5, (width, width, direction.length), surface, bevel)
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    return obj


def hose(name, points, radius=2.4, surface="rubber"):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 2
    curve.bevel_depth = radius
    curve.bevel_resolution = 2
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for point, coordinate in zip(spline.bezier_points, points):
        point.co = coordinate
        point.handle_left_type = "AUTO"
        point.handle_right_type = "AUTO"
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    curve.materials.append(material(surface))
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj["teleport_logistics_surface"] = surface
    MODEL_OBJECTS.append(obj)
    return obj


def extruded_polygon(name, points, z, depth, surface, bevel=0.8):
    count = len(points)
    vertices = [(x, y, z) for x, y in points] + [(x, y, z + depth) for x, y in points]
    faces = [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, count + (i + 1) % count, count + i)
              for i in range(count)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    apply_bevel(obj, bevel)
    return register(obj, surface)


def arrow(name, centre, output, surface):
    x, y, z = centre
    direction = 1 if output else -1
    shape = [(-31, -10), (1, -10), (1, -22), (31, 0), (1, 22), (1, 10), (-31, 10)]
    points = [(x + px * direction, y + py) for px, py in shape]
    if direction < 0:
        points.reverse()
    return extruded_polygon(name, points, z, 0.08, surface, 0)


def stencil_decal(name, body, location, size=10, surface="shell"):
    """Surface-applied glyph mesh: no thickness, no bevel, no emissive lettering.

    A 0.08 cm offset from its host plate avoids coplanar flicker. Using the
    existing factory atlas keeps the decal in the same material section and
    avoids an extra masked draw call or a separate texture mip chain.
    """
    bpy.ops.object.text_add(location=location, rotation=(math.pi / 2, 0, 0))
    obj = bpy.context.object
    obj.name = name
    obj.data.body = body
    obj.data.font = bpy.data.fonts.load(str(ROOT / "assets/vendor/OpenSans/OpenSans-Bold.ttf"),
                                      check_existing=True)
    obj.data.align_x = "CENTER"
    obj.data.align_y = "CENTER"
    obj.data.size = size
    obj.data.extrude = 0
    obj.data.bevel_depth = 0
    # Small printed glyphs need fewer subdivisions than the structural curves.
    obj.data.resolution_u = 3
    obj.data.materials.append(material(surface))
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target="MESH")
    obj["teleport_logistics_surface"] = surface
    MODEL_OBJECTS.append(obj)
    # All glyph vertices must lie in the host plate plane, not protrude like
    # cast letters. The object transform supplies only the surface offset.
    assert all(abs(vertex.co.z) < 1e-6 for vertex in obj.data.vertices)
    return obj


def detail_plane(name, centre, width, height, tile, rotation=(math.pi / 2, 0, 0)):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(-width/2,-height/2,0),(width/2,-height/2,0),
                     (width/2,height/2,0),(-width/2,height/2,0)], [], [(0,1,2,3)])
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    obj.location=centre
    obj.rotation_euler=rotation
    uv=mesh.uv_layers.new(name="UVMap")
    col,row=tile%4,tile//4
    coords=((col/4,1-(row+1)/2),((col+1)/4,1-(row+1)/2),
            ((col+1)/4,1-row/2),(col/4,1-row/2))
    for loop,coord in zip(uv.data,coords): loop.uv=coord
    obj["teleport_logistics_authored_uv"]=True
    register(obj,"decal_custom")
    return obj


def identification_plate(prefix, centre, width, body, size=8):
    """Two-triangle label in a shared masked atlas, 0.8mm off its plate."""
    x,y,z=centre
    # Offset and facing follow the side, as screen_panel does. A hardcoded -Y offset
    # puts the far plate's label inboard, behind its own backing box, so it reads
    # blank; and a bare -pi/2 would carry its up axis to -Z and print upside down.
    outward = -1 if y < 0 else 1
    box(prefix+"_plate",centre,(width,1.4,22),"shell",.6)
    tile=7 if body=="PERSONNEL TELEPORTER" else 4 if body=="TELEPORTER HUB" else (2 if 'FLUID' in body else 0)+(1 if 'Output' in body else 0)
    # Sample only the band the atlas draws into. It matches the plate's own 4.5:1
    # aspect, so the mark and lettering are not squashed; keep these two in step
    # with BAND_TOP and BAND_HEIGHT in generate-detail-atlas.py.
    obj=detail_plane(prefix+"_label",(x,y+outward*.78,z),width-6,18,tile,
                     (math.pi/2,0,0) if outward < 0 else (math.pi/2,0,math.pi))
    row=tile//4
    band_top,band_height=176/512,114/512
    for loop in obj.data.uv_layers.active.data:
        local=(1-row/2-loop.uv.y)*2
        loop.uv.y=1-(row+(band_top+local*band_height))/2


def vendor_part(source_name, object_name, location, rotation=(0, 0, 0)):
    """Place a correctly scaled, UV-authored connector without altering its UVs."""
    path = VENDOR / f"{source_name}.obj"
    if not path.is_file():
        raise RuntimeError(f"Missing vendored connector source: {path}")
    before = set(bpy.context.scene.objects)
    bpy.ops.wm.obj_import(filepath=str(path), forward_axis="Y", up_axis="Z")
    imported = [obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "MESH"]
    if len(imported) != 1:
        raise RuntimeError(f"Expected one mesh in {path}, found {len(imported)}")
    obj = imported[0]
    obj.name = object_name
    obj.location = location
    obj.rotation_euler = rotation
    for slot in obj.material_slots:
        source = slot.material.name.split(".")[0] if slot.material else ""
        target = {
            "MI_MyNewMachine": "factory",
            "Decal_Color_Masked": "decal_color",
            "Decal_Normal": "decal_normal",
        }.get(source)
        if target is None:
            raise RuntimeError(f"Unexpected material {source!r} on {source_name}")
        slot.material = material(target)
    # Native connector arrows share the factory atlas with painted metal. Route
    # only its emissive UV island through the switchable lamp slot; preserve UVs.
    signal_index = len(obj.data.materials)
    obj.data.materials.append(material("signal"))
    uv_data = obj.data.uv_layers.active.data
    for face in obj.data.polygons:
        if obj.data.materials[face.material_index].name != "factory":
            continue
        if all(uv_data[i].uv.x >= 2 / 3 - .001 and
               1 / 6 - .001 <= uv_data[i].uv.y <= 1 / 3 + .001
               for i in face.loop_indices):
            face.material_index = signal_index
    MODEL_OBJECTS.append(obj)
    return obj


def screen_face(name, location, width, height, tile, rotation=(math.pi / 2, 0, 0)):
    """One authored face: square icon tile, upright, with no tiled bevel UVs."""
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(-width/2, -height/2, 0), (width/2, -height/2, 0),
                     (width/2, height/2, 0), (-width/2, height/2, 0)], [], [(0, 1, 2, 3)])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = rotation
    uv = mesh.uv_layers.new(name="UVMap")
    col, row = tile % 4, tile // 4
    u0, u1 = (col + .015) / 4, (col + .985) / 4
    v0, v1 = 1 - (row + .985) / 2, 1 - (row + .015) / 2
    for loop, point in zip(uv.data, ((u0,v0),(u1,v0),(u1,v1),(u0,v1))):
        loop.uv = point
    obj["teleport_logistics_authored_uv"] = True
    return register(obj, "screen")


def screen_panel(prefix, location, width=60, height=60, accent="input", tile=None):
    x, y, z = location
    outward = -1 if y < 0 else 1
    if tile is None:
        tile = (2 if prefix.startswith("Fluid") else 0) + (1 if accent == "output" else 0)
    box(prefix + "_housing", (x, y, z), (width + 12, 8, height + 12), "frame", 3)
    box(prefix + "_bezel", (x, y + outward * 4, z), (width+5, 2, height+5), "steel", 1)
    # A bare -pi/2 faces +Y but carries the quad's up axis to -Z, so the glyph reads
    # upside down. Rotate the -Y orientation about Z instead, which turns the normal
    # without touching up. Blender applies XYZ in order, so this is Rz * Rx.
    screen_face(prefix + "_glass", (x, y + outward * 5.2, z), width, height, tile,
                (math.pi/2, 0, 0) if outward < 0 else (math.pi/2, 0, math.pi))
    for dx in (-width/2-2, width/2+2):
        for dz in (-height/2-2, height/2+2):
            cylinder(prefix+f"_screw_{dx}_{dz}", (x+dx,y+outward*5.3,z+dz),
                     1.6, 1.2, "shell", axis="y", vertices=8, bevel=.15)


def beacon(prefix, x, y, base_z, height=65):
    cylinder(prefix+"_base", (x,y,base_z), 8, 8, "steel", vertices=16)
    cylinder(prefix+"_stem", (x,y,base_z+height*.37), 3.5, height*.72, "frame", vertices=12, bevel=.4)
    cylinder(prefix+"_lamp", (x,y,base_z+height*.8), 5.5, height*.34, "output", vertices=16, bevel=.5)
    for z in (base_z+height*.61, base_z+height*.99):
        cylinder(prefix+f"_rim_{z}", (x,y,z), 7, 3, "steel", vertices=16, bevel=.4)
    for dx,dy in ((5.5,0),(-5.5,0),(0,5.5),(0,-5.5)):
        cylinder(prefix+f"_guard_{dx}_{dy}", (x+dx,y+dy,base_z+height*.8), 1, height*.35,
                 "frame", vertices=8, bevel=.1)


def vents(prefix, origin, columns=5, rows=3):
    ox, oy, oz = origin
    for row in range(rows):
        for column in range(columns):
            box(f"{prefix}_{row}_{column}",
                (ox + (column - (columns - 1) / 2) * 10, oy,
                 oz + (row - (rows - 1) / 2) * 10),
                (6, 2.4, 2.5), "rubber", 0.5)


def common_base(prefix, length=340, width=280):
    box(prefix + "_skid", (-10, 0, 7), (length, width, 14), "frame", 5)
    box(prefix + "_deck", (-10, 0, 17), (length - 16, width - 16, 8), "steel", 3)
    box(prefix + "_inset", (-10, 0, 22), (length - 44, width - 44, 3), "rubber", 1)
    for side in (-1,1):
        y=side*(width/2-10)
        box(f"{prefix}_rail_{side}", (-10,y,23), (length-35,15,16), "shell", 2)
        box(f"{prefix}_rail_inset_{side}", (-10,y+side*7.7,23), (length-82,1,5), "steel", .2)
        for x in (-length*.3,0,length*.3):
            cylinder(f"{prefix}_rail_screw_{side}_{x}", (x,y,32),2.4,2,"steel",vertices=8,bevel=.2)
    for side in (-1,1):
        x=-10+side*(length/2-10)
        box(f"{prefix}_end_rail_{side}", (x,0,23), (15,width-24,16), "shell", 2)
        for y in (-width*.27,width*.27):
            box(f"{prefix}_end_flash_{side}_{y}", (x,y,32), (15,26,2), "paint_primary", .4)
    for xi,x in enumerate((-10-length/2+22,-10+length/2-22)):
        for yi,y in enumerate((-width/2+22,width/2-22)):
            box(f"{prefix}_foot_{xi}_{yi}", (x,y,25), (35,35,28), "frame", 4)
            box(f"{prefix}_foot_cap_{xi}_{yi}", (x,y,38), (27,27,5), "shell", 2)
            cylinder(f"{prefix}_pad_{xi}_{yi}", (x,y,42), 10, 4,"rubber",vertices=16)
            cylinder(f"{prefix}_bolt_{xi}_{yi}", (x,y,45), 4.5, 4,"steel",vertices=6,bevel=.4)
    # Shared flat hazard decal instead of a row of raised coloured boxes.
    stripe=detail_plane(prefix+"_hazard",(-10+length/2+.1,0,23),width-90,12,5,
                        (math.pi/2,0,math.pi/2))
    for loop in stripe.data.uv_layers.active.data:
        local=(.5-loop.uv.y)*2
        loop.uv.y=1-(1+(88+local*100)/512)/2


def sign_mount(prefix, x, y, z, width=120, height=48, anchor_x=None):
    """Flat rear-facing pad for vanilla wall signs, with seated supports."""
    if anchor_x is not None:
        for side in (-1,1):
            box(prefix+f"_sign_support_{side}",((anchor_x+x)/2,y+side*(width/2-10),z),
                (abs(anchor_x-x),8,12),"steel",.8)
    box(prefix+"_sign_mount",(x,y,z),(4,width,height),"steel",1)
    box(prefix+"_sign_face",(x-2.2,y,z),(.4,width-12,height-10),"frame",0)
    for side in (-1,1):
        for level in (-1,1):
            cylinder(prefix+f"_sign_bolt_{side}_{level}",(x-2.6,y+side*(width/2-5),z+level*(height/2-5)),
                     2,1,"shell",axis="x",vertices=6,bevel=.2)


def item_terminal(output):
    clear_scene()
    prefix = "ItemOut" if output else "ItemIn"
    accent = "output" if output else "input"
    common_base(prefix)
    connector = vendor_part("FactoryBelt_Out" if output else "FactoryBelt_In",
                            prefix + "_NativeConnector", (160, 0, 100))
    # The vendored liner runs from x=120 to the front face, and the housing shell
    # stopped at the base plate, leaving its painted rear flank bare along the whole
    # bay. Wrap the rear 40 cm so only the collar past x=160 stands proud, the way a
    # stock machine seats a belt connector.
    # Overlap the housing wall rather than butting against it: a box only carries
    # vertices at its corners, so the exact wall face is not measurable from the
    # export, and an overlap inside a solid model costs nothing. Outer face matches
    # the shell's own 138 so the wrap does not stand out as a ledge.
    # Face-based coverage of the flank zone shows the body ending at x=58 and the
    # connector region starting at x=80, so the wrap has to bridge from 56 to the
    # collar rather than sit only in front of it. Measured by face span: a box only
    # carries corner vertices, so a vertex scan reports the middle of a slab empty.
    # The vendored liner's upper wall stops around x=230, so looking up from inside
    # the bay you see straight out of the front. Tuck a lintel just inside the
    # liner's silhouette; it sits far above the belt path at z=100.
    box(prefix + "_collar_lintel", (234, 0, 272), (42, 214, 16), "shell", 2)
    for side in (-1, 1):
        box(prefix + f"_collar_flank_{side}", (108, side * 122, 166), (104, 32, 232), "shell", 3)
    # No cap: the roof already spans this depth, and a plate here read as a blank
    # slab over the whole front half from above.
    box(prefix + "_collar_sill", (108, 0, 56), (104, 250, 24), "shell", 3)

    # The reference collar includes a flat dark end plate 7.5 cm behind its
    # origin. Our own circular field terminates the tunnel, so remove only that
    # plate; keep the native rim, UVs and snap geometry intact.
    collar_mesh = bmesh.new()
    collar_mesh.from_mesh(connector.data)
    cap = [face for face in collar_mesh.faces
           if all(abs(vertex.co.x - 7.5) < .01 for vertex in face.verts)
           and face.calc_area() > 100]
    if not cap:
        raise RuntimeError("Expected the native conveyor end plate")
    bmesh.ops.delete(collar_mesh, geom=cap, context="FACES_ONLY")
    collar_mesh.to_mesh(connector.data)
    collar_mesh.free()
    connector.data.update()

    # Stock connectors are mounting collars, not complete machine bodies.
    # Enclose the space from the native collar to the rear bulkhead. The field
    # remains visible through the +X belt mouth, without see-through side gaps.
    for side in (-1, 1):
        box(f"{prefix}_rear_shoulder_{side}", (24, side * 103, 162),
            (188, 12, 234), "frame", 3)
        box(f"{prefix}_shoulder_trim_{side}", (97, side * 111, 180),
            (34, 4, 136), "paint_primary", 2)
    box(prefix + "_rear_hood", (69, 0, 275), (98, 218, 14), "frame", 3)
    box(prefix + "_housing_floor", (24, 0, 48), (188, 216, 10), "frame", 2)
    # A closed service bulkhead behind the field terminates the conveyor tunnel.
    # It overlaps both pylons and the roof; the front (+X) throat stays open.
    box(prefix + "_rear_bulkhead", (-72, 0, 154), (10, 216, 224), "frame", 3)
    sign_mount(prefix,-140,0,240,150,48,anchor_x=-80)
    # IN uses one broad receiver panel; OUT repeats paired delivery modules.
    for y in ((-48, 48) if output else (0,)):
        box(prefix + f"_rear_service_panel_{y}", (-79, y, 169),
            (5, 76 if output else 174, 148), "frame", 3)
        for z in (112, 138, 164, 190, 216):
            box(prefix + f"_rear_vent_{y}_{z}", (-82, y, z),
                (2, 50 if output else 108, 5), "rubber", .5)

    box(prefix + "_belt_bed", (50, 0, 79), (178, 102, 13), "frame", 4)
    box(prefix + "_belt_surface", (52, 0, 87), (176, 88, 5), "rubber", 1.5)
    for index, x in enumerate((-20, 20, 60, 100)):
        cylinder(f"{prefix}_roller_{index}", (x, 0, 94), 7.5, 84, "shell",
                 axis="y", vertices=16, bevel=0.7)

    torus(prefix + "_field_frame", (-32, 0, 126), 74, 10, "frame", axis="x", major_segments=36)
    torus(prefix + "_field_paint", (-25, 0, 126), 65, 6, "shell",
          axis="x", major_segments=36)
    torus(prefix + "_field_light", (-18, 0, 126), 57, 2.6, accent,
          axis="x", major_segments=36)
    cylinder(prefix + "_field_depth", (-37, 0, 126), 56, 9, "rubber",
             axis="x", vertices=36, bevel=0.8)

    for side in (-1, 1):
        y = side * 105
        box(f"{prefix}_pylon_{side}", (-37, y, 150), (64, 30, 226), "frame", 7)
        box(f"{prefix}_panel_{side}", (-34, y + side * 17, 158),
            (46, 5, 168), "paint_primary", 3)
        box(f"{prefix}_trim_{side}", (-34, y + side * 20, 202),
            (34, 2.0, 14), "paint_secondary", 1)
        # Former diagonal support is enclosed by the continuous side wall.
        strut(f"{prefix}_brace_b_{side}", (-75, y, 52), (-55, y, 102), 9, "steel", 1.3)
        for z in (96, 151, 206):
            cylinder(f"{prefix}_fastener_{side}_{z}",
                     (-34, y + side * 19.7, z), 3, 1.4, "steel",
                     axis="y", vertices=6, bevel=0.2)

    box(prefix + "_crown_frame", (-28, 0, 270), (145, 236, 22), "shell", 4)
    box(prefix + "_crown_shell", (-23, 0, 280), (126, 216, 10), "frame", 3)
    box(prefix + "_crown_spine", (-20, 0, 292), (104, 186, 3), "frame", 1)
    arrow(prefix + "_direction", (-18, 0, 293.58), output, accent)
    if output:
        for side in (-1, 1):
            # Twin low delivery rails sit on the roof, clear of its direction mark.
            wedge(prefix + f"_delivery_rail_{side}", (-20, side * 65, 302),
                  (106, 20, 19), "paint_primary", slope=.45, bevel=2)
            for x in (18, 49):
                box(prefix + f"_delivery_pod_{side}_{x}", (x, side * 114, 177),
                    (20, 16, 84), "shell", 3)
                box(prefix + f"_delivery_pod_inset_{side}_{x}", (x, side * 122, 177),
                    (12, 2, 59), "paint_secondary", .5)
    else:
        box(prefix + "_receiver_cowl", (-70, 0, 298), (28, 146, 14), "paint_primary", 3)
        for y in (-50, -25, 0, 25, 50):
            box(prefix + f"_receiver_louvre_{y}", (-70, y, 305),
                (19, 14, 1.4), "rubber", .3)
        for side in (-1, 1):
            box(prefix + f"_receiver_side_cover_{side}", (30, side * 110, 177),
                (56, 4, 84), "frame", 2)

    # The roof reached |y|=120 while the shell runs to 146, leaving an open slot
    # along both top edges from x=-110 to 40. Measured by projecting near-horizontal
    # roof faces onto xy and looking for uncovered cells.
    for side in (-1, 1):
        box(prefix + f"_roof_edge_{side}", (-30, side * 133, 278), (162, 27, 17), "shell", 2)
    screen_panel(prefix + "_console", (-34, -128.5, 158), 60, 66, accent)
    # Repeat the glyph on the opposite face so it reads from either approach in
    # world, and so a build-menu capture can frame the building from either side.
    screen_panel(prefix + "_console_far", (-34, 128.5, 158), 60, 66, accent)
    # The panel face is at Y=124.5. Embed these small slots into that face;
    # the former six-column array at Y=140.5 floated beyond the narrow panel.
    vents(prefix + "_vent", (-34, 123.5, 174), 3, 3)
    # A bezel extension seats the label directly on the console, instead of
    # suspending individual letters above it. Native collar arrows remain the
    # primary flow markings; this small plate identifies the device at hand.
    label = "ITEM TELEPORTER (Output)" if output else "ITEM TELEPORTER (Input)"
    box(prefix + "_console_label_bezel", (-34, -128.5, 234), (94, 8, 30), "frame", 2)
    identification_plate(prefix + "_id", (-34, -133.2, 235), 87, label, 9)
    box(prefix + "_console_label_bezel_far", (-34, 128.5, 234), (94, 8, 30), "frame", 2)
    identification_plate(prefix + "_id_far", (-34, 133.2, 235), 87, label, 9)
    hose(prefix + "_loom", [(-75, -116, 78), (-98, -126, 133), (-76, -118, 210)], 3, "rubber")
    hose(prefix + "_data", [(12, -116, 82), (34, -125, 133), (20, -118, 192)], 2.4, "steel")
    beacon(prefix+"_status", 4, 109, 260, 62)
    for side in (-1,1):
        for x in (-82,29):
            box(prefix+f"_crown_corner_{side}_{x}", (x,side*103,278), (21,24,20), "frame", 2.5)
            cylinder(prefix+f"_crown_screw_{side}_{x}", (x,side*103,290),3,3,"steel",vertices=6,bevel=.4)
        box(prefix+f"_throat_rim_{side}", (109,side*103,172), (12,18,182), "shell", 2)
        for z in (97,178,247):
            cylinder(prefix+f"_throat_rivet_{side}_{z}", (116,side*103,z),2.8,3,"steel",axis="x",vertices=6,bevel=.3)
    box(prefix+"_throat_header", (109,0,269), (14,215,17), "shell", 3)
    box(prefix+"_control_box", (28,-116,70),(48,18,38),"frame",3)
    box(prefix+"_control_face", (28,-127,70),(38,3,27),"paint_primary",1)
    for x in (17,29,41):
        for z in (64,76):
            box(prefix+f"_indicator_{x}_{z}",(x,-129,z),(5,1,4),"glow",.3)
    return finish(f"SM_TeleporterItem{'Output' if output else 'Input'}")


def fluid_terminal(output):
    clear_scene()
    prefix = "FluidOut" if output else "FluidIn"
    accent = "output" if output else "input"
    common_base(prefix)
    vendor_part("FactoryPipe_Out" if output else "FactoryPipe_In",
                prefix + "_NativeConnector", (160, 0, 175))
    # The pipe liner's rear flank was bare from the base plate up. It is perfectly
    # round, radius 100 about the connector axis, so a cylindrical flange wraps it
    # and matches the vessel's language; a box bezel here reads as a square inlet.
    cylinder(prefix + "_collar_boss", (135, 0, 175), 112, 50, "shell", axis="x",
             vertices=36, bevel=2)
    torus(prefix + "_collar_rib", (158, 0, 175), 114, 5, "frame", major_segments=36,
          axis="x")


    cylinder(prefix + "_vessel", (-35, 0, 166), 66, 196, "shell", vertices=36, bevel=2)
    sphere(prefix + "_vessel_lower", (-35, 0, 68), (66, 66, 28), "shell", 28, 14)
    sphere(prefix + "_vessel_upper", (-35, 0, 264), (66, 66, 28), "shell", 28, 14)
    for index, z in enumerate((82, 128, 174, 220, 264)):
        torus(f"{prefix}_collar_{index}", (-35, 0, z), 70, 7, "paint_secondary" if index == 2 else "frame", major_segments=36)
        if index in (1,):
            torus(f"{prefix}_coil_{index}", (-35, 0, z + 3), 74, 1.1,
                  accent, major_segments=36)
    cylinder(prefix + "_top_plate", (-35, 0, 285), 54, 13,
             "paint_primary", vertices=32, bevel=2)
    cylinder(prefix + "_lower_plate", (-35, 0, 47), 58, 10,
             "paint_secondary", vertices=32, bevel=2)

    cylinder(prefix + "_process_neck", (78, 0, 175), 43, 92, "frame",
             axis="x", vertices=32, bevel=1.5)
    # Close the annulus between the narrow process neck and native pipe collar.
    reducer(prefix + "_rear_reducer", 48, 112, 43, 96, 175, "shell")
    cylinder(prefix + "_rear_flange", (109, 0, 175), 98, 10, "frame",
             axis="x", vertices=48, bevel=1.2)
    for index in range(8):
        angle = index * math.tau / 8
        strut(prefix + f"_reducer_rib_{index}",
              (59, math.cos(angle) * 54, 175 + math.sin(angle) * 54),
              (102, math.cos(angle) * 91, 175 + math.sin(angle) * 91),
              4, "steel", 0.6)
    torus(prefix + "_neck_signal", (46, 0, 175), 43, 3, accent,
          axis="x", major_segments=32)

    for side in (-1, 1):
        y = side * 91
        box(f"{prefix}_cage_{side}", (-39, y, 169), (36, 22, 244), "frame", 5)
        box(f"{prefix}_cage_panel_{side}", (-38, y + side * 13, 174),
            (24, 5, 188), "paint_primary", 2.5)
        strut(f"{prefix}_cage_brace_{side}", (-92, y, 53), (-67, y, 106), 9, "steel", 1.3)
        for z in (105, 170, 235):
            cylinder(f"{prefix}_bolt_{side}_{z}", (-38, y + side * 13, z),
                     4, 3, "steel", axis="y", vertices=12, bevel=0.4)

    hose(prefix + "_bypass", [(-62, 56, 244), (-5, 79, 242), (48, 70, 207), (75, 43, 183)],
         4.5, "steel")
    cylinder(prefix+"_cap_seal", (-35,0,282), 69, 6, "rubber", vertices=40)
    cylinder(prefix+"_cap_flange", (-35,0,288), 74, 8, "shell", vertices=40)
    cylinder(prefix+"_cap_armour", (-35,0,300 if output else 296),
             56 if output else 68, 20 if output else 12, "paint_primary", vertices=40, bevel=3)
    cylinder(prefix+"_cap_neck", (-35,0,312 if output else 310),
             17, 13 if output else 19, "steel", vertices=20)
    cylinder(prefix+"_cap_plug", (-35,0,321), 23, 7, "shell", vertices=8, bevel=1)
    box(prefix+"_cap_handle",(-35,0,329),(30,9,8),"steel",1)
    for index in range(10):
        theta=index*math.tau/10
        bolt_radius = 48 if output else 60
        cylinder(prefix+f"_cap_bolt_{index}",
                 (-35+math.cos(theta)*bolt_radius, math.sin(theta)*bolt_radius,
                  311.5 if output else 303.5),
                 2.8,4,"steel",vertices=6,bevel=.3)
    beacon(prefix+"_status", -25,88,269,64)
    hose(prefix+"_service_hose", [(-92,-48,250),(-118,-72,235),(-120,-74,88),(-96,-49,60)],
         4.4,"rubber")
    hose(prefix+"_copper_line", [(-18,-65,258),(5,-86,238),(7,-88,70),(-50,-82,58)],
         3,"paint_primary")
    if output:
        # Two compact pressure modules distinguish the outlet from either side
        # and the rear. Each is supported directly by brackets on the vessel.
        for y in (-38, 38):
            for z in (117, 229):
                box(prefix + f"_regulator_mount_{y}_{z}", (-88, y, z),
                    (32, 26, 10), "frame", 2)
            cylinder(prefix + f"_regulator_{y}", (-103, y, 173), 16, 126,
                     "steel", vertices=20, bevel=2)
            for z in (113, 233):
                cylinder(prefix + f"_regulator_cap_{y}_{z}", (-103, y, z),
                         18, 8, "shell", vertices=20, bevel=1)
    else:
        box(prefix + "_receiver_service_cover", (-100, 0, 173),
            (12, 88, 120), "frame", 4)
        for z in (137, 161, 185, 209):
            box(prefix + f"_receiver_service_slot_{z}", (-106, 0, z),
                (1.4, 54, 4), "rubber", .3)
    screen_panel(prefix + "_console", (-38, -110.5, 174), 60, 72, accent)
    # Repeat the glyph on the opposite face so it reads from either approach in
    # world, and so a build-menu capture can frame the building from either side.
    screen_panel(prefix + "_console_far", (-38, 110.5, 174), 60, 72, accent)
    label = "FLUID TELEPORTER (Output)" if output else "FLUID TELEPORTER (Input)"
    box(prefix + "_console_label_bezel", (-38, -110.5, 226), (80, 8, 26), "frame", 2)
    identification_plate(prefix + "_id", (-38, -115.2, 226), 73, label, 9)
    box(prefix + "_console_label_bezel_far", (-38, 110.5, 226), (80, 8, 26), "frame", 2)
    identification_plate(prefix + "_id_far", (-38, 115.2, 226), 73, label, 9)
    # The stock pipe connector already carries an authored flow-direction decal;
    # the coloured containment coils reinforce it without a floating sign.
    sign_mount(prefix,-130,0,247,120,44,anchor_x=-108)
    return finish(f"SM_TeleporterFluid{'Output' if output else 'Input'}")


def wedge(name, location, dimensions, surface="paint_primary", slope=0.30, bevel=2.5):
    sx, sy, sz = (value / 2 for value in dimensions)
    vertices = [
        (-sx, -sy, -sz), (sx, -sy, -sz), (sx, -sy, sz * slope), (-sx, -sy, sz),
        (-sx, sy, -sz), (sx, sy, -sz), (sx, sy, sz * slope), (-sx, sy, sz),
    ]
    faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (3, 7, 6, 2),
             (0, 4, 7, 3), (1, 2, 6, 5)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    apply_bevel(obj, bevel)
    return register(obj, surface)


def sloped_screen(prefix, location, width, depth, accent, angle, tile=4):
    rotation = (0, math.radians(angle), 0)
    centre = Vector(location)
    normal = Vector((math.sin(math.radians(angle)), 0, math.cos(math.radians(angle))))
    housing = box(prefix+"_housing", centre, (width+10,depth+10,7),"frame",2,rotation)
    screen_face(prefix+"_glass", centre+normal*3.6,width,depth,tile,rotation)
    return housing


def hub():
    clear_scene()
    prefix="Hub"
    common_base(prefix, length=300, width=600)
    # Assemblies are composed at final scale; circular parts remain circular.
    tx,ty=-83.64,-52.5
    for y in (ty-72,ty+72):
        box(prefix+f"_tower_post_{y}",(tx+24,y,133),(18,18,202),"steel",2)
        box(prefix+f"_tower_armour_{y}",(tx+36,y,145),(6,20,162),"paint_primary",1)
        box(prefix+f"_tower_rear_{y}",(tx-39,y,134),(12,12,202),"frame",2)
        for z in (65,145,225):
            cylinder(prefix+f"_tower_fastener_{y}_{z}",(tx+40,y,z),2.8,2,"steel",axis="x",vertices=6,bevel=.3)
    cylinder(prefix+"_core",(tx,ty,131),34,180,"rubber",vertices=32,bevel=2)
    for index,z in enumerate((48,86,124,162,200,221)):
        cylinder(prefix+f"_core_jacket_{index}",(tx,ty,z),41,11,"steel",vertices=32)
        torus(prefix+f"_core_gasket_{index}",(tx,ty,z+6),38,2,"paint_secondary",major_segments=32)
    for z in (73,188,220):
        torus(prefix+f"_core_signal_{z}",(tx,ty,z),35,1.4,"glow",major_segments=32)
    box(prefix+"_crown_frame",(tx,ty,235),(110,179,15),"shell",3)
    box(prefix+"_crown_armour",(tx,ty,244),(88,155,5),"paint_primary",1.5)
    for x in (tx-44,tx+44):
        for y in (ty-76,ty+76):
            box(prefix+f"_crown_corner_{x}_{y}",(x,y,239),(18,20,19),"frame",2)
            cylinder(prefix+f"_crown_bolt_{x}_{y}",(x,y,250),2.5,3,"steel",vertices=6,bevel=.2)
    # The topmost custom cap stays on the runtime power-wire anchor.
    cylinder(prefix+"_mast_foot",(tx,ty,249),17,8,"steel",vertices=24)
    cylinder(prefix+"_mast",(tx,ty,271.9),4.32,43.8,"steel",vertices=16,bevel=.5)
    cylinder(prefix+"_mast_light",(tx,ty,273),2,37,"glow",vertices=12,bevel=.3)
    torus(prefix+"_antenna_ring",(tx,ty,295.2),20.16,2.52,"shell",major_segments=32)
    torus(prefix+"_antenna_signal",(tx,ty,295.2),14.4,1.0,"glow",major_segments=32)
    for degrees in range(0,360,90):
        theta=math.radians(degrees)
        strut(prefix+f"_antenna_spoke_{degrees}",(tx,ty,295.2),
              (tx+math.cos(theta)*18,ty+math.sin(theta)*18,295.2),2.45,"steel",.3)
    cylinder(prefix+"_antenna_cap",(tx,ty,295.92),5.76,5.04,"steel",vertices=16,bevel=.43)
    beacon(prefix+"_status",tx,ty+61,248,48)

    # Left instrumentation and service enclosure.
    box(prefix+"_instrument_arm",(tx,-155,165),(16,64,14),"steel",2)
    screen_panel(prefix+"_network_screen",(tx,-190,163),65,82,"input",tile=4)
    box(prefix+"_service_case",(tx,-218,77),(89,82,93),"shell",4)
    box(prefix+"_service_panel",(tx,-261,78),(69,5,73),"frame",1)
    vents(prefix+"_service_vents",(tx,-264,80),5,5)
    box(prefix+"_service_flash",(tx+47,-217,77),(5,66,78),"paint_primary",1)

    # Low console foreground leaves the tower legible and keeps controls reachable.
    wedge(prefix+"_console_frame",(50,-50,75),(125,204,92),"frame",.28,4)
    wedge(prefix+"_console_armour",(51,-50,81),(112,190,83),"frame",.28,3)
    slope=(83/2*(.28-1))/112
    angle=math.degrees(math.atan(-slope))
    sx=42
    sz=81+83/2+(sx-(51-112/2))*slope
    normal=Vector((-slope,0,1)).normalized()
    sloped_screen(prefix+"_console_display",Vector((sx,-66,sz))+normal*3,75,94,"input",angle,4)
    box(prefix+"_console_front_bezel",(115,-50,68),(8,184,57),"shell",2)
    box(prefix+"_console_front_inset",(120,-50,68),(3,165,43),"frame",1)
    for z in (55,65,75,85):
        box(prefix+f"_front_vent_{z}",(123,-50,z),(3,135,3),"steel",.5)
    for y in (-133,32):
        cylinder(prefix+f"_console_screw_{y}",(121,y,70),3,2,"steel",axis="x",vertices=6,bevel=.3)
    identification_plate(prefix+"_id",(42,-154,86),86,"TELEPORTER HUB",8)
    for i in range(3):
        box(prefix+f"_console_key_{i}",(29+i*15,28,112-i*4),(9,14,3),"shell",.5,rotation=(0,math.radians(angle),0))

    # Right power cabinet: independent shoulder with grounded cables and service faces.
    box(prefix+"_power_cabinet",(-59,173,112),(137,106,162),"frame",5)
    box(prefix+"_power_armour",(-59,172,192),(119,96,14),"frame",2)
    for y in (121,225):
        box(prefix+f"_power_cheek_{y}",(-58,y,115),(113,9,145),"shell",3)
    box(prefix+"_power_front",(14,172,115),(12,96,146),"shell",3)
    box(prefix+"_power_front_recess",(21,172,115),(3,75,113),"frame",1)
    # Lightning display faces +X, same authored atlas as the network console.
    screen_face(prefix+"_power_display",(23,172,126),54,74,5,(math.pi/2,0,math.pi/2))
    for z in (53,65,77):
        box(prefix+f"_power_front_vent_{z}",(24,172,z),(2,55,3),"steel",.3)
    for y in (134,210):
        for z in (62,173):
            cylinder(prefix+f"_power_screw_{y}_{z}",(23,y,z),3,2,"steel",axis="x",vertices=6,bevel=.3)
    for y in (140,201):
        box(prefix+f"_power_top_module_{y}",(-42,y,205),(38,29,11),"steel",2)
        box(prefix+f"_power_top_light_{y}",(-41,y,211),(20,14,1.2),"glow",.3)
    for z,r,surface in ((63,6,"paint_primary"),(95,5,"rubber"),(153,4,"steel")):
        hose(prefix+f"_cabinet_loom_{z}",[(tx+9,ty+25,z),(tx+35,50,z),
             (tx+56,91,z),(tx+45,117,z+13)],r,surface)
    hose(prefix+"_power_return",[(6,195,67),(63,199,61),(75,132,47),(37,77,45)],5,"paint_primary")
    # Power wiring uses the custom mast above; no second decorative socket.
    box(prefix+"_power_rear_panel",(-129,173,113),(4,82,127),"frame",2)
    sign_mount(prefix,tx-56,ty,235,120,24)
    return finish("SM_TeleporterHub")


def project_surface_uv(obj, surface):
    """Cube-project one procedural object, then move it into its atlas cell."""
    if obj.get("teleport_logistics_authored_uv"):
        return
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=68.0, correct_aspect=True, clip_to_bounds=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    if surface in ATLAS_RECTS:
        u0, v0, u1, v1 = ATLAS_RECTS[surface]
        # Preserve continuous projected spans. Fractional wrapping per vertex
        # folds faces at integer boundaries and stretches the surface detail.
        coords=[loop.uv.copy() for loop in obj.data.uv_layers.active.data]
        low_u=min(v.x for v in coords); low_v=min(v.y for v in coords)
        span=max(max(v.x for v in coords)-low_u,max(v.y for v in coords)-low_v,1e-6)
        for loop,uv in zip(obj.data.uv_layers.active.data,coords):
            loop.uv.x=u0+(uv.x-low_u)/span*(u1-u0)
            loop.uv.y=v0+(uv.y-low_v)/span*(v1-v0)
        target = "signal" if surface in SIGNAL_SURFACES else "factory"
        for slot in obj.material_slots:
            slot.material = material(target)
    elif surface == "screen":
        for loop in obj.data.uv_layers.active.data:
            loop.uv.x -= math.floor(loop.uv.x)
            loop.uv.y -= math.floor(loop.uv.y)


def finish(filename):
    live = [obj for obj in MODEL_OBJECTS if obj and obj.name in bpy.context.scene.objects]
    if not live:
        raise RuntimeError(f"No geometry generated for {filename}")
    for obj in live:
        surface = obj.get("teleport_logistics_surface")
        if surface:
            project_surface_uv(obj, surface)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in live:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = live[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = filename
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.quads_convert_to_tris(quad_method="BEAUTY", ngon_method="BEAUTY")
    bpy.ops.mesh.dissolve_degenerate(threshold=0.005)
    bpy.ops.object.mode_set(mode="OBJECT")
    cleanup = bmesh.new()
    cleanup.from_mesh(joined.data)
    zero_faces = [face for face in cleanup.faces if face.calc_area() <= 1.0e-4]
    if zero_faces:
        bmesh.ops.delete(cleanup, geom=zero_faces, context="FACES")
    cleanup.to_mesh(joined.data)
    cleanup.free()
    joined.data.update()

    path = OUTPUT / f"{filename}.obj"
    bpy.ops.wm.obj_export(
        filepath=str(path), check_existing=False, export_selected_objects=True,
        export_uv=True, export_normals=True, export_materials=True,
        export_triangulated_mesh=True, apply_modifiers=True,
        forward_axis="Y", up_axis="Z", global_scale=1.0,
    )
    slots = [slot.material.name for slot in joined.material_slots if slot.material]
    print(f"Generated {filename}: {len(joined.data.vertices):,} vertices, "
          f"{len(joined.data.polygons):,} triangles, slots={slots}")
    for level, ratio in ((1,.55),(2,.24)):
        bpy.ops.object.select_all(action="DESELECT")
        lod=joined.copy();lod.data=joined.data.copy()
        bpy.context.collection.objects.link(lod)
        lod.select_set(True);bpy.context.view_layer.objects.active=lod
        bm=bmesh.new();bm.from_mesh(lod.data)
        remove=[f for f in bm.faces if lod.data.materials[f.material_index].name.startswith("decal_")]
        bmesh.ops.delete(bm,geom=remove,context="FACES")
        bm.to_mesh(lod.data);bm.free()
        dec=lod.modifiers.new("Distance simplification","DECIMATE");dec.ratio=ratio
        bpy.ops.object.modifier_apply(modifier=dec.name)
        lod_dir=OUTPUT/"lods";lod_dir.mkdir(exist_ok=True)
        bpy.ops.wm.obj_export(filepath=str(lod_dir/f"{filename}_LOD{level}.obj"),
            export_selected_objects=True,export_uv=True,export_normals=True,
            export_materials=True,export_triangulated_mesh=True,apply_modifiers=True,
            forward_axis="Y",up_axis="Z",global_scale=1.0)
        bpy.data.objects.remove(lod,do_unlink=True)
    return path


def travel_hub():
    clear_scene()
    common_base("Travel", length=600, width=540)
    # Walk-through ring: the empty centre and both landing pads remain clear.
    for x in (-22,22):
        torus(f"Travel_ring_{x}",(x,0,220),172,18,"frame",axis="x",major_segments=48)
        torus(f"Travel_trim_{x}",(x+3,0,220),174,4,"shell",axis="x",major_segments=48)
        torus(f"Travel_lamp_{x}",(x+5,0,220),151,2.5,"glow",axis="x",major_segments=48)
    for side in (-1,1):
        y=side*198
        box(f"Travel_pillar_{side}",(0,y,213),(82,42,364),"steel",4)
        box(f"Travel_armour_{side}",(44,y,215),(6,32,260),"paint_primary",2)
        for z in (80,180,280,370):
            cylinder(f"Travel_bolt_{side}_{z}",(49,y,z),4,3,"shell",axis="x",vertices=6)
        for x in (-120,120):
            box(f"Travel_floor_marker_{side}_{x}",(x,side*80,27),(80,5,2),"paint_primary",.4)
    box("Travel_crown",(0,0,416),(100,450,28),"frame",4)
    box("Travel_crown_trim",(54,0,416),(8,360,14),"paint_primary",2)
    box("Travel_console_stand",(170,-205,90),(90,86,130),"frame",4)
    box("Travel_console_armour",(170,-250,90),(72,5,86),"paint_primary",2)
    screen_panel("Travel_console",(170,-252,90),70,55,tile=4)
    for x in (131,209):
        box(f"Travel_label_bracket_{x}",(x,-251,170),(4,12,42),"steel",.6)
    identification_plate("Travel_label",(170,-256,185),100,"PERSONNEL TELEPORTER")
    box("Travel_power_unit",(-180,-220,115),(85,74,180),"frame",4)
    cylinder("Travel_mast",(-180,-220,248),7,110,"steel",vertices=16)
    cylinder("Travel_power_cap",(-180,-220,310),14,16,"shell",vertices=16)
    for z in (280,292,302):torus(f"Travel_insulator_{z}",(-180,-220,z),10,3,"rubber")
    sign_mount("Travel",-56,0,416,160,24,anchor_x=-48)
    arrow("Travel_exit_arrow",(190,0,23.6),True,"shell")
    return finish("SM_TeleporterTravelHub")


BUILDERS = {
    "SM_TeleporterItemInput": lambda: item_terminal(False),
    "SM_TeleporterItemOutput": lambda: item_terminal(True),
    "SM_TeleporterFluidInput": lambda: fluid_terminal(False),
    "SM_TeleporterFluidOutput": lambda: fluid_terminal(True),
    "SM_TeleporterHub": hub,
    "SM_TeleporterTravelHub": travel_hub,
}
only = os.environ.get("TELEPORTLOGISTICS_MODEL_ONLY")
if only and only not in BUILDERS:
    raise RuntimeError(f"Unknown TELEPORTLOGISTICS_MODEL_ONLY model: {only}")
for name, builder in BUILDERS.items():
    if not only or name == only:
        builder()
print("TeleportLogistics stock-scale model generation completed successfully.")
