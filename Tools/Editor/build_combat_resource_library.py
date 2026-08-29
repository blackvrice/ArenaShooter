import json

import unreal


DEFAULT_MATERIAL_PATH = "/Engine/EngineMaterials/DefaultMaterial"
OUTPUT_ROOT = "/Game/CWSResources/Enemies"

PROFILES = {
    "minion": {
        "source_skeleton": "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Core_Skeleton",
        "target_skeleton": OUTPUT_ROOT + "/Minion/SK_Minion_Skeleton",
        "meshes": {
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Melee_Dawn": OUTPUT_ROOT + "/Normal/SK_NormalMinion",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Ranged_Dawn": OUTPUT_ROOT + "/Fast/SK_FastMinion",
        },
        "animations": {
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/NonCombat_Idle": OUTPUT_ROOT + "/Normal/A_Normal_Idle",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Combat_JogFwd": OUTPUT_ROOT + "/Normal/A_Normal_Move",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Attack_A": OUTPUT_ROOT + "/Normal/A_Normal_Attack",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/HitReact_Front": OUTPUT_ROOT + "/Normal/A_Normal_Hit",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_A": OUTPUT_ROOT + "/Normal/A_Normal_Death",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Ranged/Idle_A": OUTPUT_ROOT + "/Fast/A_Fast_Idle",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Ranged/Jog_Fwd_Combat_A": OUTPUT_ROOT + "/Fast/A_Fast_Move",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Ranged/Fire_A": OUTPUT_ROOT + "/Fast/A_Fast_Attack",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Ranged/HitReact_Front_A": OUTPUT_ROOT + "/Fast/A_Fast_Hit",
            "/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Ranged/Death_Front_A": OUTPUT_ROOT + "/Fast/A_Fast_Death",
        },
    },
    "tank": {
        "source_skeleton": "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog_Skeleton",
        "target_skeleton": OUTPUT_ROOT + "/Tank/SK_Tank_Skeleton",
        "meshes": {
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog": OUTPUT_ROOT + "/Tank/SK_Tank",
        },
        "animations": {
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Idle": OUTPUT_ROOT + "/Tank/A_Tank_Idle",
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Jog_Fwd": OUTPUT_ROOT + "/Tank/A_Tank_Move",
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Swing1_Return2Idle": OUTPUT_ROOT + "/Tank/A_Tank_Attack",
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Hitreact_Front": OUTPUT_ROOT + "/Tank/A_Tank_Hit",
            "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Death_front": OUTPUT_ROOT + "/Tank/A_Tank_Death",
        },
    },
}


def replace_asset(source_path, target_path):
    if unreal.EditorAssetLibrary.does_asset_exist(target_path):
        if not unreal.EditorAssetLibrary.delete_asset(target_path):
            raise RuntimeError("Could not replace existing asset: " + target_path)
    asset = unreal.EditorAssetLibrary.duplicate_asset(source_path, target_path)
    if not asset:
        raise RuntimeError("Could not duplicate {} to {}".format(source_path, target_path))
    return asset


def use_default_materials(mesh, default_material):
    materials = list(mesh.get_editor_property("materials"))
    for material in materials:
        material.set_editor_property("material_interface", default_material)
    mesh.set_editor_property("materials", materials)


def save_asset(asset_path):
    if not unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False):
        raise RuntimeError("Could not save generated asset: " + asset_path)


default_material = unreal.load_asset(DEFAULT_MATERIAL_PATH)
if not default_material:
    raise RuntimeError("Default material is unavailable")

generated_assets = []
for profile_name, profile in PROFILES.items():
    if unreal.EditorAssetLibrary.does_asset_exist(profile["target_skeleton"]):
        if not unreal.EditorAssetLibrary.delete_asset(profile["target_skeleton"]):
            raise RuntimeError("Could not remove stale generated skeleton: " + profile["target_skeleton"])

    source_skeleton = unreal.load_asset(profile["source_skeleton"])
    if not source_skeleton:
        raise RuntimeError("Could not load source skeleton: " + profile["source_skeleton"])

    for source_path, target_path in profile["meshes"].items():
        mesh = replace_asset(source_path, target_path)
        mesh.set_editor_property("physics_asset", None)
        use_default_materials(mesh, default_material)
        save_asset(target_path)
        generated_assets.append(target_path)

    for source_path, target_path in profile["animations"].items():
        animation = replace_asset(source_path, target_path)
        save_asset(target_path)
        generated_assets.append(target_path)

validation = []
for asset_path in generated_assets:
    asset = unreal.load_asset(asset_path)
    validation.append(
        {
            "path": asset_path,
            "loaded": bool(asset),
            "class": asset.get_class().get_name() if asset else "",
        }
    )

if not all(item["loaded"] for item in validation):
    raise RuntimeError("One or more generated combat resources failed validation")

unreal.log("CWS_COMBAT_RESOURCE_LIBRARY=" + json.dumps(validation, sort_keys=True))
unreal.log(
    "CWS_COMBAT_RESOURCE_LIBRARY_SUCCESS: generated {} standalone combat assets".format(
        len(generated_assets)
    )
)
