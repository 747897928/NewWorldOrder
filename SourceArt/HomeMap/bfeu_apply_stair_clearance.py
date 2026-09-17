import bpy
import json
import os


SOURCE_FILE = "HomeMap_Master.blend"
SHIFT_X_M = 0.7


if os.path.basename(bpy.data.filepath) != SOURCE_FILE:
    raise RuntimeError(
        f"Refusing to edit unexpected Blender file: {bpy.data.filepath!r}; "
        f"expected {SOURCE_FILE!r}"
    )


def is_stair_assembly(name):
    return (
        name in {"Gallery_Stair", "Gallery_Stair_Landing"}
        or name.startswith("Stair_Rail_")
        or name.startswith("Stair_Post_")
    )


def is_rear_guard(name):
    return (
        name in {"Gallery_Rear_Guard", "Gallery_Rear_Guard_Cap"}
        or name.startswith("Gallery_Guard_Post_")
    )


targets = [obj for obj in bpy.data.objects if is_stair_assembly(obj.name) or is_rear_guard(obj.name)]
if not targets:
    raise RuntimeError("No staircase or rear-guard objects matched the guarded target rules")

for obj in targets:
    if obj.parent is not None:
        raise RuntimeError(f"Refusing to shift parented object {obj.name!r}; inspect hierarchy first")

before = {
    obj.name: {
        "group": "stair_assembly" if is_stair_assembly(obj.name) else "rear_guard",
        "location_m": [round(float(v), 6) for v in obj.location],
    }
    for obj in targets
}

for obj in targets:
    obj.location.x -= SHIFT_X_M

after = {
    obj.name: [round(float(v), 6) for v in obj.location]
    for obj in targets
}

bpy.ops.wm.save_as_mainfile(filepath=bpy.data.filepath)

print(
    json.dumps(
        {
            "source_file": bpy.data.filepath,
            "shift_x_m": -SHIFT_X_M,
            "target_count": len(targets),
            "before": before,
            "after": after,
        },
        ensure_ascii=False,
        indent=2,
    )
)
