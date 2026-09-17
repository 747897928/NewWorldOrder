import unreal


BLUEPRINT_PATH = "/Game/Assets/Furniture/Dressing_Table_Set/Blueprint/BP_Dressing_Table_Set"
COMPONENT_NAME = "WardrobeInteraction"
COMPONENT_TYPE = "ShootWardrobeInteractionComponent"
WARDROBE_SCREEN_CLASS = "/Game/UI/Mutable/W_Cloth.W_Cloth_C"


def log(message):
    unreal.log(f"[WardrobeDressingTable] {message}")
    print(f"[WardrobeDressingTable] {message}")


blueprint = unreal.load_asset(BLUEPRINT_PATH)
component_class = unreal.load_class(
    None, "/Script/NewWorldOrder.ShootWardrobeInteractionComponent")
wardrobe_class = unreal.load_class(None, WARDROBE_SCREEN_CLASS)
if not all([blueprint, component_class, wardrobe_class]):
    raise RuntimeError("Dressing table, wardrobe component, or W_Cloth class is missing")

if not unreal.BlueprintService.component_exists(BLUEPRINT_PATH, COMPONENT_NAME):
    if not unreal.BlueprintService.add_component(
            BLUEPRINT_PATH, COMPONENT_TYPE, COMPONENT_NAME):
        raise RuntimeError("Cannot add WardrobeInteraction component")

# W_Cloth 是与 M 菜单共用的页面。资产引用放在家具蓝图组件模板，不在 C++ 写 /Game 路径。
if not unreal.BlueprintService.set_component_property(
        BLUEPRINT_PATH, COMPONENT_NAME, "WardrobeScreenClass", WARDROBE_SCREEN_CLASS):
    raise RuntimeError("Cannot configure WardrobeScreenClass")

# 现有 Sphere 继续负责提示显隐；现有 Box 已配置为 Interactable_OverlapDynamic。
# 业务接口在独立 ActorComponent 上，因此 InteractionStatics 会从 Box 的命中 Actor 收集全部接口组件。

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError(f"Dressing table compile failed: {blueprint.get_editor_property('status')}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("Cannot save BP_Dressing_Table_Set")

components = unreal.BlueprintService.list_components(BLUEPRINT_PATH)
configured = next(
    (component for component in components if component.component_name == COMPONENT_NAME), None)
if configured is None or configured.component_class != COMPONENT_TYPE:
    raise RuntimeError(f"Unexpected component after save: {configured}")

screen_value = unreal.BlueprintService.get_component_property(
    BLUEPRINT_PATH, COMPONENT_NAME, "WardrobeScreenClass")
if "W_Cloth" not in str(screen_value):
    raise RuntimeError(f"WardrobeScreenClass did not persist: {screen_value}")

log(f"Configured {configured.component_name}:{configured.component_class}")
log(f"WardrobeScreenClass={screen_value}")
