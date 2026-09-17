"""把已经验证的 HomeMap 楼梯导出规则写回 Blender 源文件。"""

import json
from pathlib import Path

import bpy


TARGETS = ("Gallery_Stair", "Gallery_Stair_Landing", "Stair_Rail_330", "Stair_Rail_530")
source_path = Path(bpy.data.filepath).resolve()
if source_path.name != "HomeMap_Master.blend":
    raise RuntimeError(f"为避免误写试点副本，当前文件不是 HomeMap_Master.blend: {source_path}")

changed = []
for name in TARGETS:
    obj = bpy.data.objects.get(name)
    if obj is None:
        raise RuntimeError(f"找不到 HomeMap 楼梯对象: {name}")

    before = {
        "rotate_to_zero_for_export": obj.bfu_rotate_to_zero_for_export,
        "auto_generate_collision": obj.bfu_auto_generate_collision,
        "collision_trace_flag": obj.bfu_collision_trace_flag,
    }
    # 统一由 UE Actor 保存场景旋转，避免旋转既进入 Static Mesh 又进入 Actor。
    obj.bfu_rotate_to_zero_for_export = True
    if name == "Gallery_Stair":
        # 楼梯不能用 BFEU/UE 默认自动凸包，否则角色会撞在整段凸包斜面上。
        obj.bfu_auto_generate_collision = False
        obj.bfu_collision_trace_flag = "CTF_UseComplexAsSimple"
    after = {
        "rotate_to_zero_for_export": obj.bfu_rotate_to_zero_for_export,
        "auto_generate_collision": obj.bfu_auto_generate_collision,
        "collision_trace_flag": obj.bfu_collision_trace_flag,
    }
    changed.append({"object": name, "before": before, "after": after})

bpy.ops.wm.save_as_mainfile(filepath=str(source_path))
print(json.dumps({"source": str(source_path), "changed": changed, "saved": True}, ensure_ascii=False, indent=2))
