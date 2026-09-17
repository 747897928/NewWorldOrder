import unreal


LOBBY_WIDGET = "/Game/UI/Menu/Experiences/W_ExpeditionLobbyScreen"
LOBBY_EXPERIENCE = "/Game/GameFramework/Experiences/DA_Experience_Lobby"
LOBBY_GAME_MODE = "/Game/GameFramework/GameModes/BP_ExpeditionLobbyGameMode"
TERMINAL_BLUEPRINT = "/Game/Gameplay/Interactables/Stations/BP_ExpeditionTerminal"


def log(message):
    unreal.log(f"[SessionUILifecycleCreate] {message}")
    print(f"[SessionUILifecycleCreate] {message}")


def create_blueprint(path, parent_class, widget=False):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        log(f"Reuse blueprint {path}")
        return asset

    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, unreal.WidgetBlueprint if widget else unreal.Blueprint, factory)
    if asset is None:
        raise RuntimeError(f"Failed to create blueprint {path}")
    log(f"Created blueprint {path} parent={parent_class.get_name()}")
    return asset


def compile_and_save(path):
    blueprint = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    status = blueprint.get_editor_property("status")
    log(f"Compile {path} status={status}")
    if status != unreal.BlueprintStatus.BS_UP_TO_DATE:
        raise RuntimeError(f"Blueprint compile failed: {path}")


def ensure_widget(component_type, name, parent="", variable=True):
    if unreal.WidgetService.widget_exists(LOBBY_WIDGET, name):
        return
    result = unreal.WidgetService.add_component(
        LOBBY_WIDGET, component_type, name, parent, variable)
    if not result.success:
        raise RuntimeError(f"Failed to add {component_type} {name}: {result.message}")


def set_widget_property(name, prop, value, required=True):
    success = unreal.WidgetService.set_property(LOBBY_WIDGET, name, prop, value)
    log(f"Widget property {name}.{prop}={value} success={success}")
    if required and not success:
        raise RuntimeError(f"Failed to set {name}.{prop}")


# 等待大厅页面：固定布局留在 Widget Blueprint；C++ 只提供复制数据和 Session 动作。
lobby_parent = unreal.load_class(None, "/Script/NewWorldOrder.ShootExpeditionLobbyScreen")
create_blueprint(LOBBY_WIDGET, lobby_parent, widget=True)

ensure_widget("Overlay", "RootOverlay", "", False)
ensure_widget("Border", "Backdrop", "RootOverlay", False)
ensure_widget("VerticalBox", "ContentBox", "RootOverlay", False)
ensure_widget("TextBlock", "TitleText", "ContentBox", False)
ensure_widget("TextBlock", "StatusText", "ContentBox", True)
ensure_widget("TextBlock", "ExpeditionText", "ContentBox", False)
ensure_widget("TextBlock", "PlayerCountLabel", "ContentBox", False)
ensure_widget("TextBlock", "PlayerCountText", "ContentBox", True)
ensure_widget("TextBlock", "JoinPolicyText", "ContentBox", True)
ensure_widget("TextBlock", "BotPolicyText", "ContentBox", False)
# 项目已有 W_MenuButton；不要重新引入 Lyra 同名按钮及其字体材质依赖。
ensure_widget("W_MenuButton", "StartButton", "ContentBox", True)
ensure_widget("W_MenuButton", "LeaveButton", "ContentBox", True)

set_widget_property("Backdrop", "BrushColor", "(R=0.015,G=0.025,B=0.055,A=0.96)", False)
for widget_name, text in (
        ("TitleText", "EXPEDITION LOBBY"),
        ("StatusText", "Waiting for session state..."),
        ("ExpeditionText", "Selected Expedition: Dungeon Test"),
        ("PlayerCountLabel", "Players Connected"),
        ("PlayerCountText", "1"),
        ("JoinPolicyText", "Join In Progress: Allowed"),
        ("BotPolicyText", "AI Teammate Fill: Coming Later")):
    set_widget_property(widget_name, "Text", text)
    set_widget_property(widget_name, "Justification", "Center", False)
    set_widget_property(
        widget_name, "ColorAndOpacity",
        "(SpecifiedColor=(R=0.85,G=0.93,B=1.0,A=1.0),ColorUseRule=UseColor_Specified)",
        False)

set_widget_property("StartButton", "ButtonText", "START EXPEDITION")
set_widget_property("LeaveButton", "ButtonText", "LEAVE LOBBY")
compile_and_save(LOBBY_WIDGET)

# 重跑脚本时只在空 EventGraph 建图，避免制造重复事件。
graph_nodes = unreal.BlueprintService.get_nodes_in_graph(LOBBY_WIDGET, "EventGraph")
if not graph_nodes:
    data_build = unreal.BlueprintService.build_graph(
        LOBBY_WIDGET,
        "EventGraph",
        [
            {"ref": "LobbyData", "type": "event",
             "params": {"event": "BP_OnLobbyDataChanged"}},
            {"ref": "SessionState", "type": "event",
             "params": {"event": "BP_OnLobbySessionStateChanged"}},
            {"ref": "StartButtonGet", "type": "variable_get",
             "params": {"variable": "StartButton"}},
            {"ref": "SetStartEnabled", "type": "function_call",
             "params": {"class": "Widget", "function": "SetIsEnabled"}},
            {"ref": "PlayerCountGet", "type": "variable_get",
             "params": {"variable": "PlayerCountText"}},
            {"ref": "ArrayLength", "type": "function_call",
             "params": {"class": "KismetArrayLibrary", "function": "Array_Length"}},
            {"ref": "PlayerCountToText", "type": "function_call",
             "params": {"class": "KismetTextLibrary", "function": "Conv_IntToText"}},
            {"ref": "SetPlayerCount", "type": "function_call",
             "params": {"class": "TextBlock", "function": "SetText"}},
            {"ref": "JoinPolicyGet", "type": "variable_get",
             "params": {"variable": "JoinPolicyText"}},
            {"ref": "JoinBranch", "type": "branch", "params": {}},
            {"ref": "SetJoinAllowed", "type": "function_call",
             "params": {"class": "TextBlock", "function": "SetText"}},
            {"ref": "SetJoinLocked", "type": "function_call",
             "params": {"class": "TextBlock", "function": "SetText"}},
            {"ref": "JoinAllowedLiteral", "type": "function_call",
             "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},
            {"ref": "JoinLockedLiteral", "type": "function_call",
             "params": {"class": "KismetTextLibrary", "function": "Conv_StringToText"}},
            {"ref": "StatusGet", "type": "variable_get",
             "params": {"variable": "StatusText"}},
            {"ref": "SetStatus", "type": "function_call",
             "params": {"class": "TextBlock", "function": "SetText"}},
        ],
        [
            {"from_": "LobbyData.then", "to": "SetStartEnabled.execute"},
            {"from_": "LobbyData.bCanStart", "to": "SetStartEnabled.bInIsEnabled"},
            {"from_": "StartButtonGet.StartButton", "to": "SetStartEnabled.self"},
            {"from_": "SetStartEnabled.then", "to": "SetPlayerCount.execute"},
            {"from_": "LobbyData.Players", "to": "ArrayLength.TargetArray"},
            {"from_": "ArrayLength.ReturnValue", "to": "PlayerCountToText.Value"},
            {"from_": "PlayerCountToText.ReturnValue", "to": "SetPlayerCount.InText"},
            {"from_": "PlayerCountGet.PlayerCountText", "to": "SetPlayerCount.self"},
            {"from_": "SetPlayerCount.then", "to": "JoinBranch.execute"},
            {"from_": "LobbyData.bAllowJoinInProgress", "to": "JoinBranch.Condition"},
            {"from_": "JoinBranch.then", "to": "SetJoinAllowed.execute"},
            {"from_": "JoinBranch.else", "to": "SetJoinLocked.execute"},
            {"from_": "JoinPolicyGet.JoinPolicyText", "to": "SetJoinAllowed.self"},
            {"from_": "JoinPolicyGet.JoinPolicyText", "to": "SetJoinLocked.self"},
            {"from_": "JoinAllowedLiteral.ReturnValue", "to": "SetJoinAllowed.InText"},
            {"from_": "JoinLockedLiteral.ReturnValue", "to": "SetJoinLocked.InText"},
            {"from_": "SessionState.then", "to": "SetStatus.execute"},
            {"from_": "SessionState.Status", "to": "SetStatus.InText"},
            {"from_": "StatusGet.StatusText", "to": "SetStatus.self"},
        ],
        [
            {"node_ref": "JoinAllowedLiteral", "pin_name": "InString",
             "value": "Join In Progress: Allowed"},
            {"node_ref": "JoinLockedLiteral", "pin_name": "InString",
             "value": "Join In Progress: Disabled After Start"},
        ],
        True,
        False)
    log(f"Lobby data graph success={data_build.success} errors={list(data_build.errors)}")
    if not data_build.success:
        raise RuntimeError(f"Lobby data graph failed: {list(data_build.errors)}")

start_event = unreal.BlueprintService.create_component_bound_event(
    LOBBY_WIDGET, "EventGraph", "StartButton", "OnButtonBaseClicked", 100, 950)
leave_event = unreal.BlueprintService.create_component_bound_event(
    LOBBY_WIDGET, "EventGraph", "LeaveButton", "OnButtonBaseClicked", 100, 1150)

for event_id, ref, function_name in (
        (start_event, "StartExpedition", "StartExpedition"),
        (leave_event, "LeaveLobby", "ConfirmLeaveLobby")):
    existing = [
        node for node in unreal.BlueprintService.get_nodes_in_graph(LOBBY_WIDGET, "EventGraph")
        if node.node_type.endswith("K2Node_CallFunction") and
        node.node_title.replace(" ", "").lower() == function_name.lower()
    ]
    if existing:
        call_id = existing[0].node_id
    else:
        action_build = unreal.BlueprintService.build_graph(
            LOBBY_WIDGET, "EventGraph",
            [{"ref": ref, "type": "function_call",
              "params": {"class": "ShootExpeditionLobbyScreen", "function": function_name}}],
            [], [], True, False)
        if not action_build.success:
            raise RuntimeError(f"Lobby action {function_name} failed: {list(action_build.errors)}")
        call_id = action_build.ref_to_node_id[ref]
    unreal.BlueprintService.disconnect_pin(LOBBY_WIDGET, "EventGraph", event_id, "then")
    if not unreal.BlueprintService.connect_nodes(
            LOBBY_WIDGET, "EventGraph", event_id, "then", call_id, "execute"):
        raise RuntimeError(f"Cannot connect lobby action {function_name}")

unreal.BlueprintService.add_comment_node(
    LOBBY_WIDGET, "EventGraph",
    "大厅数据来自 GameState.ShootExpeditionLobbyComponent；Start 只允许 Listen Server 房主，"
    "Leave 统一进入 ShootSessionCoordinatorSubsystem。AI 队友尚未实现，页面固定显示 Coming Later。",
    -100, -180, 1750, 1500)
compile_and_save(LOBBY_WIDGET)

# Lobby Experience 只负责把等待大厅推给每个 LocalPlayer，不授予副本技能或 HUD 片段。
if unreal.EditorAssetLibrary.does_asset_exist(LOBBY_EXPERIENCE):
    lobby_experience = unreal.load_asset(LOBBY_EXPERIENCE)
else:
    data_factory = unreal.DataAssetFactory()
    data_factory.set_editor_property("data_asset_class", unreal.ShootExperienceDefinition)
    lobby_experience = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_Experience_Lobby", "/Game/GameFramework/Experiences",
        unreal.ShootExperienceDefinition, data_factory)
    if lobby_experience is None:
        raise RuntimeError("Cannot create DA_Experience_Lobby")

layer_tag = unreal.GameplayTag()
if not layer_tag.import_text("UI.Layer.GameMenu"):
    raise RuntimeError("Cannot import UI.Layer.GameMenu as GameplayTag")
layout = unreal.ShootHUDLayoutRequest()
layout.set_editor_property(
    "layout_class",
    unreal.load_class(None, LOBBY_WIDGET + ".W_ExpeditionLobbyScreen_C"))
layout.set_editor_property("layer_id", layer_tag)
lobby_experience.set_editor_property("hud_layouts", [layout])
lobby_experience.set_editor_property("hud_widgets", [])
unreal.EditorAssetLibrary.save_loaded_asset(lobby_experience, only_if_is_dirty=False)
log(f"Configured lobby Experience layouts={lobby_experience.get_editor_property('hud_layouts')}")

# 继承当前 BP_ShootGameMode，保留项目 Pawn/Controller/HUD 配置，只覆盖 Lobby Experience。
base_game_mode_class = unreal.load_class(
    None, "/Game/GameFramework/GameModes/BP_ShootGameMode.BP_ShootGameMode_C")
create_blueprint(LOBBY_GAME_MODE, base_game_mode_class, widget=False)
if not unreal.BlueprintService.set_property(
        LOBBY_GAME_MODE, "ExperienceDefinition",
        LOBBY_EXPERIENCE + ".DA_Experience_Lobby"):
    raise RuntimeError("Cannot set Lobby GameMode ExperienceDefinition")
compile_and_save(LOBBY_GAME_MODE)

# HomeMap 的交互终端替代旧的直接 Travel Portal，界面类仍由蓝图配置。
terminal_parent = unreal.load_class(None, "/Script/NewWorldOrder.ShootExpeditionTerminal")
create_blueprint(TERMINAL_BLUEPRINT, terminal_parent, widget=False)
if not unreal.BlueprintService.set_property(
        TERMINAL_BLUEPRINT, "ExpeditionScreenClass",
        "/Game/UI/Menu/Experiences/W_HostSessionScreen.W_HostSessionScreen_C"):
    raise RuntimeError("Cannot set terminal ExpeditionScreenClass")
compile_and_save(TERMINAL_BLUEPRINT)
terminal_class = unreal.load_class(
    None, TERMINAL_BLUEPRINT + ".BP_ExpeditionTerminal_C")
terminal_cdo = unreal.get_default_object(terminal_class)
terminal_visual = terminal_cdo.get_editor_property("visual_component")
if terminal_visual.get_outer() != terminal_cdo:
    raise RuntimeError("Terminal VisualComponent is not owned by the Blueprint CDO")
terminal_visual.set_editor_property(
    "static_mesh", unreal.load_asset("/Engine/BasicShapes/Cube"))
terminal_visual.set_editor_property(
    "relative_scale3d", unreal.Vector(0.8, 0.8, 1.8))
unreal.EditorAssetLibrary.save_asset(TERMINAL_BLUEPRINT, only_if_is_dirty=False)

# LobbyMap 使用专用 GameMode，让 ExperienceManager 自动推入等待大厅页面。
lobby_world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/LobbyMap")
lobby_game_mode_class = unreal.load_class(
    None, LOBBY_GAME_MODE + ".BP_ExpeditionLobbyGameMode_C")
lobby_world.get_world_settings().set_editor_property("default_game_mode", lobby_game_mode_class)
if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Cannot save LobbyMap")
log(f"LobbyMap GameMode={lobby_game_mode_class.get_path_name()}")

# 精确替换 HomeMap 中旧的单一步骤 Portal Actor；保留 Portal 蓝图资产供其他测试图使用。
home_world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/HomeMap")
terminal_actor = None
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if actor.get_class() == terminal_class:
        terminal_actor = actor
    elif actor.get_actor_label() == "DungeonPortal_ToTestMap" and \
            "BP_DungeonPortal_ToTestMap" in actor.get_class().get_name():
        log(f"Removing legacy direct portal actor {actor.get_actor_label()}")
        unreal.EditorLevelLibrary.destroy_actor(actor)

if terminal_actor is None:
    terminal_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        terminal_class, unreal.Vector(650.0, 0.0, 125.0), unreal.Rotator(0.0, 0.0, 0.0))
    if terminal_actor is None:
        raise RuntimeError("Cannot spawn BP_ExpeditionTerminal in HomeMap")
terminal_actor.set_actor_label("ExpeditionTerminal")
if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Cannot save HomeMap")
log(f"HomeMap terminal={terminal_actor.get_class().get_name()} at={terminal_actor.get_actor_location()}")

# POC 卡片指向 Listen Server 测试副本；首版录像保持关闭。
ufe = unreal.load_asset("/Game/GameFramework/Experiences/UserFacing/DA_UFE_DungeonTest")
ufe.set_editor_property("tile_title", unreal.Text("丧尸测试副本"))
ufe.set_editor_property("tile_sub_title", unreal.Text("本地或 Listen Server"))
ufe.set_editor_property(
    "tile_description",
    unreal.Text("本地模式直接进入副本；在线模式先进入 LobbyMap，等待房主开始。"))
ufe.set_editor_property("record_replay", False)
ufe.set_editor_property("max_player_count", 4)
unreal.EditorAssetLibrary.save_loaded_asset(ufe, only_if_is_dirty=False)
log("Lifecycle asset creation completed")
