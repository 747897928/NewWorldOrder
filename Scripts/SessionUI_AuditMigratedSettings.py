import unreal


ASSETS = [
    "/Game/UI/Settings/W_LyraSettingScreen",
    "/Game/UI/Settings/W_SettingsPanel",
    "/Game/UI/Settings/W_GameSettingsDetailView",
    "/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding",
    "/Game/UI/FrontEnd/W_FrontEnd",
]

SETTINGS_VISUAL_DATA = "/Game/UI/Settings/GameSettingRegistryVisuals"
KEYBOARD_SETTING_CLASS = "/Script/NewWorldOrder.LyraSettingKeyboardInput"
KEYBOARD_ENTRY_CLASS = (
    "/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding."
    "W_SettingsListEntry_KBMBinding_C")
PRESS_ANY_KEY_CLASS = "/Game/UI/Settings/Screens/W_PressAnyKey.W_PressAnyKey_C"
KEY_ALREADY_BOUND_CLASS = (
    "/Game/UI/Settings/Screens/W_KeyAlreadyBoundWarning.W_KeyAlreadyBoundWarning_C")
ACTION_TABLE_PATH = "/Game/UI/DT_UniversalActions"


def log(message):
    unreal.log(f"[SessionUISettingsAudit] {message}")
    print(f"[SessionUISettingsAudit] {message}")


def audit_keyboard_editor_registration():
    visual_data = unreal.load_asset(SETTINGS_VISUAL_DATA)
    setting_class = unreal.load_class(None, KEYBOARD_SETTING_CLASS)
    entry_class = unreal.load_class(None, KEYBOARD_ENTRY_CLASS)
    if not all([visual_data, setting_class, entry_class]):
        raise RuntimeError("Keyboard editor registration assets are missing")

    entry_map = visual_data.get_editor_property("entry_widget_for_class")
    if None in entry_map:
        raise RuntimeError("Keyboard entry is still registered under a None class key")
    if entry_map.get(setting_class) != entry_class:
        raise RuntimeError("LyraSettingKeyboardInput is not mapped to the KBM entry widget")

    keyboard_blueprint = unreal.load_asset(
        "/Game/UI/Settings/Editors/W_SettingsListEntry_KBMBinding")
    keyboard_cdo = unreal.get_default_object(keyboard_blueprint.generated_class())
    press_class = keyboard_cdo.get_editor_property("press_any_key_panel_class")
    warning_class = keyboard_cdo.get_editor_property("key_already_bound_warning_panel_class")
    if not press_class or press_class.get_path_name() != PRESS_ANY_KEY_CLASS:
        raise RuntimeError(f"Unexpected PressAnyKeyPanelClass: {press_class}")
    if not warning_class or warning_class.get_path_name() != KEY_ALREADY_BOUND_CLASS:
        raise RuntimeError(f"Unexpected KeyAlreadyBoundWarningPanelClass: {warning_class}")
    log("keyboard_editor_registration=OK")


def audit_settings_actions():
    screen_blueprint = unreal.load_asset("/Game/UI/Settings/W_LyraSettingScreen")
    action_table = unreal.load_asset(ACTION_TABLE_PATH)
    if not screen_blueprint or not action_table:
        raise RuntimeError("Settings screen or CommonUI action table is missing")

    available_rows = {
        str(name) for name in unreal.DataTableFunctionLibrary.get_data_table_row_names(action_table)
    }
    if "Input_ResetDefaults" not in available_rows:
        raise RuntimeError("Input_ResetDefaults is missing from DT_UniversalActions")

    screen_cdo = unreal.get_default_object(screen_blueprint.generated_class())
    reset_handle = screen_cdo.get_editor_property("reset_to_defaults_input_action_data")
    if reset_handle.data_table != action_table or str(reset_handle.row_name) != "Input_ResetDefaults":
        raise RuntimeError(f"Unexpected Reset Defaults action handle: {reset_handle}")
    log("settings_reset_defaults_action=OK")


for path in ASSETS:
    asset = unreal.load_asset(path)
    log(f"asset={path} loaded={asset is not None}")
    if asset is None:
        continue
    try:
        log(f"  parent={unreal.BlueprintService.get_parent_class(path)}")
        log(f"  status={asset.get_editor_property('status')}")
    except Exception as exc:
        log(f"  blueprint_info_error={exc}")
    try:
        widgets = unreal.WidgetService.list_components(path)
        log("  widgets=" + ",".join(
            f"{widget.widget_name}:{widget.widget_class}" for widget in widgets))
    except Exception as exc:
        log(f"  widget_info_error={exc}")
    try:
        for graph in unreal.BlueprintService.list_graphs(path):
            log(f"  graph={graph.graph_name}:{graph.graph_kind}:{graph.node_count}")
            for node in unreal.BlueprintService.get_nodes_in_graph(path, graph.graph_name):
                log(f"    node={node.node_id}|{node.node_type}|{node.node_title}|{list(node.pin_names)}")
    except Exception as exc:
        log(f"  graph_info_error={exc}")

audit_keyboard_editor_registration()
audit_settings_actions()
