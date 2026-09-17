import unreal


PATH = "/Game/UI/Menu/Experiences/W_HostSessionScreen"
GRAPH = "EventGraph"


def log(message):
    unreal.log(f"[SessionUIHostInteraction] {message}")
    print(f"[SessionUIHostInteraction] {message}")


def ensure_widget(widget_type, name, parent, variable=False):
    if unreal.WidgetService.widget_exists(PATH, name):
        return
    result = unreal.WidgetService.add_component(PATH, widget_type, name, parent, variable)
    if not result.success:
        raise RuntimeError(f"Cannot add {widget_type} {name}: {result.message}")


def set_property(name, prop, value, required=True):
    success = unreal.WidgetService.set_property(PATH, name, prop, value)
    if required and not success:
        raise RuntimeError(f"Cannot set {name}.{prop}={value}")


def set_asset_property(asset_path, name, prop, value, required=True):
    success = unreal.WidgetService.set_property(asset_path, name, prop, value)
    if required and not success:
        raise RuntimeError(f"Cannot set {asset_path}:{name}.{prop}={value}")


def find_node(title_contains, node_type_contains=""):
    for node in unreal.BlueprintService.get_nodes_in_graph(PATH, GRAPH):
        if title_contains.lower() in node.node_title.lower() and \
                node_type_contains in node.node_type:
            return node
    return None


def component_event(component_name):
    title = f"On Clicked ({component_name})"
    existing = find_node(title, "K2Node_ComponentBoundEvent")
    if existing:
        return existing.node_id
    event_id = unreal.BlueprintService.create_component_bound_event(
        PATH, GRAPH, component_name, "OnButtonBaseClicked", 80, 1400)
    if not event_id:
        raise RuntimeError(f"Cannot create click event for {component_name}")
    return event_id


# 固定布局留在 Widget Blueprint。Tile 只负责选择；创建、搜索、返回都有独立可见入口。
ensure_widget("Spacer", "ActionTopSpacer", "VerticalBox_0")
ensure_widget("Border", "ActionPanel", "VerticalBox_0")
ensure_widget("VerticalBox", "ActionPanelBox", "ActionPanel")
ensure_widget("HorizontalBox", "LocalPlayersRow", "ActionPanelBox")
ensure_widget("CommonTextBlock", "LocalPlayersLabel", "LocalPlayersRow")
ensure_widget("W_MenuButton", "RemoveLocalPlayerButton", "LocalPlayersRow", True)
ensure_widget("CommonTextBlock", "LocalPlayerCountText", "LocalPlayersRow", True)
ensure_widget("W_MenuButton", "AddLocalPlayerButton", "LocalPlayersRow", True)
ensure_widget("HorizontalBox", "JoinPolicyRow", "ActionPanelBox")
ensure_widget("CommonTextBlock", "JoinPolicyLabel", "JoinPolicyRow")
ensure_widget("W_MenuButton", "JoinPolicyButton", "JoinPolicyRow", True)
ensure_widget("CommonTextBlock", "JoinPolicyValueText", "JoinPolicyRow", True)
ensure_widget("CommonTextBlock", "SessionStatusText", "ActionPanelBox", True)
ensure_widget("HorizontalBox", "PrimaryActionsRow", "ActionPanelBox")
ensure_widget("W_MenuButton", "HostButton", "PrimaryActionsRow", True)
ensure_widget("W_MenuButton", "SessionBrowserButton", "PrimaryActionsRow", True)
ensure_widget("W_MenuButton", "CloseButton", "PrimaryActionsRow", True)

set_property("ActionTopSpacer", "Size", "(X=1.0,Y=8.0)", False)
set_property("ActionPanel", "Padding", "(Left=24,Top=10,Right=24,Bottom=10)", False)
set_property("ActionPanel", "BrushColor", "(R=0.20,G=0.33,B=0.46,A=0.98)", False)
action_panel_brush = unreal.WidgetService.get_brush(PATH, "RingBorder", "Brush")
if not unreal.WidgetService.set_brush(PATH, "ActionPanel", "Brush", action_panel_brush):
    raise RuntimeError("Cannot reuse the host screen ring material on ActionPanel")
# 原 Lyra 卡片高度为 384，在 Host 页追加操作区后会把固定按钮挤出 16:9 安全区。
# 这里仍由 Widget Blueprint 保存视觉参数，仅收紧本页面实际使用的卡片与选项垂直占用。
set_property("SizeBox_0", "Padding", "(Top=14,Bottom=14)", False)
set_asset_property("/Game/UI/Menu/W_LyraExperienceTileButton", "ButtonHeightSB", "HeightOverride", "280", False)
set_asset_property("/Game/UI/Menu/W_LyraExperienceTileButton", "TextBoxSB", "HeightOverride", "104", False)
set_asset_property("/Game/UI/Menu/W_LyraExperienceTileButton", "TextBoxSB", "MinDesiredHeight", "104", False)
for name, text in (
        ("LocalPlayersLabel", "LOCAL PLAYERS"),
        ("LocalPlayerCountText", "1"),
        ("JoinPolicyLabel", "MID-JOIN"),
        ("JoinPolicyValueText", "ALLOWED"),
        ("SessionStatusText", "Select an expedition, then create a room or enter locally.")):
    set_property(name, "Text", text)
    set_property(name, "ColorAndOpacity",
                 "(SpecifiedColor=(R=0.72,G=0.9,B=1.0,A=1.0),ColorUseRule=UseColor_Specified)",
                 False)
    set_property(name, "Justification", "Center", False)

for name, text, width in (
        ("RemoveLocalPlayerButton", "-", 96),
        ("AddLocalPlayerButton", "+", 96),
        ("JoinPolicyButton", "CHANGE", 192),
        ("HostButton", "CREATE / ENTER", 256),
        ("SessionBrowserButton", "SEARCH ROOMS", 256),
        ("CloseButton", "BACK", 160)):
    set_property(name, "ButtonText", text)
    set_property(name, "MinWidth", str(width), False)
    set_property(name, "MinHeight", "46", False)

# 迁移资产曾把 Tile 选择直接接到 Host；断开后玩家可以先阅读和选择，再明确创建房间。
if find_node("On Clicked (HostButton)", "K2Node_ComponentBoundEvent"):
    raise RuntimeError(
        "Host interaction graph is already rebuilt; use SessionUI_AuditExperienceCatalog.py to verify it")

select_node = find_node("Select Experience", "K2Node_CallFunction")
host_node = find_node("Host Selected Experience", "K2Node_CallFunction")
if not select_node or not host_node:
    raise RuntimeError("Existing selection/session nodes are missing")
unreal.BlueprintService.disconnect_pin(PATH, GRAPH, select_node.node_id, "then")

host_event = component_event("HostButton")
browser_event = component_event("SessionBrowserButton")
close_event = component_event("CloseButton")
add_event = component_event("AddLocalPlayerButton")
remove_event = component_event("RemoveLocalPlayerButton")
join_event = component_event("JoinPolicyButton")

options_event = find_node("Event On Expedition Options Changed", "K2Node_Event")
mode_branch = find_node("Branch", "K2Node_IfThenElse")
if not options_event or not mode_branch:
    raise RuntimeError("Options changed graph is missing")
unreal.BlueprintService.disconnect_pin(PATH, GRAPH, options_event.node_id, "then")

build = unreal.BlueprintService.build_graph(
    PATH,
    GRAPH,
    [
        {"ref": "OpenBrowser", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "OpenSessionBrowser"}},
        {"ref": "CloseScreen", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "CloseScreen"}},
        {"ref": "AddLocal", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "AddLocalPlayer"}},
        {"ref": "RemoveLocal", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "RemoveLocalPlayer"}},
        {"ref": "GetJoin", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "IsJoinInProgressAllowed"}},
        {"ref": "NotJoin", "type": "function_call",
         "params": {"class": "KismetMathLibrary", "function": "Not_PreBool"}},
        {"ref": "SetJoin", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "SetAllowJoinInProgress"}},
        {"ref": "JoinBranch", "type": "branch", "params": {}},
        {"ref": "JoinTextGet", "type": "variable_get",
         "params": {"variable": "JoinPolicyValueText"}},
        {"ref": "SetJoinAllowedText", "type": "function_call",
         "params": {"class": "TextBlock", "function": "SetText"}},
        {"ref": "SetJoinLockedText", "type": "function_call",
         "params": {"class": "TextBlock", "function": "SetText"}},
        {"ref": "JoinAllowedLiteral", "type": "function_call",
         "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},
        {"ref": "JoinLockedLiteral", "type": "function_call",
         "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},

        {"ref": "GetLocalCount", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "GetLocalPlayerCount"}},
        {"ref": "CountToText", "type": "function_call",
         "params": {"class": "KismetTextLibrary", "function": "Conv_IntToText"}},
        {"ref": "CountTextGet", "type": "variable_get",
         "params": {"variable": "LocalPlayerCountText"}},
        {"ref": "SetCountText", "type": "function_call",
         "params": {"class": "TextBlock", "function": "SetText"}},
        {"ref": "CanRemove", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "CanRemoveLocalPlayer"}},
        {"ref": "RemoveButtonGet", "type": "variable_get",
         "params": {"variable": "RemoveLocalPlayerButton"}},
        {"ref": "SetRemoveEnabled", "type": "function_call",
         "params": {"class": "Widget", "function": "SetIsEnabled"}},
        {"ref": "CanAdd", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "CanAddLocalPlayer"}},
        {"ref": "AddButtonGet", "type": "variable_get",
         "params": {"variable": "AddLocalPlayerButton"}},
        {"ref": "SetAddEnabled", "type": "function_call",
         "params": {"class": "Widget", "function": "SetIsEnabled"}},
        {"ref": "SupportsOnline", "type": "function_call",
         "params": {"class": "ShootHostSessionScreen", "function": "DoesSelectedExperienceSupportOnline"}},
        {"ref": "SessionButtonGet", "type": "variable_get",
         "params": {"variable": "SessionTypeButton"}},
        {"ref": "SetSessionEnabled", "type": "function_call",
         "params": {"class": "Widget", "function": "SetIsEnabled"}},

        {"ref": "SessionState", "type": "event",
         "params": {"event": "BP_OnExpeditionSessionStateChanged"}},
        {"ref": "StatusTextGet", "type": "variable_get",
         "params": {"variable": "SessionStatusText"}},
        {"ref": "SetStatusText", "type": "function_call",
         "params": {"class": "TextBlock", "function": "SetText"}},
    ],
    [
        {"from_": f"{host_event}.then", "to": f"{host_node.node_id}.execute"},
        {"from_": f"{browser_event}.then", "to": "OpenBrowser.execute"},
        {"from_": f"{close_event}.then", "to": "CloseScreen.execute"},
        {"from_": f"{add_event}.then", "to": "AddLocal.execute"},
        {"from_": f"{remove_event}.then", "to": "RemoveLocal.execute"},
        {"from_": f"{join_event}.then", "to": "SetJoin.execute"},
        {"from_": "GetJoin.ReturnValue", "to": "NotJoin.A"},
        {"from_": "NotJoin.ReturnValue", "to": "SetJoin.bAllowed"},
        {"from_": "SetJoin.then", "to": "JoinBranch.execute"},
        # SetJoin 执行后重新读取最终状态；复用 NotJoin 会再次求值并把显示文本反转。
        {"from_": "GetJoin.ReturnValue", "to": "JoinBranch.Condition"},
        {"from_": "JoinBranch.then", "to": "SetJoinAllowedText.execute"},
        {"from_": "JoinBranch.else", "to": "SetJoinLockedText.execute"},
        {"from_": "JoinTextGet.JoinPolicyValueText", "to": "SetJoinAllowedText.self"},
        {"from_": "JoinTextGet.JoinPolicyValueText", "to": "SetJoinLockedText.self"},
        {"from_": "JoinAllowedLiteral.ReturnValue", "to": "SetJoinAllowedText.InText"},
        {"from_": "JoinLockedLiteral.ReturnValue", "to": "SetJoinLockedText.InText"},

        {"from_": f"{options_event.node_id}.then", "to": "SetCountText.execute"},
        {"from_": "GetLocalCount.ReturnValue", "to": "CountToText.Value"},
        {"from_": "CountToText.ReturnValue", "to": "SetCountText.InText"},
        {"from_": "CountTextGet.LocalPlayerCountText", "to": "SetCountText.self"},
        {"from_": "SetCountText.then", "to": "SetRemoveEnabled.execute"},
        {"from_": "CanRemove.ReturnValue", "to": "SetRemoveEnabled.bInIsEnabled"},
        {"from_": "RemoveButtonGet.RemoveLocalPlayerButton", "to": "SetRemoveEnabled.self"},
        {"from_": "SetRemoveEnabled.then", "to": "SetAddEnabled.execute"},
        {"from_": "CanAdd.ReturnValue", "to": "SetAddEnabled.bInIsEnabled"},
        {"from_": "AddButtonGet.AddLocalPlayerButton", "to": "SetAddEnabled.self"},
        {"from_": "SetAddEnabled.then", "to": "SetSessionEnabled.execute"},
        {"from_": "SupportsOnline.ReturnValue", "to": "SetSessionEnabled.bInIsEnabled"},
        {"from_": "SessionButtonGet.SessionTypeButton", "to": "SetSessionEnabled.self"},
        {"from_": "SetSessionEnabled.then", "to": f"{mode_branch.node_id}.execute"},

        {"from_": "SessionState.then", "to": "SetStatusText.execute"},
        {"from_": "SessionState.Status", "to": "SetStatusText.InText"},
        {"from_": "StatusTextGet.SessionStatusText", "to": "SetStatusText.self"},
    ],
    [
        {"node_ref": "JoinAllowedLiteral", "pin_name": "InString", "value": "ALLOWED"},
        {"node_ref": "JoinLockedLiteral", "pin_name": "InString", "value": "LOCKED AFTER START"},
    ],
    True,
    False)

log(f"Build success={build.success} created={build.nodes_created} failed={build.nodes_failed} "
    f"connections={build.connections_made}/{build.connections_failed} errors={list(build.errors)}")
if not build.success:
    raise RuntimeError(f"Host interaction graph failed: {list(build.errors)}")

unreal.BlueprintService.add_comment_node(
    PATH, GRAPH,
    "Tile 只选择副本；HostButton 才创建本地游戏或 Listen Server。Search 进入会话浏览器，"
    "Back 调用 DeactivateWidget。在线只允许一个 LocalPlayer；分屏测试 Experience 禁用 Online。",
    0, 1250, 2350, 1550)

tile_blueprint = unreal.load_asset("/Game/UI/Menu/W_LyraExperienceTileButton")
unreal.BlueprintEditorLibrary.compile_blueprint(tile_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(tile_blueprint, only_if_is_dirty=False)
if tile_blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError("Experience tile button compile failed after compact layout update")

blueprint = unreal.load_asset(PATH)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
if blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError(f"Host screen compile failed: {blueprint.get_editor_property('status')}")
log("Host interaction rebuilt with explicit select, create, search, local-player, join-policy and back controls")
