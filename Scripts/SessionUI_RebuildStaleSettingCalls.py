import unreal


FRONT_END = "/Game/UI/FrontEnd/W_FrontEnd"


def log(message):
    unreal.log(f"[SessionUIStaleCallRepair] {message}")
    print(f"[SessionUIStaleCallRepair] {message}")


def compile_twice(path):
    blueprint = unreal.load_asset(path)
    if blueprint is None:
        raise RuntimeError(f"Cannot load {path}")
    for _ in range(2):
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    status = blueprint.get_editor_property("status")
    log(f"Compile {path} status={status}")
    if status != unreal.BlueprintStatus.BS_UP_TO_DATE:
        raise RuntimeError(f"Blueprint still has compile errors: {path}")


def rebuild_button_text_call(path, cast_id, text_source_id, old_call_id):
    graph = "EventGraph"
    if not unreal.BlueprintService.delete_node(path, graph, old_call_id):
        raise RuntimeError(f"Cannot delete stale SetButtonText node in {path}")
    build = unreal.BlueprintService.build_graph(
        path,
        graph,
        [{"ref": "SetButtonTextCurrent", "type": "function_call",
          "params": {"class": "LyraButtonBase", "function": "SetButtonText"}}],
        [
            {"from_": f"{cast_id}.then", "to": "SetButtonTextCurrent.execute"},
            {"from_": f"{cast_id}.AsW Lyra Menu Button", "to": "SetButtonTextCurrent.self"},
            {"from_": f"{text_source_id}.OutputPin", "to": "SetButtonTextCurrent.InText"},
        ],
        [],
        True,
        False)
    log(f"Rebuild SetButtonText {path} success={build.success} errors={list(build.errors)}")
    if not build.success:
        raise RuntimeError(f"SetButtonText rebuild failed in {path}: {list(build.errors)}")
    compile_twice(path)


rebuild_button_text_call(
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Action",
    "FF2D2748429D41589365DDBD4964C749",
    "CB67E98C452B78561F9C5193A83E6C8D",
    "4B79D3B444F4868A19FFBDAF8CC1BF15")

rebuild_button_text_call(
    "/Game/UI/Settings/Editors/W_SettingsListEntry_SubCollection",
    "2B3479E34895671C9BE2FF9CDA4D4231",
    "E1A8D22046B5E505B633D09B99CD6B51",
    "A2D03119460B03D0130FB2BA42926047")

# 迁移资产的两个节点仍指向 /Script/LyraGame 的函数和结构；重建为项目模块同名类型。
settings_path = "/Game/UI/Settings/W_LyraSettingScreen"
settings_graph = "RegisterTopLevelTab"
for node_id in (
        "D59680F242BDE087F2E32D88C6AACD08",
        "3529F0D34982CED3903909BD18C33CD7"):
    if not unreal.BlueprintService.delete_node(settings_path, settings_graph, node_id):
        raise RuntimeError(f"Cannot delete stale settings tab node {node_id}")

tab_build = unreal.BlueprintService.build_graph(
    settings_path,
    settings_graph,
    [
        {"ref": "RegisterDynamicTabCurrent", "type": "function_call",
         "params": {"class": "LyraTabListWidgetBase", "function": "RegisterDynamicTab"}},
        {"ref": "MakeTabDescriptorCurrent", "type": "make_struct",
         "params": {"struct": "/Script/NewWorldOrder.LyraTabDescriptor"}},
    ],
    [
        {"from_": "E9EA12C742237E507CE99B8A41040366.then",
         "to": "RegisterDynamicTabCurrent.execute"},
        {"from_": "9906B1604BDC0273512C44AB9BF869E5.TopSettingsTabs",
         "to": "RegisterDynamicTabCurrent.self"},
        {"from_": "MakeTabDescriptorCurrent.LyraTabDescriptor",
         "to": "RegisterDynamicTabCurrent.TabDescriptor"},
        {"from_": "C6E072204846086D99D1469452216879.SettingDevName",
         "to": "MakeTabDescriptorCurrent.TabId"},
        {"from_": "17562A804C508728FA0563B834E6827F.ReturnValue",
         "to": "MakeTabDescriptorCurrent.bHidden"},
        {"from_": "5B43C57E4A7F9E7719EF30A2CC020637.ReturnValue",
         "to": "MakeTabDescriptorCurrent.TabText"},
    ],
    [],
    True,
    False)
log(f"Rebuild tabs success={tab_build.success} errors={list(tab_build.errors)}")
if not tab_build.success:
    raise RuntimeError(f"Settings tab rebuild failed: {list(tab_build.errors)}")
compile_twice(settings_path)

# 上一轮因错误 Widget 中断在保存前；这里一次性建立真正的 ComponentBoundEvent 调用链。
event_id = unreal.BlueprintService.create_component_bound_event(
    FRONT_END, "EventGraph", "OptionsButton", "OnButtonBaseClicked", 850, 350)
if not event_id:
    raise RuntimeError("Cannot create OptionsButton bound event")

old_open_calls = [
    node for node in unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph")
    if node.node_type.endswith("K2Node_CallFunction") and
    node.node_title.replace(" ", "").lower() == "opensettingsscreen"
]
if old_open_calls:
    open_settings_id = old_open_calls[0].node_id
else:
    front_build = unreal.BlueprintService.build_graph(
        FRONT_END,
        "EventGraph",
        [{"ref": "OpenSettingsCurrent", "type": "function_call",
          "params": {"class": "ShootFrontEndScreen", "function": "OpenSettingsScreen"}}],
        [],
        [],
        True,
        False)
    if not front_build.success:
        raise RuntimeError(f"Cannot create OpenSettingsScreen call: {list(front_build.errors)}")
    open_settings_id = front_build.ref_to_node_id["OpenSettingsCurrent"]

unreal.BlueprintService.disconnect_pin(FRONT_END, "EventGraph", event_id, "then")
if not unreal.BlueprintService.connect_nodes(
        FRONT_END, "EventGraph", event_id, "then", open_settings_id, "execute"):
    raise RuntimeError("Cannot connect OptionsButton to OpenSettingsScreen")

unreal.BlueprintService.add_comment_node(
    FRONT_END,
    "EventGraph",
    "Options 使用 ShootFrontEndScreen.OpenSettingsScreen 推入当前 LocalPlayer 的 UI.Layer.Menu。"
    "SettingsScreenClass 在本蓝图 CDO 指向迁移后的 W_LyraSettingScreen。",
    760,
    250,
    720,
    360)
compile_twice(FRONT_END)

front_nodes = unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph")
event_node = next(node for node in front_nodes if node.node_id == event_id)
log(f"Options event={event_node.node_type}|{event_node.node_title}; "
    f"SettingsScreenClass={unreal.BlueprintService.get_property(FRONT_END, 'SettingsScreenClass')}")
