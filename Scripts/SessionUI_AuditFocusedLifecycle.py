import unreal


def log(message):
    unreal.log(f"[SessionUIFocusedAudit] {message}")
    print(f"[SessionUIFocusedAudit] {message}")


for bp_path in (
        "/Game/GameFramework/GameModes/BP_ShootGameMode",
        "/Game/Gameplay/Interactables/Portals/BP_DungeonPortal_ToTestMap",
        "/Game/Gameplay/Interactables/Portals/BP_DungeonPortal_ToHomeMap"):
    log(f"BP {bp_path} parent={unreal.BlueprintService.get_parent_class(bp_path)}")
    for prop in (
            "ExperienceDefinition", "DestinationMap", "bClearAllRuntimeSessionsBeforeTravel",
            "InteractionText", "InteractionSubText"):
        value = unreal.BlueprintService.get_property(bp_path, prop)
        if value is not None:
            log(f"  {prop}={value}")

experience = unreal.load_asset("/Game/GameFramework/Experiences/DA_Experience_DungeonTest")
if experience:
    for prop in (
            "hud_layouts", "hud_widgets", "male_protagonist_ability_set",
            "female_protagonist_ability_set", "common_ability_set"):
        log(f"EXPERIENCE {prop}={experience.get_editor_property(prop)}")

for map_path in ("/Game/Maps/TestMap_ListenServer", "/Game/Maps/LobbyMap"):
    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    log(f"MAP {map_path}")
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if any(token in class_name for token in ("PlayerStart", "Portal", "Terminal")):
            log(f"  {actor.get_actor_label()}|{class_name}|{actor.get_actor_location()}")
