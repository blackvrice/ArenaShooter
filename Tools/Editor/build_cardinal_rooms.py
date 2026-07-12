import json
import os
import pathlib

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
CUBE_PATH = "/Engine/BasicShapes/Cube"
ROOM_FOLDER_ROOT = "LevelGeometry/CardinalRooms"
ROOM_PREFIX = "Room_"
ROOM_TAG = unreal.Name("CardinalRoomGenerated")
BLOCKOUT_TAG = unreal.Name("ManualBlockout")
CONTENT_DIR = pathlib.Path(unreal.Paths.project_content_dir()).resolve()

FLOOR_SURFACE_Z = 350.0
ROOM_TOP_Z = 875.0
WALL_HEIGHT = ROOM_TOP_Z - FLOOR_SURFACE_Z
WALL_CENTER_Z = FLOOR_SURFACE_Z + (WALL_HEIGHT / 2.0)
WALL_THICKNESS = 100.0
DOOR_WIDTH = 700.0

SOURCE_BLOCKS = {
    "North": (0.0, 4550.0, 525.0),
    "South": (0.0, -4550.0, 525.0),
    "East": (4550.0, 0.0, 525.0),
    "West": (-4550.0, 0.0, 525.0),
}


def room_piece_specs():
    front_segment = (2000.0 - DOOR_WIDTH) / 2.0
    front_offset = (DOOR_WIDTH / 2.0) + (front_segment / 2.0)

    return [
        ("Room_North_Side_West", "North", (-950.0, 4550.0, WALL_CENTER_Z), (100.0, 1000.0, WALL_HEIGHT)),
        ("Room_North_Side_East", "North", (950.0, 4550.0, WALL_CENTER_Z), (100.0, 1000.0, WALL_HEIGHT)),
        ("Room_North_Front_West", "North", (-front_offset, 4050.0, WALL_CENTER_Z), (front_segment, 100.0, WALL_HEIGHT)),
        ("Room_North_Front_East", "North", (front_offset, 4050.0, WALL_CENTER_Z), (front_segment, 100.0, WALL_HEIGHT)),
        ("Room_South_Side_West", "South", (-950.0, -4550.0, WALL_CENTER_Z), (100.0, 1000.0, WALL_HEIGHT)),
        ("Room_South_Side_East", "South", (950.0, -4550.0, WALL_CENTER_Z), (100.0, 1000.0, WALL_HEIGHT)),
        ("Room_South_Front_West", "South", (-front_offset, -4050.0, WALL_CENTER_Z), (front_segment, 100.0, WALL_HEIGHT)),
        ("Room_South_Front_East", "South", (front_offset, -4050.0, WALL_CENTER_Z), (front_segment, 100.0, WALL_HEIGHT)),
        ("Room_East_Side_North", "East", (4550.0, 950.0, WALL_CENTER_Z), (1000.0, 100.0, WALL_HEIGHT)),
        ("Room_East_Side_South", "East", (4550.0, -950.0, WALL_CENTER_Z), (1000.0, 100.0, WALL_HEIGHT)),
        ("Room_East_Front_North", "East", (4050.0, front_offset, WALL_CENTER_Z), (100.0, front_segment, WALL_HEIGHT)),
        ("Room_East_Front_South", "East", (4050.0, -front_offset, WALL_CENTER_Z), (100.0, front_segment, WALL_HEIGHT)),
        ("Room_West_Side_North", "West", (-4550.0, 950.0, WALL_CENTER_Z), (1000.0, 100.0, WALL_HEIGHT)),
        ("Room_West_Side_South", "West", (-4550.0, -950.0, WALL_CENTER_Z), (1000.0, 100.0, WALL_HEIGHT)),
        ("Room_West_Front_North", "West", (-4050.0, front_offset, WALL_CENTER_Z), (100.0, front_segment, WALL_HEIGHT)),
        ("Room_West_Front_South", "West", (-4050.0, -front_offset, WALL_CENTER_Z), (100.0, front_segment, WALL_HEIGHT)),
    ]


def near(left, right, tolerance=0.1):
    return abs(left - right) <= tolerance


def location_matches(actor, expected):
    location = actor.get_actor_location()
    return near(location.x, expected[0]) and near(location.y, expected[1]) and near(location.z, expected[2])


def get_room_actors(actor_subsystem):
    return [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor and actor.get_actor_label().startswith(ROOM_PREFIX)
    ]


def find_source_blocks(actor_subsystem):
    found = {}
    for actor in actor_subsystem.get_all_level_actors():
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        component = actor.static_mesh_component
        mesh = component.static_mesh
        if not mesh or mesh.get_path_name() != "/Engine/BasicShapes/Cube.Cube":
            continue
        for direction, expected_location in SOURCE_BLOCKS.items():
            if location_matches(actor, expected_location):
                found[direction] = actor
    return found


def actor_package_filename(actor):
    package_name = actor.get_package().get_path_name()
    if not package_name.startswith("/Game/__ExternalActors__/Variant_Combat/Lvl_Combat/"):
        raise RuntimeError(f"Unexpected cardinal room actor package: {package_name}")
    filename = (CONTENT_DIR / (package_name[len("/Game/"):] + ".uasset")).resolve()
    if CONTENT_DIR not in filename.parents:
        raise RuntimeError(f"Unexpected cardinal room actor filename: {filename}")
    return filename


def generated_external_object_files():
    root = CONTENT_DIR / "__ExternalObjects__" / "Variant_Combat" / "Lvl_Combat"
    if not root.exists():
        return []
    matches = []
    for filename in root.rglob("*.uasset"):
        data = filename.read_bytes()
        if b"CardinalRoomGenerated" in data or b"Room_" in data:
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
    unreal.log(f"Cardinal room old package files removed: {len(removed)}")


def spawn_room_piece(actor_subsystem, cube_mesh, material, label, direction, location, size):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(label, mark_dirty=True)
    actor.set_folder_path(f"{ROOM_FOLDER_ROOT}/{direction}")
    actor.set_editor_property("tags", [ROOM_TAG, BLOCKOUT_TAG])
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))

    component = actor.static_mesh_component
    component.set_static_mesh(cube_mesh)
    if material:
        component.set_material(0, material)
    return actor


def validate_rooms(actor_subsystem):
    expected = {label: (direction, location, size) for label, direction, location, size in room_piece_specs()}
    room_actors = get_room_actors(actor_subsystem)
    actual = {actor.get_actor_label(): actor for actor in room_actors}

    missing = sorted(set(expected) - set(actual))
    unexpected = sorted(set(actual) - set(expected))
    if missing or unexpected:
        raise RuntimeError(f"Cardinal room actor mismatch: missing={missing}, unexpected={unexpected}")

    rows = []
    for label in sorted(expected):
        direction, location, size = expected[label]
        actor = actual[label]
        if not location_matches(actor, location):
            raise RuntimeError(f"Unexpected location for {label}: {actor.get_actor_location()}")

        scale = actor.get_actor_scale3d()
        expected_scale = (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0)
        if not (near(scale.x, expected_scale[0]) and near(scale.y, expected_scale[1]) and near(scale.z, expected_scale[2])):
            raise RuntimeError(f"Unexpected scale for {label}: {scale}")

        rows.append({
            "label": label,
            "direction": direction,
            "location": [location[0], location[1], location[2]],
            "size": [size[0], size[1], size[2]],
        })

    solid_blocks = find_source_blocks(actor_subsystem)
    if solid_blocks:
        raise RuntimeError(f"Solid cardinal blocks still exist: {sorted(solid_blocks)}")

    summary = {
        "rooms": 4,
        "pieces": len(rows),
        "door_width": DOOR_WIDTH,
        "wall_height": WALL_HEIGHT,
        "open_roof": True,
        "actors": rows,
    }
    unreal.log("CARDINAL_ROOMS_INSPECT=" + json.dumps(summary, separators=(",", ":")))
    return summary


def build_rooms(level_subsystem, actor_subsystem):
    cube_mesh = unreal.load_asset(CUBE_PATH)
    if not cube_mesh:
        raise RuntimeError(f"Could not load cube mesh: {CUBE_PATH}")

    existing_rooms = get_room_actors(actor_subsystem)
    source_blocks = find_source_blocks(actor_subsystem)
    if not existing_rooms and set(source_blocks) != set(SOURCE_BLOCKS):
        missing = sorted(set(SOURCE_BLOCKS) - set(source_blocks))
        raise RuntimeError(f"Could not find the four cardinal source blocks: missing={missing}")

    material = None
    material_sources = list(source_blocks.values()) + existing_rooms
    if material_sources:
        material = material_sources[0].static_mesh_component.get_material(0)

    old_package_files = [actor_package_filename(actor) for actor in existing_rooms]
    old_package_files.extend(actor_package_filename(actor) for actor in source_blocks.values())
    old_package_files.extend(generated_external_object_files())

    for actor in existing_rooms:
        actor_subsystem.destroy_actor(actor)
    for actor in source_blocks.values():
        actor_subsystem.destroy_actor(actor)

    for label, direction, location, size in room_piece_specs():
        spawn_room_piece(actor_subsystem, cube_mesh, material, label, direction, location, size)

    for root_name in ("__ExternalActors__", "__ExternalObjects__"):
        external_root = CONTENT_DIR / root_name / "Variant_Combat" / "Lvl_Combat"
        for bucket in "0123456789ABCDEF":
            (external_root / bucket).mkdir(parents=True, exist_ok=True)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    remove_old_package_files(old_package_files)
    validate_rooms(actor_subsystem)
    unreal.log("Cardinal rooms rebuilt in /Game/Variant_Combat/Lvl_Combat")


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")

    if os.environ.get("ARENA_CARDINAL_ROOMS_MODE", "build").lower() == "inspect":
        validate_rooms(actor_subsystem)
    else:
        build_rooms(level_subsystem, actor_subsystem)


if __name__ == "__main__":
    main()
