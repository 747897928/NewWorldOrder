import unreal


EXPERIENCE_ROOT = "/Game/GameFramework/Experiences"
UFE_ROOT = EXPERIENCE_ROOT + "/UserFacing"
MAP_ROOT = "/Game/Maps"
GAME_MODE_ROOT = "/Game/GameFramework/GameModes"
LOBBY_MAP = "/Game/Maps/LobbyMap"


def log(message):
    unreal.log(f"[SessionUIExperienceCatalog] {message}")
    print(f"[SessionUIExperienceCatalog] {message}")


def require_asset(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def duplicate_asset(source, target):
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        log(f"Reuse asset {target}")
        return require_asset(target)
    asset = unreal.EditorAssetLibrary.duplicate_asset(source, target)
    if not asset:
        raise RuntimeError(f"Cannot duplicate {source} -> {target}")
    log(f"Duplicated {source} -> {target}")
    return asset


def create_blueprint(path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(f"Reuse blueprint {path}")
        return require_asset(path)
    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError(f"Cannot create blueprint {path}")
    log(f"Created blueprint {path}")
    return asset


def primary_asset_id(asset_type, asset_name):
    return unreal.PrimaryAssetId(
        primary_asset_type=unreal.PrimaryAssetType(asset_type),
        primary_asset_name=asset_name)


def compile_and_save_blueprint(path):
    blueprint = require_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
        raise RuntimeError(f"Blueprint compile failed: {path}")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)


def configure_game_mode(path, experience_path):
    if not unreal.BlueprintService.set_property(
            path, "ExperienceDefinition", experience_path + "." + experience_path.rsplit("/", 1)[1]):
        raise RuntimeError(f"Cannot set ExperienceDefinition on {path}")
    compile_and_save_blueprint(path)
    game_mode_class = unreal.load_class(None, path + "." + path.rsplit("/", 1)[1] + "_C")
    cdo = unreal.get_default_object(game_mode_class)
    configured = cdo.get_editor_property("experience_definition")
    if not configured or configured.get_path_name() != experience_path + "." + experience_path.rsplit("/", 1)[1]:
        raise RuntimeError(f"GameMode Experience did not persist: {path}")
    return game_mode_class


def configure_map(map_path, game_mode_class):
    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    world.get_world_settings().set_editor_property("default_game_mode", game_mode_class)
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError(f"Cannot save map {map_path}")
    configured = world.get_world_settings().get_editor_property("default_game_mode")
    if configured != game_mode_class:
        raise RuntimeError(f"Map GameMode did not persist: {map_path}")
    log(f"Configured map {map_path} -> {game_mode_class.get_path_name()}")


def configure_ufe(path, map_path, experience_name, title, subtitle, description,
                  max_players, supports_online, is_default=False):
    ufe = require_asset(path)
    ufe.set_editor_property("map_id", primary_asset_id("Map", map_path))
    ufe.set_editor_property("lobby_map_id", primary_asset_id("Map", LOBBY_MAP))
    ufe.set_editor_property(
        "experience_id", primary_asset_id("ShootExperienceDefinition", experience_name))
    ufe.set_editor_property("tile_title", unreal.Text(title))
    ufe.set_editor_property("tile_sub_title", unreal.Text(subtitle))
    ufe.set_editor_property("tile_description", unreal.Text(description))
    ufe.set_editor_property("is_default_experience", is_default)
    ufe.set_editor_property("show_in_front_end", True)
    ufe.set_editor_property("supports_online", supports_online)
    ufe.set_editor_property("record_replay", False)
    ufe.set_editor_property("max_player_count", max_players)
    unreal.EditorAssetLibrary.save_loaded_asset(ufe, only_if_is_dirty=False)
    log(f"Configured {path}: map={map_path}, experience={experience_name}, online={supports_online}")


# 三张卡片各自拥有独立的 Map、GameMode 与 ShootExperienceDefinition，后续可分别追加能力、HUD 和规则。
dungeon_experience = EXPERIENCE_ROOT + "/DA_Experience_DungeonTest"
split_experience = EXPERIENCE_ROOT + "/DA_Experience_SplitScreenTest"
sandbox_experience = EXPERIENCE_ROOT + "/DA_Experience_ExpeditionSandbox"
duplicate_asset(dungeon_experience, split_experience)
duplicate_asset(dungeon_experience, sandbox_experience)

dungeon_map = MAP_ROOT + "/TestMap_ListenServer"
split_map = MAP_ROOT + "/TestMap_SplitScreen"
sandbox_map = MAP_ROOT + "/TestMap_ExpeditionSandbox"
duplicate_asset(dungeon_map, sandbox_map)

dungeon_game_mode_path = GAME_MODE_ROOT + "/BP_TestListenGameMode"
split_game_mode_path = GAME_MODE_ROOT + "/BP_TestGameMode"
sandbox_game_mode_path = GAME_MODE_ROOT + "/BP_ExpeditionSandboxGameMode"
dungeon_game_mode_class = unreal.load_class(
    None, dungeon_game_mode_path + ".BP_TestListenGameMode_C")
create_blueprint(sandbox_game_mode_path, dungeon_game_mode_class)

dungeon_game_mode_class = configure_game_mode(dungeon_game_mode_path, dungeon_experience)
split_game_mode_class = configure_game_mode(split_game_mode_path, split_experience)
sandbox_game_mode_class = configure_game_mode(sandbox_game_mode_path, sandbox_experience)
configure_map(dungeon_map, dungeon_game_mode_class)
configure_map(split_map, split_game_mode_class)
configure_map(sandbox_map, sandbox_game_mode_class)

dungeon_ufe = UFE_ROOT + "/DA_UFE_DungeonTest"
split_ufe = UFE_ROOT + "/DA_UFE_SplitScreenTest"
sandbox_ufe = UFE_ROOT + "/DA_UFE_ExpeditionSandbox"
duplicate_asset(dungeon_ufe, split_ufe)
duplicate_asset(dungeon_ufe, sandbox_ufe)

configure_ufe(
    dungeon_ufe, dungeon_map, "DA_Experience_DungeonTest",
    "丧尸测试副本", "本地或 Listen Server",
    "标准丧尸战斗 POC。本地直接进入；在线先进入等待大厅，由房主开始副本。",
    4, True, True)
configure_ufe(
    split_ufe, split_map, "DA_Experience_SplitScreenTest",
    "本地双人分屏测试", "仅本地 / 1-2 名本地玩家",
    "验证同机双主角、独立输入与返回 HomeMap 的测试副本；该入口不创建在线房间。",
    2, False)
configure_ufe(
    sandbox_ufe, sandbox_map, "DA_Experience_ExpeditionSandbox",
    "沙盒副本测试", "本地或 Listen Server",
    "独立的玩法扩展槽，后续用于实验新属性、技能、敌人规则与 HUD 组合。",
    4, True)

unreal.EditorAssetLibrary.save_directory(EXPERIENCE_ROOT, only_if_is_dirty=False, recursive=True)
log("Experience catalog now contains three complete Map + GameMode + Experience configurations")
