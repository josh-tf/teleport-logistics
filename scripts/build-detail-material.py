"""Opaque printed plates: avoid the DX12 masked depth pipeline crash in 0.4.7.

Alpha blends the artwork onto a dark plate in base colour, rather than clipping
geometry. This deliberately trades transparent margins for a stable opaque pass.
"""
import unreal

unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/TeleportLogistics"], True)
path = "/TeleportLogistics/Models/M_TeleporterDetails"
material = (unreal.EditorAssetLibrary.load_asset(path)
            if unreal.EditorAssetLibrary.does_asset_exist(path) else None)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_TeleporterDetails", "/TeleportLogistics/Models", unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
details = unreal.EditorAssetLibrary.load_asset("/TeleportLogistics/Textures/T_TeleporterDetails")
assert details is not None
details.set_editor_property("srgb", True)
details.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
details.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
unreal.EditorAssetLibrary.save_loaded_asset(details)

edit = unreal.MaterialEditingLibrary
sample = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample)
sample.set_editor_property("texture", details)
blend = edit.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate)
blend.set_editor_property("const_a", .012)
edit.connect_material_expressions(sample, "RGB", blend, "B")
edit.connect_material_expressions(sample, "A", blend, "Alpha")
edit.connect_material_property(blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
rough.set_editor_property("r", .65)
edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
assert material.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_OPAQUE
unreal.log("TeleportLogistics: detail artwork uses opaque printed plates; no opacity-mask input")
