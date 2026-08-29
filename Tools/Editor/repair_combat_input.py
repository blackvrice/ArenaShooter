import json
import os

import unreal


MAPPING_CONTEXT_PATH = "/Game/Variant_Combat/Input/IMC_Combat"


def mapping_summary(mapping):
    action = mapping.get_editor_property("action")
    key = mapping.get_editor_property("key")
    return {
        "action": action.get_path_name() if action else None,
        "key": str(key),
    }


def main():
    mapping_context = unreal.load_asset(MAPPING_CONTEXT_PATH)
    if not mapping_context:
        raise RuntimeError(f"Unable to load {MAPPING_CONTEXT_PATH}")

    mappings = list(mapping_context.get_editor_property("mappings"))
    valid_mappings = []
    invalid_mappings = []

    for mapping in mappings:
        if mapping.get_editor_property("action"):
            valid_mappings.append(mapping)
        else:
            invalid_mappings.append(mapping_summary(mapping))

    inspect_only = os.environ.get("ARENA_COMBAT_INPUT_MODE") == "inspect"
    if invalid_mappings and not inspect_only:
        mapping_context.set_editor_property("mappings", valid_mappings)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mapping_context, only_if_is_dirty=False):
            raise RuntimeError(f"Unable to save {MAPPING_CONTEXT_PATH}")

    result = {
        "asset": MAPPING_CONTEXT_PATH,
        "mode": "inspect" if inspect_only else "repair",
        "mapping_count": len(mappings),
        "valid_mapping_count": len(valid_mappings),
        "removed_mapping_count": 0 if inspect_only else len(invalid_mappings),
        "invalid_mappings": invalid_mappings,
    }
    unreal.log(f"COMBAT_INPUT_REPAIR={json.dumps(result, sort_keys=True)}")


if __name__ == "__main__":
    main()
