#!/usr/bin/env bash
set -uo pipefail

failures=0
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        printf 'OK   command: %s\n' "$1"
    else
        printf 'MISS command: %s\n' "$1"
        failures=$((failures + 1))
    fi
}
check_file() {
    if [[ -f "$1" ]]; then
        printf 'OK   file: %s\n' "$1"
    else
        printf 'MISS file: %s\n' "$1"
        failures=$((failures + 1))
    fi
}

for command_name in git git-lfs msiextract wine tar zstd dos2unix python3; do
    check_command "$command_name"
done

if command -v gh >/dev/null 2>&1; then
    if gh api repos/EpicGames/UnrealEngine --silent >/dev/null 2>&1; then
        printf 'OK   GitHub: Epic Unreal Engine access\n'
    else
        printf 'MISS GitHub: Epic Unreal Engine access for the authenticated gh account\n'
        failures=$((failures + 1))
    fi
else
    printf 'INFO GitHub: gh is absent; engine access was not checked\n'
fi

if (( $# >= 1 )); then
    project_root=${1%/}
    check_file "$project_root/FactoryGame.uproject"
    check_file "$project_root/Mods/SML/SML.uplugin"
    check_file "$project_root/Source/FactoryEditor.Target.cs"
    check_file "$project_root/Plugins/Wwise/Wwise.uplugin"
fi
if (( $# >= 2 )); then
    engine_root=${2%/}
    check_file "$engine_root/Engine/Build/BatchFiles/Linux/Build.sh"
    check_file "$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
    check_file "$engine_root/Engine/Binaries/Linux/UnrealEditor-Cmd"
fi

if [[ -n "${UE_WINE_MSVC:-}" ]]; then
    if [[ -d "$UE_WINE_MSVC" ]]; then
        printf 'OK   UE_WINE_MSVC: %s\n' "$UE_WINE_MSVC"
    else
        printf 'MISS UE_WINE_MSVC directory: %s\n' "$UE_WINE_MSVC"
        failures=$((failures + 1))
    fi
else
    printf 'INFO UE_WINE_MSVC is unset; Linux editor/server builds can still work\n'
fi

if (( failures )); then
    printf 'Doctor found %d missing requirement(s).\n' "$failures"
    exit 1
fi
printf 'Doctor found no missing requirements.\n'
