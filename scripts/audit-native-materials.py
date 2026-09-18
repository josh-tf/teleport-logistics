"""Read-only material and mesh audit; run with UnrealEditor-Cmd -run=pythonscript.

Everything is reported through log_warning: the commandlet filters LogPython
Display lines out of the log, so Display output is silently lost.
"""
import unreal


def say(message):
    unreal.log_warning('TELEPORTLOGISTICS_AUDIT ' + message)


def scalar_names(asset):
    """Every scalar the material exposes, not just the overridden ones."""
    library = unreal.MaterialEditingLibrary
    for getter in ('get_scalar_parameter_names', 'get_material_instance_scalar_parameter_names'):
        if hasattr(library, getter):
            try:
                return list(getattr(library, getter)(asset))
            except Exception as error:
                say('enumeration via %s failed: %s' % (getter, error))
    return []


for path in (
    '/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01',
    '/Game/FactoryGame/Buildable/-Shared/Material/MI_Factory2D_01',
    '/Game/FactoryGame/Buildable/-Shared/Material/InputFog',
    '/Game/FactoryGame/Buildable/-Shared/Material/InputFogPlane',
):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        say('missing ' + path)
        continue
    say('%s class=%s' % (path, asset.get_class().get_name()))

    if isinstance(asset, unreal.MaterialInstanceConstant):
        say('  parent=' + str(asset.get_editor_property('parent')))
        for value in asset.get_editor_property('scalar_parameter_values'):
            info = value.get_editor_property('parameter_info')
            say('  override scalar %s=%s' % (info.get_editor_property('name'),
                                             value.get_editor_property('parameter_value')))
    if isinstance(asset, (unreal.MaterialInstanceConstant, unreal.Material)):
        for name in scalar_names(asset):
            say('  exposed scalar ' + str(name))

    # The connector fade sizes itself from this mesh at runtime, and the SDK ships
    # the asset without render data, so record what the loaded mesh actually reports.
    if isinstance(asset, unreal.StaticMesh):
        try:
            bounds = asset.get_bounds()
            say('  bounds origin=%s extent=%s' % (bounds.origin, bounds.box_extent))
        except Exception as error:
            say('  bounds unavailable: %s' % error)
        try:
            say('  materials=%s' % [str(m.material_interface)
                                    for m in asset.get_editor_property('static_materials')])
        except Exception as error:
            say('  materials unavailable: %s' % error)
