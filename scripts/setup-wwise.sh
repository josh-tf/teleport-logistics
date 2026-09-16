#!/usr/bin/env bash
set -euo pipefail

teleport_logistics_root=$(cd "$(dirname "$0")/.." && pwd)
cli=${WWISE_CLI:-"$teleport_logistics_root/.toolchains/wwise-cli/wwise-cli_linux_amd64"}
cache=${WWISE_CACHE:-"$teleport_logistics_root/.toolchains/wwise-cache"}
project=${SML_PROJECT:-"$teleport_logistics_root/.toolchains/sml-project/FactoryGame.uproject"}

[[ -x "$cli" ]] || { echo "Missing executable wwise-cli: $cli" >&2; exit 1; }
[[ -f "$project" ]] || { echo "Missing starter project: $project" >&2; exit 1; }
mkdir -p "$cache"

echo "Wwise will securely prompt for your Audiokinetic email and password."
"$cli" --cache-dir "$cache" download \
    --sdk-version 2023.1.14.8770 \
    --filter Packages=SDK \
    --filter DeploymentPlatforms=Windows_vc160 \
    --filter DeploymentPlatforms=Windows_vc170 \
    --filter DeploymentPlatforms=Linux \
    --filter DeploymentPlatforms= \
    --filter Packages=Authoring

echo "Authenticate once more to download and integrate the matching Unreal plugin."
"$cli" --cache-dir "$cache" integrate-ue \
    --integration-version 2023.1.14.3555 \
    --project "$project"

echo "Wwise integration complete: $project"
