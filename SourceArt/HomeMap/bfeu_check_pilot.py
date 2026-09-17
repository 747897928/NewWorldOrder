import bpy
import math
import json


# 只把四个楼梯对象临时交给 BFEU 检查；所有临时属性、选择和场景设置都会恢复，不保存源文件。
TARGETS = ("Gallery_Stair", "Gallery_Stair_Landing", "Stair_Rail_330", "Stair_Rail_530")


def transform_command_data(obj):
    from bl_ext.user_default.unrealengine_assets_exporter import bfu_utils
    from bl_ext.user_default.unrealengine_assets_exporter.bfu_object import bfu_object_write_paste_commands

    location = bfu_utils.get_object_location_vector_for_unreal(obj)
    scale = bfu_utils.get_object_scale_vector_for_unreal(obj)
    ok, rotation_command, message = bfu_object_write_paste_commands.get_object_rotation_for_unreal_script_command(obj)
    return {
        "location_command": "Location=(X=%.6f,Y=%.6f,Z=%.6f)" % (location.x, location.y, location.z),
        "rotation_command": rotation_command if ok else message,
        "scale_command": "Scale=(X=%.6f,Y=%.6f,Z=%.6f)" % (scale.x, scale.y, scale.z),
    }


scene = bpy.context.scene
objects = [bpy.data.objects[name] for name in TARGETS if bpy.data.objects.get(name)]
saved_selection = {obj.name: obj.select_get() for obj in scene.objects}
saved_active = bpy.context.view_layer.objects.active.name if bpy.context.view_layer.objects.active else None
saved_filter = scene.bfu_export_selection_filter
saved_export_types = {obj.name: obj.bfu_export_type for obj in objects}

result = {
    "scene": scene.name,
    "targets": [obj.name for obj in objects],
    "copy_transform_commands": {obj.name: transform_command_data(obj) for obj in objects},
}

try:
    scene.bfu_export_selection_filter = "only_object"
    for obj in objects:
        obj.bfu_export_type = "export_self_only"
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]

    from bl_ext.user_default.unrealengine_assets_exporter import bfu_cached_assets
    from bl_ext.user_default.unrealengine_assets_exporter.bfu_assets_manager.bfu_asset_manager_type import AssetToSearch, AssetDataSearchMode
    from bl_ext.user_default.unrealengine_assets_exporter.bfu_check_potential_error import bfu_check_list

    cache = bfu_cached_assets.bfu_cached_assets_blender_class.get_final_asset_cache()
    assets = cache.get_final_asset_list(AssetToSearch.ALL_ASSETS, AssetDataSearchMode.FULL, force_cache_update=True)
    result["asset_list"] = [
        {
            "name": asset.name,
            "type": str(asset.asset_type),
            "packages": [package.name for package in asset.asset_packages],
        }
        for asset in assets
    ]

    bfu_check_list.run_all_check()
    result["potential_errors"] = []
    for issue in scene.bfu_export_potential_errors:
        result["potential_errors"].append({
            "type": int(issue.type),
            "text": issue.text,
            "object": issue.object.name if issue.object else None,
            "correction": issue.correct_ref,
        })
finally:
    scene.bfu_export_selection_filter = saved_filter
    for obj in objects:
        obj.bfu_export_type = saved_export_types[obj.name]
    for obj in scene.objects:
        obj.select_set(saved_selection.get(obj.name, False))
    bpy.context.view_layer.objects.active = bpy.data.objects.get(saved_active) if saved_active else None

print(json.dumps(result, ensure_ascii=False, indent=2, default=str))
