import unreal


SCREEN_PATH = "/Game/UI/Settings/W_LyraSettingScreen"
ACTION_TABLE_PATH = "/Game/UI/DT_UniversalActions"
TAB_BUTTON_CLASS_PATH = "/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/Foundation/Buttons/W_LyraButtonTab.W_LyraButtonTab_C'"
BOUND_ACTION_BUTTON_PATH = "/Game/UI/Foundation/Widgets/BottomBar/W_BoundActionButton"
CLEAR_BUTTON_STYLE_CLASS_PATH = "/Game/UI/Foundation/Buttons/ButtonStyle-Clear.ButtonStyle-Clear_C"
TAB_DESCRIPTOR_NODE_ID = "5114B96C4D4A611DF795868EE8C904AD"
REGISTER_TAB_NODE_ID = "78AB88664F5EEACD012A4CB1C1B9E61D"


def log(message):
    unreal.log(f"[SessionUISettingsNavigation] {message}")
    print(f"[SessionUISettingsNavigation] {message}")


screen = unreal.EditorAssetLibrary.load_asset(SCREEN_PATH)
action_table = unreal.EditorAssetLibrary.load_asset(ACTION_TABLE_PATH)
bound_action_button = unreal.EditorAssetLibrary.load_asset(BOUND_ACTION_BUTTON_PATH)
clear_button_style_class = unreal.load_class(None, CLEAR_BUTTON_STYLE_CLASS_PATH)
if not screen or not action_table or not bound_action_button or not clear_button_style_class:
    raise RuntimeError("Settings screen, action table, bound action button, or clear button style is missing")

# CommonBoundActionBar 会动态生成 W_BoundActionButton；样式必须配置在该按钮类默认值上，
# 不能只修改设置页中的某个实例，否则 Back / Apply / Cancel 会继续使用 CommonButton 默认白色边框。
bound_action_button_class = unreal.load_class(None, f"{BOUND_ACTION_BUTTON_PATH}.W_BoundActionButton_C")
bound_action_button_cdo = unreal.get_default_object(bound_action_button_class)
bound_action_button_cdo.modify()
bound_action_button_cdo.set_editor_property("style", clear_button_style_class)
unreal.BlueprintEditorLibrary.compile_blueprint(bound_action_button)
if not unreal.EditorAssetLibrary.save_asset(BOUND_ACTION_BUTTON_PATH, only_if_is_dirty=False):
    raise RuntimeError("Failed to save W_BoundActionButton")
if bound_action_button_cdo.get_editor_property("style") != clear_button_style_class:
    raise RuntimeError("W_BoundActionButton did not retain ButtonStyle-Clear")
log("Configured W_BoundActionButton -> ButtonStyle-Clear")

available_rows = {
    str(name) for name in unreal.DataTableFunctionLibrary.get_data_table_row_names(action_table)
}
required_rows = {"Input_Back", "Input_ApplyChanges", "Input_Cancel", "Input_ResetDefaults"}
if not required_rows.issubset(available_rows):
    raise RuntimeError(f"Project CommonUI action rows are missing: {required_rows - available_rows}")

# TabDescriptor 的 ButtonType 是运行时创建五个分类按钮的必要配置；迁移后该 Pin 丢失会只显示根设置列表。
if not unreal.BlueprintService.set_node_pin_value(
    SCREEN_PATH,
    "RegisterTopLevelTab",
    TAB_DESCRIPTOR_NODE_ID,
    "TabButtonType",
    TAB_BUTTON_CLASS_PATH,
):
    raise RuntimeError("Failed to assign the project's W_LyraButtonTab to the tab descriptor")

generated_class = unreal.load_object(None, f"{SCREEN_PATH}.W_LyraSettingScreen_C")
screen_cdo = unreal.get_default_object(generated_class)
screen_cdo.modify()

for property_name, row_name in (
    ("back_input_action_data", "Input_Back"),
    ("apply_input_action_data", "Input_ApplyChanges"),
    ("cancel_changes_input_action_data", "Input_Cancel"),
    ("reset_to_defaults_input_action_data", "Input_ResetDefaults"),
):
    handle = unreal.DataTableRowHandle()
    handle.data_table = action_table
    handle.row_name = row_name
    screen_cdo.set_editor_property(property_name, handle)
    log(f"Configured {property_name} -> {ACTION_TABLE_PATH}:{row_name}")

comment_text = "TabDescriptor 必须使用项目 W_LyraButtonTab；为空时 Registry 已加载但运行时不会生成五个可见分类 Tab。"
existing_comments = [
    node
    for node in unreal.BlueprintService.get_nodes_in_graph(SCREEN_PATH, "RegisterTopLevelTab")
    if str(node.node_type) == "EdGraphNode_Comment" and str(node.node_title) == comment_text
]
if not existing_comments:
    unreal.BlueprintService.add_comment_around_nodes(
        SCREEN_PATH,
        "RegisterTopLevelTab",
        comment_text,
        [TAB_DESCRIPTOR_NODE_ID, REGISTER_TAB_NODE_ID],
        36.0,
    )

unreal.BlueprintEditorLibrary.compile_blueprint(screen)
unreal.BlueprintEditorLibrary.compile_blueprint(screen)
if not unreal.EditorAssetLibrary.save_asset(SCREEN_PATH, only_if_is_dirty=False):
    raise RuntimeError("Failed to save W_LyraSettingScreen")

for property_name, row_name in (
    ("back_input_action_data", "Input_Back"),
    ("apply_input_action_data", "Input_ApplyChanges"),
    ("cancel_changes_input_action_data", "Input_Cancel"),
    ("reset_to_defaults_input_action_data", "Input_ResetDefaults"),
):
    persisted = screen_cdo.get_editor_property(property_name)
    if str(persisted.row_name) != row_name or persisted.data_table != action_table:
        raise RuntimeError(f"Action handle did not persist: {property_name}={persisted}")

log(f"Compiled status={screen.status}; five-tab button class and action handles are configured")
