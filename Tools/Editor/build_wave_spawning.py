import json
import os
import pathlib

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
GENERATED_TAG = unreal.Name("CWSWaveGenerated")
CONTENT_DIR = pathlib.Path(unreal.Paths.project_content_dir()).resolve()

SPAWN_POINTS = {
    "North": ((0.0, 3500.0, 450.0), "CWS_Spawn_North"),
    "South": ((0.0, -3500.0, 450.0), "CWS_Spawn_South"),
    "East": ((3500.0, 0.0, 450.0), "CWS_Spawn_East"),
    "West": ((-3500.0, 0.0, 450.0), "CWS_Spawn_West"),
    "NorthEast": ((3500.0, 3500.0, 450.0), "CWS_Spawn_NorthEast"),
    "NorthWest": ((-3500.0, 3500.0, 450.0), "CWS_Spawn_NorthWest"),
    "SouthEast": ((3500.0, -3500.0, 450.0), "CWS_Spawn_SouthEast"),
    "SouthWest": ((-3500.0, -3500.0, 450.0), "CWS_Spawn_SouthWest"),
    "Center": ((0.0, 0.0, 450.0), "CWS_Spawn_Center"),
}


def native_classes():
    wave_manager_class = getattr(unreal, "CWSWaveManager", None)
    spawn_point_class = getattr(unreal, "CWSSpawnPoint", None)
    if not wave_manager_class or not spawn_point_class:
        raise RuntimeError("ArenaShooter C++ wave classes are unavailable. Build ArenaShooterEditor first.")
    return wave_manager_class, spawn_point_class


def generated_actors(actor_subsystem):
    return [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor and GENERATED_TAG in actor.tags
    ]


def actor_package_filename(actor):
    package_name = actor.get_package().get_path_name()
    if not package_name.startswith("/Game/__ExternalActors__/Variant_Combat/Lvl_Combat/"):
        raise RuntimeError(f"Unexpected wave actor package: {package_name}")
    filename = (CONTENT_DIR / (package_name[len("/Game/"):] + ".uasset")).resolve()
    if CONTENT_DIR not in filename.parents:
        raise RuntimeError(f"Unexpected wave actor filename: {filename}")
    return filename


def generated_external_object_files():
    root = CONTENT_DIR / "__ExternalObjects__" / "Variant_Combat" / "Lvl_Combat"
    if not root.exists():
        return []
    matches = []
    for filename in root.rglob("*.uasset"):
        data = filename.read_bytes()
        if b"CWSSpawnPoint" in data or b"CWSWaveManager" in data or b"CWSWaveGenerated" in data:
            matches.append(filename.resolve())
    return matches


def remove_old_package_files(filenames):
    removed = []
    for filename in sorted(set(filenames)):
        if CONTENT_DIR not in filename.parents:
            raise RuntimeError(f"Refusing to remove package outside Content: {filename}")
        if filename.exists():
            filename.unlink()
            removed.append(str(filename))
    unreal.log(f"CWS wave old package files removed: {len(removed)}")


def set_common_actor_properties(actor, label, folder, extra_tags):
    actor.set_actor_label(label, mark_dirty=True)
    actor.set_folder_path(folder)
    actor.set_editor_property("tags", [GENERATED_TAG] + [unreal.Name(tag) for tag in extra_tags])
    try:
        actor.set_editor_property("is_spatially_loaded", False)
    except Exception:
        pass


def location_list(actor):
    location = actor.get_actor_location()
    return [round(location.x, 3), round(location.y, 3), round(location.z, 3)]


def read_round_totals(manager):
    totals = []
    for round_definition in manager.get_editor_property("rounds"):
        groups = round_definition.get_editor_property("spawn_groups")
        totals.append({
            "round": round_definition.get_editor_property("round_number"),
            "count": sum(group.get_editor_property("count") for group in groups),
        })
    return totals


def validate(actor_subsystem):
    wave_manager_class, spawn_point_class = native_classes()
    managers = [actor for actor in actor_subsystem.get_all_level_actors() if isinstance(actor, wave_manager_class)]
    spawn_points = [actor for actor in actor_subsystem.get_all_level_actors() if isinstance(actor, spawn_point_class)]
    if len(managers) != 1:
        raise RuntimeError(f"Expected one CWSWaveManager, found {len(managers)}")
    if len(spawn_points) != len(SPAWN_POINTS):
        raise RuntimeError(f"Expected {len(SPAWN_POINTS)} CWS spawn points, found {len(spawn_points)}")

    by_label = {actor.get_actor_label(): actor for actor in spawn_points}
    rows = []
    for direction, (expected_location, direction_tag) in SPAWN_POINTS.items():
        label = f"CWS_SpawnPoint_{direction}"
        actor = by_label.get(label)
        if not actor:
            raise RuntimeError(f"Missing spawn point: {label}")
        actual_location = location_list(actor)
        if any(abs(actual_location[index] - expected_location[index]) > 0.1 for index in range(3)):
            raise RuntimeError(f"Unexpected location for {label}: {actual_location}")
        if unreal.Name(direction_tag) not in actor.tags:
            raise RuntimeError(f"Missing direction tag {direction_tag} on {label}")
        rows.append({"direction": direction, "location": actual_location, "tag": direction_tag})

    manager = managers[0]
    round_totals = read_round_totals(manager)
    expected_totals = [8, 16, 24, 34, 15]
    if [entry["count"] for entry in round_totals] != expected_totals:
        raise RuntimeError(f"Unexpected round totals: {round_totals}")
    if not manager.get_editor_property("default_enemy_class"):
        raise RuntimeError("CWSWaveManager has no default enemy class")

    summary = {
        "manager": manager.get_actor_label(),
        "spawn_points": rows,
        "round_totals": round_totals,
        "default_enemy_class": str(manager.get_editor_property("default_enemy_class")),
    }
    unreal.log("WAVE_SPAWN_SYSTEM_INSPECT=" + json.dumps(summary, separators=(",", ":")))
    return summary


def build(actor_subsystem):
    wave_manager_class, spawn_point_class = native_classes()
    old_actors = generated_actors(actor_subsystem)
    old_package_files = [actor_package_filename(actor) for actor in old_actors]
    old_package_files.extend(generated_external_object_files())
    for actor in old_actors:
        actor_subsystem.destroy_actor(actor)

    manager = actor_subsystem.spawn_actor_from_class(
        wave_manager_class,
        unreal.Vector(0.0, 0.0, 1000.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    set_common_actor_properties(manager, "CWS_WaveManager", "Gameplay/Wave", ["CWS_WaveManager"])

    for direction, (location, direction_tag) in SPAWN_POINTS.items():
        spawn_point = actor_subsystem.spawn_actor_from_class(
            spawn_point_class,
            unreal.Vector(location[0], location[1], location[2]),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        set_common_actor_properties(
            spawn_point,
            f"CWS_SpawnPoint_{direction}",
            "Gameplay/SpawnPoints",
            [direction_tag],
        )

    for root_name in ("__ExternalActors__", "__ExternalObjects__"):
        external_root = CONTENT_DIR / root_name / "Variant_Combat" / "Lvl_Combat"
        for bucket in "0123456789ABCDEF":
            (external_root / bucket).mkdir(parents=True, exist_ok=True)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    remove_old_package_files(old_package_files)
    validate(actor_subsystem)
    unreal.log("CWS wave spawn system rebuilt in /Game/Variant_Combat/Lvl_Combat")


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")
    if os.environ.get("ARENA_WAVE_SPAWN_MODE", "build").lower() == "inspect":
        validate(actor_subsystem)
    else:
        build(actor_subsystem)


if __name__ == "__main__":
    main()
