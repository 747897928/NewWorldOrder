import unreal


EXPECTED = {
    "/Game/GameFramework/Experiences/UserFacing/DA_UFE_DungeonTest": (
        "/Game/Maps/TestMap_ListenServer", "DA_Experience_DungeonTest", True),
    "/Game/GameFramework/Experiences/UserFacing/DA_UFE_SplitScreenTest": (
        "/Game/Maps/TestMap_SplitScreen", "DA_Experience_SplitScreenTest", False),
    "/Game/GameFramework/Experiences/UserFacing/DA_UFE_ExpeditionSandbox": (
        "/Game/Maps/TestMap_ExpeditionSandbox", "DA_Experience_ExpeditionSandbox", True),
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)
    print(f"[SessionUIExperienceAudit] PASS {message}")


for ufe_path, (map_path, experience_name, supports_online) in EXPECTED.items():
    ufe = unreal.load_asset(ufe_path)
    require(ufe is not None, f"UFE exists: {ufe_path}")
    require(str(ufe.get_editor_property("map_id").primary_asset_name) == map_path,
            f"UFE map: {ufe_path} -> {map_path}")
    require(str(ufe.get_editor_property("experience_id").primary_asset_name) == experience_name,
            f"UFE experience: {ufe_path} -> {experience_name}")
    require(ufe.get_editor_property("supports_online") == supports_online,
            f"UFE online boundary: {ufe_path} -> {supports_online}")
    require(not ufe.get_editor_property("record_replay"),
            f"POC replay disabled: {ufe_path}")

    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    game_mode_class = world.get_world_settings().get_editor_property("default_game_mode")
    require(game_mode_class is not None, f"Map has GameMode: {map_path}")
    game_mode_cdo = unreal.get_default_object(game_mode_class)
    configured_experience = game_mode_cdo.get_editor_property("experience_definition")
    require(configured_experience is not None and
            configured_experience.get_name() == experience_name,
            f"Map GameMode uses Experience: {map_path} -> {experience_name}")

    return_portals = [
        actor for actor in unreal.EditorLevelLibrary.get_all_level_actors()
        if "BP_DungeonPortal_ToHomeMap" in actor.get_class().get_name()
    ]
    require(len(return_portals) >= 1, f"Map has return path to HomeMap: {map_path}")

host_path = "/Game/UI/Menu/Experiences/W_HostSessionScreen"
host_blueprint = unreal.load_asset(host_path)
required_widgets = {
    "HostButton", "SessionBrowserButton", "CloseButton",
    "AddLocalPlayerButton", "RemoveLocalPlayerButton",
    "JoinPolicyButton", "SessionStatusText",
}
existing_widgets = {
    str(widget.widget_name) for widget in unreal.WidgetService.get_hierarchy(host_path)
}
require(required_widgets.issubset(existing_widgets),
        "Host screen has explicit create/search/back/local-player/join-policy controls")
require(host_blueprint.get_editor_property("status") == unreal.BlueprintStatus.BS_UP_TO_DATE,
        "Host screen Blueprint is UpToDate")

definition = unreal.BlueprintService.get_graph_definition(host_path, "EventGraph")
connections = definition[1]
direct_select_to_host = any(
    str(connection.from_).startswith("SelectExperience") and
    str(connection.to).startswith("HostSelectedExperience")
    for connection in connections)
require(not direct_select_to_host, "Selecting a Tile does not immediately create a session")

print("[SessionUIExperienceAudit] Three complete and extensible expedition configurations passed")
