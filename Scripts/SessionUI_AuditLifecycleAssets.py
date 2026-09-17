import unreal


def log(message):
    print(f"[SessionUILifecycleAudit] {message}")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


LOBBY_WIDGET = "/Game/UI/Menu/Experiences/W_ExpeditionLobbyScreen"
LOBBY_EXPERIENCE = "/Game/GameFramework/Experiences/DA_Experience_Lobby"
LOBBY_GAME_MODE = "/Game/GameFramework/GameModes/BP_ExpeditionLobbyGameMode"
TERMINAL_BLUEPRINT = "/Game/Gameplay/Interactables/Stations/BP_ExpeditionTerminal"

for asset_path in (LOBBY_WIDGET, LOBBY_EXPERIENCE, LOBBY_GAME_MODE, TERMINAL_BLUEPRINT):
    require(unreal.EditorAssetLibrary.does_asset_exist(asset_path), f"Missing asset {asset_path}")

for blueprint_path in (LOBBY_WIDGET, LOBBY_GAME_MODE, TERMINAL_BLUEPRINT):
    blueprint = unreal.load_asset(blueprint_path)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    status = blueprint.get_editor_property("status")
    log(f"Blueprint {blueprint_path} status={status}")
    require("UP_TO_DATE" in str(status), f"Blueprint is not up to date: {blueprint_path}")

lobby_widget_class = unreal.load_class(None, LOBBY_WIDGET + ".W_ExpeditionLobbyScreen_C")
require(lobby_widget_class is not None, "Lobby widget generated class is missing")
require(
    "ShootExpeditionLobbyScreen" in str(unreal.BlueprintService.get_parent_class(LOBBY_WIDGET)),
    "Lobby widget native parent is wrong")
widget_names = {item.widget_name for item in unreal.WidgetService.list_components(LOBBY_WIDGET)}
required_widgets = {
    "StatusText", "PlayerCountText", "JoinPolicyText", "BotPolicyText",
    "StartButton", "LeaveButton",
}
require(required_widgets.issubset(widget_names), f"Lobby widget tree missing {required_widgets - widget_names}")
log(f"Lobby widget components={sorted(widget_names)}")

lobby_experience = unreal.load_asset(LOBBY_EXPERIENCE)
layouts = lobby_experience.get_editor_property("hud_layouts")
require(len(layouts) == 1, f"Lobby Experience must have one HUD layout, got {len(layouts)}")
layout_class = layouts[0].get_editor_property("layout_class")
layer_id = layouts[0].get_editor_property("layer_id")
log(f"Lobby Experience layout={layout_class} layer={layer_id.export_text()}")
require("W_ExpeditionLobbyScreen_C" in str(layout_class), "Lobby Experience layout class is wrong")
require("UI.Layer.GameMenu" in layer_id.export_text(), "Lobby Experience layer is wrong")

lobby_game_mode_class = unreal.load_class(None, LOBBY_GAME_MODE + ".BP_ExpeditionLobbyGameMode_C")
lobby_game_mode_cdo = unreal.get_default_object(lobby_game_mode_class)
experience = lobby_game_mode_cdo.get_editor_property("experience_definition")
log(f"Lobby GameMode ExperienceDefinition={experience}")
require("DA_Experience_Lobby" in str(experience), "Lobby GameMode ExperienceDefinition is wrong")

terminal_class = unreal.load_class(None, TERMINAL_BLUEPRINT + ".BP_ExpeditionTerminal_C")
terminal_cdo = unreal.get_default_object(terminal_class)
screen_class = terminal_cdo.get_editor_property("expedition_screen_class")
visual = terminal_cdo.get_editor_property("visual_component")
log(f"Terminal screen={screen_class} visual={visual} visual_outer={visual.get_outer().get_path_name()}")
require("W_HostSessionScreen_C" in str(screen_class), "Terminal screen class is wrong")

# BlueprintService 只枚举 SCS 组件，无法设置继承自原生类的 VisualComponent。
# 生成类 CDO 上的组件 Outer 属于该蓝图 CDO，可安全保存为蓝图默认覆盖。
if visual.get_outer() == terminal_cdo:
    mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
    visual.set_editor_property("static_mesh", mesh)
    visual.set_editor_property("relative_scale3d", unreal.Vector(0.8, 0.8, 1.8))
    unreal.EditorAssetLibrary.save_asset(TERMINAL_BLUEPRINT, only_if_is_dirty=False)
else:
    raise RuntimeError(f"Terminal visual is not owned by Blueprint CDO: {visual.get_outer()}")
require(visual.get_editor_property("static_mesh") is not None, "Terminal visual mesh is missing")
log(f"Terminal visual mesh={visual.get_editor_property('static_mesh')} scale={visual.get_editor_property('relative_scale3d')}")

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/LobbyMap")
lobby_world = unreal.EditorLevelLibrary.get_editor_world()
map_game_mode = lobby_world.get_world_settings().get_editor_property("default_game_mode")
log(f"LobbyMap GameMode={map_game_mode}")
require("BP_ExpeditionLobbyGameMode_C" in str(map_game_mode), "LobbyMap GameMode is wrong")

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/HomeMap")
home_actors = unreal.EditorLevelLibrary.get_all_level_actors()
terminals = [actor for actor in home_actors if actor.get_class() == terminal_class]
legacy_portals = [actor for actor in home_actors if actor.get_actor_label() == "DungeonPortal_ToTestMap"]
log(f"HomeMap terminals={len(terminals)} legacy_direct_portals={len(legacy_portals)}")
require(len(terminals) == 1, "HomeMap must contain exactly one expedition terminal")
require(len(legacy_portals) == 0, "HomeMap still contains legacy direct portal")

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/TestMap_ListenServer")
return_portals = [
    actor for actor in unreal.EditorLevelLibrary.get_all_level_actors()
    if actor.get_actor_label() == "DungeonPortal_ToHomeMap"
]
log(f"TestMap_ListenServer return_portals={len(return_portals)}")
require(len(return_portals) >= 1, "Dungeon has no return portal to HomeMap")

ufe = unreal.load_asset("/Game/GameFramework/Experiences/UserFacing/DA_UFE_DungeonTest")
log(
    f"UFE map={ufe.get_editor_property('map_id')} lobby={ufe.get_editor_property('lobby_map_id')} "
    f"experience={ufe.get_editor_property('experience_id')} max={ufe.get_editor_property('max_player_count')} "
    f"record_replay={ufe.get_editor_property('record_replay')}")
require(ufe.get_editor_property("max_player_count") == 4, "UFE max players is wrong")
require(not ufe.get_editor_property("record_replay"), "Replay must remain disabled for first version")

log("Lifecycle asset audit passed")
