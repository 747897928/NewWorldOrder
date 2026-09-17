import unreal


PREFIX = "[TacticalOverloadAssets]"
STATUS_WIDGET_SOURCE = "/Game/UI/Foundation/Widgets/Interactive_Progress_Bar"
STATUS_WIDGET = "/Game/UI/Skills/Status/W_TacticalOverloadStatus"
DEFAULT_HUD = "/Game/UI/Hud/W_DefaultHUD"
NIAGARA_SOURCE = "/Game/NiagaraExamples/FX_Player/NS_Player_Electricity_Looping"
NIAGARA_TARGET = "/Game/Effects/Niagara/Skills/TacticalOverload/NS_TacticalOverload_Aura"
ACTIVE_CUE = "/Game/Effects/GameplayCues/Abilities/Skills/GCN_Skill_TacticalOverload_Active"

EXPERIENCES = (
    "/Game/GameFramework/Experiences/DA_Experience_ExpeditionSandbox",
    "/Game/GameFramework/Experiences/DA_Experience_DungeonTest",
    "/Game/GameFramework/Experiences/DA_Experience_SplitScreenTest",
)


def log(message):
    unreal.log(f"{PREFIX} {message}")
    print(f"{PREFIX} {message}")


def require_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def find_asset_by_name(asset_name):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    matches = [
        str(data.package_name)
        for data in registry.get_assets_by_path("/Game", recursive=True)
        if str(data.asset_name) == asset_name
    ]
    if len(matches) != 1:
        raise RuntimeError(f"Expected one asset named {asset_name}, found {matches}")
    return matches[0]


def compile_and_save(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)


def ensure_duplicate(source, target):
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        return require_asset(target)
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(source, target)
    if duplicated is None:
        raise RuntimeError(f"Failed to duplicate {source} -> {target}")
    unreal.EditorAssetLibrary.save_loaded_asset(duplicated, only_if_is_dirty=False)
    log(f"Duplicated {source} -> {target}")
    return duplicated


def ensure_status_widget():
    if not hasattr(unreal, "ShootTacticalOverloadStatusWidget"):
        raise RuntimeError("UShootTacticalOverloadStatusWidget is not loaded; compile C++ and restart the editor first")

    widget_bp = ensure_duplicate(STATUS_WIDGET_SOURCE, STATUS_WIDGET)
    parent_class = unreal.ShootTacticalOverloadStatusWidget.static_class()
    unreal.BlueprintEditorLibrary.reparent_blueprint(widget_bp, parent_class)
    compile_and_save(widget_bp)

    widget_class = widget_bp.generated_class()
    widget_cdo = unreal.get_default_object(widget_class)
    widget_cdo.modify()
    widget_cdo.set_editor_property("active_label", "战术超载")
    widget_bp.modify()
    compile_and_save(widget_bp)
    log(f"Configured {STATUS_WIDGET} parent={parent_class.get_name()}")
    return widget_class


def ensure_status_extension_point():
    name = "StatusEffectExtensionPoint"
    if not unreal.WidgetService.widget_exists(DEFAULT_HUD, name):
        result = unreal.WidgetService.add_component(
            DEFAULT_HUD, "UIExtensionPointWidget", name, "CanvasPanel_0", True)
        if not result.success:
            raise RuntimeError(f"Failed to add {name}: {result.message}")

    properties = (
        ("ExtensionPointTag", '(TagName="HUD.Slot.StatusEffects")'),
        ("Anchor Min X", "0.5"),
        ("Anchor Min Y", "0.86"),
        ("Anchor Max X", "0.5"),
        ("Anchor Max Y", "0.86"),
        ("Alignment X", "0.5"),
        ("Alignment Y", "0.5"),
        ("Position X", "0"),
        ("Position Y", "0"),
        ("Slot.bAutoSize", "True"),
        ("ZOrder", "20"),
    )
    for property_name, value in properties:
        if not unreal.WidgetService.set_property(DEFAULT_HUD, name, property_name, value):
            raise RuntimeError(f"Failed to set {name}.{property_name}={value}")

    hud_bp = require_asset(DEFAULT_HUD)
    compile_and_save(hud_bp)
    validation = unreal.WidgetService.validate(DEFAULT_HUD)
    log(f"Configured {DEFAULT_HUD}:{name}; validation={validation}")


def ensure_looping_cue(niagara_system):
    if unreal.EditorAssetLibrary.does_asset_exist(ACTIVE_CUE):
        cue_bp = require_asset(ACTIVE_CUE)
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.GameplayCueNotify_Looping)
        cue_bp = asset_tools.create_asset(
            "GCN_Skill_TacticalOverload_Active",
            "/Game/Effects/GameplayCues/Abilities/Skills",
            unreal.Blueprint,
            factory,
        )
        if cue_bp is None:
            raise RuntimeError(f"Failed to create {ACTIVE_CUE}")

    unreal.BlueprintEditorLibrary.reparent_blueprint(cue_bp, unreal.GameplayCueNotify_Looping)
    compile_and_save(cue_bp)
    cue_cdo = unreal.get_default_object(cue_bp.generated_class())
    cue_cdo.modify()
    cue_cdo.set_editor_property(
        "gameplay_cue_tag",
        unreal.GameplayTagService.request_tag("GameplayCue.Skill.TacticalOverload.Active"),
    )

    # UE 5.8 的 Python 反射不允许逐字段修改该 UScriptStruct 实例；
    # 必须在构造时一次性传值，否则会报 "cannot be edited on instances"。
    placement = unreal.GameplayCueNotify_PlacementInfo(
        attach_policy=unreal.GameplayCueNotify_AttachPolicy.ATTACH_TO_TARGET,
        attachment_rule=unreal.AttachmentRule.SNAP_TO_TARGET,
        override_scale=True,
        scale_override=unreal.Vector(0.75, 0.75, 0.75),
    )
    cue_cdo.set_editor_property("default_placement_info", placement)

    particle = unreal.GameplayCueNotify_ParticleInfo(niagara_system=niagara_system)
    looping_effects = unreal.GameplayCueNotify_LoopingEffects(looping_particles=[particle])
    cue_cdo.set_editor_property("looping_effects", looping_effects)

    cue_bp.modify()
    compile_and_save(cue_bp)
    log(f"Configured looping cue {ACTIVE_CUE} with {NIAGARA_TARGET}")


def configure_skill_blueprints():
    if not hasattr(unreal, "ShootEffect_TacticalOverloadState"):
        raise RuntimeError("UShootEffect_TacticalOverloadState is not loaded; compile C++ and restart the editor first")

    tactical_path = find_asset_by_name("GA_TacticalOverload")
    tactical_bp = require_asset(tactical_path)
    compile_and_save(tactical_bp)
    tactical_cdo = unreal.get_default_object(tactical_bp.generated_class())
    tactical_cdo.modify()
    tactical_cdo.set_editor_property(
        "overload_buff_effect", unreal.ShootEffect_TacticalOverloadState.static_class())
    tactical_cdo.set_editor_property(
        "activation_gameplay_cue_tag",
        unreal.GameplayTagService.request_tag("GameplayCue.Skill.TacticalOverload.Activate"),
    )
    tactical_cdo.set_editor_property(
        "active_gameplay_cue_tag",
        unreal.GameplayTagService.request_tag("GameplayCue.Skill.TacticalOverload.Active"),
    )
    tactical_bp.modify()
    compile_and_save(tactical_bp)

    robot_path = find_asset_by_name("GA_RobotCompanion")
    robot_bp = require_asset(robot_path)
    compile_and_save(robot_bp)
    robot_cdo = unreal.get_default_object(robot_bp.generated_class())
    robot_cdo.modify()
    robot_cdo.set_editor_property("companion_lifetime", 30.0)
    robot_cdo.set_editor_property("companion_lifetime_per_level", 5.0)
    robot_bp.modify()
    compile_and_save(robot_bp)
    log(f"Configured skill blueprints: {tactical_path}, {robot_path}")


def add_status_widget_to_experiences(widget_class):
    status_tag = unreal.GameplayTagService.request_tag("HUD.Slot.StatusEffects")
    for path in EXPERIENCES:
        experience = require_asset(path)
        entries = list(experience.get_editor_property("hud_widgets"))
        already_present = any(
            str(entry.get_editor_property("slot_id").get_editor_property("tag_name"))
            == "HUD.Slot.StatusEffects"
            and entry.get_editor_property("widget_class") == widget_class
            for entry in entries
        )
        if not already_present:
            entry = unreal.ShootHUDElementEntry()
            entry.set_editor_property("slot_id", status_tag)
            entry.set_editor_property("widget_class", widget_class)
            entries.append(entry)
            experience.modify()
            experience.set_editor_property("hud_widgets", entries)
        unreal.EditorAssetLibrary.save_loaded_asset(experience, only_if_is_dirty=False)
        log(f"Registered status HUD in {path}")


def main():
    niagara_system = ensure_duplicate(NIAGARA_SOURCE, NIAGARA_TARGET)
    widget_class = ensure_status_widget()
    ensure_status_extension_point()
    ensure_looping_cue(niagara_system)
    configure_skill_blueprints()
    add_status_widget_to_experiences(widget_class)
    unreal.EditorAssetLibrary.save_directory("/Game/UI/Skills/Status", only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory("/Game/Effects/GameplayCues/Abilities/Skills", only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory("/Game/Effects/Niagara/Skills/TacticalOverload", only_if_is_dirty=False, recursive=True)
    log("Completed tactical overload assets and Experience HUD registration")


main()
