import json
import os
import pathlib

import unreal


LEVEL_PATH = "/Game/Variant_Combat/Lvl_Combat"
CUBE_PATH = "/Engine/BasicShapes/Cube"
ROOM_FOLDER = "LevelGeometry/CentralRoom"
ROOM_PREFIX = "CentralRoom_"
ROOM_TAG = unreal.Name("CentralRoomGenerated")
BLOCKOUT_TAG = unreal.Name("ManualBlockout")
CONTENT_DIR = pathlib.Path(unreal.Paths.project_content_dir()).resolve()

FLOOR_SURFACE_Z = 350.0
ROOM_TOP_Z = 875.0
WALL_HEIGHT = ROOM_TOP_Z - FLOOR_SURFACE_Z
WALL_CENTER_Z = FLOOR_SURFACE_Z + (WALL_HEIGHT / 2.0)
ROOM_SIZE = 4000.0
ROOM_HALF = ROOM_SIZE / 2.0
WALL_THICKNESS = 100.0
DOOR_WIDTH = 1000.0
SOURCE_LOCATION = (0.0, 0.0, 525.0)
SOURCE_SCALE = (40.0, 40.0, 7.0)


def room_piece_specs():
    wall_segment = (ROOM_SIZE - DOOR_WIDTH) / 2.0
    segment_offset = (DOOR_WIDTH / 2.0) + (wall_segment / 2.0)
    return [
        ("CentralRoom_North_West", (-segment_offset, ROOM_HALF, WALL_CENTER_Z), (wall_segment, WALL_THICKNESS, WALL_HEIGHT)),
        ("CentralRoom_North_East", (segment_offset, ROOM_HALF, WALL_CENTER_Z), (wall_segment, WALL_THICKNESS, WALL_HEIGHT)),
        ("CentralRoom_South_West", (-segment_offset, -ROOM_HALF, WALL_CENTER_Z), (wall_segment, WALL_THICKNESS, WALL_HEIGHT)),
        ("CentralRoom_South_East", (segment_offset, -ROOM_HALF, WALL_CENTER_Z), (wall_segment, WALL_THICKNESS, WALL_HEIGHT)),
        ("CentralRoom_East_North", (ROOM_HALF, segment_offset, WALL_CENTER_Z), (WALL_THICKNESS, wall_segment, WALL_HEIGHT)),
        ("CentralRoom_East_South", (ROOM_HALF, -segment_offset, WALL_CENTER_Z), (WALL_THICKNESS, wall_segment, WALL_HEIGHT)),
        ("CentralRoom_West_North", (-ROOM_HALF, segment_offset, WALL_CENTER_Z), (WALL_THICKNESS, wall_segment, WALL_HEIGHT)),
        ("CentralRoom_West_South", (-ROOM_HALF, -segment_offset, WALL_CENTER_Z), (WALL_THICKNESS, wall_segment, WALL_HEIGHT)),
    ]


def near(left, right, tolerance=0.1):
    return abs(left - right) <= tolerance


def location_matches(actor, expected):
    location = actor.get_actor_location()
    return near(location.x, expected[0]) and near(location.y, expected[1]) and near(location.z, expected[2])


def scale_matches(actor, expected):
    scale = actor.get_actor_scale3d()
    return near(scale.x, expected[0]) and near(scale.y, expected[1]) and near(scale.z, expected[2])


def get_room_actors(actor_subsystem):
    return [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor and actor.get_actor_label().startswith(ROOM_PREFIX)
    ]


def find_source_block(actor_subsystem):
    matches = []
    for actor in actor_subsystem.get_all_level_actors():
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        mesh = actor.static_mesh_component.static_mesh
        if not mesh or mesh.get_path_name() != "/Engine/BasicShapes/Cube.Cube":
            continue
        if location_matches(actor, SOURCE_LOCATION) and scale_matches(actor, SOURCE_SCALE):
            matches.append(actor)
    if len(matches) > 1:
        raise RuntimeError(f"Found multiple central source blocks: {[actor.get_actor_label() for actor in matches]}")
    return matches[0] if matches else None


def actor_package_filename(actor):
    package_name = actor.get_package().get_path_name()
    if not package_name.startswith("/Game/__ExternalActors__/Variant_Combat/Lvl_Combat/"):
        raise RuntimeError(f"Unexpected central room actor package: {package_name}")
    filename = (CONTENT_DIR / (package_name[len("/Game/"):] + ".uasset")).resolve()
    if CONTENT_DIR not in filename.parents:
        raise RuntimeError(f"Unexpected central room actor filename: {filename}")
    return filename


def generated_external_object_files():
    root = CONTENT_DIR / "__ExternalObjects__" / "Variant_Combat" / "Lvl_Combat"
    if not root.exists():
        return []
    matches = []
    for filename in root.rglob("*.uasset"):
        data = filename.read_bytes()
        if b"CentralRoomGenerated" in data or b"CentralRoom_" in data:
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
    unreal.log(f"Central room old package files removed: {len(removed)}")


def spawn_room_piece(actor_subsystem, cube_mesh, material, label, location, size):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(label, mark_dirty=True)
    actor.set_folder_path(ROOM_FOLDER)
    actor.set_editor_property("tags", [ROOM_TAG, BLOCKOUT_TAG])
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    component = actor.static_mesh_component
    component.set_static_mesh(cube_mesh)
    if material:
        component.set_material(0, material)
    return actor


def validate_room(actor_subsystem):
    expected = {label: (location, size) for label, location, size in room_piece_specs()}
    actual = {actor.get_actor_label(): actor for actor in get_room_actors(actor_subsystem)}
    missing = sorted(set(expected) - set(actual))
    unexpected = sorted(set(actual) - set(expected))
    if missing or unexpected:
        raise RuntimeError(f"Central room actor mismatch: missing={missing}, unexpected={unexpected}")

    rows = []
    for label in sorted(expected):
        location, size = expected[label]
        actor = actual[label]
        if not location_matches(actor, location):
            raise RuntimeError(f"Unexpected location for {label}: {actor.get_actor_location()}")
        expected_scale = (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0)
        if not scale_matches(actor, expected_scale):
            raise RuntimeError(f"Unexpected scale for {label}: {actor.get_actor_scale3d()}")
        rows.append({
            "label": label,
            "location": [location[0], location[1], location[2]],
            "size": [size[0], size[1], size[2]],
        })

    if find_source_block(actor_subsystem):
        raise RuntimeError("Central solid source block still exists")

    summary = {
        "rooms": 1,
        "pieces": len(rows),
        "footprint": [ROOM_SIZE, ROOM_SIZE],
        "door_count": 4,
        "door_width": DOOR_WIDTH,
        "wall_height": WALL_HEIGHT,
        "open_roof": True,
        "actors": rows,
    }
    unreal.log("CENTRAL_ROOM_INSPECT=" + json.dumps(summary, separators=(",", ":")))
    return summary


def build_room(actor_subsystem):
    cube_mesh = unreal.load_asset(CUBE_PATH)
    if not cube_mesh:
        raise RuntimeError(f"Could not load cube mesh: {CUBE_PATH}")

    existing_rooms = get_room_actors(actor_subsystem)
    source_block = find_source_block(actor_subsystem)
    if not existing_rooms and not source_block:
        raise RuntimeError("Could not find the 4,000 x 4,000 central source block")

    material_sources = ([source_block] if source_block else []) + existing_rooms
    material = material_sources[0].static_mesh_component.get_material(0) if material_sources else None

    old_package_files = [actor_package_filename(actor) for actor in existing_rooms]
    if source_block:
        old_package_files.append(actor_package_filename(source_block))
    old_package_files.extend(generated_external_object_files())

    for actor in existing_rooms:
        actor_subsystem.destroy_actor(actor)
    if source_block:
        actor_subsystem.destroy_actor(source_block)

    for label, location, size in room_piece_specs():
        spawn_room_piece(actor_subsystem, cube_mesh, material, label, location, size)

    for root_name in ("__ExternalActors__", "__ExternalObjects__"):
        external_root = CONTENT_DIR / root_name / "Variant_Combat" / "Lvl_Combat"
        for bucket in "0123456789ABCDEF":
            (external_root / bucket).mkdir(parents=True, exist_ok=True)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    remove_old_package_files(old_package_files)
    validate_room(actor_subsystem)
    unreal.log("Central room rebuilt in /Game/Variant_Combat/Lvl_Combat")


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load level: {LEVEL_PATH}")
    if os.environ.get("ARENA_CENTRAL_ROOM_MODE", "build").lower() == "inspect":
        validate_room(actor_subsystem)
    else:
        build_room(actor_subsystem)


if __name__ == "__main__":
    main()
