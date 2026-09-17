import unreal


REPARENTS = [
    ("/Game/UI/Foundation/Widgets/BottomBar/W_BoundActionButton", "/Script/NewWorldOrder.LyraBoundActionButton"),
    ("/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding", "/Script/NewWorldOrder.LyraSettingsListEntrySetting_KeyboardInput"),
    ("/Game/UI/Settings/W_LyraSettingScreen", "/Script/NewWorldOrder.LyraSettingScreen"),
]

COMPILE_ORDER = [
    # 按钮、箭头和 Modal 使用项目 Foundation 资产，不再编译已删除的 Lyra 重复资产。
    "/Game/UI/Foundation/Buttons/W_MenuButton",
    "/Game/UI/Foundation/Buttons/W_MenuButton_Modal",
    "/Game/UI/Foundation/Buttons/W_LyraArrowButton",
    "/Game/UI/Foundation/Widgets/BottomBar/W_BoundActionButton",
    "/Game/UI/Foundation/Widgets/BottomBar/W_BottomActionBar",
    "/Game/UI/Settings/Editors/W_SettingEntryBackground",
    "/Game/UI/Settings/Editors/W_SettingsRotator",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Header",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Missing",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Action",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Discrete",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_Scalar",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_SubCollection",
    "/Game/UI/Settings/Screens/W_PressAnyKey",
    "/Game/UI/Settings/Screens/W_KeyAlreadyBoundWarning",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding",
    "/Game/UI/Settings/W_GameSettingsDetailView",
    "/Game/UI/Settings/W_SettingsPanel",
    "/Game/UI/Settings/W_LyraSettingScreen",
]

FRONT_END = "/Game/UI/FrontEnd/W_FrontEnd"
SETTINGS_SCREEN_CLASS = "/Game/UI/Settings/W_LyraSettingScreen.W_LyraSettingScreen_C"
SETTINGS_VISUAL_DATA = "/Game/UI/Settings/GameSettingRegistryVisuals"
KEYBOARD_SETTING_CLASS = "/Script/NewWorldOrder.LyraSettingKeyboardInput"
KEYBOARD_ENTRY = "/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding"
KEYBOARD_ENTRY_CLASS = KEYBOARD_ENTRY + ".W_SettingsListEntry_KBMBinding_C"
PRESS_ANY_KEY_CLASS = "/Game/UI/Settings/Screens/W_PressAnyKey.W_PressAnyKey_C"
KEY_ALREADY_BOUND_CLASS = (
    "/Game/UI/Settings/Screens/W_KeyAlreadyBoundWarning.W_KeyAlreadyBoundWarning_C")


def log(message):
    unreal.log(f"[SessionUISettingsRepair] {message}")
    print(f"[SessionUISettingsRepair] {message}")


def compile_and_save(path):
    blueprint = unreal.load_asset(path)
    if blueprint is None:
        raise RuntimeError(f"Cannot load blueprint: {path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return blueprint.get_editor_property("status")


def repair_keyboard_editor_registration():
    visual_data = unreal.load_asset(SETTINGS_VISUAL_DATA)
    keyboard_setting_class = unreal.load_class(None, KEYBOARD_SETTING_CLASS)
    keyboard_entry_class = unreal.load_class(None, KEYBOARD_ENTRY_CLASS)
    if not all([visual_data, keyboard_setting_class, keyboard_entry_class]):
        raise RuntimeError("Cannot load keyboard setting visual registration assets")

    # 迁移脚本曾把键位编辑器写到 None 键，运行时因此只能生成 Missing entry 并显示 No Editor Found。
    # 必须按实际 ULyraSettingKeyboardInput 类型注册，不能依赖列表控件的默认 EntryWidgetClass。
    entry_map = visual_data.get_editor_property("entry_widget_for_class")
    if None in entry_map:
        del entry_map[None]
    entry_map[keyboard_setting_class] = keyboard_entry_class
    visual_data.set_editor_property("entry_widget_for_class", entry_map)
    visual_data.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(visual_data, only_if_is_dirty=False)

    # 这两个类来自 ULyraSettingsListEntrySetting_KeyboardInput 父类；未配置时点击键位不会弹出改键页面。
    if not unreal.BlueprintService.set_property(
            KEYBOARD_ENTRY, "PressAnyKeyPanelClass", PRESS_ANY_KEY_CLASS):
        raise RuntimeError("Failed to configure PressAnyKeyPanelClass")
    if not unreal.BlueprintService.set_property(
            KEYBOARD_ENTRY, "KeyAlreadyBoundWarningPanelClass", KEY_ALREADY_BOUND_CLASS):
        raise RuntimeError("Failed to configure KeyAlreadyBoundWarningPanelClass")

    log("Repaired keyboard setting editor registration and modal classes")


for asset_path, class_path in REPARENTS:
    blueprint = unreal.load_asset(asset_path)
    parent_class = unreal.load_class(None, class_path)
    if blueprint is None or parent_class is None:
        raise RuntimeError(f"Cannot reparent {asset_path} to {class_path}")
    unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    log(f"Reparented {asset_path} -> {class_path}; status={blueprint.get_editor_property('status')}")

repair_keyboard_editor_registration()

# UE 5.8 会在迁移后第一次 Widget 编译时清除旧 Designer 变量 GUID；第二轮必须仍为 UpToDate。
for pass_index in range(2):
    for path in COMPILE_ORDER:
        status = compile_and_save(path)
        log(f"Compile pass={pass_index + 1} path={path} status={status}")

if not unreal.BlueprintService.set_property(
        FRONT_END, "SettingsScreenClass", SETTINGS_SCREEN_CLASS):
    raise RuntimeError("Failed to configure W_FrontEnd.SettingsScreenClass")

event_id = unreal.BlueprintService.create_component_bound_event(
    FRONT_END, "EventGraph", "OptionsButton", "OnButtonBaseClicked", 850, 350)
if not event_id:
    raise RuntimeError("Failed to create OptionsButton component-bound event")

existing_calls = [
    node for node in unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph")
    if node.node_type.endswith("K2Node_CallFunction") and node.node_title == "Open Settings Screen"
]
if existing_calls:
    open_settings_id = existing_calls[0].node_id
else:
    build = unreal.BlueprintService.build_graph(
        FRONT_END,
        "EventGraph",
        [{"ref": "OpenSettings", "type": "function_call",
          "params": {"class": "ShootFrontEndScreen", "function": "OpenSettingsScreen"}}],
        [],
        [],
        True,
        False)
    if not build.success:
        raise RuntimeError(f"Failed to create OpenSettingsScreen node: {list(build.errors)}")
    open_settings_id = next(
        node.node_id for node in unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph")
        if node.node_type.endswith("K2Node_CallFunction") and node.node_title == "Open Settings Screen")

if not unreal.BlueprintService.connect_nodes(
        FRONT_END, "EventGraph", event_id, "then", open_settings_id, "execute"):
    raise RuntimeError("Failed to connect OptionsButton to OpenSettingsScreen")

unreal.BlueprintService.add_comment_node(
    FRONT_END,
    "EventGraph",
    "Options 使用 ShootFrontEndScreen.OpenSettingsScreen 推入当前 LocalPlayer 的 UI.Layer.Menu。"
    "SettingsScreenClass 在本蓝图 CDO 指向迁移后的 W_LyraSettingScreen。",
    760,
    250,
    720,
    360)

front_status = compile_and_save(FRONT_END)
log(f"FrontEnd status={front_status} settings={unreal.BlueprintService.get_property(FRONT_END, 'SettingsScreenClass')}")

for path, expected_parent in REPARENTS:
    actual_parent = unreal.BlueprintService.get_parent_class(path)
    log(f"Verify parent {path}={actual_parent}; expected={expected_parent.rsplit('.', 1)[-1]}")
    if actual_parent != expected_parent.rsplit(".", 1)[-1]:
        raise RuntimeError(f"Unexpected parent for {path}: {actual_parent}")

front_nodes = unreal.BlueprintService.get_nodes_in_graph(FRONT_END, "EventGraph")
bound_event = next(node for node in front_nodes if node.node_id == event_id)
log(f"Verify Options event={bound_event.node_type}|{bound_event.node_title}|{list(bound_event.pin_names)}")
