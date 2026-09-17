import unreal


TARGETS = {
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Action": {"EventGraph": ("Set Button Text",)},
    "/Game/UI/Settings/Editors/W_SettingsListEntry_SubCollection": {"EventGraph": ("Set Button Text",)},
    "/Game/UI/Settings/W_LyraSettingScreen": {
        "RegisterTopLevelTab": ("Register Dynamic Tab", "Make <unknown struct>")},
}

FRONT_END = "/Game/UI/FrontEnd/W_FrontEnd"


def log(message):
    unreal.log(f"[SessionUISettingsRefresh] {message}")
    print(f"[SessionUISettingsRefresh] {message}")


for path, graphs in TARGETS.items():
    for graph_name, title_fragments in graphs.items():
        nodes = unreal.BlueprintService.get_nodes_in_graph(path, graph_name)
        log(f"Before {path}:{graph_name}")
        for node in nodes:
            log(f"  {node.node_id}|{node.node_type}|{node.node_title}|{list(node.pin_names)}")
            if any(fragment in node.node_title for fragment in title_fragments):
                refreshed = unreal.BlueprintService.refresh_node(
                    path, graph_name, node.node_id, False)
                log(f"  refreshed={refreshed} id={node.node_id} title={node.node_title}")

    blueprint = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    log(f"After {path} parent={unreal.BlueprintService.get_parent_class(path)} "
        f"status={blueprint.get_editor_property('status')}")

# 读取上一轮已创建的 Options 事件/函数节点；仅审计，不重复创建。
for node in unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph"):
    if "OptionsButton" in node.node_title or "Settings" in node.node_title:
        log(f"FrontEnd node={node.node_id}|{node.node_type}|{node.node_title}|{list(node.pin_names)}")
log(f"FrontEnd SettingsScreenClass="
    f"{unreal.BlueprintService.get_property(FRONT_END, 'SettingsScreenClass')}")
