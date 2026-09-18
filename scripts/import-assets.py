"""Run inside CSS Unreal Editor to build TeleportLogistics's cookable content.

The models deliberately use Satisfactory's native factory atlas material for
stock shading and Customizer support. The screen and zero-emission lamp materials are authored by
TeleportLogistics. Meshes have three authored LODs; distant levels omit decals and
use reduced geometry while preserving the source model and connector transforms.
"""

from pathlib import Path
import unreal
import json


root = Path(__file__).resolve().parents[1] / "assets"
tools = unreal.AssetToolsHelpers.get_asset_tools()
models = ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub", "TravelHub")


def path_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError("Required native asset is missing: " + path)
    return asset


def import_task(source, destination):
    result = unreal.AssetImportTask()
    result.filename = str(source)
    result.destination_path = destination
    result.destination_name = source.stem
    result.automated = True
    result.replace_existing = True
    result.save = True
    return result


tasks = []
for source in sorted((root / "models").glob("*.obj")):
    item = import_task(source, "/TeleportLogistics/Models")
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = False
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = True
    options.static_mesh_import_data.import_mesh_lods = False
    item.options = options
    item.factory = unreal.FbxFactory()
    tasks.append(item)
for source in sorted((root / "icons").glob("*.png")):
    tasks.append(import_task(source, "/TeleportLogistics/Icons"))
tasks.append(import_task(root / "textures" / "T_TeleporterScreen_Grid.png", "/TeleportLogistics/Textures"))
tasks.append(import_task(root / "textures" / "T_TeleporterDetails.png", "/TeleportLogistics/Textures"))

expected_icons = 22
if len(tasks) != 6 + expected_icons + 2:
    raise RuntimeError(
        f"Expected six meshes, {expected_icons} UI/map icons, and two textures; "
        "run generate-assets.py first."
    )

unreal.log("TeleportLogistics: importing six clean meshes, twenty-two icons, and two textures.")
for source in sorted((root / "ui" / "generated").glob("*.png")):
    tasks.append(import_task(source, "/TeleportLogistics/UI"))
if len(tasks) != 33:
    raise RuntimeError("Expected three SFUIKIT panels; run prepare-ui-assets.py")
tools.import_asset_tasks(tasks)
for item in tasks:
    expected = item.destination_path + "/" + item.destination_name
    if not unreal.EditorAssetLibrary.does_asset_exist(expected):
        raise RuntimeError("Asset import failed: " + expected)
    asset = unreal.EditorAssetLibrary.load_asset(expected)
    if isinstance(asset, unreal.Texture2D):
        if item.destination_path in ("/TeleportLogistics/Icons", "/TeleportLogistics/UI"):
            asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            asset.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)


# Both map and sidebar need persistent cooked resources.
import runpy
runpy.run_path(str(Path(__file__).with_name("build-map-materials.py")))
runpy.run_path(str(Path(__file__).with_name("build-power-material.py")))

# Shared printed plates avoid the masked depth/shadow permutation that failed on DX12.
runpy.run_path(str(Path(__file__).with_name("build-detail-material.py")))
detail_material = path_asset("/TeleportLogistics/Models/M_TeleporterDetails")

screen_grid = path_asset("/TeleportLogistics/Textures/T_TeleporterScreen_Grid")


def expression(material, cls, x, y, **properties):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, cls, x, y)
    for name, value in properties.items():
        node.set_editor_property(name, value)
    return node


# Tiling of the shared surface detail. Stock MI_Factory2D_01 uses 20; sitting on
# the stock value keeps our panels tiling exactly like every other building, which
# matters most on the large flat bodies. This is the knob for wear size.
FACTORY_DETAIL_SCALE = 20.0


def build_screen_material():
    path = "/TeleportLogistics/Models/M_TeleporterScreen"
    material = None
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        material = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(material, unreal.Material):
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:
        material = tools.create_asset("M_TeleporterScreen", "/TeleportLogistics/Models", unreal.Material,
                                      unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create " + path)

    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -820, 20,
                    coordinate_index=0, u_tiling=1.0, v_tiling=1.0)
    grid = expression(material, unreal.MaterialExpressionTextureSample, -610, 20,
                      texture=screen_grid)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", grid, "Coordinates")

    power = expression(material, unreal.MaterialExpressionScalarParameter, -370, 205,
                       parameter_name="TeleporterPower", default_value=1.0,
                       slider_min=0.0, slider_max=1.0)
    # An unpowered display should be dark glass, not a blue grid that still
    # appears switched on in daylight. Power gates base colour and emission.
    display_colour = expression(material, unreal.MaterialExpressionMultiply, -140, -100)
    unreal.MaterialEditingLibrary.connect_material_expressions(grid, "RGB", display_colour, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(power, "", display_colour, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        display_colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    strength = expression(material, unreal.MaterialExpressionConstant, -370, 300, r=1.1)
    powered_strength = expression(material, unreal.MaterialExpressionMultiply, -140, 240)
    emission = expression(material, unreal.MaterialExpressionMultiply, 80, 100)
    unreal.MaterialEditingLibrary.connect_material_expressions(power, "", powered_strength, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", powered_strength, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(grid, "RGB", emission, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(powered_strength, "", emission, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    metallic = expression(material, unreal.MaterialExpressionConstant, -100, 430, r=0.0)
    roughness = expression(material, unreal.MaterialExpressionConstant, -100, 500, r=0.33)
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    specular = expression(material, unreal.MaterialExpressionConstant, -100, 570, r=0.20)
    unreal.MaterialEditingLibrary.connect_material_property(
        specular, "", unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def build_factory_material():
    """Our own instance of the stock factory material, so surface tiling can differ.

    MI_Factory_Base_01 is the game's asset and is shared by every base-game
    building, so overriding a parameter on it would change all of them. A child
    instance keeps the change to this mod.

    `Scale` is the tiling of the shared surface detail on MM_Factory_Array, which
    MI_Factory2D_01 sets to 20; both were read off the loaded assets by
    scripts/audit-native-materials.py. A lower number tiles the detail less often,
    so the wear reads slightly larger.
    """
    parent = path_asset("/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01.MI_Factory_Base_01")
    path = "/TeleportLogistics/Models/MI_TeleporterFactory"
    instance = (unreal.EditorAssetLibrary.load_asset(path)
                if unreal.EditorAssetLibrary.does_asset_exist(path) else None)
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        instance = tools.create_asset("MI_TeleporterFactory", "/TeleportLogistics/Models",
                                      unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        raise RuntimeError("Could not create " + path)
    instance.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, "Scale", FACTORY_DETAIL_SCALE)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


screen_material = build_screen_material()
factory_material = build_factory_material()
decal_color_material = path_asset(
    "/Game/FactoryGame/Buildable/-Shared/Material/DecalColor_Masked.DecalColor_Masked")
decal_normal_material = path_asset(
    "/Game/FactoryGame/Buildable/-Shared/Material/Decal_Normal.Decal_Normal")
materials = {
    "factory": factory_material,
    "signal": factory_material,
    "screen": screen_material,
    "decal_color": decal_color_material,
    "decal_custom": detail_material,
    "decal_normal": decal_normal_material,
}

for model_name in models:
    path = "/TeleportLogistics/Models/SM_Teleporter" + model_name
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Missing TeleportLogistics static mesh: " + path)
    static_materials = mesh.get_editor_property("static_materials")
    for material_index, static_material in enumerate(static_materials):
        slot_name = str(static_material.get_editor_property("material_slot_name"))
        if slot_name not in materials:
            raise RuntimeError(f"Unexpected material slot {slot_name} on {path}")
        static_material.set_editor_property("material_interface", materials[slot_name])
        mesh.set_material(material_index, materials[slot_name])
    mesh.set_editor_property("static_materials", static_materials)
    mesh.set_editor_property("lod_group", "None")
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
    editor.remove_collisions(mesh)
    body = mesh.get_editor_property("body_setup")
    aggregate = unreal.KAggregateGeom()
    boxes = []
    for spec in json.loads((root / "models" / "collision.json").read_text())[model_name]:
        box = unreal.KBoxElem()
        box.set_editor_property("center", unreal.Vector(*spec["center"]))
        for axis, size in zip(("x", "y", "z"), spec["size"]):
            box.set_editor_property(axis, size)
        boxes.append(box)
    aggregate.set_editor_property("box_elems", boxes)
    body.set_editor_property("agg_geom", aggregate)

    for level in (1, 2):
        source = str(root / "models" / "lods" / f"SM_Teleporter{model_name}_LOD{level}.obj")
        if editor.import_lod(mesh, level, source) != level:
            raise RuntimeError("Failed importing authored LOD: " + source)
    if mesh.get_num_lods() != 3:
        raise RuntimeError("Expected three authored LODs on " + path)
    if not editor.set_lod_screen_sizes(mesh, [1.0, .10, .035]):
        raise RuntimeError("Failed setting LOD screen sizes")
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)

unreal.EditorAssetLibrary.save_directory("/TeleportLogistics", only_if_is_dirty=False, recursive=True)
unreal.log(
    "TeleportLogistics: six stock-scale three-LOD meshes, native factory/decal materials, "
    "one power-aware screen material, and twenty-two native-style icons imported."
)
