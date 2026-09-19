#!/usr/bin/env bash
set -euo pipefail
# Prevent multi-gigabyte editor/Wine core dumps during unattended builds.
ulimit -c 0
if (( $# < 2 || $# > 3 )); then
    echo "Usage: $0 PROJECT_ROOT ENGINE_ROOT [Editor|Windows|WindowsServer|WindowsAll|LinuxServer|All]" >&2
    exit 2
fi
teleport_logistics_root=$(cd "$(dirname "$0")/.." && pwd)
project_root=$(realpath "$1")
engine_root=$(realpath "$2")
target=${3:-LinuxServer}
case "$target" in Editor|Windows|WindowsServer|WindowsAll|LinuxServer|All) ;; *) echo "Unknown target: $target" >&2; exit 2;; esac
parallel_args=()
if [[ -n "${TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS:-}" ]]; then
    [[ "$TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS" =~ ^[1-9][0-9]*$ ]] || {
        echo "TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS must be a positive integer." >&2
        exit 2
    }
    parallel_args+=("-MaxParallelActions=$TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS")
fi
project="$project_root/FactoryGame.uproject"
build="$engine_root/Engine/Build/BatchFiles/Linux/Build.sh"
uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor="$engine_root/Engine/Binaries/Linux/UnrealEditor-Cmd"
for required in "$project" "$build" "$uat" "$editor" "$project_root/Mods/SML/SML.uplugin" "$project_root/Source/FactoryEditor.Target.cs"; do
    [[ -f "$required" ]] || { echo "Missing: $required" >&2; exit 1; }
done
python3 - "$project" <<'PY'
import json, sys
with open(sys.argv[1]) as f:
    if json.load(f).get('EngineAssociation') != '5.6.1-CSS':
        sys.exit('Expected CSS Unreal 5.6.1; see sdk-lock.json.')
PY
if [[ "$target" == Windows* || "$target" == All ]]; then
    [[ -n "${UE_WINE_MSVC:-}" && -d "$UE_WINE_MSVC" ]] || {
        echo 'Windows targets require UE_WINE_MSVC pointing to the configured MSVC-Wine toolchain.' >&2
        exit 1
    }
fi
destination="$project_root/Mods/TeleportLogistics"
[[ "$(realpath -m "$destination")" != "$(realpath "$teleport_logistics_root/TeleportLogistics")" ]] || { echo "Project staging must not be the source plugin." >&2; exit 1; }
# This directory is a generated staging copy. Clear it so removed or renamed
# content cannot leak into a later cook.
rm -rf "$destination"
mkdir -p "$destination"
cp "$teleport_logistics_root/TeleportLogistics/TeleportLogistics.uplugin" "$destination/"
for folder in Source Config Resources; do
    mkdir -p "$destination/$folder"
    cp -a "$teleport_logistics_root/TeleportLogistics/$folder/." "$destination/$folder/"
done
# TeleportLogistics's cooked content is entirely generated from the checked-in OBJ and PNG
# sources. Start each import with an empty package directory so Unreal never has
# to force-delete referenced StaticMesh packages (a crash-prone UE 5.6 path),
# and so stale materials or LODs cannot leak into a release.
mkdir -p "$destination/Content"
bash "$build" FactoryEditor Linux Development "-Project=$project" -WaitMutex "${parallel_args[@]}"
"$editor" "$project" -run=pythonscript "-script=$teleport_logistics_root/scripts/import-assets.py" \
    -unattended -nosplash -nullrhi -DDC-ForceMemoryCache
"$editor" "$project" -run=pythonscript "-script=$teleport_logistics_root/scripts/inspect-content.py" \
    -unattended -nosplash -nullrhi -DDC-ForceMemoryCache
for model in ItemInput ItemOutput FluidInput FluidOutput Hub; do
    [[ -f "$destination/Content/Models/SM_Teleporter$model.uasset" ]] || { echo "Missing imported mesh: $model" >&2; exit 1; }
    [[ -f "$destination/Content/Icons/T_Teleporter${model}_256.uasset" ]] || { echo "Missing imported small icon: $model" >&2; exit 1; }
    [[ -f "$destination/Content/Icons/T_Teleporter${model}_512.uasset" ]] || { echo "Missing imported big icon: $model" >&2; exit 1; }
    [[ -f "$destination/Content/Icons/M_Teleporter$model.uasset" ]] || { echo "Missing imported map marker: $model" >&2; exit 1; }
done
[[ -f "$destination/Content/Icons/T_TeleporterCategory_128.uasset" ]] || { echo "Missing category icon" >&2; exit 1; }
[[ -f "$destination/Content/Icons/T_TeleporterMilestone_256.uasset" ]] || { echo "Missing small milestone icon" >&2; exit 1; }
[[ -f "$destination/Content/Icons/T_TeleporterMilestone_512.uasset" ]] || { echo "Missing big milestone icon" >&2; exit 1; }
# The Personnel Teleporter is not in the loop above: it deliberately has no map marker
# of its own name, so the loop's M_Teleporter<model> check would demand an asset that by
# design does not exist.
[[ -f "$destination/Content/Models/SM_TeleporterTravelHub.uasset" ]] || { echo "Missing imported personnel mesh" >&2; exit 1; }
for icon in T_TeleporterTravelHub_256 T_TeleporterTravelHub_512 \
            T_TeleporterPersonnelMilestone_256 T_TeleporterPersonnelMilestone_512; do
    [[ -f "$destination/Content/Icons/$icon.uasset" ]] || { echo "Missing personnel icon: $icon" >&2; exit 1; }
done
[[ -f "$destination/Content/Models/M_TeleporterScreen.uasset" ]] || { echo "Missing generated screen material" >&2; exit 1; }
[[ -f "$destination/Content/Textures/T_TeleporterScreen_Grid.uasset" ]] || { echo "Missing generated screen texture" >&2; exit 1; }
# Retain the successfully imported packages in the source tree used by source
# archives and future clean staging builds.
rm -rf "$teleport_logistics_root/TeleportLogistics/Content"
mkdir -p "$teleport_logistics_root/TeleportLogistics/Content"
cp -a "$destination/Content/." "$teleport_logistics_root/TeleportLogistics/Content/"
[[ "$target" != Editor ]] || exit 0
args=("-ScriptsForProject=$project" PackagePlugin "-project=$project" -DLCName=TeleportLogistics
    -clientconfig=Shipping -serverconfig=Shipping -utf8output -build -nocompileeditor)
if [[ -n "${TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS:-}" ]]; then
    args+=("-ubtargs=-MaxParallelActions=$TELEPORTLOGISTICS_MAX_PARALLEL_ACTIONS")
fi
case "$target" in
    Windows) args+=(-platform=Win64);;
    WindowsServer) args+=(-target=FactoryServer -server -serverplatform=Win64 -noclient);;
    WindowsAll) args+=(-platform=Win64 -server -serverplatform=Win64 -merge);;
    LinuxServer) args+=(-target=FactoryServer -server -serverplatform=Linux -noclient);;
    All) args+=(-platform=Win64 -server -serverplatform=Win64+Linux -merge);;
esac
bash "$uat" "${args[@]}"
echo "Packages: $project_root/Saved/ArchivedPlugins/TeleportLogistics"
