import unreal


IMC_PATH = "/Game/Input/MappingContexts/IMC_Default"
SENSITIVITY_PATH = "/Game/Input/Data/DA_AimSensitivity"


def log(message):
    unreal.log(f"[SessionUI] {message}")
    print(f"[SessionUI] {message}")


def new_subobject(cls, outer, name):
    existing = unreal.find_object(outer, name)
    if existing:
        return existing
    return unreal.new_object(cls, outer=outer, name=name)


def configure_player_mappable_keys(imc, mappings):
    # 同一个语义方向（例如 W 和上方向键）共享 MappingName，玩家改键时按语义而不是资产索引保存。
    semantic_rows = {
        0: ("Jump", "Jump"),
        2: ("MoveForward", "Move Forward"), 6: ("MoveForward", "Move Forward"),
        3: ("MoveBackward", "Move Backward"), 7: ("MoveBackward", "Move Backward"),
        4: ("MoveLeft", "Move Left"), 9: ("MoveLeft", "Move Left"),
        5: ("MoveRight", "Move Right"), 8: ("MoveRight", "Move Right"),
        14: ("Interact", "Interact"),
        16: ("Skill1", "Skill 1"), 17: ("Skill2", "Skill 2"),
        18: ("Skill3", "Skill 3"), 19: ("Skill4", "Skill 4"),
        20: ("Run", "Run"), 22: ("Crouch", "Crouch"),
        24: ("Reload", "Reload"), 26: ("Aim", "Aim"),
        28: ("Attack", "Attack"), 30: ("Dodge", "Dodge"),
        # 30-34 是手柄专用 LB Modifier 与 LB+Y/X/A/B 技能映射，不进入键鼠改键页。
        35: ("OpenMenu", "Open Menu"),
        37: ("WeaponNext", "Next Weapon"), 38: ("WeaponPrevious", "Previous Weapon"),
        41: ("DropWeapon", "Drop Weapon"),
        43: ("ToggleCamera", "Toggle Camera"),
    }
    settings_by_name = {}
    for index, mapping in enumerate(mappings):
        if index not in semantic_rows:
            mapping.set_editor_property(
                "setting_behavior", unreal.PlayerMappableKeySettingBehaviors.IGNORE_SETTINGS)
            mapping.set_editor_property("player_mappable_key_settings", None)
            continue

        mapping_name, display_name = semantic_rows[index]
        settings = settings_by_name.get(mapping_name)
        if settings is None:
            settings = new_subobject(
                unreal.PlayerMappableKeySettings, imc, f"PMKS_{mapping_name}")
            settings.set_editor_property("name", unreal.Name(mapping_name))
            settings.set_editor_property("display_name", unreal.Text(display_name))
            settings.set_editor_property("display_category", unreal.Text("Keyboard & Mouse"))
            settings_by_name[mapping_name] = settings

        mapping.set_editor_property(
            "setting_behavior", unreal.PlayerMappableKeySettingBehaviors.OVERRIDE_SETTINGS)
        mapping.set_editor_property("player_mappable_key_settings", settings)

    return len(settings_by_name)


def configure_live_setting_modifiers(imc, mappings):
    sensitivity = unreal.load_asset(SENSITIVITY_PATH)
    if not sensitivity:
        raise RuntimeError(f"Missing sensitivity data asset: {SENSITIVITY_PATH}")

    # 左摇杆移动：移除固定 DeadZone，改为实时读取 SharedSettings 的 MoveStickDeadZone。
    move_mapping = mappings[10]
    move_modifiers = [
        modifier for modifier in list(move_mapping.get_editor_property("modifiers"))
        if modifier.get_class().get_name() not in {
            "InputModifierDeadZone", "LyraInputModifierDeadZone"
        }
    ]
    move_deadzone = new_subobject(
        unreal.LyraInputModifierDeadZone, imc, "NWO_MoveSettingsDeadZone")
    move_deadzone.set_editor_property("deadzone_stick", unreal.DeadzoneStick.MOVE_STICK)
    move_modifiers.append(move_deadzone)
    move_mapping.set_editor_property("modifiers", move_modifiers)

    # 鼠标 Look：保留项目已有轴向修饰器，再叠加鼠标灵敏度、ADS 倍率和反转设置。
    mouse_mapping = mappings[11]
    mouse_modifiers = [
        modifier for modifier in list(mouse_mapping.get_editor_property("modifiers"))
        if modifier.get_class().get_name() not in {
            "LyraSettingBasedScalar", "LyraInputModifierAimInversion"
        }
    ]
    mouse_scalar = new_subobject(
        unreal.LyraSettingBasedScalar, imc, "NWO_MouseSettingsScalar")
    mouse_scalar.set_editor_property("x_axis_scalar_setting_name", unreal.Name("MouseSensitivityX"))
    mouse_scalar.set_editor_property("y_axis_scalar_setting_name", unreal.Name("MouseSensitivityY"))
    mouse_scalar.set_editor_property("apply_targeting_multiplier_when_ads", True)
    mouse_inversion = new_subobject(
        unreal.LyraInputModifierAimInversion, imc, "NWO_MouseAimInversion")
    mouse_modifiers.extend([mouse_scalar, mouse_inversion])
    mouse_mapping.set_editor_property("modifiers", mouse_modifiers)

    # 右摇杆 Look：固定 DeadZone 改为设置驱动，并实时选择普通/ADS 手柄灵敏度预设。
    gamepad_mapping = mappings[12]
    gamepad_modifiers = [
        modifier for modifier in list(gamepad_mapping.get_editor_property("modifiers"))
        if modifier.get_class().get_name() not in {
            "InputModifierDeadZone", "LyraInputModifierDeadZone",
            "LyraInputModifierGamepadSensitivity", "LyraInputModifierAimInversion"
        }
    ]
    look_deadzone = new_subobject(
        unreal.LyraInputModifierDeadZone, imc, "NWO_LookSettingsDeadZone")
    look_deadzone.set_editor_property("deadzone_stick", unreal.DeadzoneStick.LOOK_STICK)
    gamepad_sensitivity = new_subobject(
        unreal.LyraInputModifierGamepadSensitivity, imc, "NWO_GamepadSensitivity")
    gamepad_sensitivity.set_editor_property("sensitivity_level_table", sensitivity)
    gamepad_sensitivity.set_editor_property("use_ads_preset_when_aiming", True)
    gamepad_inversion = new_subobject(
        unreal.LyraInputModifierAimInversion, imc, "NWO_GamepadAimInversion")
    gamepad_modifiers.extend([look_deadzone, gamepad_sensitivity, gamepad_inversion])
    gamepad_mapping.set_editor_property("modifiers", gamepad_modifiers)


def configure_input_mapping():
    imc = unreal.load_asset(IMC_PATH)
    if not imc:
        raise RuntimeError(f"Missing input mapping context: {IMC_PATH}")

    default_mappings = imc.get_editor_property("default_key_mappings")
    mappings = list(default_mappings.get_editor_property("mappings"))
    if len(mappings) != 44:
        raise RuntimeError(
            f"{IMC_PATH} mapping count changed from audited value 44 to {len(mappings)}; "
            "refusing index-based migration so a human can re-audit the asset.")

    semantic_count = configure_player_mappable_keys(imc, mappings)
    configure_live_setting_modifiers(imc, mappings)
    default_mappings.set_editor_property("mappings", mappings)
    imc.set_editor_property("default_key_mappings", default_mappings)
    unreal.EditorAssetLibrary.save_loaded_asset(imc, only_if_is_dirty=False)

    mappable_count = sum(
        mapping.get_editor_property("setting_behavior") ==
        unreal.PlayerMappableKeySettingBehaviors.OVERRIDE_SETTINGS
        for mapping in mappings)
    log(f"Configured {IMC_PATH}: {len(mappings)} mappings, "
        f"{mappable_count} keyboard/mouse entries, {semantic_count} semantic rows")
    for index in (10, 11, 12):
        names = [modifier.get_class().get_name()
                 for modifier in mappings[index].get_editor_property("modifiers")]
        log(f"Mapping[{index}] modifiers={names}")


def compile_and_report_blueprint(asset_path):
    blueprint = unreal.load_asset(asset_path)
    if not blueprint:
        raise RuntimeError(f"Missing Blueprint: {asset_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    status = blueprint.get_editor_property("status")
    generated_class = blueprint.generated_class()
    log(f"Blueprint {asset_path}: status={status}, generated_class="
        f"{generated_class.get_name() if generated_class else 'None'}")


configure_input_mapping()
compile_and_report_blueprint("/Game/UI/FrontEnd/W_FrontEnd")
compile_and_report_blueprint("/Game/UI/Menu/Experiences/W_HostSessionScreen")
log("Input configuration and Blueprint parent verification completed")
