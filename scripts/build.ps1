param(
    [Parameter(Mandatory = $true)][string]$ProjectRoot,
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [ValidateSet("Editor", "Windows", "WindowsServer", "WindowsAll", "LinuxServer", "All")]
    [string]$Target = "Windows",
    [ValidateRange(0, 64)][int]$MaxParallelActions = 0
)
$ErrorActionPreference = "Stop"
$teleporterSource = Join-Path $PSScriptRoot "../TeleportLogistics"
$project = Join-Path $ProjectRoot "FactoryGame.uproject"
$build = Join-Path $EngineRoot "Engine/Build/BatchFiles/Build.bat"
$uat = Join-Path $EngineRoot "Engine/Build/BatchFiles/RunUAT.bat"
$editor = Join-Path $EngineRoot "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
foreach ($required in @($project, $build, $uat, $editor, (Join-Path $ProjectRoot "Mods/SML/SML.uplugin"),
        (Join-Path $ProjectRoot "Source/FactoryEditor.Target.cs"))) {
    if (!(Test-Path $required)) { throw "Required modding-environment file missing: $required" }
}
$projectInfo = Get-Content $project -Raw | ConvertFrom-Json
if ($projectInfo.EngineAssociation -ne "5.6.1-CSS") {
    throw "This source targets 5.6.1-CSS / game CL502094. Check sdk-lock.json before changing the SDK."
}
$teleportLogisticsDestination = Join-Path $ProjectRoot "Mods/TeleportLogistics"
# The clear below is recursive: refuse to run when staging resolves onto the authored plugin.
$sourceFull = (Resolve-Path -LiteralPath $teleporterSource).Path
$staging = Get-Item -LiteralPath $teleportLogisticsDestination -Force -ErrorAction SilentlyContinue
if ($staging -and ($staging.FullName -eq $sourceFull -or $staging.Target -eq $sourceFull)) {
    throw "Project staging must not be the source plugin."
}
# TeleportLogistics is a generated staging copy. Start clean so renamed assets cannot leak
# into a later cook.
if (Test-Path $teleportLogisticsDestination) { Remove-Item $teleportLogisticsDestination -Recurse -Force }
New-Item -ItemType Directory -Force $teleportLogisticsDestination | Out-Null
Copy-Item (Join-Path $teleporterSource "TeleportLogistics.uplugin") $teleportLogisticsDestination -Force
foreach ($folder in @("Source", "Config", "Resources")) {
    $sourcePath = Join-Path $teleporterSource $folder
    $destinationPath = Join-Path $teleportLogisticsDestination $folder
    New-Item -ItemType Directory -Force $destinationPath | Out-Null
    if (Test-Path $sourcePath) { Copy-Item (Join-Path $sourcePath "*") $destinationPath -Recurse -Force }
}
# All TeleportLogistics packages are generated from the checked-in source assets. Import
# into an empty directory so stale materials/LODs cannot survive and Unreal's
# crash-prone force-delete path is never used.
New-Item -ItemType Directory -Force (Join-Path $teleportLogisticsDestination "Content") | Out-Null
$editorBuildArguments = @("FactoryEditor", "Win64", "Development", "-Project=$project", "-WaitMutex")
if ($MaxParallelActions -gt 0) { $editorBuildArguments += "-MaxParallelActions=$MaxParallelActions" }
& $build @editorBuildArguments
if ($LASTEXITCODE -ne 0) { throw "Unreal editor compilation failed ($LASTEXITCODE). See the build output." }
$importScript = (Resolve-Path (Join-Path $PSScriptRoot "import-assets.py")).Path
& $editor $project "-run=pythonscript" "-script=$importScript" "-unattended" "-nosplash" "-nullrhi" "-DDC-ForceMemoryCache"
if ($LASTEXITCODE -ne 0) { throw "TeleportLogistics mesh/icon import failed ($LASTEXITCODE)." }
$inspectScript = (Resolve-Path (Join-Path $PSScriptRoot "inspect-content.py")).Path
& $editor $project "-run=pythonscript" "-script=$inspectScript" "-unattended" "-nosplash" "-nullrhi" "-DDC-ForceMemoryCache"
if ($LASTEXITCODE -ne 0) { throw "TeleportLogistics content validation failed ($LASTEXITCODE)." }
foreach ($model in @("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub")) {
    if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Models/SM_Teleporter$model.uasset"))) {
        throw "Expected imported mesh is missing: SM_Teleporter$model"
    }
    if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Icons/T_Teleporter${model}_256.uasset"))) {
        throw "Expected imported small icon is missing: T_Teleporter${model}_256"
    }
    if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Icons/T_Teleporter${model}_512.uasset"))) {
        throw "Expected imported big icon is missing: T_Teleporter${model}_512"
    }
    if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Icons/M_Teleporter$model.uasset"))) {
        throw "Expected imported map marker is missing: M_Teleporter$model"
    }
}
# The Personnel Teleporter is not in the model loop above: it deliberately has no map
# marker of its own name, so that loop would demand an asset that by design does not exist.
if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Models/SM_TeleporterTravelHub.uasset"))) {
    throw "Expected imported mesh is missing: SM_TeleporterTravelHub"
}
foreach ($icon in @("T_TeleporterCategory_128", "T_TeleporterMilestone_256", "T_TeleporterMilestone_512",
                    "T_TeleporterTravelHub_256", "T_TeleporterTravelHub_512",
                    "T_TeleporterPersonnelMilestone_256", "T_TeleporterPersonnelMilestone_512")) {
    if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Icons/$icon.uasset"))) {
        throw "Expected UI icon is missing: $icon"
    }
}
if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Models/M_TeleporterScreen.uasset"))) {
    throw "Expected generated screen material is missing: M_TeleporterScreen"
}
if (!(Test-Path (Join-Path $teleportLogisticsDestination "Content/Textures/T_TeleporterScreen_Grid.uasset"))) {
    throw "Expected generated screen texture is missing: T_TeleporterScreen_Grid"
}
# Preserve the validated imported packages for source archives and future clean stages.
$retainedContent = Join-Path $teleporterSource "Content"
if (Test-Path $retainedContent) { Remove-Item $retainedContent -Recurse -Force }
New-Item -ItemType Directory -Force $retainedContent | Out-Null
Copy-Item (Join-Path $teleportLogisticsDestination "Content/*") $retainedContent -Recurse -Force
if ($Target -eq "Editor") { return }
$arguments = @("-ScriptsForProject=$project", "PackagePlugin", "-project=$project", "-DLCName=TeleportLogistics",
    "-clientconfig=Shipping", "-serverconfig=Shipping", "-utf8output", "-build", "-nocompileeditor")
if ($MaxParallelActions -gt 0) { $arguments += "-ubtargs=-MaxParallelActions=$MaxParallelActions" }
switch ($Target) {
    "Windows"       { $arguments += "-platform=Win64" }
    "WindowsServer" { $arguments += @("-target=FactoryServer", "-server", "-serverplatform=Win64", "-noclient") }
    "WindowsAll"    { $arguments += @("-platform=Win64", "-server", "-serverplatform=Win64", "-merge") }
    "LinuxServer"   { $arguments += @("-target=FactoryServer", "-server", "-serverplatform=Linux", "-noclient") }
    "All"           { $arguments += @("-platform=Win64", "-server", "-serverplatform=Win64+Linux", "-merge") }
}
& $uat @arguments
if ($LASTEXITCODE -ne 0) { throw "Alpakit packaging failed ($LASTEXITCODE). See the packaging output." }
Write-Host "Packages: $(Join-Path $ProjectRoot 'Saved/ArchivedPlugins/TeleportLogistics')"
