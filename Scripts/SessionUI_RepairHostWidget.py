import unreal


PATH = "/Game/UI/Menu/Experiences/W_HostSessionScreen"
GRAPH = "EventGraph"


def log(message):
    unreal.log(f"[SessionUIRepair] {message}")
    print(f"[SessionUIRepair] {message}")


def delete_graph_nodes(graph_name, keep_types=(), keep_ids=()):
    deleted = 0
    for node in list(unreal.BlueprintService.get_nodes_in_graph(PATH, graph_name)):
        if node.node_id in keep_ids:
            continue
        if any(keep_type in node.node_type for keep_type in keep_types):
            continue
        if unreal.BlueprintService.delete_node(PATH, graph_name, node.node_id):
            deleted += 1
    return deleted


# 先保留两个原资产已经生成的 ComponentBoundEvent。W_MenuButton 的 OnClicked 是蓝图父类代理，
# VibeUE 无法从生成类重新反射创建，但原节点的运行时绑定是有效的。
old_event_nodes = unreal.BlueprintService.get_nodes_in_graph(PATH, GRAPH)
session_event = next(
    node.node_id for node in old_event_nodes
    if "On Clicked (SessionTypeButton)" in node.node_title)
experience_event = next(
    node.node_id for node in old_event_nodes
    if "On Experience Selected (ExperienceList)" in node.node_title)

# 旧 Lyra 图表直接调用 CommonSession.HostSession，并维护另一套 OnlineMode/Bots/SelectedExperience。
# C++ 父类已经统一到 ShootSessionCoordinatorSubsystem，必须删除旧 EventGraph，防止一次点击触发两条会话链。
deleted_event_nodes = delete_graph_nodes(GRAPH, keep_ids=(session_event, experience_event))
log(f"Deleted {deleted_event_nodes} legacy EventGraph nodes")

# 旧的 CreateHostingRequest 函数仍是资产成员，UE Python 没有安全的 RemoveFunctionGraph 接口。
# 保留函数签名以兼容资产序列化，但清空函数体并返回空值；运行时不再有任何调用点。
deleted_request_nodes = delete_graph_nodes(
    "CreateHostingRequest", ("K2Node_FunctionEntry", "K2Node_FunctionResult"))
request_nodes = unreal.BlueprintService.get_nodes_in_graph(PATH, "CreateHostingRequest")
entry = next(node for node in request_nodes if "FunctionEntry" in node.node_type)
result = next(node for node in request_nodes if "FunctionResult" in node.node_type)
unreal.BlueprintService.connect_nodes(
    PATH, "CreateHostingRequest", entry.node_id, "then", result.node_id, "execute")
log(f"Neutralized legacy CreateHostingRequest body ({deleted_request_nodes} nodes removed)")

# Designer 中保留 Bot 行，明确表达最终语义；AI 队友未实现前禁用，避免让玩家误以为开关已生效。
unreal.WidgetService.set_property(PATH, "BotsToggleButton", "bIsEnabled", "False")
unreal.WidgetService.set_property(PATH, "BotsToggleText", "Text", "AI Teammates (Coming Later)")
unreal.WidgetService.set_property(PATH, "SessionTypeText", "Text", "Local")

nodes = [
    {"ref": "IsOnlineForToggle", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "IsOnlineMode"}},
    {"ref": "NotOnline", "type": "function_call",
     "params": {"class": "KismetMathLibrary", "function": "Not_PreBool"}},
    {"ref": "SetOnline", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "SetOnlineMode"}},
    {"ref": "SelectTile", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "SelectExperience"}},
    {"ref": "HostSelected", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "HostSelectedExperience"}},
    {"ref": "CatalogChanged", "type": "event",
     "params": {"event": "BP_OnExperienceCatalogChanged"}},
    {"ref": "SelectSuggested", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "SelectExperience"}},
    {"ref": "OptionsChanged", "type": "event",
     "params": {"event": "BP_OnExpeditionOptionsChanged"}},
    {"ref": "IsOnlineForText", "type": "function_call",
     "params": {"class": "ShootHostSessionScreen", "function": "IsOnlineMode"}},
    {"ref": "ModeBranch", "type": "branch", "params": {}},
    {"ref": "ModeText", "type": "variable_get",
     "params": {"variable": "SessionTypeText"}},
    {"ref": "SetOnlineText", "type": "function_call",
     "params": {"class": "TextBlock", "function": "SetText"}},
    {"ref": "SetLocalText", "type": "function_call",
     "params": {"class": "TextBlock", "function": "SetText"}},
    {"ref": "OnlineLiteral", "type": "function_call",
     "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},
    {"ref": "LocalLiteral", "type": "function_call",
     "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},
]

connections = [
    {"from_": f"{session_event}.then", "to": "SetOnline.execute"},
    {"from_": "IsOnlineForToggle.ReturnValue", "to": "NotOnline.A"},
    {"from_": "NotOnline.ReturnValue", "to": "SetOnline.bOnline"},

    {"from_": f"{experience_event}.then", "to": "SelectTile.execute"},
    {"from_": f"{experience_event}.ExperienceDef", "to": "SelectTile.Experience"},
    {"from_": "SelectTile.then", "to": "HostSelected.execute"},

    {"from_": "CatalogChanged.then", "to": "SelectSuggested.execute"},
    {"from_": "CatalogChanged.SuggestedSelection", "to": "SelectSuggested.Experience"},

    {"from_": "OptionsChanged.then", "to": "ModeBranch.execute"},
    {"from_": "IsOnlineForText.ReturnValue", "to": "ModeBranch.Condition"},
    {"from_": "ModeBranch.then", "to": "SetOnlineText.execute"},
    {"from_": "ModeBranch.else", "to": "SetLocalText.execute"},
    {"from_": "ModeText.SessionTypeText", "to": "SetOnlineText.self"},
    {"from_": "ModeText.SessionTypeText", "to": "SetLocalText.self"},
    {"from_": "OnlineLiteral.ReturnValue", "to": "SetOnlineText.InText"},
    {"from_": "LocalLiteral.ReturnValue", "to": "SetLocalText.InText"},
]

defaults = [
    {"node_ref": "OnlineLiteral", "pin_name": "InString", "value": "Online"},
    {"node_ref": "LocalLiteral", "pin_name": "InString", "value": "Local"},
]

build = unreal.BlueprintService.build_graph(
    PATH, GRAPH, nodes, connections, defaults, True, False)
log(f"BuildGraph success={build.success} created={build.nodes_created} "
    f"failed={build.nodes_failed} connections={build.connections_made} "
    f"connection_failures={build.connections_failed} defaults={build.defaults_set}/"
    f"{build.defaults_failed} errors={list(build.errors)} warnings={list(build.warnings)}")
if not build.success:
    raise RuntimeError(f"Host Widget graph rebuild failed: {list(build.errors)}")

unreal.BlueprintService.add_comment_node(
    PATH, GRAPH,
    "会话与副本状态只走 ShootHostSessionScreen / ShootSessionCoordinatorSubsystem。"
    "点击 Tile：先选择 Experience，再由统一入口创建 Local 或 Online Lobby 会话。",
    -100, -180, 1150, 1050)

blueprint = unreal.load_asset(PATH)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
log(f"Compile status={blueprint.get_editor_property('status')}")
