"""Refresh only UI textures; leave validated meshes and materials untouched."""
from pathlib import Path
import unreal
root=Path(__file__).resolve().parents[1]
for source in sorted((root/'assets/icons').glob('T_Teleporter*.png')):
    task=unreal.AssetImportTask()
    task.filename=str(source);task.destination_path='/TeleportLogistics/Icons';task.destination_name=source.stem
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset=unreal.EditorAssetLibrary.load_asset('/TeleportLogistics/Icons/'+source.stem)
    if not isinstance(asset,unreal.Texture2D):raise RuntimeError('Texture import failed: '+source.stem)
    asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log('TeleportLogistics UI symbol textures imported')
