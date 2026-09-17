import json

import unreal


PREFIX = "[RobotCompanionAssets]"
ROBOT_ROOT = "/Game/AI/Robot/Blueprints"
ABILITY_ROOT = "/Game/GameFramework/Skills/Abilities/Robot"
ABILITY_SET_ROOT = "/Game/GameFramework/Skills/AbilitySets"
DEFINITION_ROOT = "/Game/GameFramework/Skills/Definitions"
ANIM_ROOT = "/Game/Characters/RadicalMike/Animations"
# 生产 Cue 按内容规范归到 Effects；DefaultGame.ini 已把该根目录加入 GameplayCueManager 扫描路径。
CUE_ROOT = "/Game/Effects/GameplayCues/Abilities/RobotCompanion"


def log(message):
    unreal.log(f"{PREFIX} {message}")
    print(f"{PREFIX} {message}")


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    require(tag.import_text(name), f"Cannot import GameplayTag {name}")
    return tag


def create_blueprint(path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        log(f"Reuse Blueprint {path}")
        return asset

    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, unreal.Blueprint, factory)
    return require(asset, f"Cannot create Blueprint {path}")


def compile_blueprint(path):
    blueprint = require(unreal.load_asset(path), f"Cannot load Blueprint {path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    status = blueprint.get_editor_property("status")
    require(status == unreal.BlueprintStatus.BS_UP_TO_DATE,
            f"Blueprint compile failed: {path}, status={status}")
    log(f"Compiled {path}")
    return blueprint


def generated_class(path):
    asset_name = path.rsplit("/", 1)[1]
    return require(unreal.load_class(None, f"{path}.{asset_name}_C"),
                   f"Cannot load generated class for {path}")


def create_data_asset(path, data_asset_class):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        log(f"Reuse DataAsset {path}")
        return asset

    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", data_asset_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, data_asset_class, factory)
    return require(asset, f"Cannot create DataAsset {path}")


def save(asset):
    require(unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
            f"Cannot save {asset.get_path_name()}")


def make_ability_entry(ability_class):
    # 该 UStruct 字段是 EditDefaultsOnly；UE Python 不允许逐项 set，必须一次性 ImportText。
    entry = unreal.ShootAbilitySet_GameplayAbility()
    require(entry.import_text(
        f'(Ability="{ability_class.get_path_name()}",AbilityLevel=1,InputTag=(TagName=""))'),
        f"Cannot construct AbilitySet entry for {ability_class.get_path_name()}")
    return entry


def make_icon(texture_path):
    brush = unreal.SlateBrush()
    brush.set_editor_property("resource_object", require(unreal.load_asset(texture_path),
                                                         f"Missing icon {texture_path}"))
    return brush


def ensure_blackboard_and_behavior_tree():
    blackboard_path = "/Game/AI/Robot/BB_RobotCompanion"
    behavior_tree_path = "/Game/AI/Robot/BT_RobotCompanion"

    if not unreal.EditorAssetLibrary.does_asset_exist(blackboard_path):
        require(unreal.BlackboardService.create_blackboard(blackboard_path, ""),
                "Cannot create robot Blackboard")
        for key_name, key_type in (
                ("OwnerActor", "Object"),
                ("TargetActor", "Object"),
                ("DesiredMoveLocation", "Vector"),
                ("CommandLocation", "Vector"),
                ("CommandMode", "Name"),
                ("HasTarget", "Bool"),
                ("TargetInAttackRange", "Bool"),
                ("HasLineOfSight", "Bool"),
                ("IsDead", "Bool")):
            unreal.BlackboardService.add_blackboard_key(
                blackboard_path, key_name, key_type, False)
        unreal.BlackboardService.set_blackboard_key_object_class(
            blackboard_path, "OwnerActor", "/Script/Engine.Pawn")
        unreal.BlackboardService.set_blackboard_key_object_class(
            blackboard_path, "TargetActor", "/Script/Engine.Actor")
        log(f"Created {blackboard_path}")

    if not unreal.EditorAssetLibrary.does_asset_exist(behavior_tree_path):
        error = unreal.BehaviorTreeService.create_behavior_tree(
            behavior_tree_path, blackboard_path)
        require(not error, f"Cannot create robot BehaviorTree: {error}")

    tree = unreal.BehaviorTreeService.get_tree(behavior_tree_path)
    if "BTComposite_Selector" not in tree:
        selector = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, "Root", "/Script/AIModule.BTComposite_Selector", 0)
        service = unreal.BehaviorTreeService.add_service(
            behavior_tree_path, selector,
            "/Script/NewWorldOrder.ShootBTService_RobotRefreshContext", -1)
        require(not service.startswith("ERROR:"), service)

        attack = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, selector, "/Script/AIModule.BTComposite_Sequence", 0)
        attack_decorator = unreal.BehaviorTreeService.add_decorator(
            behavior_tree_path, attack, "/Script/AIModule.BTDecorator_Blackboard", -1)
        result = unreal.BehaviorTreeService.set_node_blackboard_key(
            behavior_tree_path, attack_decorator, "BlackboardKey", "TargetInAttackRange")
        require(result.success, result.error)

        fire = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, attack,
            "/Script/NewWorldOrder.ShootBTTask_RobotFire", 0)
        result = unreal.BehaviorTreeService.set_node_blackboard_key(
            behavior_tree_path, fire, "TargetActorKey", "TargetActor")
        require(result.success, result.error)

        fire_wait = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, attack, "/Script/AIModule.BTTask_Wait", 1)
        result = unreal.BehaviorTreeService.set_node_property_value(
            behavior_tree_path, fire_wait, "WaitTime",
            '(DefaultValue=0.150000,Key="")')
        require(result.success, result.error)

        move = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, selector, "/Script/AIModule.BTTask_MoveTo", 1)
        result = unreal.BehaviorTreeService.set_node_blackboard_key(
            behavior_tree_path, move, "BlackboardKey", "DesiredMoveLocation")
        require(result.success, result.error)
        result = unreal.BehaviorTreeService.set_node_property_value(
            behavior_tree_path, move, "AcceptableRadius",
            '(DefaultValue=120.000000,Key="")')
        require(result.success, result.error)

        idle_wait = unreal.BehaviorTreeService.add_node(
            behavior_tree_path, selector, "/Script/AIModule.BTTask_Wait", 2)
        result = unreal.BehaviorTreeService.set_node_property_value(
            behavior_tree_path, idle_wait, "WaitTime",
            '(DefaultValue=0.200000,Key="")')
        require(result.success, result.error)

    # 早期试跑脚本可能保留 BTTask_Wait 的默认 5 秒；这里总是把正式节奏回写到磁盘。
    for node_path, property_name, value in (
            ("Root/Selector[0]/Sequence[0]/@decorator[0]", "FlowAbortMode",
             "Both"),
            ("Root/Selector[0]/Sequence[0]/Wait[0]", "WaitTime",
             '(DefaultValue=0.150000,Key="")'),
            ("Root/Selector[0]/Move To[0]", "AcceptableRadius",
             '(DefaultValue=120.000000,Key="")'),
            ("Root/Selector[0]/Wait[0]", "WaitTime",
             '(DefaultValue=0.200000,Key="")')):
        result = unreal.BehaviorTreeService.set_node_property_value(
            behavior_tree_path, node_path, property_name, value)
        require(result.success, f"{node_path}.{property_name}: {result.error}")

    tree_snapshot = unreal.BehaviorTreeService.get_tree(behavior_tree_path)
    if "Robot Fire Through GAS" in tree_snapshot:
        error = unreal.BehaviorTreeService.set_node_name(
            behavior_tree_path,
            "Root/Selector[0]/Sequence[0]/Robot Fire Through GAS[0]",
            "Robot Attack Through GAS")
        require(not error, f"Cannot rename robot attack task: {error}")

    error = unreal.BehaviorTreeService.compile_and_save(behavior_tree_path)
    require(not error, f"BehaviorTree compile failed: {error}")
    findings = list(unreal.BehaviorTreeService.validate_tree(behavior_tree_path))
    require(not findings, f"BehaviorTree validation failed: {findings}")
    log("Robot Blackboard and BehaviorTree are valid")


def ensure_animation_assets():
    anim_bp_path = f"{ANIM_ROOT}/ABP_RobotCompanion"
    if not unreal.EditorAssetLibrary.does_asset_exist(anim_bp_path):
        factory = unreal.AnimBlueprintFactory()
        factory.set_editor_property(
            "target_skeleton",
            require(unreal.load_asset("/Game/Characters/RadicalMike/Mesh/SK_MegaMikeZ"),
                    "Missing RadicalMike skeleton"))
        factory.set_editor_property("parent_class", unreal.ShootRobotAnimInstance)
        anim_bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "ABP_RobotCompanion", ANIM_ROOT, unreal.AnimBlueprint, factory)
        require(anim_bp, "Cannot create ABP_RobotCompanion")

        spec = {
            "states": [
                {"name": "Idle", "animation": f"{ANIM_ROOT}/Anim_ZMIKE_Idle",
                 "loop": True, "pos": [0, 0]},
                {"name": "Walk", "animation": f"{ANIM_ROOT}/Anim_ZMIKE_Walk",
                 "loop": True, "pos": [350, 0]},
                {"name": "Run", "animation": f"{ANIM_ROOT}/Anim_ZMIKE_Run",
                 "loop": True, "pos": [700, 0]},
                {"name": "Death", "animation": f"{ANIM_ROOT}/Anim_ZMIKE_DeathState",
                 "loop": False, "pos": [350, 300]},
            ],
            "transitions": [
                {"from": "Idle", "to": "Walk",
                 "rule": {"type": "comparison", "variable": "GroundSpeed",
                          "op": "greater", "value": 10}, "blend": 0.2},
                {"from": "Walk", "to": "Idle",
                 "rule": {"type": "comparison", "variable": "GroundSpeed",
                          "op": "less_equal", "value": 10}, "blend": 0.2},
                {"from": "Walk", "to": "Run",
                 "rule": {"type": "comparison", "variable": "GroundSpeed",
                          "op": "greater", "value": 300}, "blend": 0.2},
                {"from": "Run", "to": "Walk",
                 "rule": {"type": "comparison", "variable": "GroundSpeed",
                          "op": "less_equal", "value": 300}, "blend": 0.2},
                {"from": "Idle", "to": "Death",
                 "rule": {"type": "bool", "variable": "bIsDead"}, "blend": 0.1},
                {"from": "Walk", "to": "Death",
                 "rule": {"type": "bool", "variable": "bIsDead"}, "blend": 0.1},
                {"from": "Run", "to": "Death",
                 "rule": {"type": "bool", "variable": "bIsDead"}, "blend": 0.1},
            ],
            "entry": "Idle",
        }
        report = json.loads(unreal.AnimGraphService.build_state_machine(
            anim_bp_path, "RobotLocomotion", json.dumps(spec), -350.0, 0.0))
        require(report.get("success"), f"State machine build failed: {report}")
        info = unreal.AnimGraphService.get_state_machine_info(
            anim_bp_path, "RobotLocomotion")
        output_node = unreal.AnimGraphService.get_output_pose_node_id(
            anim_bp_path, "AnimGraph")
        slot_node = unreal.AnimGraphService.add_slot_node(
            anim_bp_path, "AnimGraph", "DefaultSlot", 0.0, 0.0)
        require(info and output_node and slot_node, "Cannot find AnimGraph nodes")
        unreal.AnimGraphService.disconnect_anim_node(
            anim_bp_path, "AnimGraph", str(info.node_id), "Pose")
        require(unreal.AnimGraphService.connect_anim_nodes(
            anim_bp_path, "AnimGraph", str(info.node_id), "Pose", slot_node, "Source"),
            "Cannot connect RobotLocomotion to DefaultSlot")
        require(unreal.AnimGraphService.connect_anim_nodes(
            anim_bp_path, "AnimGraph", slot_node, "Pose", output_node, "Result"),
            "Cannot connect DefaultSlot to Output Pose")

    compile_blueprint(anim_bp_path)

    # C++ ShootRobotAnimInstance 已持续更新 GroundSpeed/bIsFalling/bIsDead；AnimGraph 负责消费。
    # 旧首版只有地面状态，500cm 召唤下落时会僵在 Idle，因此补齐空中态而不另写 Tick 表现链。
    state_names = {
        str(state.state_name)
        for state in unreal.AnimGraphService.list_states_in_machine(
            anim_bp_path, "RobotLocomotion")
        if str(state.state_type) == "State"
    }
    if "Airborne" not in state_names:
        airborne_spec = {
            "states": [
                {"name": "Airborne", "animation": f"{ANIM_ROOT}/Anim_ZMIKE_JumpApex",
                 "loop": True, "pos": [700, 300]},
            ],
            "transitions": [
                {"from": "Idle", "to": "Airborne",
                 "rule": {"type": "bool", "variable": "bIsFalling"}, "blend": 0.1},
                {"from": "Walk", "to": "Airborne",
                 "rule": {"type": "bool", "variable": "bIsFalling"}, "blend": 0.1},
                {"from": "Run", "to": "Airborne",
                 "rule": {"type": "bool", "variable": "bIsFalling"}, "blend": 0.1},
                {"from": "Airborne", "to": "Idle",
                 "rule": {"type": "bool", "variable": "bIsFalling"},
                 "blend": 0.12},
                {"from": "Airborne", "to": "Death",
                 "rule": {"type": "bool", "variable": "bIsDead"}, "blend": 0.1},
            ],
        }
        report = json.loads(unreal.AnimGraphService.build_state_machine(
            anim_bp_path, "RobotLocomotion", json.dumps(airborne_spec), -350.0, 0.0))
        require(report.get("success"), f"Airborne state build failed: {report}")

    # BuildStateMachine 当前不会消费 spec 中的 negate 字段；无论状态是新建还是已存在，
    # 都显式把回落规则写成 !bIsFalling，避免机器人落地后一直卡在 Airborne。
    require(unreal.AnimGraphService.set_transition_rule_from_bool(
        anim_bp_path, "RobotLocomotion", "Airborne", "Idle", "bIsFalling", True),
        "Cannot set Airborne -> Idle transition to !bIsFalling")

    compile_blueprint(anim_bp_path)
    validation = unreal.AnimGraphService.validate_state_machine(
        anim_bp_path, "RobotLocomotion")
    require(validation.is_valid,
            f"RobotLocomotion validation failed: {list(validation.errors)}")

    montage_specs = (
        ("AM_Robot_FireR", "Anim_ZMIKE_FireR",
         "/Script/NewWorldOrder.ShootAnimNotify_RobotFire", 0.25, "RobotFire"),
        ("AM_Robot_FireL", "Anim_ZMIKE_FireL",
         "/Script/NewWorldOrder.ShootAnimNotify_RobotFire", 0.25, "RobotFire"),
        ("AM_Robot_ClawL", "Anim_ZMIKE_ClawL",
         "/Script/NewWorldOrder.ShootAnimNotify_RobotMelee", 0.45, "RobotMelee"),
        ("AM_Robot_Chomp", "Anim_ZMIKE_Chomp",
         "/Script/NewWorldOrder.ShootAnimNotify_RobotMelee", 0.25, "RobotMelee"),
        ("AM_Robot_Death", "Anim_ZMIKE_DeathState", "", 0.0, ""),
    )
    for montage_name, sequence_name, notify_class, notify_time, notify_name in montage_specs:
        montage_path = f"{ANIM_ROOT}/{montage_name}"
        was_created = False
        if not unreal.EditorAssetLibrary.does_asset_exist(montage_path):
            result = unreal.AnimMontageService.create_montage_from_animation(
                f"{ANIM_ROOT}/{sequence_name}", ANIM_ROOT, montage_name)
            require(result, f"Cannot create {montage_path}")
            was_created = True
        montage = require(unreal.load_asset(montage_path), f"Cannot load {montage_path}")
        # UE 5.8 把 UAnimMontage.Notifies 标成 protected，Python 无法安全回读。Notify 与 Montage
        # 在同一创建事务中写入；已存在资产不重复追加，避免每次脚本执行产生双重伤害帧。
        if notify_class and was_created:
            notify_index = unreal.AnimMontageService.add_notify(
                montage_path, notify_class, notify_time, notify_name)
            require(notify_index >= 0, f"Cannot add {notify_name} notify to {montage_path}")
        save(montage)
    log("Robot AnimBP and montages are configured")


def ensure_gameplay_cue(path, tag_name, particle_path, effect_scale, niagara_path=None):
    create_blueprint(path, unreal.ShootGameplayCueNotify_Presentation)
    compile_blueprint(path)
    cdo = unreal.get_default_object(generated_class(path))
    cdo.set_editor_property("gameplay_cue_tag", gameplay_tag(tag_name))
    cdo.set_editor_property("cascade_system", require(unreal.load_asset(particle_path),
                                                      f"Missing particle {particle_path}"))
    cdo.set_editor_property(
        "niagara_system",
        require(unreal.load_asset(niagara_path), f"Missing Niagara {niagara_path}")
        if niagara_path else None)
    cdo.set_editor_property("cascade_lifetime_seconds", 1.5)
    cdo.set_editor_property("effect_scale", effect_scale)
    compile_blueprint(path)


def ensure_blueprints_and_data():
    controller_bp = f"{ROBOT_ROOT}/BP_RobotCompanionController"
    character_bp = f"{ROBOT_ROOT}/BP_RobotCompanionCharacter"
    summon_ga_bp = f"{ABILITY_ROOT}/GA_RobotCompanion"
    fire_ga_bp = f"{ABILITY_ROOT}/GA_RobotFire"
    melee_ga_bp = f"{ABILITY_ROOT}/GA_RobotMelee"

    for path, parent in (
            (controller_bp, unreal.ShootRobotCompanionController),
            (character_bp, unreal.ShootRobotCompanionCharacter),
            (summon_ga_bp, unreal.ShootGA_RobotCompanion),
            (fire_ga_bp, unreal.ShootGA_RobotFire),
            (melee_ga_bp, unreal.ShootGA_RobotMelee)):
        create_blueprint(path, parent)
        compile_blueprint(path)

    ensure_gameplay_cue(
        f"{CUE_ROOT}/GCN_Skill_RobotCompanion_Fire",
        "GameplayCue.Skill.RobotCompanion.Fire",
        "/Game/Effects/FXVarietyPack/Particles/P_ky_shotShockwave",
        unreal.Vector(0.35, 0.35, 0.35))
    ensure_gameplay_cue(
        f"{CUE_ROOT}/GCN_Skill_RobotCompanion_Melee",
        "GameplayCue.Skill.RobotCompanion.Melee",
        "/Game/Effects/FXVarietyPack/Particles/P_ky_hit1",
        unreal.Vector(0.75, 0.75, 0.75))
    ensure_gameplay_cue(
        f"{CUE_ROOT}/GCN_Skill_RobotCompanion_SelfDestruct",
        "GameplayCue.Skill.RobotCompanion.SelfDestruct",
        "/Game/Effects/FXVarietyPack/Particles/P_ky_explosion",
        unreal.Vector(1.0, 1.0, 1.0))
    ensure_gameplay_cue(
        f"{CUE_ROOT}/GCN_Skill_RobotCompanion_SummonImpact",
        "GameplayCue.Skill.RobotCompanion.SummonImpact",
        "/Game/Effects/FXVarietyPack/Particles/P_ky_explosion",
        unreal.Vector(0.55, 0.55, 0.55),
        "/Game/Effects/Particles/Impacts/NS_ImpactConcrete")

    fire_cdo = unreal.get_default_object(generated_class(fire_ga_bp))
    fire_cdo.set_editor_property(
        "fire_gameplay_cue_tag", gameplay_tag("GameplayCue.Skill.RobotCompanion.Fire"))
    compile_blueprint(fire_ga_bp)

    melee_cdo = unreal.get_default_object(generated_class(melee_ga_bp))
    melee_cdo.set_editor_property(
        "melee_gameplay_cue_tag", gameplay_tag("GameplayCue.Skill.RobotCompanion.Melee"))
    compile_blueprint(melee_ga_bp)

    robot_combat_set_path = f"{ABILITY_SET_ROOT}/DA_AbilitySet_RobotCombat"
    robot_combat_set = create_data_asset(robot_combat_set_path, unreal.ShootAbilitySet)
    robot_combat_set.set_editor_property(
        "granted_gameplay_abilities",
        [make_ability_entry(generated_class(fire_ga_bp)),
         make_ability_entry(generated_class(melee_ga_bp))])
    robot_combat_set.set_editor_property("granted_gameplay_effects", [])
    robot_combat_set.set_editor_property("granted_attributes", [])
    save(robot_combat_set)

    controller_class = generated_class(controller_bp)
    character_class = generated_class(character_bp)
    character_cdo = unreal.get_default_object(character_class)
    character_cdo.set_editor_property("ai_controller_class", controller_class)
    character_cdo.set_editor_property(
        "auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
    character_cdo.set_editor_property(
        "behavior_tree", unreal.load_asset("/Game/AI/Robot/BT_RobotCompanion"))
    character_cdo.set_editor_property("robot_ability_set", robot_combat_set)
    character_cdo.set_editor_property(
        "robot_vitals_effect_class", unreal.ShootEffect_RobotCompanionVitals)
    character_cdo.set_editor_property(
        "damage_effect_class", unreal.ShootEffect_DamageSetByCaller)
    character_cdo.set_editor_property(
        "fire_right_montage", unreal.load_asset(f"{ANIM_ROOT}/AM_Robot_FireR"))
    character_cdo.set_editor_property(
        "fire_left_montage", unreal.load_asset(f"{ANIM_ROOT}/AM_Robot_FireL"))
    character_cdo.set_editor_property(
        "claw_left_montage", unreal.load_asset(f"{ANIM_ROOT}/AM_Robot_ClawL"))
    character_cdo.set_editor_property(
        "chomp_montage", unreal.load_asset(f"{ANIM_ROOT}/AM_Robot_Chomp"))
    character_cdo.set_editor_property(
        "death_montage", unreal.load_asset(f"{ANIM_ROOT}/AM_Robot_Death"))
    character_cdo.set_editor_property(
        "self_destruct_cue_tag",
        gameplay_tag("GameplayCue.Skill.RobotCompanion.SelfDestruct"))
    character_cdo.set_editor_property(
        "summon_impact_cue_tag",
        gameplay_tag("GameplayCue.Skill.RobotCompanion.SummonImpact"))

    # RadicalMike SkeletalMesh 自带三个正式 Socket，已编码武器与背包的相对偏移和朝向。
    # 不能退化成 hand_r/hand_l/spine_03 骨骼直绑，否则零相对变换会嵌入手腕或躯干。
    character_cdo.set_editor_property("right_weapon_socket", unreal.Name("Buster_RSocket"))
    character_cdo.set_editor_property("left_weapon_socket", unreal.Name("Buster_LSocket"))
    character_cdo.set_editor_property("power_pod_socket", unreal.Name("PowerPod"))
    character_cdo.set_editor_property("summon_impact_damage", 45.0)
    character_cdo.set_editor_property("summon_impact_radius", 350.0)
    character_cdo.set_editor_property("self_destruct_damage", 120.0)
    character_cdo.set_editor_property("self_destruct_radius", 450.0)
    character_cdo.set_editor_property("balanced_aggro_range", 2400.0)
    character_cdo.set_editor_property("ranged_aggro_range", 3400.0)
    character_cdo.set_editor_property("melee_aggro_range", 3600.0)
    character_cdo.set_editor_property("balanced_desired_combat_range", 600.0)
    character_cdo.set_editor_property("ranged_desired_combat_range", 950.0)
    character_cdo.set_editor_property("melee_desired_combat_range", 165.0)
    character_cdo.set_editor_property("balanced_leash_distance", 1400.0)
    character_cdo.set_editor_property("base_shot_damage", 8.0)
    character_cdo.set_editor_property("damage_per_level", 2.0)
    character_cdo.set_editor_property("base_fire_interval", 0.7)
    character_cdo.set_editor_property("fire_interval_per_level", 0.05)
    character_cdo.set_editor_property("ranged_fire_interval_multiplier", 0.6)
    character_cdo.set_editor_property("balanced_fire_interval_multiplier", 0.9)
    character_cdo.set_editor_property("melee_range", 250.0)
    character_cdo.set_editor_property("base_melee_damage", 32.0)
    character_cdo.set_editor_property("melee_damage_per_level", 6.0)
    character_cdo.set_editor_property("chomp_damage_multiplier", 1.35)
    character_cdo.set_editor_property("claw_interval", 1.0)
    character_cdo.set_editor_property("chomp_interval", 1.35)

    mesh_component = character_cdo.get_editor_property("mesh")
    mesh_component.set_editor_property(
        "skeletal_mesh_asset",
        unreal.load_asset("/Game/Characters/RadicalMike/Mesh/SKM_MegaMikeZ"))
    mesh_component.set_editor_property(
        "anim_class", generated_class(f"{ANIM_ROOT}/ABP_RobotCompanion"))
    mesh_component.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -88.0))
    # Unreal Python 的 Rotator 位置参数顺序是 Roll/Pitch/Yaw；这里明确把 -90 写入 Yaw。
    mesh_component.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, -90.0))

    movement_component = character_cdo.get_editor_property("character_movement")
    movement_component.set_editor_property("use_controller_desired_rotation", True)
    movement_component.set_editor_property("orient_rotation_to_movement", False)

    buster = unreal.load_asset("/Game/Characters/RadicalMike/Mesh/SM_RadicalBuster")
    power_pod = unreal.load_asset("/Game/Characters/RadicalMike/Mesh/SM_PowerPod")
    character_cdo.get_editor_property("right_weapon").set_editor_property("static_mesh", buster)
    character_cdo.get_editor_property("left_weapon").set_editor_property("static_mesh", buster)
    character_cdo.get_editor_property("power_pod").set_editor_property("static_mesh", power_pod)
    character_cdo.get_editor_property("right_muzzle").set_editor_property(
        "relative_location", unreal.Vector(75.0, 0.0, 0.0))
    character_cdo.get_editor_property("left_muzzle").set_editor_property(
        "relative_location", unreal.Vector(75.0, 0.0, 0.0))
    compile_blueprint(character_bp)

    summon_cdo = unreal.get_default_object(generated_class(summon_ga_bp))
    summon_cdo.set_editor_property("robot_character_class", character_class)
    summon_cdo.set_editor_property(
        "destroyed_cooldown_effect_class", unreal.ShootEffect_RobotCompanionCooldown)
    summon_cdo.set_editor_property("companion_lifetime", 18.0)
    compile_blueprint(summon_ga_bp)

    player_skill_set_path = f"{ABILITY_SET_ROOT}/DA_AbilitySet_Skill_RobotCompanion"
    player_skill_set = create_data_asset(player_skill_set_path, unreal.ShootAbilitySet)
    player_skill_set.set_editor_property(
        "granted_gameplay_abilities",
        [make_ability_entry(generated_class(summon_ga_bp))])
    player_skill_set.set_editor_property("granted_gameplay_effects", [])
    player_skill_set.set_editor_property("granted_attributes", [])
    save(player_skill_set)

    definition_path = f"{DEFINITION_ROOT}/DA_Skill_RobotCompanion"
    definition = create_data_asset(definition_path, unreal.ShootSkillDefinition)
    ranged_tag = gameplay_tag("Ability.Mode.Robot.Ranged")
    melee_tag = gameplay_tag("Ability.Mode.Robot.Melee")
    balanced_tag = gameplay_tag("Ability.Mode.Robot.Balanced")
    definition.set_editor_property(
        "skill_tag", gameplay_tag("Ability.Skill.RobotCompanion"))
    definition.set_editor_property("display_name", unreal.Text("机器人伙伴"))
    definition.set_editor_property(
        "description",
        unreal.Text("召唤限时作战的 RadicalMike。远程压制以低单发高射频输出，近战强袭追近后用左爪与咬击造成高伤害，均衡护卫优先处理主人附近威胁并按距离择招。再次按技能键切换模式；被击毁或到期自爆后进入重召冷却。"))
    definition.set_editor_property("ability_set", player_skill_set)
    definition.set_editor_property("max_level", 3)
    definition.set_editor_property(
        "cooldown_tag", gameplay_tag("Cooldown.Skill.RobotCompanion"))
    definition.set_editor_property("mode_tags", [ranged_tag, melee_tag, balanced_tag])

    ranged_icon = make_icon("/Game/UI/Skills/Art/T_Skill_TacticalScan")
    melee_icon = make_icon("/Game/UI/Skills/Art/T_Skill_TacticalAssault")
    balanced_icon = make_icon("/Game/UI/Skills/Art/T_Skill_SteelBulwark")
    definition.set_editor_property("icon", ranged_icon)
    definition.set_editor_property(
        "mode_icons",
        {ranged_tag: ranged_icon, melee_tag: melee_icon, balanced_tag: balanced_icon})
    definition.set_editor_property(
        "mode_display_names",
        {ranged_tag: unreal.Text("机器人：远程压制"),
         melee_tag: unreal.Text("机器人：近战强袭"),
         balanced_tag: unreal.Text("机器人：均衡护卫")})
    save(definition)

    config = require(unreal.load_asset("/Game/GameFramework/Skills/DA_SkillLoadoutConfig_PVE"),
                     "Missing PVE skill config")
    pool = list(config.get_editor_property("random_skill_pool"))
    if definition not in pool:
        pool.append(definition)
    config.set_editor_property("random_skill_pool", pool)
    save(config)

    pickup_bp = "/Game/Gameplay/Interactables/SkillAcquisition/BP_RobotSkillPickup"
    create_blueprint(pickup_bp, unreal.ShootRandomSkillPickup)
    compile_blueprint(pickup_bp)
    pickup_cdo = unreal.get_default_object(generated_class(pickup_bp))
    pickup_cdo.set_editor_property("offered_skill_definition", definition)
    pickup_cdo.set_editor_property("interaction_text", unreal.Text("机器人伙伴"))
    pickup_cdo.set_editor_property(
        "interaction_sub_text", unreal.Text("获得或升级机器人伙伴技能"))
    pickup_visual = pickup_cdo.get_editor_property("visual_component")
    pickup_visual.set_editor_property("static_mesh", power_pod)
    pickup_visual.set_editor_property("relative_scale3d", unreal.Vector(1.25, 1.25, 1.25))
    compile_blueprint(pickup_bp)
    log("Robot Blueprints, GameplayCues, AbilitySets and Definition are configured")
    return generated_class(pickup_bp)


def place_fixed_pickup(pickup_class):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    for map_path in ("/Game/Maps/TestMap_ListenServer", "/Game/Maps/TestMap_SplitScreen"):
        require(unreal.EditorLoadingAndSavingUtils.load_map(map_path),
                f"Cannot load {map_path}")
        actors = list(actor_subsystem.get_all_level_actors())
        pickup = next((actor for actor in actors
                       if actor.get_actor_label() == "RobotSkillPickup"), None)
        if pickup is None:
            starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
            require(starts, f"No PlayerStart in {map_path}")
            start = sorted(starts, key=lambda actor: actor.get_actor_label())[0]
            location = (start.get_actor_location()
                        + start.get_actor_forward_vector() * 320.0
                        + start.get_actor_right_vector() * 160.0
                        + unreal.Vector(0.0, 0.0, 45.0))
            pickup = actor_subsystem.spawn_actor_from_class(
                pickup_class, location, unreal.Rotator(0.0, 0.0, 0.0))
            require(pickup, f"Cannot place RobotSkillPickup in {map_path}")
        pickup.set_actor_label("RobotSkillPickup")
        pickup.set_folder_path("SkillAcquisition")
        require(level_subsystem.save_current_level(), f"Cannot save {map_path}")
        log(f"{map_path}: RobotSkillPickup at {pickup.get_actor_location()}")


ensure_blackboard_and_behavior_tree()
ensure_animation_assets()
pickup_class = ensure_blueprints_and_data()
place_fixed_pickup(pickup_class)
log("Robot companion asset pipeline completed")
