"""Fast source, model, and retained-package checks before Unreal packaging."""

from pathlib import Path
import json
import math
import re

from PIL import Image


root = Path(__file__).resolve().parents[1]
plugin = root / "TeleportLogistics"
manifest = json.loads((plugin / "TeleportLogistics.uplugin").read_text())
assert manifest["Modules"][0]["Name"] == "TeleportLogistics"
assert manifest["Plugins"][0]["Name"] == "SML"
assert manifest["AcceptsAnyRemoteVersion"] is False

building_header = (plugin / "Source/TeleportLogistics/Public/TeleportLogisticsBuilding.h").read_text()
building_source = (plugin / "Source/TeleportLogistics/Private/TeleportLogisticsBuilding.cpp").read_text()
subsystem_source = (plugin / "Source/TeleportLogistics/Private/TeleportLogisticsSubsystem.cpp").read_text()
remote_header = (plugin / "Source/TeleportLogistics/Public/TeleportLogisticsRemoteCall.h").read_text()
remote_source = (plugin / "Source/TeleportLogistics/Private/TeleportLogisticsRemoteCall.cpp").read_text()
types_header = (plugin / "Source/TeleportLogistics/Public/TeleportLogisticsTypes.h").read_text()
widget_source = (plugin / "Source/TeleportLogistics/Private/TeleportLogisticsWidget.cpp").read_text()
content_header = (plugin / "Source/TeleportLogistics/Public/TeleportLogisticsContent.h").read_text()
content_source = (plugin / "Source/TeleportLogistics/Private/TeleportLogisticsContent.cpp").read_text()
linux_build = (root / "scripts/build-linux.sh").read_text()

for token in (
    "IFGActorRepresentationInterface", "GetActorShouldShowOnMap()",
    "UTeleportLogisticsActorRepresentation", "mAllowColoring = true;",
    "mShouldApplyCustomizationData = true;",
    "Belt->SetRelativeLocation(FVector(160, 0, 100));",
    "Pipe->SetRelativeLocation(FVector(160, 0, 175));",
    "Power->SetRelativeLocation(FVector(-83.64, 52.5, 298.8));",
    "Power->SetRelativeTransform(GetDefault<ATeleportLogisticsHub>()->Power->GetRelativeTransform());",
    "Super::GetLookAtDecription_Implementation(Player, State)",
    "SetTimer(PowerVisualTimer, this, &ATeleportLogisticsHub::RefreshPowerVisual,",
    "ClearTimer(PowerVisualTimer)",
    "if (!IsInGameThread() || GetNetMode() == NM_DedicatedServer)",
    "M_TeleporterSignalOff",
    "SetScalarParameterValue(TEXT(\"TeleporterPower\")",
):
    assert token in building_header or token in building_source, token
assert "DOREPLIFETIME(ATeleportLogisticsEndpoint, RoutePath)" in building_source, \
    "the look-at route text must replicate; the panel is built client-side"
assert "Endpoint->RoutePath = Path" in (root / "TeleportLogistics/Source/TeleportLogistics/Private"
                                        "/TeleportLogisticsSubsystem.cpp").read_text(), \
    "RefreshMap must feed the replicated route text"
assert "CreateAndAddNewRepresentation" in building_source
assert "mInteractWidgetSoftClass = UTeleportLogisticsWidget::StaticClass();" in building_source
assert "Super::OnUse_Implementation(Player, State);" in building_source
assert "RequestInteractWidget(" not in widget_source
assert "GameUI->PopWidget(this)" in widget_source
assert "GameUI->RemoveInteractWidget(this)" not in widget_source
assert "PopAllWidgets(" not in widget_source
assert "NativeOnPreviewKeyDown" in widget_source
assert "ContentRoot->SetContent(SNullWidget::NullWidget)" in widget_source
assert "Cast<ATeleportLogisticsBuilding>(mInteractObject)" in widget_source
assert "void UTeleportLogisticsWidget::Init_Implementation()" in widget_source
assert "void UTeleportLogisticsWidget::NativeConstruct()" in widget_source
# Prevent the exact 0.3.1 crash route: factory workers and power delegates must
# never drive rendering, even when the renderer happens to tolerate a test run.
hub_tick = building_source.split("void ATeleportLogisticsHub::Factory_Tick(float)", 1)[1].split(
    "bool ATeleportLogisticsHub::ControlPowered()", 1)[0]
assert "ApplyPowerVisual(" not in hub_tick
assert "RefreshPowerVisual(" not in hub_tick
assert "mOnHasPowerChanged.AddDynamic" not in building_source
visual = building_source.split("void ATeleportLogisticsBuilding::ApplyPowerVisual(bool HasPower)", 1)[1]
assert visual.index("!IsInGameThread()") < visual.index("VisualPowerInitialized")
assert visual.index("!IsInGameThread()") < visual.index("GetDefaultSubobjectByName")
assert "RemoveRepresentationOfActor" in building_source
assert "void ATeleportLogisticsSubsystem::RefreshMap()" in subsystem_source
assert "ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;" in subsystem_source
assert "UTeleportLogisticsSubCategory : public UFGBuildSubCategory" in content_header
assert "mSubCategories.Add(UTeleportLogisticsSubCategory::StaticClass())" in content_source
assert content_source.count("DescriptorDescription") == 5
assert "T_TeleporterCategory_128" in content_source
assert "T_TeleporterMilestone_512" in content_source
assert content_source.count("_256.T_Teleporter") >= 6
assert content_source.count("_512.T_Teleporter") >= 6
assert "RemoteCallObjects.Add(UTeleportLogisticsRemoteCall::StaticClass())" in content_source
assert remote_header.count("UFUNCTION(Server, Reliable, WithValidation)") == 8
assert remote_source.count("_Validate(") == 8
assert "FGuid Context;" in types_header and "uint32 RequestId = 0;" in types_header
assert "Snapshot.Context != Context->TeleporterId" in widget_source
assert "Snapshot.RequestId <= LastReceivedRequestId" in widget_source
assert "IsRunningDedicatedServer()" in widget_source
# build-linux.sh empties the repo plugin's Content during staging and refills it
# from the staged copy. Without that copy back, a build leaves the retained
# packages checked at the end of this file deleted.
assert re.search(r'cp -a\s+"?\$destination/Content[^"]*"?\s+"?\S*TeleportLogistics/Content',
                 linux_build), "build-linux.sh must copy staged Content back into the repo plugin"

# build.ps1 is not exercised on this host, so check it still targets the current plugin.
# A rename that misses the PowerShell entry point is the failure this catches.
windows_build = (root / "scripts/build.ps1").read_text()
for token in ("-DLCName=TeleportLogistics", "Mods/TeleportLogistics"):
    assert token in windows_build, f"build.ps1 must target the current plugin: {token}"

native_class_count = 0
for path in (plugin / "Source").rglob("*.h"):
    text = path.read_text()
    includes = re.findall(r'#include "([^"]+)"', text)
    if "GENERATED_BODY" in text:
        assert includes[-1] == path.stem + ".generated.h", path
        native_class_count += len(re.findall(r"\bclass\s+TELEPORTLOGISTICS_API\s+(\w+)\s*:", text))
for path in (plugin / "Source").rglob("*.cpp"):
    text = path.read_text()
    assert "TODO" not in text, path
print(f"PASS: native source structure, multiplayer RPC validation, use prompt, map support, "
      f"and hub power visuals; {native_class_count} native classes")

model_names = ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub", "TravelHub")
expected_slots = {
    "SM_TeleporterItemInput": {"decal_custom", "factory", "decal_color", "signal", "screen"},
    "SM_TeleporterItemOutput": {"decal_custom", "factory", "decal_color", "signal", "screen"},
    "SM_TeleporterFluidInput": {"decal_custom", "factory", "decal_color", "decal_normal", "signal", "screen"},
    "SM_TeleporterFluidOutput": {"decal_custom", "factory", "decal_color", "decal_normal", "signal", "screen"},
    "SM_TeleporterTravelHub": {"decal_custom", "factory", "signal", "screen"},
    "SM_TeleporterHub": {"decal_custom", "factory", "signal", "screen"},
}
models = sorted((root / "assets/models").glob("*.obj"))
assert {path.stem for path in models} == {"SM_Teleporter" + name for name in model_names}
triangles = 0
for path in models:
    vertices, faces, materials = [], [], set()
    uv_count = normal_count = 0
    corner_attributes = []
    for line in path.read_text().splitlines():
        if line.startswith("v "):
            point = tuple(map(float, line.split()[1:]))
            assert len(point) == 3 and all(map(math.isfinite, point)), path
            vertices.append(point)
        elif line.startswith("vt "):
            uv = tuple(map(float, line.split()[1:]))
            assert len(uv) == 2 and all(map(math.isfinite, uv)), path
            uv_count += 1
        elif line.startswith("vn "):
            normal = tuple(map(float, line.split()[1:]))
            assert len(normal) == 3 and all(map(math.isfinite, normal)), path
            assert sum(v*v for v in normal) > 0.5, (path, normal)
            normal_count += 1
        elif line.startswith("usemtl "):
            materials.add(line.split()[1])
        elif line.startswith("f "):
            corners = [value.split("/") for value in line.split()[1:]]
            assert all(len(corner) == 3 and all(corner) for corner in corners), path
            faces.append(tuple(int(corner[0]) - 1 for corner in corners))
            corner_attributes.extend((int(corner[1]), int(corner[2])) for corner in corners)
    # Validate actual attribute coverage rather than a minimum vertex count:
    # removing extruded text should reduce geometry without failing integrity.
    assert faces and all(1 <= uv <= uv_count and 1 <= normal <= normal_count
                         for uv, normal in corner_attributes), path
    assert materials == expected_slots[path.stem], (path, materials)
    low = tuple(min(point[i] for point in vertices) for i in range(3))
    high = tuple(max(point[i] for point in vertices) for i in range(3))
    assert low[2] >= -0.01, path
    if path.stem == "SM_TeleporterHub":
        assert 290 < high[0] - low[0] < 310 and 599 < high[1] - low[1] < 601, path
        assert 298 < high[2] - low[2] < 302, path
    else:
        assert high[0] - low[0] >= 350 and high[1] - low[1] >= 280, path
    for face in faces:
        assert len(face) == 3 and all(0 <= index < len(vertices) for index in face), path
        a, b, c = (vertices[index] for index in face)
        ab = [q - p for p, q in zip(a, b)]
        ac = [q - p for p, q in zip(a, c)]
        cross = (ab[1] * ac[2] - ab[2] * ac[1],
                 ab[2] * ac[0] - ab[0] * ac[2],
                 ab[0] * ac[1] - ab[1] * ac[0])
        assert sum(value * value for value in cross) > 1e-8, (path, face)
    triangles += len(faces)
assert triangles < 170000, triangles
print(f"PASS: six stock-scale UV model sources, stable consolidated slots, "
      f"and {triangles:,} total triangles")

expected_icons = ({f"M_Teleporter{name}.png" for name in model_names if name != "TravelHub"} |
                  {f"T_Teleporter{name}_{size}.png" for name in model_names for size in (256, 512)} |
                  {"T_TeleporterCategory_128.png", "T_TeleporterMilestone_256.png",
                   "T_TeleporterMilestone_512.png", "T_TeleporterPersonnelMilestone_256.png", "T_TeleporterPersonnelMilestone_512.png"})
icons = sorted((root / "assets/icons").glob("*.png"))
assert {path.name for path in icons} == expected_icons
for path in icons:
    image = Image.open(path)
    expected_size = 128 if path.name.startswith("M_") or path.name.endswith("_128.png") else (
        256 if path.name.endswith("_256.png") else 512)
    assert image.mode == "RGBA" and image.size == (expected_size, expected_size), path
    alpha = image.getchannel("A").getextrema()
    assert alpha == (0, 255), (path, alpha)
textures = sorted((root / "assets/textures").glob("*.png"))
assert [path.name for path in textures] == ["T_TeleporterDetails.png", "T_TeleporterScreen_Grid.png"]
assert Image.open(textures[0]).size == (2048, 1024)
print("PASS: twelve 3D descriptor captures, five transparent map markers, native-style "
      "category/milestone art, and two atlas textures")

expected_packages = {
    "Models": [f"SM_Teleporter{name}" for name in model_names] + ["M_TeleporterScreen", "M_TeleporterSignalOff", "M_TeleporterDetails"],
    "Icons": [path.stem for path in icons] + [f"MI_TeleporterMap{name}" for name in model_names if name != "TravelHub"],
    "UI": [f"T_TeleporterUI_{name}" for name in ("Plate", "Well", "Meter")],
    "Textures": ["T_TeleporterScreen_Grid", "T_TeleporterDetails"],
}
for asset_type, names in expected_packages.items():
    directory = plugin / "Content" / asset_type
    for name in names:
        path = directory / f"{name}.uasset"
        assert path.is_file() and path.stat().st_size > 1024, path
    actual = {path.stem for path in directory.glob("*.uasset")}
    assert actual == set(names), (asset_type, sorted(actual - set(names)),
                                  sorted(set(names) - actual))

import_script = (root / "scripts/import-assets.py").read_text()
for token in (
    "MI_Factory_Base_01.MI_Factory_Base_01", "DecalColor_Masked.DecalColor_Masked",
    '"TeleporterPower"', 'mesh.set_editor_property("lod_group", "None")',
    "mesh.get_num_lods() != 3", "mesh.set_material(material_index, materials[slot_name])",
):
    assert token in import_script, token
inspect_script = (root / "scripts/inspect-content.py").read_text()
assert "M_TeleporterSignalOff" in inspect_script
assert "get_actor_representation_compass_material()" in inspect_script
assert "get_actor_representation_texture()" in inspect_script
print(f"PASS: {sum(map(len, expected_packages.values()))} retained Unreal packages; native factory/decal bindings, "
      "three authored LODs, and power-aware screen material configured")
