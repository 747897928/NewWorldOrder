import unreal


PATH = "/Game/UI/Menu/W_LyraMenuButton"
GRAPH = "ResetMaterials"
CANONICAL_BUTTON_PATH = "/Game/UI/Foundation/Buttons/W_MenuButton"


def log(message):
    print(f"[SessionUIMenuButtonRepair] {message}")


# 该脚本只记录早期迁入按钮的历史修复，不能再对重定向资产执行图表重建。
# 当前项目以 Foundation/W_MenuButton 为唯一实现；保留显式门禁，避免上下文丢失后误恢复重复按钮。
if unreal.EditorAssetLibrary.does_asset_exist(CANONICAL_BUTTON_PATH):
    raise RuntimeError(
        "Deprecated repair script: use the project's Foundation/W_MenuButton; "
        "do not rebuild the redirected W_LyraMenuButton asset"
    )

nodes = unreal.BlueprintService.get_nodes_in_graph(PATH, GRAPH)
entries = [node for node in nodes if node.node_type.endswith("K2Node_FunctionEntry")]
if len(entries) != 1:
    raise RuntimeError(f"Expected one ResetMaterials entry, got {len(entries)}")
entry_id = entries[0].node_id

# Lyra 原图把 Border MID 与 Font MID 同时接到了同一个 self pin；迁移后任一动态材质
# 在页面卸载阶段为空都会触发 Blueprint Runtime Error。保留函数入口，重建两条独立安全链。
for node in nodes:
    if node.node_id != entry_id and not unreal.BlueprintService.delete_node(PATH, GRAPH, node.node_id):
        raise RuntimeError(f"Failed to delete stale ResetMaterials node {node.node_id}")

build = unreal.BlueprintService.build_graph(
    PATH,
    GRAPH,
    [
        {"ref": "PressedAnimation", "type": "variable_get", "params": {"variable": "OnPressed"}},
        {"ref": "StopPressed", "type": "function_call",
         "params": {"class": "UserWidget", "function": "StopAnimation"}},
        {"ref": "ButtonBorder", "type": "variable_get", "params": {"variable": "ButtonBorder"}},
        {"ref": "GetBorderMID", "type": "function_call",
         "params": {"class": "Border", "function": "GetDynamicMaterial"}},
        {"ref": "IsBorderMIDValid", "type": "function_call",
         "params": {"class": "KismetSystemLibrary", "function": "IsValid"}},
        {"ref": "BorderValidBranch", "type": "branch", "params": {}},
        {"ref": "BorderHover", "type": "function_call",
         "params": {"class": "MaterialInstanceDynamic", "function": "SetScalarParameterValue"}},
        {"ref": "BorderPressed", "type": "function_call",
         "params": {"class": "MaterialInstanceDynamic", "function": "SetScalarParameterValue"}},
        {"ref": "ButtonText", "type": "variable_get", "params": {"variable": "ButtonTextBlock"}},
        {"ref": "GetFontMID", "type": "function_call",
         "params": {"class": "TextBlock", "function": "GetDynamicFontMaterial"}},
        {"ref": "IsFontMIDValid", "type": "function_call",
         "params": {"class": "KismetSystemLibrary", "function": "IsValid"}},
        {"ref": "FontValidBranch", "type": "branch", "params": {}},
        {"ref": "FontHover", "type": "function_call",
         "params": {"class": "MaterialInstanceDynamic", "function": "SetScalarParameterValue"}},
        {"ref": "FontPressed", "type": "function_call",
         "params": {"class": "MaterialInstanceDynamic", "function": "SetScalarParameterValue"}},
    ],
    [
        {"from_": f"{entry_id}.then", "to": "StopPressed.execute"},
        {"from_": "PressedAnimation.OnPressed", "to": "StopPressed.InAnimation"},
        {"from_": "StopPressed.then", "to": "GetBorderMID.execute"},
        {"from_": "ButtonBorder.ButtonBorder", "to": "GetBorderMID.self"},
        {"from_": "GetBorderMID.then", "to": "BorderValidBranch.execute"},
        {"from_": "GetBorderMID.ReturnValue", "to": "IsBorderMIDValid.Object"},
        {"from_": "IsBorderMIDValid.ReturnValue", "to": "BorderValidBranch.Condition"},
        {"from_": "BorderValidBranch.then", "to": "BorderHover.execute"},
        {"from_": "BorderValidBranch.else", "to": "GetFontMID.execute"},
        {"from_": "GetBorderMID.ReturnValue", "to": "BorderHover.self"},
        {"from_": "GetBorderMID.ReturnValue", "to": "BorderPressed.self"},
        {"from_": "BorderHover.then", "to": "BorderPressed.execute"},
        {"from_": "BorderPressed.then", "to": "GetFontMID.execute"},
        {"from_": "ButtonText.ButtonTextBlock", "to": "GetFontMID.self"},
        {"from_": "GetFontMID.then", "to": "FontValidBranch.execute"},
        {"from_": "GetFontMID.ReturnValue", "to": "IsFontMIDValid.Object"},
        {"from_": "IsFontMIDValid.ReturnValue", "to": "FontValidBranch.Condition"},
        {"from_": "FontValidBranch.then", "to": "FontHover.execute"},
        {"from_": "GetFontMID.ReturnValue", "to": "FontHover.self"},
        {"from_": "GetFontMID.ReturnValue", "to": "FontPressed.self"},
        {"from_": "FontHover.then", "to": "FontPressed.execute"},
    ],
    [
        {"node_ref": "BorderHover", "pin_name": "ParameterName", "value": "Hover_Animate"},
        {"node_ref": "BorderHover", "pin_name": "Value", "value": "0.0"},
        {"node_ref": "BorderPressed", "pin_name": "ParameterName", "value": "Pressed_Animate"},
        {"node_ref": "BorderPressed", "pin_name": "Value", "value": "0.0"},
        {"node_ref": "FontHover", "pin_name": "ParameterName", "value": "Hover_Animate"},
        {"node_ref": "FontHover", "pin_name": "Value", "value": "0.0"},
        {"node_ref": "FontPressed", "pin_name": "ParameterName", "value": "Pressed_Animate"},
        {"node_ref": "FontPressed", "pin_name": "Value", "value": "0.0"},
    ],
    True,
    False,
)
log(f"build success={build.success} errors={list(build.errors)} warnings={list(build.warnings)}")
if not build.success:
    raise RuntimeError(f"ResetMaterials rebuild failed: {list(build.errors)}")

unreal.BlueprintService.add_comment_around_nodes(
    PATH,
    GRAPH,
    "迁移后动态材质在卸载阶段可能为空；Border 与 Font 分别校验并复位，禁止共用 self 引脚。",
    list(build.ref_to_node_id.values()),
    40.0,
)
blueprint = unreal.load_asset(PATH)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
status = blueprint.get_editor_property("status")
log(f"status={status} nodes={len(unreal.BlueprintService.get_nodes_in_graph(PATH, GRAPH))}")
if status != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError("W_LyraMenuButton did not compile after ResetMaterials repair")
