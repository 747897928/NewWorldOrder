import unreal


GAME_MENU_PATH = "/Game/UI/Menu/WBP_GameMenu"
LOBBY_PATH = "/Game/UI/Menu/Experiences/W_ExpeditionLobbyScreen"
SETTINGS_CLASS_PATH = "/Game/UI/Settings/W_LyraSettingScreen.W_LyraSettingScreen_C"
GRAPH = "EventGraph"


def log(message):
    unreal.log(f"[SessionUIPlayerLifecycle] {message}")
    print(f"[SessionUIPlayerLifecycle] {message}")


def ensure_button(path, name, parent, child_index, text):
    if not unreal.WidgetService.widget_exists(path, name):
        result = unreal.WidgetService.add_component(
            path, "W_MenuButton", name, parent, True, child_index)
        if not result.success:
            raise RuntimeError(f"Cannot add {path}:{name}: {result.message}")
    for prop, value in (
            ("ButtonText", text),
            ("MinWidth", "320"),
            ("MinHeight", "64")):
        if not unreal.WidgetService.set_property(path, name, prop, value):
            raise RuntimeError(f"Cannot set {path}:{name}.{prop}={value}")


def find_bound_event(path, component_name):
    title = f"On Clicked ({component_name})"
    for node in unreal.BlueprintService.get_nodes_in_graph(path, GRAPH):
        if node.node_type.endswith("K2Node_ComponentBoundEvent") and node.node_title == title:
            return node.node_id
    return ""


def ensure_click_call(path, component_name, owner_class, function_name, y):
    event_id = find_bound_event(path, component_name)
    if not event_id:
        event_id = unreal.BlueprintService.create_component_bound_event(
            path, GRAPH, component_name, "OnButtonBaseClicked", 100, y)
    if not event_id:
        raise RuntimeError(f"Cannot create click event for {path}:{component_name}")

    calls = [
        node for node in unreal.BlueprintService.get_nodes_in_graph(path, GRAPH)
        if function_name.lower() in node.node_title.lower() and node.node_type.endswith("K2Node_CallFunction")
    ]
    if calls:
        call_id = calls[0].node_id
    else:
        build = unreal.BlueprintService.build_graph(
            path,
            GRAPH,
            [{"ref": function_name, "type": "function_call",
              "params": {"class": owner_class, "function": function_name}}],
            [],
            [],
            False,
            False,
        )
        if not build.success:
            raise RuntimeError(f"Cannot add {function_name}: {list(build.errors)}")
        call_id = str(build.ref_to_node_id[function_name])
        unreal.BlueprintService.set_node_position(path, GRAPH, call_id, 520, y)

    connections = unreal.BlueprintService.get_connections(path, GRAPH)
    already_connected = any(
        connection.source_node_id == event_id and connection.target_node_id == call_id
        for connection in connections
    )
    if not already_connected and not unreal.BlueprintService.connect_nodes(
            path, GRAPH, event_id, "then", call_id, "execute"):
        raise RuntimeError(f"Cannot connect {component_name} to {function_name}")


# WBP_GameMenu 是同一个 LocalPlayer 在 HomeMap 与副本内使用的菜单。
# 设置始终可见；退出副本和 Home-only 按钮由父类 NativeOnActivated 按地图上下文切换。
ensure_button(GAME_MENU_PATH, "SettingsButton", "VerticalBox_144", 3, "设置")
ensure_button(GAME_MENU_PATH, "ExitExpeditionButton", "VerticalBox_144", 4, "退出副本")

# Lobby 的邀请入口必须直接可见，不能要求房主退回旧 SessionScreen 寻找隐藏入口。
ensure_button(LOBBY_PATH, "InviteButton", "ContentBox", 7, "邀请好友")

game_menu_class = unreal.load_class(None, GAME_MENU_PATH + ".WBP_GameMenu_C")
game_menu_cdo = unreal.get_default_object(game_menu_class) if game_menu_class else None
if game_menu_cdo and hasattr(game_menu_cdo, "open_settings_screen"):
    settings_class = unreal.load_class(None, SETTINGS_CLASS_PATH)
    game_menu_cdo.modify()
    game_menu_cdo.set_editor_property("settings_screen_class", settings_class)
    ensure_click_call(GAME_MENU_PATH, "SettingsButton", "ShootMainMenuScreen", "OpenSettingsScreen", 1780)
    ensure_click_call(GAME_MENU_PATH, "ExitExpeditionButton", "ShootMainMenuScreen", "RequestExitExpedition", 1960)
    ensure_click_call(LOBBY_PATH, "InviteButton", "ShootExpeditionLobbyScreen", "InviteFriends", 900)
    log("C++ lifecycle functions found; button events and settings class are configured")
else:
    log("Layout phase complete; rerun after C++ build to bind new lifecycle functions")

for path in (GAME_MENU_PATH, LOBBY_PATH):
    blueprint = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError(f"Cannot save {path}")
    if blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
        raise RuntimeError(f"Blueprint did not compile: {path}")
    validation = unreal.WidgetService.validate(path)
    if not validation.is_valid:
        raise RuntimeError(f"Widget hierarchy invalid: {path}: {list(validation.errors)}")

log("Player lifecycle controls saved")
