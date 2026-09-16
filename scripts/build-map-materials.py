"""Create persistent native map/compass materials for TeleportLogistics's five markers.

The sidebar uses GetActorRepresentationTexture, but the map's shared
Widget_MapCompass_Icon uses a material. A null compass material leaves its
image brush white even when the sidebar texture works.
"""
import unreal

unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/TeleportLogistics"], True)
parent_path = "/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MI_CompassIcon_Static"
parent = unreal.EditorAssetLibrary.load_asset(parent_path)
if not isinstance(parent, unreal.MaterialInstanceConstant):
    raise RuntimeError("Missing native static map icon material")
tools = unreal.AssetToolsHelpers.get_asset_tools()
for name in ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub"):
    asset_name = f"MI_TeleporterMap{name}"
    path = f"/TeleportLogistics/Icons/{asset_name}"
    material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if material is None:
        material = tools.create_asset(asset_name, "/TeleportLogistics/Icons", unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
    if not isinstance(material, unreal.MaterialInstanceConstant):
        raise RuntimeError("Could not create " + path)
    texture = unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/Icons/M_Teleporter{name}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Missing map texture " + name)
    unreal.MaterialEditingLibrary.set_material_instance_parent(material, parent)
    # The SDK strips the master shader graph, so its parameter-name lookup is
    # empty. Copy the actual native portal override record (Icon) instead.
    template = unreal.load_asset("/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MI_CompassIcon_Portal")
    values = template.get_editor_property("texture_parameter_values")
    if len(values) != 1 or str(values[0].parameter_info.name) != "Icon":
        raise RuntimeError("Native portal Icon parameter contract changed")
    override = values[0]
    override.set_editor_property("parameter_value", texture)
    values[0] = override  # Unreal array indexing returns a struct copy.
    material.set_editor_property("texture_parameter_values", values)
    material.set_editor_property("scalar_parameter_values", template.get_editor_property("scalar_parameter_values"))
    unreal.MaterialEditingLibrary.update_material_instance(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log(f"TeleportLogistics: map material {path} uses native static parent and {texture.get_path_name()}")
