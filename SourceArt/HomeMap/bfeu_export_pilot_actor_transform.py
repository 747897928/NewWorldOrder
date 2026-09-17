import bpy
import json
import os
from pathlib import Path


# 试点第二种规则：Static Mesh 导出时清零对象旋转，UE Actor 承担 Copy Transform 的旋转。
TARGETS = ("Gallery_Stair", "Gallery_Stair_Landing", "Stair_Rail_330", "Stair_Rail_530")
blend_path = Path(bpy.data.filepath).resolve()
export_root = blend_path.parent / "BFEU_Pilot_Exports_ActorTransform"
export_root.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
objects = [bpy.data.objects[name] for name in TARGETS if bpy.data.objects.get(name)]
saved_selection = {obj.name: obj.select_get() for obj in scene.objects}
saved_active = bpy.context.view_layer.objects.active.name if bpy.context.view_layer.objects.active else None
scene_props = (
    "bfu_export_static_mesh_file_path",
    "bfu_unreal_import_module",
    "bfu_unreal_import_location",
    "bfu_export_selection_filter",
    "bfu_use_static_export",
    "bfu_use_static_collection_export",
    "bfu_use_skeletal_export",
    "bfu_use_animation_export",
    "bfu_use_alembic_export",
    "bfu_use_groom_simulation_export",
    "bfu_use_camera_export",
    "bfu_use_spline_export",
    "bfu_use_text_export_log",
    "bfu_use_text_import_asset_script",
    "bfu_use_text_import_sequence_script",
    "bfu_use_text_additional_data",
)
saved_scene_props = {prop: getattr(scene, prop) for prop in scene_props}
saved_object_props = {
    obj.name: {
        "bfu_export_type": obj.bfu_export_type,
        "bfu_rotate_to_zero_for_export": obj.bfu_rotate_to_zero_for_export,
        "bfu_auto_generate_collision": obj.bfu_auto_generate_collision,
        "bfu_collision_trace_flag": obj.bfu_collision_trace_flag,
    }
    for obj in objects
}

result = {
    "blend_file": str(blend_path),
    "export_root": str(export_root),
    "targets": [obj.name for obj in objects],
    "scene_unit_scale_length": scene.unit_settings.scale_length,
    "scene_unit_system": scene.unit_settings.system,
    "rotate_to_zero_for_export": True,
    # 楼梯要保留逐多边形碰撞作为简单碰撞，避免 BFEU/UE 默认自动凸包把整段台阶封成不可走的斜面。
    "stair_collision": {
        "auto_generate_collision": False,
        "collision_trace_flag": "CTF_UseComplexAsSimple",
    },
}

try:
    scene.bfu_export_static_mesh_file_path = str(export_root / "StaticMesh") + os.sep
    scene.bfu_unreal_import_module = "Game"
    scene.bfu_unreal_import_location = "Environment/HomeMap/Architecture/Hub/BFEU_Pilot_ActorTransform"
    scene.bfu_export_selection_filter = "only_object"
    scene.bfu_use_static_export = True
    scene.bfu_use_static_collection_export = False
    scene.bfu_use_skeletal_export = False
    scene.bfu_use_animation_export = False
    scene.bfu_use_alembic_export = False
    scene.bfu_use_groom_simulation_export = False
    scene.bfu_use_camera_export = False
    scene.bfu_use_spline_export = False
    scene.bfu_use_text_export_log = True
    scene.bfu_use_text_import_asset_script = True
    scene.bfu_use_text_import_sequence_script = False
    scene.bfu_use_text_additional_data = True

    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.bfu_export_type = "export_self_only"
        obj.bfu_rotate_to_zero_for_export = True
        if obj.name == "Gallery_Stair":
            obj.bfu_auto_generate_collision = False
            obj.bfu_collision_trace_flag = "CTF_UseComplexAsSimple"
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]

    result["operator_result"] = str(bpy.ops.object.exportforunreal())
finally:
    for prop, value in saved_scene_props.items():
        setattr(scene, prop, value)
    for obj in objects:
        obj.bfu_export_type = saved_object_props[obj.name]["bfu_export_type"]
        obj.bfu_rotate_to_zero_for_export = saved_object_props[obj.name]["bfu_rotate_to_zero_for_export"]
        obj.bfu_auto_generate_collision = saved_object_props[obj.name]["bfu_auto_generate_collision"]
        obj.bfu_collision_trace_flag = saved_object_props[obj.name]["bfu_collision_trace_flag"]
    for obj in scene.objects:
        obj.select_set(saved_selection.get(obj.name, False))
    bpy.context.view_layer.objects.active = bpy.data.objects.get(saved_active) if saved_active else None

files = []
if export_root.exists():
    for path in sorted(export_root.rglob("*")):
        if path.is_file():
            files.append({
                "relative_path": str(path.relative_to(export_root)),
                "size": path.stat().st_size,
            })
result["files"] = files
print(json.dumps(result, ensure_ascii=False, indent=2, default=str))
