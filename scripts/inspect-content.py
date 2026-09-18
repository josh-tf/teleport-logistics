"""Run in the editor commandlet to validate TeleportLogistics's cooked-content defaults."""

import unreal
import json
from pathlib import Path

# Fresh commandlets may still be loading yesterday's cached registry. Register
# newly imported DLC folders before asking EditorAssetLibrary to resolve them.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/TeleportLogistics"], True)

# 0.4.7's masked detail material produced a rejected DX12 depth pipeline.
# Labels now blend their alpha into base colour on opaque printed plates.
details = unreal.EditorAssetLibrary.load_asset("/TeleportLogistics/Models/M_TeleporterDetails")
if details is None or details.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE:
    raise RuntimeError("TeleportLogistics detail plates must use the opaque material hotfix")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


models = ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub")
milestone_class = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsMilestone")
require(milestone_class is not None, "TeleportLogistics milestone class did not load")
unlocks = unreal.FGSchematic.get_unlocks(milestone_class)
cost = unreal.FGSchematic.get_cost(milestone_class)
icon = unreal.FGSchematic.get_item_icon(milestone_class)
category_class = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsCategory")
subcategory_class = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsSubCategory")
representation_class = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsActorRepresentation")
require(len(unlocks) == 1, f"Expected one recipe unlock, found {len(unlocks)}")
recipes = unlocks[0].get_recipes_to_unlock()
require(len(recipes) == 5, f"Expected five unlocked recipes, found {len(recipes)}")
require(len(cost) == 3, f"Expected three milestone costs, found {len(cost)}")
require(icon.resource_object is not None, "Milestone brush has no retained texture")
require(icon.resource_object.get_path_name() ==
        "/TeleportLogistics/Icons/T_TeleporterMilestone_512.T_TeleporterMilestone_512",
        "Milestone brush is not using the 512 px research capture")
require(category_class is not None, "TeleportLogistics category class did not load")
require(subcategory_class is not None, "TeleportLogistics build subcategory class did not load")
require(representation_class is not None, "TeleportLogistics map representation class did not load")

for recipe in recipes:
    require(recipe is not None, "Recipe unlock contains a null class")
    products = unreal.FGRecipe.get_products(recipe)
    produced_in = unreal.FGRecipe.get_produced_in(recipe)
    require(len(products) == 1 and products[0].item_class is not None,
            f"{recipe.get_name()} has no valid building product")
    require(len(produced_in) > 0, f"{recipe.get_name()} has no producer")
    descriptor = products[0].item_class
    require(unreal.FGItemDescriptor.get_category(descriptor) == category_class,
            f"{recipe.get_name()} is not assigned directly to the TeleportLogistics category")
    descriptor_default = unreal.get_default_object(descriptor)
    sub_categories = descriptor_default.get_sub_categories_from_instance()
    require(len(sub_categories) == 1 and sub_categories[0] == subcategory_class,
            f"{recipe.get_name()} is not assigned to the Teleporter Network subcategory")

for name in models:
    for asset_name in (f"M_Teleporter{name}", f"T_Teleporter{name}_256", f"T_Teleporter{name}_512"):
        asset = unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/Icons/{asset_name}")
        require(isinstance(asset, unreal.Texture2D), f"Missing icon texture {asset_name}")
for asset_name in ("T_TeleporterCategory_128", "T_TeleporterMilestone_256", "T_TeleporterMilestone_512"):
    asset = unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/Icons/{asset_name}")
    require(isinstance(asset, unreal.Texture2D), f"Missing UI texture {asset_name}")

screen = unreal.EditorAssetLibrary.load_asset("/TeleportLogistics/Models/M_TeleporterScreen")
require(isinstance(screen, unreal.Material), "Missing generated power-aware screen material")
screen_grid = unreal.EditorAssetLibrary.load_asset("/TeleportLogistics/Textures/T_TeleporterScreen_Grid")
require(isinstance(screen_grid, unreal.Texture2D), "Missing screen grid texture")
no_power_signal = unreal.EditorAssetLibrary.load_asset(
    "/TeleportLogistics/Models/M_TeleporterSignalOff")
require(no_power_signal is not None,
        "Missing explicit zero-emission material used to darken the Teleporter Hub")

# The factory surface is our own instance of the stock material so the detail
# tiling can differ from every base-game building. Its parent is asserted below,
# which is the property that actually matters: inheriting the stock parameter
# interface is what keeps Customizer paint and the build effect working.
FACTORY_MATERIAL = "/TeleportLogistics/Models/MI_TeleporterFactory.MI_TeleporterFactory"
FACTORY_PARENT = "/Game/FactoryGame/-Shared/Material/MI_Factory_Base_01.MI_Factory_Base_01"
material_paths = {
    "decal_custom": "/TeleportLogistics/Models/M_TeleporterDetails.M_TeleporterDetails",
    "factory": FACTORY_MATERIAL,
    "signal": FACTORY_MATERIAL,
    "screen": "/TeleportLogistics/Models/M_TeleporterScreen.M_TeleporterScreen",
    "decal_color": ("/Game/FactoryGame/Buildable/-Shared/Material/"
                    "DecalColor_Masked.DecalColor_Masked"),
    "decal_normal": ("/Game/FactoryGame/Buildable/-Shared/Material/"
                     "Decal_Normal.Decal_Normal"),
}
expected_slots = {
    "ItemInput": {"decal_custom", "factory", "decal_color", "signal", "screen"},
    "ItemOutput": {"decal_custom", "factory", "decal_color", "signal", "screen"},
    "FluidInput": {"decal_custom", "factory", "decal_color", "decal_normal", "signal", "screen"},
    "FluidOutput": {"decal_custom", "factory", "decal_color", "decal_normal", "signal", "screen"},
    "Hub": {"decal_custom", "factory", "signal", "screen"},
}
for model_name in models:
    mesh = unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/Models/SM_Teleporter{model_name}")
    require(isinstance(mesh, unreal.StaticMesh), f"Missing static mesh SM_Teleporter{model_name}")
    require(mesh.get_num_lods() == 3,
            f"SM_Teleporter{model_name} should contain three authored LODs")
    collision_specs = json.loads((Path(__file__).resolve().parents[1] / "assets/models/collision.json").read_text())
    boxes = mesh.get_editor_property("body_setup").get_editor_property("agg_geom").get_editor_property("box_elems")
    require(len(boxes) == len(collision_specs[model_name]), "Missing authored collision boxes: " + model_name)
    slots = mesh.get_editor_property("static_materials")
    actual_slots = {str(item.get_editor_property("material_slot_name")) for item in slots}
    require(actual_slots == expected_slots[model_name],
            f"Unexpected slots on SM_Teleporter{model_name}: {sorted(actual_slots)}")
    for item in slots:
        slot_name = str(item.get_editor_property("material_slot_name"))
        interface = item.get_editor_property("material_interface")
        actual_path = interface.get_path_name() if interface else "None"
        require(actual_path == material_paths[slot_name],
                f"SM_Teleporter{model_name} {slot_name}: expected {material_paths[slot_name]}, "
                f"got {actual_path}")
        if actual_path == FACTORY_MATERIAL:
            parent = interface.get_editor_property("parent")
            require(parent is not None and parent.get_path_name() == FACTORY_PARENT,
                    f"{FACTORY_MATERIAL} must inherit {FACTORY_PARENT}, got "
                    f"{parent.get_path_name() if parent else 'None'}")
    building_class = unreal.load_class(None, f"/Script/TeleportLogistics.TeleportLogistics{model_name}")
    require(building_class is not None, f"Missing building class TeleportLogistics{model_name}")
    building_default = unreal.get_default_object(building_class)
    require(building_default.get_interact_widget_class() ==
            unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsWidget"),
            f"TeleportLogistics{model_name} has no native TeleportLogistics interaction widget")
    if model_name == "Hub":
        power = building_default.get_editor_property("power")
        require(power is not None, "Hub power connection is missing")
        contact = power.get_editor_property("relative_location")
        require(abs(contact.x + 83.64) < 0.01 and abs(contact.y - 52.5) < 0.01 and
                abs(contact.z - 298.8) < 0.01, "Hub power socket missed the custom mast cap")
        require(len(power.get_editor_property("mWireConnectionLocations")) == 0,
                "Wire-location overrides would bypass the mast contact position")
    map_icon = building_default.get_actor_representation_texture()
    require(isinstance(map_icon, unreal.Texture2D),
            f"TeleportLogistics{model_name} CDO did not retain its map icon")
    map_material = building_default.get_actor_representation_compass_material()
    require(isinstance(map_material, unreal.MaterialInstanceConstant),
            f"TeleportLogistics{model_name} is missing its map material")
    require(map_material.get_path_name() == f"/TeleportLogistics/Icons/MI_TeleporterMap{model_name}.MI_TeleporterMap{model_name}",
            f"TeleportLogistics{model_name} uses the wrong map material")
    require(map_material.get_editor_property("parent").get_name() == "MI_CompassIcon_Static",
            f"TeleportLogistics{model_name} does not use the native static map material")
    parameters = map_material.get_editor_property("texture_parameter_values")
    require(len(parameters) == 1 and str(parameters[0].parameter_info.name) == "Icon"
            and parameters[0].parameter_value == map_icon,
            f"TeleportLogistics{model_name} map material does not bind its sidebar texture")
    require(map_icon.get_path_name() == f"/TeleportLogistics/Icons/M_Teleporter{model_name}.M_Teleporter{model_name}",
            f"TeleportLogistics{model_name} CDO uses the wrong map icon: {map_icon.get_path_name()}")

for old_name in ("Primary", "Secondary", "Shell", "Steel", "Frame", "Rubber",
                 "Input", "Output", "Glow", "Warning"):
    require(not unreal.EditorAssetLibrary.does_asset_exist(f"/TeleportLogistics/Models/M_Teleporter{old_name}"),
            f"Legacy material M_Teleporter{old_name} survived clean import")

unreal.log(
    f"TeleportLogistics content valid: {len(recipes)} recipe rewards, {len(cost)} milestone costs, "
    "retained milestone/category/model icons, five stable meshes, native factory/decal "
    "bindings, power-aware screen material, CDO map textures/materials, and explicit zero-emission material."
)

for name in ("Plate", "Well", "Meter"):
    texture = unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/UI/T_TeleporterUI_{name}")
    require(isinstance(texture, unreal.Texture2D), f"Missing SFUIKIT panel {name}")
    require(texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_UI,
            f"SFUIKIT panel {name} is not in the UI texture group")
for name in ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput"):
    cdo = unreal.get_default_object(unreal.load_class(None, f"/Script/TeleportLogistics.TeleportLogistics{name}"))
    require(not cdo.get_editor_property("mIsTickRateManaged"), f"{name} still uses idle tick throttling")

# Personnel travel is a separate Tier 9 unlock; logistics rewards stay unchanged.
travel = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsTravelMilestone")
require(travel is not None and unreal.FGSchematic.get_tech_tier(travel) == 9, "Personnel milestone must be Tier 9")
travel_cost = unreal.FGSchematic.get_cost(travel)
require([x.amount for x in travel_cost] == [200, 100, 50], "Personnel HUB resource costs changed")
require(all(x.item_class is not None for x in travel_cost), "Personnel milestone has missing resource descriptors")
travel_rewards = unreal.FGSchematic.get_unlocks(travel)[0].get_recipes_to_unlock()
require(len(travel_rewards) == 1, "Personnel milestone must unlock one build recipe")
require([x.amount for x in unreal.FGRecipe.get_ingredients(None, travel_rewards[0])] == [20, 10, 5], "Personnel construction costs changed")
for size in (256, 512):
    require(unreal.EditorAssetLibrary.load_asset(f"/TeleportLogistics/Icons/T_TeleporterTravelHub_{size}") is not None, "Missing Personnel capture")
mesh = unreal.EditorAssetLibrary.load_asset("/TeleportLogistics/Models/SM_TeleporterTravelHub")
require(mesh is not None and mesh.get_num_lods() == 3, "Personnel model needs three LODs")
require(len(mesh.get_editor_property("body_setup").get_editor_property("agg_geom").get_editor_property("box_elems")) == 7, "Personnel landing/sign collision missing")
for slot in mesh.get_editor_property("static_materials"):
    key = str(slot.get_editor_property("material_slot_name"))
    require(slot.get_editor_property("material_interface").get_path_name() == material_paths[key], "Wrong Personnel material: " + key)
unreal.log("Personnel content valid: Tier 9, 200/100/50 unlock, 20/10/5 construction, three LODs, seven collision boxes and retained icons.")

travel_class = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsTravelHub")
travel_default = unreal.get_default_object(travel_class)
require(travel_default.get_interact_widget_class() == unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsTravelWidget"), "Personnel E interaction has the wrong widget")
travel_power = travel_default.get_component_by_class(unreal.FGPowerConnectionComponent).get_editor_property("relative_location")
require((travel_power.x, travel_power.y, travel_power.z) == (-180.0, 220.0, 310.0), "Personnel power socket misses its mast cap")
require(travel_default.get_actor_representation_compass_material() is not None, "Personnel map material missing")
require(unreal.FGSchematic.get_item_icon(travel).resource_object is not None, "Personnel milestone icon is not retained")
require(len(unreal.FGRecipe.get_produced_in(travel_rewards[0])) > 0, "Personnel recipe missing construction producer")
unreal.log("Personnel interaction, mast contact, map material and milestone icon validated.")

# Native CDO creation must not construct the stock Blueprint reward object.
for name, expected in (("TeleportLogisticsMilestone", "TeleportLogisticsUnlock"), ("TeleportLogisticsTravelMilestone", "TeleportLogisticsTravelUnlock")):
    rewards = unreal.FGSchematic.get_unlocks(unreal.load_class(None, "/Script/TeleportLogistics." + name))
    require(len(rewards) == 1 and rewards[0].get_class().get_path_name() == "/Script/TeleportLogistics." + expected,
            name + " loaded Blueprint presentation during native construction")
unreal.TeleportLogisticsGameInstanceModule.prepare_reward_presentation()
# Force collection before reading the replaced objects; CDO runtime references
# alone are insufficient in retail builds with disregard-for-GC enabled.
unreal.SystemLibrary.collect_garbage()
first_rewards = [unreal.FGSchematic.get_unlocks(unreal.load_class(None, "/Script/TeleportLogistics." + n))[0]
                 for n in ("TeleportLogisticsMilestone", "TeleportLogisticsTravelMilestone")]
unreal.TeleportLogisticsGameInstanceModule.prepare_reward_presentation()
for n, first in zip(("TeleportLogisticsMilestone", "TeleportLogisticsTravelMilestone"), first_rewards):
    require(unreal.FGSchematic.get_unlocks(unreal.load_class(None, "/Script/TeleportLogistics." + n))[0] == first,
            "Repeated presentation initialization replaced " + n)
require(len(first_rewards[0].get_recipes_to_unlock()) == 5 and len(first_rewards[1].get_recipes_to_unlock()) == 1,
        "Presentation replacement lost recipe rewards")

# Reward counts alone do not prove native HUB widgets can render the unlock.
reward_interface = unreal.load_class(None, "/Game/FactoryGame/Unlocks/BPI_UnlockableInterface.BPI_UnlockableInterface_C")
require(reward_interface is not None, "Stock reward interface missing")
for milestone_name in ("TeleportLogisticsMilestone", "TeleportLogisticsTravelMilestone"):
    milestone_class = unreal.load_class(None, "/Script/TeleportLogistics." + milestone_name)
    for unlock in unreal.FGSchematic.get_unlocks(milestone_class):
        require(unreal.SystemLibrary.does_implement_interface(unlock, reward_interface),
                milestone_name + " cannot supply native reward widgets")
        require(unlock.get_class().get_path_name() == "/Game/FactoryGame/Unlocks/BP_UnlockRecipe.BP_UnlockRecipe_C",
                milestone_name + " is missing the stock reward-widget Blueprint wrapper")
unreal.log("Both milestones use stock BP_UnlockRecipe reward presentation.")

# Research contract: validate identities and amounts, not only array lengths.
def amounts(entries):
    require(all(x.item_class is not None and x.amount > 0 for x in entries), "Missing/invalid cost resource")
    require(len({x.item_class.get_name() for x in entries}) == len(entries), "Duplicate cost resource")
    return {x.item_class.get_name(): x.amount for x in entries}
require(unreal.FGSchematic.get_tech_tier(milestone_class) in (5, 9), "Unexpected research tier")
logistics = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogisticsMilestone")
require(unreal.FGSchematic.get_tech_tier(logistics) == 5, "Logistics must remain Tier 5")
require(amounts(unreal.FGSchematic.get_cost(logistics)) == {
    "Desc_CircuitBoard_C": 100, "Desc_IronPlateReinforced_C": 100, "Desc_Cable_C": 200}, "Logistics unlock costs differ")
require(amounts(unreal.FGSchematic.get_cost(travel)) == {
    "Desc_TimeCrystal_C": 200, "Desc_ComputerSuper_C": 100, "Desc_MotorLightweight_C": 50}, "Personnel unlock costs differ")
for kind in ("ItemInput", "ItemOutput", "FluidInput", "FluidOutput", "Hub", "Travel"):
    recipe = unreal.load_class(None, "/Script/TeleportLogistics.TeleportLogistics" + kind + "Recipe")
    product = unreal.FGRecipe.get_products(recipe)
    require(len(product) == 1 and product[0].amount == 1, kind + " must produce exactly one building")
    require(product[0].item_class.get_path_name() == "/Script/TeleportLogistics.TeleportLogistics" + kind + "Descriptor", kind + " product mismatch")
    # SDK GetBuildableClass is a reconstructed stub; inspect the stored class instead.
    building = unreal.get_default_object(product[0].item_class).get_editor_property("mBuildableClass")
    expected_building = "TeleportLogisticsTravelHub" if kind == "Travel" else "TeleportLogistics" + kind
    require(building is not None and building.get_path_name() == "/Script/TeleportLogistics." + expected_building, kind + " buildable mismatch")
    require(unreal.get_default_object(building).get_editor_property("mHologramClass") is not None, kind + " lacks a hologram")
    expected = {"Desc_IronPlateReinforced_C": 4, "Desc_CircuitBoard_C": 4, "Desc_Cable_C": 10}
    if kind == "Hub": expected["Desc_Computer_C"] = 5
    if kind == "Travel": expected = {"Desc_TimeCrystal_C": 20, "Desc_ComputerSuper_C": 10, "Desc_MotorLightweight_C": 5}
    require(amounts(unreal.FGRecipe.get_ingredients(None, recipe)) == expected, kind + " construction costs differ")
unreal.log("Research contract passed: exact tiers, cost resources, quantities, products, building classes and holograms.")

# Milestone symbols are distinct from the individual 3D building reward images.
for class_name, icon_name in (("TeleportLogisticsMilestone", "Milestone"), ("TeleportLogisticsTravelMilestone", "PersonnelMilestone")):
    cls = unreal.load_class(None, "/Script/TeleportLogistics." + class_name)
    icon = unreal.FGSchematic.get_item_icon(cls).resource_object
    require(icon is not None and icon.get_name() == "T_Teleporter" + icon_name + "_512", class_name + " uses the wrong milestone icon")
unreal.log("Milestone tile symbols use separate retained textures from building rewards.")
