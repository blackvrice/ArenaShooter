import json
import os

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
EXPECTED_BOSS_CLASS = "/Script/ArenaShooter.CWSBossEnemy"


def main():
    wave_manager_class = getattr(unreal, "CWSWaveManager", None)
    boss_class = getattr(unreal, "CWSBossEnemy", None)
    if not wave_manager_class or not boss_class:
        raise RuntimeError("Boss native classes are unavailable. Build ArenaShooterEditor first.")

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")

    managers = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, wave_manager_class)
    ]
    if len(managers) != 1:
        raise RuntimeError(f"Expected one CWSWaveManager, found {len(managers)}")

    manager = managers[0]
    inspect_only = os.environ.get("ARENA_BOSS_ROUND_MODE") == "inspect"
    previous_class = manager.get_editor_property("boss_enemy_class")
    previous_path = previous_class.get_path_name() if previous_class else None

    if not inspect_only and previous_path != EXPECTED_BOSS_CLASS:
        manager.set_editor_property("boss_enemy_class", boss_class)
        if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
            raise RuntimeError("Could not save the configured boss round")

    configured_class = manager.get_editor_property("boss_enemy_class")
    configured_path = configured_class.get_path_name() if configured_class else None
    if (inspect_only and previous_path != EXPECTED_BOSS_CLASS) or configured_path != EXPECTED_BOSS_CLASS:
        raise RuntimeError(f"Unexpected boss class: {configured_path}")

    result = {
        "mode": "inspect" if inspect_only else "configure",
        "manager": manager.get_actor_label(),
        "manager_package": manager.get_package().get_path_name(),
        "previous_boss_class": previous_path,
        "boss_class": configured_path,
    }
    unreal.log("BOSS_ROUND_CONFIG=" + json.dumps(result, separators=(",", ":")))


if __name__ == "__main__":
    main()
