"""Render transparent 512 px building icons from TeleportLogistics's final OBJ models.

Satisfactory building descriptors use model captures rather than flat symbols.
This script mirrors that pipeline with an orthographic three-point studio setup.
The 256 px variants are produced with Lanczos resampling by generate-assets.py.
"""

from pathlib import Path
import math
import os

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "assets" / "models"
ICONS = ROOT / "assets" / "icons"
ATLAS = ROOT / "assets" / "vendor" / "Satisfactory_ModelingTools" / "Factory_Base_Plain.png"
SCREEN = ROOT / "assets" / "textures" / "T_TeleporterScreen_Grid.png"
NAMES = ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub", "TravelHub")
# Graphite, baked into every shipped icon.
SECONDARY_PAINT = (0.045, 0.052, 0.06, 1.0)
DEBUG_KNOBS = ("RENDER_CLAY", "RENDER_TARGET", "RENDER_SCALE", "RENDER_SIZE")
ICONS.mkdir(parents=True, exist_ok=True)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)
    for datablock in list(bpy.data.materials):
        bpy.data.materials.remove(datablock)


def look_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def configure_material(value):
    name = value.name.split(".")[0]
    value.use_nodes = True
    nodes = value.node_tree.nodes
    links = value.node_tree.links
    shader = nodes.get("Principled BSDF")
    if not shader:
        return
    for node in list(nodes):
        if node not in {shader, nodes.get("Material Output")}:
            nodes.remove(node)
    if name in {"factory", "signal", "screen", "decal_custom"}:
        uv = nodes.new("ShaderNodeTexCoord")
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = bpy.data.images.load(str(ROOT / "assets/textures/T_TeleporterDetails.png" if name == "decal_custom" else SCREEN if name == "screen" else ATLAS),
                                             check_existing=True)
        texture.interpolation = "Linear"
        links.new(uv.outputs["UV"], texture.inputs["Vector"])
        colour = texture.outputs["Color"]
        if name == "decal_custom":
            links.new(texture.outputs["Alpha"], shader.inputs["Alpha"])
        if name == "factory":
            # The vendor sheet uses violet to label the secondary paint cell.
            # Preview a normal graphite Customizer swatch instead of that key colour.
            xyz = nodes.new("ShaderNodeSeparateXYZ")
            links.new(uv.outputs["UV"], xyz.inputs[0])
            def compare(output, operation, threshold):
                node = nodes.new("ShaderNodeMath")
                node.operation = operation
                links.new(output, node.inputs[0])
                node.inputs[1].default_value = threshold
                return node.outputs[0]
            x_mask = compare(xyz.outputs["X"], "LESS_THAN", 1/3)
            y_low = compare(xyz.outputs["Y"], "GREATER_THAN", 1/3)
            y_high = compare(xyz.outputs["Y"], "LESS_THAN", 2/3)
            mask = nodes.new("ShaderNodeMath")
            mask.operation = "MULTIPLY"
            links.new(x_mask, mask.inputs[0]); links.new(y_low, mask.inputs[1])
            mask2 = nodes.new("ShaderNodeMath")
            mask2.operation = "MULTIPLY"
            links.new(mask.outputs[0], mask2.inputs[0]); links.new(y_high, mask2.inputs[1])
            paint = nodes.new("ShaderNodeMixRGB")
            links.new(mask2.outputs[0], paint.inputs[0])
            links.new(colour, paint.inputs[1])
            paint.inputs[2].default_value = SECONDARY_PAINT
            colour = paint.outputs[0]
            if os.environ.get("TELEPORTLOGISTICS_PRIMARY_PAINT"):
                primary_y = compare(xyz.outputs["Y"], "GREATER_THAN", 2/3)
                primary_mask = nodes.new("ShaderNodeMath")
                primary_mask.operation = "MULTIPLY"
                links.new(x_mask, primary_mask.inputs[0])
                links.new(primary_y, primary_mask.inputs[1])
                primary = nodes.new("ShaderNodeMixRGB")
                links.new(primary_mask.outputs[0], primary.inputs[0])
                links.new(colour, primary.inputs[1])
                primary.inputs[2].default_value = tuple(float(v) for v in os.environ["TELEPORTLOGISTICS_PRIMARY_PAINT"].split(",")) + (1,)
                colour = primary.outputs[0]
        links.new(colour, shader.inputs["Base Color"])
        shader.inputs["Metallic"].default_value = 0.34 if name != "screen" else 0.0
        shader.inputs["Roughness"].default_value = 0.40 if name != "screen" else 0.33
        if name == "screen":
            shader.inputs["Specular IOR Level"].default_value = 0.20
        if name in {"signal", "screen"}:
            links.new(texture.outputs["Color"], shader.inputs["Emission Color"])
            shader.inputs["Emission Strength"].default_value = 1.25 if name == "signal" else 0.38
    else:
        shader.inputs["Base Color"].default_value = (0.085, 0.09, 0.095, 1.0)
        shader.inputs["Metallic"].default_value = 0.35
        shader.inputs["Roughness"].default_value = 0.44

    if os.environ.get("TELEPORTLOGISTICS_RENDER_CLAY") and name != "decal_custom":
        for input_name in ("Base Color", "Emission Color"):
            for link in list(shader.inputs[input_name].links): links.remove(link)
        shader.inputs["Base Color"].default_value=(.46,.49,.51,1)
        shader.inputs["Emission Strength"].default_value=0
        shader.inputs["Metallic"].default_value=0
        shader.inputs["Roughness"].default_value=.7


def add_area(name, location, energy, size, colour, target):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = colour
    light = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(light)
    light.location = location
    look_at(light, target)


def bounds(objects):
    points = [obj.matrix_world @ Vector(corner) for obj in objects for corner in obj.bound_box]
    low = Vector(tuple(min(point[i] for point in points) for i in range(3)))
    high = Vector(tuple(max(point[i] for point in points) for i in range(3)))
    return low, high


def render(name):
    clear_scene()
    before = set(bpy.context.scene.objects)
    bpy.ops.wm.obj_import(filepath=str(MODELS / f"SM_Teleporter{name}.obj"),
                          forward_axis="Y", up_axis="Z")
    imported = [obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "MESH"]
    for obj in imported:
        obj.scale = (0.01, 0.01, 0.01)
        for slot in obj.material_slots:
            if slot.material:
                configure_material(slot.material)
    bpy.context.view_layer.update()

    low, high = bounds(imported)
    centre = (low + high) * 0.5
    extent = high - low
    span = max(extent.x, extent.y, extent.z)

    camera_data = bpy.data.cameras.new("Camera")
    camera = bpy.data.objects.new("Camera", camera_data)
    bpy.context.collection.objects.link(camera)
    bpy.context.scene.camera = camera
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = span * 1.16
    target = centre + Vector((0, 0, extent.z * 0.015))
    if name == "TravelHub":
        target = Vector((-0.1, 0, 2.1))
        camera_data.ortho_scale = 8.4
    if os.environ.get("TELEPORTLOGISTICS_RENDER_TARGET"):
        # Metres in model space, for reviewing small mounted details at full resolution.
        target = Vector(tuple(float(v) for v in os.environ["TELEPORTLOGISTICS_RENDER_TARGET"].split(",")))
        camera_data.ortho_scale = float(os.environ.get("TELEPORTLOGISTICS_RENDER_SCALE", "1.2"))
    direction = Vector(tuple(float(value) for value in os.environ.get(
        "TELEPORTLOGISTICS_RENDER_DIRECTION", "1.55,-1.75,1.22").split(","))).normalized()
    camera.location = target + direction * span * 2.8
    look_at(camera, target)

    add_area("Key", centre + Vector((-span, -span * 1.2, span * 1.9)),
             1450, span * 1.2, (0.86, 0.94, 1.0), centre)
    add_area("Fill", centre + Vector((span * 1.4, span * 0.2, span)),
             950, span, (1.0, 0.88, 0.73), centre)
    add_area("Rim", centre + Vector((-span * 0.8, span * 1.3, span * 1.4)),
             1250, span * 0.85, (0.78, 0.88, 1.0), centre)

    world = bpy.context.scene.world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.025, 0.03, 0.035, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.50

    scene = bpy.context.scene
    if os.environ.get("TELEPORTLOGISTICS_ICON_CPU"):
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = int(os.environ.get("TELEPORTLOGISTICS_RENDER_SAMPLES", "128"))
        scene.cycles.use_denoising = False
    else:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = int(os.environ.get("TELEPORTLOGISTICS_RENDER_SIZE", "512"))
    scene.render.resolution_y = scene.render.resolution_x
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = True
    # A clay or zoomed render is not a descriptor icon, so it must not land on the
    # shipped one, which passes validate.py whenever it is 512 px RGBA.
    debug = any(os.environ.get("TELEPORTLOGISTICS_" + knob) for knob in DEBUG_KNOBS)
    default = ROOT / "reports" / "icon-debug" if debug else ICONS
    destination = Path(os.environ.get("TELEPORTLOGISTICS_RENDER_OUTPUT", str(default)))
    destination.mkdir(parents=True, exist_ok=True)
    scene.render.filepath = str(destination / f"T_Teleporter{name}_512.png")
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.30
    bpy.ops.render.render(write_still=True)
    print(f"Rendered descriptor capture for TeleportLogistics{name}")


only = os.environ.get("TELEPORTLOGISTICS_ICON_ONLY")
if only and only not in NAMES:
    raise RuntimeError(f"Unknown TELEPORTLOGISTICS_ICON_ONLY model: {only}")
for model_name in NAMES:
    if not only or model_name == only:
        render(model_name)
print("TeleportLogistics descriptor model captures generated successfully.")
