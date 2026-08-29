import json
import os

import unreal


RESOURCE_ASSETS = {
    "normal_mesh": "/Game/CWSResources/Enemies/Normal/SK_NormalMinion",
    "normal_idle": "/Game/CWSResources/Enemies/Normal/A_Normal_Idle",
    "normal_move": "/Game/CWSResources/Enemies/Normal/A_Normal_Move",
    "normal_attack": "/Game/CWSResources/Enemies/Normal/A_Normal_Attack",
    "normal_hit": "/Game/CWSResources/Enemies/Normal/A_Normal_Hit",
    "normal_death": "/Game/CWSResources/Enemies/Normal/A_Normal_Death",
    "fast_mesh": "/Game/CWSResources/Enemies/Fast/SK_FastMinion",
    "fast_idle": "/Game/CWSResources/Enemies/Fast/A_Fast_Idle",
    "fast_move": "/Game/CWSResources/Enemies/Fast/A_Fast_Move",
    "fast_attack": "/Game/CWSResources/Enemies/Fast/A_Fast_Attack",
    "fast_hit": "/Game/CWSResources/Enemies/Fast/A_Fast_Hit",
    "fast_death": "/Game/CWSResources/Enemies/Fast/A_Fast_Death",
    "tank_mesh": "/Game/CWSResources/Enemies/Tank/SK_Tank",
    "tank_idle": "/Game/CWSResources/Enemies/Tank/A_Tank_Idle",
    "tank_move": "/Game/CWSResources/Enemies/Tank/A_Tank_Move",
    "tank_attack": "/Game/CWSResources/Enemies/Tank/A_Tank_Attack",
    "tank_hit": "/Game/CWSResources/Enemies/Tank/A_Tank_Hit",
    "tank_death": "/Game/CWSResources/Enemies/Tank/A_Tank_Death",
    "boss_fallback_mesh": "/Game/CWSResources/Enemies/Normal/SK_NormalMinion",
    "boss_fallback_idle": "/Game/CWSResources/Enemies/Normal/A_Normal_Idle",
    "boss_fallback_move": "/Game/CWSResources/Enemies/Normal/A_Normal_Move",
    "boss_fallback_attack": "/Game/CWSResources/Enemies/Normal/A_Normal_Attack",
    "boss_fallback_hit": "/Game/CWSResources/Enemies/Normal/A_Normal_Hit",
    "boss_fallback_death": "/Game/CWSResources/Enemies/Normal/A_Normal_Death",
}


def object_path(value):
    return value.get_path_name() if value else ""


def inspect_asset(label, asset_path):
    asset = unreal.load_asset(asset_path)
    if not asset:
        return {"label": label, "path": asset_path, "loaded": False}

    result = {
        "label": label,
        "path": asset_path,
        "loaded": True,
        "class": asset.get_class().get_name(),
    }
    for property_name in ("skeleton", "physics_asset"):
        try:
            result[property_name] = object_path(asset.get_editor_property(property_name))
        except Exception:
            pass
    return result


results = [inspect_asset(label, asset_path) for label, asset_path in RESOURCE_ASSETS.items()]
failed = [result["label"] for result in results if not result["loaded"]]
unreal.log("CWS_RESOURCE_INSPECTION=" + json.dumps(results, ensure_ascii=True, sort_keys=True))
if failed:
    raise RuntimeError("Failed to load combat resources: " + ", ".join(failed))

asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
dependency_options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=False,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)
pending_packages = []
for asset_path in RESOURCE_ASSETS.values():
    asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    if asset_data.is_valid():
        pending_packages.append(str(asset_data.package_name))

dependency_packages = set()
while pending_packages:
    package_name = pending_packages.pop()
    if package_name in dependency_packages:
        continue
    dependency_packages.add(package_name)
    for dependency in asset_registry.get_dependencies(package_name, dependency_options):
        dependency_name = str(dependency)
        if dependency_name.startswith("/Game/") and dependency_name not in dependency_packages:
            pending_packages.append(dependency_name)

dependency_report = {
    "root_assets": RESOURCE_ASSETS,
    "game_packages": sorted(dependency_packages),
}
report_directory = os.path.join(unreal.Paths.project_saved_dir(), "Automation")
os.makedirs(report_directory, exist_ok=True)
report_path = os.path.join(report_directory, "combat_resource_dependencies.json")
with open(report_path, "w", encoding="utf-8") as report_file:
    json.dump(dependency_report, report_file, indent=2, sort_keys=True)
unreal.log(
    "CWS_RESOURCE_DEPENDENCIES: count={} report={}".format(
        len(dependency_packages), report_path
    )
)
unreal.log("CWS_RESOURCE_INSPECTION_SUCCESS: all selected combat resources loaded")
