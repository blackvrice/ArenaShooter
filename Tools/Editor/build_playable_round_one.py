import json
import os
import pathlib

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
GENERATED_TAG = unreal.Name("CWSPlayableGenerated")
CONTENT_DIR = pathlib.Path(unreal.Paths.project_content_dir()).resolve()
PLAYER_START_LOCATION = (0.0, 1200.0, 450.0)
PLAYER_START_ROTATION = (0.0, -90.0, 0.0)
NAVMESH_LOCATION = (0.0, 0.0, 1000.0)
NAVMESH_SCALE = (55.0, 55.0, 10.0)


def native_classes():
    game_mode_class = getattr(unreal, "CWSGameMode", None)
    enemy_class = getattr(unreal, "CWSEnemyBase", None)
    fast_enemy_class = getattr(unreal, "CWSFastEnemy", None)
    tank_enemy_class = getattr(unreal, "CWSTankEnemy", None)
    boss_enemy_class = getattr(unreal, "CWSBossEnemy", None)
    wave_manager_class = getattr(unreal, "CWSWaveManager", None)
    if not all(
        (
            game_mode_class,
            enemy_class,
            fast_enemy_class,
            tank_enemy_class,
            boss_enemy_class,
            wave_manager_class,
        )
    ):
        raise RuntimeError("Playable Round 1 native classes are unavailable. Build ArenaShooterEditor first.")
    return (
        game_mode_class,
        enemy_class,
        fast_enemy_class,
        tank_enemy_class,
        boss_enemy_class,
        wave_manager_class,
    )


def package_filename(package_name):
    valid_prefixes = (
        "/Game/__ExternalActors__/Variant_Combat/Lvl_Combat/",
        "/Game/__ExternalObjects__/Variant_Combat/Lvl_Combat/",
    )
    if not package_name.startswith(valid_prefixes):
        raise RuntimeError(f"Unexpected generated package: {package_name}")
    filename = (CONTENT_DIR / (package_name[len("/Game/"):] + ".uasset")).resolve()
    if CONTENT_DIR not in filename.parents:
        raise RuntimeError(f"Unexpected generated filename: {filename}")
    return filename


def actor_package_filenames(actor):
    packages = {actor.get_package().get_path_name()}
    for component in actor.get_components_by_class(unreal.ActorComponent):
        package_name = component.get_package().get_path_name()
        if package_name.startswith("/Game/__ExternalObjects__/Variant_Combat/Lvl_Combat/"):
            packages.add(package_name)
    return [package_filename(package_name) for package_name in packages]


def generated_actors(actor_subsystem):
    return [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor and GENERATED_TAG in actor.tags
    ]


def set_common_actor_properties(actor, label, folder):
    actor.set_actor_label(label, mark_dirty=True)
    actor.set_folder_path(folder)
    actor.set_editor_property("tags", [GENERATED_TAG])
    try:
        actor.set_editor_property("is_spatially_loaded", False)
    except Exception:
        pass


def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def configure_runtime_classes(actor_subsystem):
    (
        game_mode_class,
        enemy_class,
        fast_enemy_class,
        tank_enemy_class,
        boss_enemy_class,
        wave_manager_class,
    ) = native_classes()
    world = editor_world()
    if not world:
        raise RuntimeError("Editor world is unavailable")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode_class)

    managers = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, wave_manager_class)
    ]
    if len(managers) != 1:
        raise RuntimeError(f"Expected one CWSWaveManager, found {len(managers)}")
    managers[0].set_editor_property("default_enemy_class", enemy_class)
    managers[0].set_editor_property("fast_enemy_class", fast_enemy_class)
    managers[0].set_editor_property("tank_enemy_class", tank_enemy_class)
    managers[0].set_editor_property("boss_enemy_class", boss_enemy_class)
    managers[0].set_editor_property("initial_start_delay", 2.0)
    return managers[0]


def rounded_vector(vector):
    return [round(vector.x, 3), round(vector.y, 3), round(vector.z, 3)]


def rounded_rotation(rotation):
    return [round(rotation.pitch, 3), round(rotation.yaw, 3), round(rotation.roll, 3)]


def validate(actor_subsystem):
    (
        game_mode_class,
        enemy_class,
        fast_enemy_class,
        tank_enemy_class,
        boss_enemy_class,
        wave_manager_class,
    ) = native_classes()
    actors = actor_subsystem.get_all_level_actors()
    player_starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
    navmesh_bounds = [actor for actor in actors if isinstance(actor, unreal.NavMeshBoundsVolume)]
    managers = [actor for actor in actors if isinstance(actor, wave_manager_class)]

    if len(player_starts) != 1:
        raise RuntimeError(f"Expected one PlayerStart, found {len(player_starts)}")
    if len(navmesh_bounds) != 1:
        raise RuntimeError(f"Expected one NavMeshBoundsVolume, found {len(navmesh_bounds)}")
    if len(managers) != 1:
        raise RuntimeError(f"Expected one CWSWaveManager, found {len(managers)}")

    player_location = rounded_vector(player_starts[0].get_actor_location())
    if any(abs(player_location[index] - PLAYER_START_LOCATION[index]) > 0.1 for index in range(3)):
        raise RuntimeError(f"Unexpected PlayerStart location: {player_location}")
    player_rotation = rounded_rotation(player_starts[0].get_actor_rotation())
    if any(abs(player_rotation[index] - PLAYER_START_ROTATION[index]) > 0.1 for index in range(3)):
        raise RuntimeError(f"Unexpected PlayerStart rotation: {player_rotation}")

    nav_location = rounded_vector(navmesh_bounds[0].get_actor_location())
    nav_scale = rounded_vector(navmesh_bounds[0].get_actor_scale3d())
    if any(abs(nav_location[index] - NAVMESH_LOCATION[index]) > 0.1 for index in range(3)):
        raise RuntimeError(f"Unexpected NavMesh location: {nav_location}")
    if any(abs(nav_scale[index] - NAVMESH_SCALE[index]) > 0.1 for index in range(3)):
        raise RuntimeError(f"Unexpected NavMesh scale: {nav_scale}")

    world_game_mode = editor_world().get_world_settings().get_editor_property("default_game_mode")
    manager_classes = {
        "default_enemy_class": managers[0].get_editor_property("default_enemy_class"),
        "fast_enemy_class": managers[0].get_editor_property("fast_enemy_class"),
        "tank_enemy_class": managers[0].get_editor_property("tank_enemy_class"),
        "boss_enemy_class": managers[0].get_editor_property("boss_enemy_class"),
    }
    expected_manager_classes = {
        "default_enemy_class": "/Script/ArenaShooter.CWSEnemyBase",
        "fast_enemy_class": "/Script/ArenaShooter.CWSFastEnemy",
        "tank_enemy_class": "/Script/ArenaShooter.CWSTankEnemy",
        "boss_enemy_class": "/Script/ArenaShooter.CWSBossEnemy",
    }
    if world_game_mode.get_path_name() != "/Script/ArenaShooter.CWSGameMode":
        raise RuntimeError(f"Unexpected world game mode: {world_game_mode}")
    for property_name, expected_class_path in expected_manager_classes.items():
        configured_class = manager_classes[property_name]
        if configured_class.get_path_name() != expected_class_path:
            raise RuntimeError(
                f"Unexpected {property_name}: {configured_class}; expected {expected_class_path}"
            )

    summary = {
        "player_start": player_location,
        "player_start_rotation": player_rotation,
        "player_start_packages": [
            filename.relative_to(CONTENT_DIR).as_posix()
            for filename in actor_package_filenames(player_starts[0])
        ],
        "navmesh_location": nav_location,
        "navmesh_scale": nav_scale,
        "navmesh_packages": [
            filename.relative_to(CONTENT_DIR).as_posix()
            for filename in actor_package_filenames(navmesh_bounds[0])
        ],
        "game_mode": world_game_mode.get_path_name(),
        "enemy_classes": {
            property_name: configured_class.get_path_name()
            for property_name, configured_class in manager_classes.items()
        },
        "manager": managers[0].get_actor_label(),
    }
    unreal.log("PLAYABLE_ROUND_ONE_INSPECT=" + json.dumps(summary, separators=(",", ":")))
    return summary


def build(actor_subsystem):
    old_actors = generated_actors(actor_subsystem)
    old_package_files = [
        filename
        for actor in old_actors
        for filename in actor_package_filenames(actor)
    ]
    for actor in old_actors:
        actor_subsystem.destroy_actor(actor)

    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(*PLAYER_START_LOCATION),
        unreal.Rotator(
            roll=PLAYER_START_ROTATION[2],
            pitch=PLAYER_START_ROTATION[0],
            yaw=PLAYER_START_ROTATION[1],
        ),
    )
    set_common_actor_properties(player_start, "CWS_PlayerStart", "Gameplay/Player")

    navmesh_bounds = actor_subsystem.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume,
        unreal.Vector(*NAVMESH_LOCATION),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    navmesh_bounds.set_actor_scale3d(unreal.Vector(*NAVMESH_SCALE))
    set_common_actor_properties(navmesh_bounds, "CWS_NavMeshBounds", "Gameplay/Navigation")

    configure_runtime_classes(actor_subsystem)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    for filename in sorted(set(old_package_files)):
        if CONTENT_DIR not in filename.parents:
            raise RuntimeError(f"Refusing to remove package outside Content: {filename}")
        if filename.exists():
            filename.unlink()

    validate(actor_subsystem)
    unreal.log("Playable Round 1 runtime map setup rebuilt")


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")
    if os.environ.get("ARENA_PLAYABLE_ROUND_ONE_MODE", "build").lower() == "inspect":
        validate(actor_subsystem)
    else:
        build(actor_subsystem)


if __name__ == "__main__":
    main()
