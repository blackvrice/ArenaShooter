import math
import pathlib

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
FOLDER_PATH = "CWS_Blockout"
PREFIX = "CWS_"
ARENA_SIZE = 4000
ARENA_HALF = ARENA_SIZE / 2.0
BOUNDARY_THICKNESS = 80

CUBE_PATH = "/Game/LevelPrototyping/Meshes/SM_Cube"
CYLINDER_PATH = "/Game/LevelPrototyping/Meshes/SM_Cylinder"
CHAMFER_CUBE_PATH = "/Game/LevelPrototyping/Meshes/SM_ChamferCube"
FLOOR_MAT_PATH = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray"
DARK_MAT_PATH = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark"
COLOR_MAT_PATH = "/Game/LevelPrototyping/Materials/MI_DefaultColorway"
EXTERNAL_ACTOR_DIR = pathlib.Path(unreal.Paths.project_content_dir()) / "__ExternalActors__" / "Variant_Combat" / "Lvl_Combat"


def load_asset(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Could not load asset: {path}")
    return asset


def set_label(actor, label, hide_in_game=False, editor_only=False):
    actor.set_actor_label(label, mark_dirty=True)
    actor.set_folder_path(FOLDER_PATH)
    actor.set_editor_property("tags", [unreal.Name("CWS"), unreal.Name("GeneratedBlockout")])
    if hide_in_game:
        actor.set_actor_hidden_in_game(True)
    if editor_only:
        try:
            actor.set_editor_property("is_editor_only_actor", True)
        except Exception:
            pass


def cube_scale(size):
    return unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0)


def spawn_mesh(
    actor_subsystem,
    mesh,
    material,
    label,
    location,
    size,
    rotation=(0.0, 0.0, 0.0),
    hide_in_game=False,
    editor_only=False,
):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(rotation[0], rotation[1], rotation[2]),
    )
    set_label(actor, label, hide_in_game=hide_in_game, editor_only=editor_only)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    if material:
        component.set_material(0, material)
    actor.set_actor_scale3d(cube_scale(size))
    return actor


def spawn_cylinder(
    actor_subsystem,
    mesh,
    material,
    label,
    location,
    radius,
    height,
    hide_in_game=False,
    editor_only=False,
):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    set_label(actor, label, hide_in_game=hide_in_game, editor_only=editor_only)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    if material:
        component.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(radius / 50.0, radius / 50.0, height / 100.0))
    return actor


def spawn_text(actor_subsystem, label, text, location, yaw=0.0):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.TextRenderActor,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, yaw, 0.0),
    )
    set_label(actor, label, hide_in_game=True, editor_only=True)
    component = actor.get_text_render() if hasattr(actor, "get_text_render") else None
    if component:
        component.set_text(text)
        try:
            component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        except Exception:
            pass
        component.set_world_size(110.0)
        component.set_text_render_color(unreal.Color(255, 230, 130, 255))
    return actor


def spawn_target(actor_subsystem, label, location):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.TargetPoint,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    set_label(actor, label, hide_in_game=True)
    return actor


def spawn_spawner_or_target(actor_subsystem, label, location):
    spawner_class = unreal.load_object(
        None,
        "/Game/Variant_Combat/Blueprints/AI/BP_Combat_EnemySpawner.BP_Combat_EnemySpawner_C",
    )
    actor_class = spawner_class if spawner_class else unreal.TargetPoint
    actor = actor_subsystem.spawn_actor_from_class(
        actor_class,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    set_label(actor, label, hide_in_game=True)
    return actor


def remove_generated_external_actor_files():
    root = EXTERNAL_ACTOR_DIR.resolve()
    content_root = pathlib.Path(unreal.Paths.project_content_dir()).resolve()
    if content_root not in root.parents:
        raise RuntimeError(f"Unexpected external actor path: {root}")
    if not root.exists():
        return 0

    removed_count = 0
    for asset_path in root.rglob("*.uasset"):
        data = asset_path.read_bytes()
        if b"CWS_" in data and b"GeneratedBlockout" in data:
            asset_path.unlink()
            removed_count += 1
    if removed_count:
        unreal.log(f"CWS removed generated external actor files before rebuild: {removed_count}")
    return removed_count


def destroy_generated_actors(actor_subsystem):
    for actor in list(actor_subsystem.get_all_level_actors()):
        if not actor:
            continue
        label = actor.get_actor_label()
        folder = str(actor.get_folder_path())
        if label.startswith(PREFIX) or folder == FOLDER_PATH:
            actor_subsystem.destroy_actor(actor)


def summarize_generated_actors(actor_subsystem):
    labels = sorted(
        actor.get_actor_label()
        for actor in actor_subsystem.get_all_level_actors()
        if actor and actor.get_actor_label().startswith(PREFIX)
    )
    buckets = {
        "boundary": sum(1 for label in labels if label.startswith(PREFIX + "Boundary_")),
        "cover": sum(1 for label in labels if "Cover_" in label),
        "ring": sum(1 for label in labels if label.startswith(PREFIX + "CombatRing_")),
        "spawn": sum(1 for label in labels if label.startswith(PREFIX + "Spawn_")),
        "spawn_marker": sum(1 for label in labels if label.startswith(PREFIX + "SpawnMarker_")),
        "spawn_label": sum(1 for label in labels if label.startswith(PREFIX + "SpawnLabel_")),
        "item": sum(1 for label in labels if label.startswith(PREFIX + "Item_")),
        "item_label": sum(1 for label in labels if label.startswith(PREFIX + "ItemLabel_")),
    }
    summary = ", ".join(f"{name}={count}" for name, count in buckets.items())
    unreal.log(f"CWS generated actor count: total={len(labels)}, {summary}")


def place_boundary(actor_subsystem, cube_mesh, dark_mat):
    edge = ARENA_HALF + (BOUNDARY_THICKNESS / 2.0)
    wall_span = ARENA_SIZE + BOUNDARY_THICKNESS
    spawn_mesh(actor_subsystem, cube_mesh, dark_mat, PREFIX + "Boundary_North", (0, edge, 120), (wall_span, 80, 240))
    spawn_mesh(actor_subsystem, cube_mesh, dark_mat, PREFIX + "Boundary_South", (0, -edge, 120), (wall_span, 80, 240))
    spawn_mesh(actor_subsystem, cube_mesh, dark_mat, PREFIX + "Boundary_East", (edge, 0, 120), (80, wall_span, 240))
    spawn_mesh(actor_subsystem, cube_mesh, dark_mat, PREFIX + "Boundary_West", (-edge, 0, 120), (80, wall_span, 240))


def place_cover(actor_subsystem, cube_mesh, chamfer_mesh, dark_mat, color_mat):
    low_cover = [
        (-850, 450, 30, 25),
        (-450, 850, -30, -20),
        (450, 850, 30, 20),
        (850, 450, -30, -25),
        (850, -450, 30, 25),
        (450, -850, -30, -20),
        (-450, -850, 30, 20),
        (-850, -450, -30, -25),
    ]
    for index, (x, y, yaw, pitch) in enumerate(low_cover, start=1):
        spawn_mesh(
            actor_subsystem,
            chamfer_mesh,
            color_mat,
            f"{PREFIX}LowCover_{index:02d}",
            (x, y, 60),
            (360, 95, 130),
            (0.0, pitch, yaw),
        )

    high_cover = [
        (-1200, 0, 90),
        (1200, 0, 90),
        (0, 1120, 0),
        (0, -1120, 0),
    ]
    for index, (x, y, yaw) in enumerate(high_cover, start=1):
        spawn_mesh(
            actor_subsystem,
            cube_mesh,
            dark_mat,
            f"{PREFIX}HighCover_{index:02d}",
            (x, y, 90),
            (320, 120, 180),
            (0.0, 0.0, yaw),
        )


def place_spawn_points(actor_subsystem, cylinder_mesh, color_mat):
    spawn_points = [
        ("North", "N", (0, 1900, 70)),
        ("South", "S", (0, -1900, 70)),
        ("East", "E", (1900, 0, 70)),
        ("West", "W", (-1900, 0, 70)),
        ("NorthEast", "NE", (1350, 1350, 70)),
        ("SouthWest", "SW", (-1350, -1350, 70)),
        ("NorthWest", "NW", (-1350, 1350, 70)),
        ("SouthEast", "SE", (1350, -1350, 70)),
    ]
    for direction, short_name, location in spawn_points:
        spawn_spawner_or_target(actor_subsystem, f"{PREFIX}Spawn_{direction}", location)
        spawn_cylinder(
            actor_subsystem,
            cylinder_mesh,
            color_mat,
            f"{PREFIX}SpawnMarker_{direction}",
            location,
            90,
            140,
            hide_in_game=True,
            editor_only=True,
        )
        spawn_text(
            actor_subsystem,
            f"{PREFIX}SpawnLabel_{direction}",
            short_name,
            (location[0], location[1], location[2] + 140),
        )


def place_items(actor_subsystem, cube_mesh, cylinder_mesh, color_mat):
    items = [
        ("Ammo_West", (-650, 0, 55), (120, 120, 110)),
        ("Ammo_East", (650, 0, 55), (120, 120, 110)),
        ("Health_NorthEast", (620, 620, 40), (80, 80, 80)),
        ("Health_SouthWest", (-620, -620, 40), (80, 80, 80)),
    ]
    for name, location, size in items:
        mesh = cube_mesh if name.startswith("Ammo") else cylinder_mesh
        if mesh == cylinder_mesh:
            spawn_cylinder(actor_subsystem, mesh, color_mat, f"{PREFIX}Item_{name}", location, 45, 80)
        else:
            spawn_mesh(actor_subsystem, mesh, color_mat, f"{PREFIX}Item_{name}", location, size)
        spawn_text(actor_subsystem, f"{PREFIX}ItemLabel_{name}", name.replace("_", " "), (location[0], location[1], 175))


def place_navigation_and_starts(actor_subsystem):
    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(0.0, -250.0, 90.0),
        unreal.Rotator(0.0, 90.0, 0.0),
    )
    set_label(player_start, PREFIX + "PlayerStart")

    spawn_target(actor_subsystem, PREFIX + "BossSpawn_Center", (0, 0, 90))
    spawn_text(actor_subsystem, PREFIX + "BossSpawn_Label", "BOSS", (0, 0, 260))

    nav_volume = actor_subsystem.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume,
        unreal.Vector(0.0, 0.0, 180.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    set_label(nav_volume, PREFIX + "NavMeshBounds")
    nav_volume.set_actor_scale3d(unreal.Vector(44.0, 44.0, 4.0))


def main():
    remove_generated_external_actor_files()

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")

    cube_mesh = load_asset(CUBE_PATH)
    cylinder_mesh = load_asset(CYLINDER_PATH)
    chamfer_mesh = load_asset(CHAMFER_CUBE_PATH)
    floor_mat = load_asset(FLOOR_MAT_PATH)
    dark_mat = load_asset(DARK_MAT_PATH)
    color_mat = load_asset(COLOR_MAT_PATH)

    destroy_generated_actors(actor_subsystem)

    spawn_mesh(
        actor_subsystem,
        cube_mesh,
        floor_mat,
        PREFIX + "ArenaFloor_4000x4000",
        (0, 0, -5),
        (ARENA_SIZE, ARENA_SIZE, 10),
    )
    spawn_cylinder(actor_subsystem, cylinder_mesh, color_mat, PREFIX + "CenterSafeZone_R300", (0, 0, 5), 300, 10)

    for index in range(16):
        angle = (math.tau / 16.0) * index
        x = math.cos(angle) * 800.0
        y = math.sin(angle) * 800.0
        yaw = math.degrees(angle)
        spawn_mesh(
            actor_subsystem,
            cube_mesh,
            dark_mat,
            f"{PREFIX}CombatRing_{index + 1:02d}",
            (x, y, 8),
            (220, 24, 16),
            (0.0, 0.0, yaw),
        )

    place_boundary(actor_subsystem, cube_mesh, dark_mat)
    place_cover(actor_subsystem, cube_mesh, chamfer_mesh, dark_mat, color_mat)
    place_spawn_points(actor_subsystem, cylinder_mesh, color_mat)
    place_items(actor_subsystem, cube_mesh, cylinder_mesh, color_mat)
    place_navigation_and_starts(actor_subsystem)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    summarize_generated_actors(actor_subsystem)
    unreal.log("CWS blockout rebuilt in /Game/Variant_Combat/Lvl_Combat")


if __name__ == "__main__":
    main()
