"""A guaranteed non-emissive hub lamp material, independent of factory atlas CPD."""
import unreal
p='/TeleportLogistics/Models/M_TeleporterSignalOff'
m=unreal.EditorAssetLibrary.load_asset(p) if unreal.EditorAssetLibrary.does_asset_exist(p) else None
if m is None:
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_TeleporterSignalOff','/TeleportLogistics/Models',unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
def scalar(value, prop):
    n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant)
    n.set_editor_property('r',value)
    unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
scalar(0.012,unreal.MaterialProperty.MP_BASE_COLOR)
scalar(0.0,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
scalar(0.25,unreal.MaterialProperty.MP_METALLIC)
scalar(0.38,unreal.MaterialProperty.MP_ROUGHNESS)
unreal.MaterialEditingLibrary.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('TeleportLogistics: generated explicitly zero-emission hub lamp material')
