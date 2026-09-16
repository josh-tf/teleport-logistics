"""Read-only material audit; run with UnrealEditor-Cmd -run=pythonscript."""
import unreal
for path in (
    '/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01',
    '/Game/FactoryGame/Buildable/-Shared/Material/MI_Factory2D_01',
    '/Game/FactoryGame/Buildable/-Shared/Material/InputFog',
    '/Game/FactoryGame/Buildable/-Shared/Material/InputFogPlane',
):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        unreal.log_warning('TELEPORTLOGISTICS_AUDIT missing ' + path)
        continue
    unreal.log('TELEPORTLOGISTICS_AUDIT ' + path + ' class=' + asset.get_class().get_name())
    if isinstance(asset, unreal.MaterialInstanceConstant):
        unreal.log('TELEPORTLOGISTICS_AUDIT parent=' + str(asset.get_editor_property('parent')))
        for value in asset.get_editor_property('scalar_parameter_values'):
            unreal.log('TELEPORTLOGISTICS_AUDIT scalar ' + str(value.get_editor_property('parameter_info').get_editor_property('name')) + '=' + str(value.get_editor_property('parameter_value')))
