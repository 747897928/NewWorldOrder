# UIExtension HUD 插槽学习笔记

note_id: ui-003-uiextension-hud-slots
category: UI
status: Active
last_updated: 2026-07-24

# 来源

- Lyra MCP 端口：11000
- Lyra Widget 资产：`/ShooterCore/UserInterface/W_ShooterHUDLayout`
- Lyra ActionSet 资产：`/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD`
- Lyra C++：
  - `G:/Documents/Unreal Projects/LyraStarterGame/Source/LyraGame/GameFeatures/GameFeatureAction_AddWidget.h`
  - `G:/Documents/Unreal Projects/LyraStarterGame/Source/LyraGame/GameFeatures/GameFeatureAction_AddWidget.cpp`
  - `G:/Documents/Unreal Projects/LyraStarterGame/Source/LyraGame/UI/LyraHUDLayout.cpp`
- 本项目插件源码：
  - `Plugins/UIExtension/Source/Public/Widgets/UIExtensionPointWidget.h`
  - `Plugins/UIExtension/Source/Private/Widgets/UIExtensionPointWidget.cpp`
  - `Plugins/UIExtension/Source/Public/UIExtensionSystem.h`
  - `Plugins/UIExtension/Source/Private/UIExtensionSystem.cpp`
  - `Plugins/CommonGame/Source/Public/CommonUIExtensions.h`
  - `Plugins/CommonGame/Source/Private/CommonUIExtensions.cpp`

# 结论

Lyra 的 HUD 不是把所有 HUD 子控件直接写死在一个 Widget 蓝图里，也不是到处 `AddToViewport`。它分两层：

1. Layout 层
- `LAS_ShooterGame_StandardHUD` 的 `layout` 数组配置一条：
  - `layoutClass = /ShooterCore/UserInterface/W_ShooterHUDLayout.W_ShooterHUDLayout_C`
  - `layerId = UI.Layer.Game`
- `UGameFeatureAction_AddWidgets::AddWidgets` 里调用：
  - `UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, Entry.LayerID, ConcreteWidgetClass)`
- 这一层负责把 HUD layout 推到目标本地玩家的 `PrimaryGameLayout` 层级栈。

2. HUD 片段层
- `W_ShooterHUDLayout` 的 Widget 树里放多个 `UUIExtensionPointWidget`。
- `LAS_ShooterGame_StandardHUD` 的 `widgets` 数组配置 `WidgetClass + SlotID`。
- `UGameFeatureAction_AddWidgets::AddWidgets` 里调用：
  - `UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext(Entry.SlotID, LocalPlayer, Entry.WidgetClass.Get(), -1)`
- 这一层负责把普通 HUD 片段注册到对应 GameplayTag 插槽。

# Lyra 资产事实

`W_ShooterHUDLayout`：

- 父类：`ULyraHUDLayout`
- 根结构：`SafeZone_0 -> Overlay_0 -> CanvasPanel_0`
- 资产中有大量 `UUIExtensionPointWidget`，例如：
  - `ExtensionPoint_Reticle`
  - `ExtensionPoint_Equipment`
  - `ExtensionPoint_ModeStatus`
  - `ExtensionPoint_LeftSideTouchInputs`
  - `ExtensionPoint_RightSideTouchInputs`
  - `ExtensionPoint_LeftSide_TouchRegion`
  - `ExtensionPoint_RightSide_TouchRegion`
  - `ExtensionPoint_GraphStats`
  - `ExtensionPoint_TextStats`
- 2026-07-08 通过 Lyra MCP 端口 11000 的 `UMGToolSet.UMGToolSet.GetWidgets` 复核：
  - `rootWidgetClass = /Script/UMG.SafeZone`
  - `widgetCount = 30`
  - 多数 HUD 插槽直接挂在 `CanvasPanel_0` 下。
  - 触摸区等需要局部布局的插槽会先挂 `VerticalBox` / `HorizontalBox`，再把 `UUIExtensionPointWidget` 放进这些局部容器。
  - 结论：屏幕锚点和预留区域属于 HUD layout，不属于被注册进去的内容 Widget。

已通过 Lyra MCP 读取到的具体扩展点示例：

- `ExtensionPoint_Reticle`
  - `ExtensionPointTag = HUD.Slot.Reticle`
  - `ExtensionPointTagMatch = ExactMatch`
  - `DataClasses = []`
- `ExtensionPoint_Equipment`
  - `ExtensionPointTag = HUD.Slot.Equipment`
  - `ExtensionPointTagMatch = ExactMatch`
  - `DataClasses = []`
- `ExtensionPoint_Reticle` 的 CanvasPanelSlot 示例：
  - `layoutData.offsets = left 0, top 0, right 100, bottom 30`
  - `anchors.minimum = 0.5,0.5`
  - `anchors.maximum = 0.5,0.5`
  - `alignment = 0.5,0.5`
  - `bAutoSize = true`
  - `zOrder = 0`
  - 说明：Lyra 可以让插槽自动按内容尺寸布局，也可以通过 CanvasPanelSlot 固定锚点；关键是插槽放在 HUD layout 的 CanvasPanel 上。

`LAS_ShooterGame_StandardHUD`：

- 类型：`ULyraExperienceActionSet`
- `actions` 中只有一个 `GameFeatureAction_AddWidgets_0`
- 2026-07-08 通过 ObjectTools 复核：
  - ActionSet 对外属性名为 `actions`。
  - `actions[0] = /ShooterCore/Experiences/LAS_ShooterGame_StandardHUD.LAS_ShooterGame_StandardHUD:GameFeatureAction_AddWidgets_0`
  - `GameFeatureAction_AddWidgets_0` 的属性是 `layout` 和 `widgets`。
  - `layout` 条目结构：`layoutClass`、`layerId`。
  - `widgets` 条目结构：`widgetClass`、`slotId`。
- 该 Action 的 `layout`：
  - `W_ShooterHUDLayout_C -> UI.Layer.Game`
- 该 Action 的 `widgets` 包含：
  - `W_EliminationFeed_C -> HUD.Slot.EliminationFeed`
  - `W_QuickBar_C -> HUD.Slot.Equipment`
  - `W_AccoladeHostWidget_C -> HUD.Slot.TopAccolades`
  - `W_WeaponReticleHost_C -> HUD.Slot.Reticle`
  - `W_PerfStatContainer_GraphOnly_C -> HUD.Slot.PerfStats.Graph`
  - `W_PerfStatContainer_TextOnly_C -> HUD.Slot.PerfStats.Text`
  - `W_OnScreenJoystick_Left_C -> HUD.Slot.LeftSideTouchInputs`
  - `W_OnScreenJoystick_Right_C -> HUD.Slot.RightSideTouchInputs`
  - `W_FireButton_C -> HUD.Slot.RightSideTouchInputs`
  - `W_TouchRegion_Right_C -> HUD.Slot.RightSideTouchRegion`
  - `W_TouchRegion_Left_C -> HUD.Slot.LeftSideTouchRegion`

# C++ 调用链

`UGameFeatureAction_AddWidgets::AddToWorld`

- 只在 GameWorld 中运行。
- 通过 `UGameFrameworkComponentManager` 给 `ALyraHUD` 注册扩展处理器。
- HUD Actor ready 后进入 `HandleActorExtension`。

`UGameFeatureAction_AddWidgets::AddWidgets`

- 从 HUD 拿 OwningPlayerController。
- 从 PlayerController 拿 `ULocalPlayer`。
- 遍历 `layout`：
  - 加载 `LayoutClass`
  - 调用 `UCommonUIExtensions::PushContentToLayer_ForPlayer`
  - 返回的 layout widget 存入 `LayoutsAdded`
- 遍历 `widgets`：
  - 从 World 拿 `UUIExtensionSubsystem`
  - 调用 `RegisterExtensionAsWidgetForContext(SlotID, LocalPlayer, WidgetClass, -1)`
  - 返回的 `FUIExtensionHandle` 存入 `ExtensionHandles`

`UGameFeatureAction_AddWidgets::RemoveWidgets`

- 对 `LayoutsAdded` 调用 `DeactivateWidget()`。
- 对 `ExtensionHandles` 调用 `Unregister()`。
- 因此 GameFeature 停用、HUD 移除、本地玩家切换时，layout 和 HUD 片段都能清理。

`UCommonUIExtensions::PushContentToLayer_ForPlayer`

- 输入是 `ULocalPlayer`，不是无 Owner 的全局 Widget。
- 通过 `UGameUIManagerSubsystem -> UGameUIPolicy -> UPrimaryGameLayout` 找到该本地玩家自己的根布局。
- 调用 `UPrimaryGameLayout::PushWidgetToLayerStack(LayerName, WidgetClass)`。

`UUIExtensionPointWidget::RebuildWidget`

- 非设计时且 `ExtensionPointTag` 有效时注册扩展点。
- 同时注册：
  - 全局扩展点：`RegisterExtensionPoint(ExtensionPointTag, ...)`
  - 本地玩家上下文扩展点：`RegisterExtensionPointForContext(ExtensionPointTag, GetOwningLocalPlayer(), ...)`
- LocalPlayer 的 PlayerState 可用后，再注册：
  - PlayerState 上下文扩展点：`RegisterExtensionPointForContext(ExtensionPointTag, PlayerState, ...)`
- 这让 HUD 插槽天然支持本地多人和玩家私有 HUD。

`UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext`

- 实际转到 `RegisterExtensionAsData(ExtensionPointTag, ContextObject, WidgetClass, Priority)`。
- `ContextObject` 必须与扩展点上下文匹配。
- `Data` 可以是 WidgetClass，也可以是自定义 DataObject。

`UUIExtensionPointWidget::OnAddOrRemoveExtension`

- 如果 `Request.Data` 是 `UClass`，就把它当 `UUserWidget` 类创建 entry。
- 如果配置了 `DataClasses`，可以让扩展点接收数据对象，再通过 `GetWidgetClassForData` 和 `ConfigureWidgetForData` 决定展示 Widget。

# 插件加载要求

本项目的 `UIExtension` 是项目插件，不是引擎内置插件。要在 UMG Palette、MCP、蓝图编译和运行时看到 `/Script/UIExtension.UIExtensionPointWidget`，必须同时满足：

1. `NewWorldOrder.uproject` 的 `Plugins` 数组启用：
   - `UIExtension`
   - `CommonUI`
   - `CommonGame`
2. `Plugins/UIExtension/UIExtension.uplugin` 只能有一个 `Plugins` 数组。
   - 2026-07-08 发现该文件曾有两个同名 `Plugins` 字段，后一个 `CommonGame` 覆盖了前一个 `CommonUI`。
   - 修复为同一个数组里同时列出 `CommonUI` 和 `CommonGame`。
3. 修改 `.uplugin` 或 `.uproject` 后必须重新构建并重启编辑器。
   - 仅 C++ 编译成功不代表当前已打开的 UE 进程会重新挂载插件。
   - 当前进程如果执行 `unreal.load_module('UIExtension')` 返回 `isn't a known module name`，说明插件还没有进入模块表，需要重启 NewWorldOrder 编辑器。
4. 验证命令：

```python
import unreal
print(unreal.load_object(None, '/Script/UIExtension.UIExtensionPointWidget'))
print(unreal.load_object(None, '/Script/UIExtension.UIExtensionSubsystem'))
```

期望结果不是 `None`。

# UIExtension Point Tag 使用说明

`UIExtension Point Tag` 是把 HUD layout 中的一个位置命名为 GameplayTag，并允许其它系统在运行时向该位置登记内容的契约。它解决的是稳定 HUD 片段的装配问题，不是通用的菜单跳转机制。

## 先做产品与交互判断

适合使用插槽的内容：

- 准星、快捷栏、资源提示、模式状态、击杀播报、触摸按键等 HUD 片段。
- 内容与角色操作并存，不要求菜单焦点或返回栈。
- 位置属于 HUD 的固定区域，替换内容时不应改 HUD layout 的控件树。

不适合使用插槽的内容：

- 主菜单、暂停菜单、衣柜、角色切换、姿势选择等需要焦点、滚动、确认或返回的页面。
- 需要暂时接管输入、恢复前一页面或由 CommonUI Back Action 关闭的页面。
- 这类页面应由 `UPrimaryGameLayout` 推入目标 LocalPlayer 的 `UI.Layer.Menu`、`UI.Layer.GameMenu` 或 `UI.Layer.Modal`，通过 `DeactivateWidget()` 退栈。

2026-07-24 的姿势库结论：姿势选择含可选择卡片与可滚动列表，属于 `WBP_GameMenu` 的内容区。旧的 `W_PoseLibrary` 已删除；`WBP_GameMenu` 直接在 `PoseLibraryUniformGrid` 生成三列 `W_PoseLibraryEntry`。它不再是 HUD extension，不再使用 `HUD.Slot.PoseLibrary`、`FUIExtensionHandle` 或独立关闭按钮；玩家从 GameMenu 的返回动作整体关闭菜单。

## 资产和代码各自负责什么

- HUD layout，例如 `/Game/UI/Hud/W_DefaultHUD`，负责固定根结构、屏幕锚点、安全区和视觉层级。
- `UUIExtensionPointWidget` 是 HUD layout 里的占位控件。它的 `ExtensionPointTag` 是插槽名，`ExtensionPointTagMatch` 决定 Exact 或 IncludeChildren 匹配，`EntryBoxType` 决定多条内容如何排列。
- 注册者负责在正确的生命周期，以正确的 ContextObject 向同名 Tag 登记 `WidgetClass` 或数据对象。
- `UUIExtensionSubsystem` 保存注册关系，返回 `FUIExtensionHandle`；注册者拥有 handle，也拥有注销责任。
- 被创建的 HUD 内容 Widget 只负责自身表现与数据，不负责猜测屏幕位置、创建全局 Viewport 或替别的 LocalPlayer 管理输入。

## 配置步骤

1. 定义稳定的 GameplayTag。

- 在 `Config/DefaultGameplayTags.ini` 添加 `HUD.Slot.<Feature>`。
- 名称表达 HUD 位置职责，不表达某个临时实现。例如 `HUD.Slot.Reticle` 合理，`HUD.Slot.TempWidget` 不合理。
- 只有确认至少一个 HUD extension 会使用该 Tag 时才保留；迁移或删功能时同步删 Tag、插槽和注册代码。

2. 在 HUD layout 放置扩展点。

- 在 `/Game/UI/Hud/W_DefaultHUD` 或未来的 HUD layout 内添加 `UUIExtensionPointWidget`。
- 把它放到 `CanvasPanel` 或局部 `HorizontalBox`、`VerticalBox` 内；父容器负责布局，内容 Widget 不负责绝对坐标。
- 配置 `ExtensionPointTag`、`ExtensionPointTagMatch` 和 `EntryBoxType`。
- 多内容共存时选择能表达产品语义的 EntryBox；独占区域通常使用 Overlay，并由注册优先级决定顺序。

3. 让 HUD layout 进入玩家自己的 CommonUI 层级。

- HUD 入口通过 `UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, UI.Layer.Game, LayoutClass)` 或等价项目入口推入。
- 不使用无 Owner 的 `CreateWidget + AddToViewport`，否则本地分屏会把 UI 显示或输入交给错误玩家。

4. 在拥有正确上下文的系统登记内容。

```cpp
UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
FUIExtensionHandle Handle = ExtensionSubsystem->RegisterExtensionAsWidgetForContext(
    SlotTag,
    GetLocalPlayer(),
    WidgetClass,
    Priority);
```

- HUD 玩家私有内容通常使用目标 `ULocalPlayer` 作为 ContextObject。
- 如果扩展点按 PlayerState 或其它对象建立上下文，注册也必须使用同一对象；上下文不匹配时不能假定会出现在目标插槽。
- 不要在 C++ 中保存裸 Widget 指针后直接 `RemoveFromParent()`；通过 handle 注销才会同步清理 subsystem 的登记状态。

## 完整运行生命周期

1. 模块和插件加载。

- `NewWorldOrder.uproject` 启用 `UIExtension`、`CommonUI`、`CommonGame`。
- `Plugins/UIExtension/UIExtension.uplugin` 在同一个 `Plugins` 数组声明依赖。
- 改动插件声明后重新构建并重启编辑器；仅热编译不保证模块进入当前进程。

2. Layout 创建。

- HUD 通过 CommonUI 推入目标 LocalPlayer 的 `UI.Layer.Game`。
- `UUIExtensionPointWidget::RebuildWidget` 在非设计时读取有效 Tag，向 `UUIExtensionSubsystem` 注册全局与 LocalPlayer 上下文扩展点。
- LocalPlayer 的 PlayerState 可用后，它还能为 PlayerState 上下文建立扩展点，支持玩家私有 HUD 和分屏。

3. 内容登记和实例化。

- 注册者调用 `RegisterExtensionAsWidgetForContext`，实际把 `WidgetClass` 作为 data 登记到 Tag 与 ContextObject。
- Subsystem 找到匹配扩展点后调用 `UUIExtensionPointWidget::OnAddOrRemoveExtension`。
- 当 data 是 `UClass` 时，扩展点创建对应 `UUserWidget` 实例并加入自己的 EntryBox；有 `DataClasses` 时则由扩展点将数据对象映射、配置为 Widget。

4. 正常运行。

- HUD layout 继续归 CommonUI 层级管理，extension 内容随 HUD 存在。
- 内容自己的数据刷新应走 MVVM 或 GameplayMessage，不要为刷新一个 HUD 数值反复注销、重新注册 Widget。
- 注册者如果需要临时隐藏内容，可以注销旧 handle 或使用已有状态驱动可见性；不要让多个未追踪 handle 叠加。

5. 清理。

- 功能停用、HUD 移除、拥有者 EndPlay、LocalPlayer 切换或替换内容前，调用 `Handle.Unregister()`。
- `UGameFeatureAction_AddWidgets::RemoveWidgets` 的标准顺序是：对 layout 调用 `DeactivateWidget()`，对每个 extension handle 调用 `Unregister()`。
- 注销会让扩展点移除对应 entry；不需要、也不允许调用 entry 的 `RemoveFromParent()` 代替注销。
- handle 注销后重置为默认值，避免下一次清理重复操作或误判为仍在显示。

## 分屏、输入和优先级

- 每个本地玩家都有自己的 `UPrimaryGameLayout`、LocalPlayer 上下文扩展点和内容实例。注册时不能使用 `GetFirstPlayerController`、`GetPlayerController(0)` 或没有 OwningPlayer 的 `CreateWidget`。
- UIExtension 只解决内容装配，不自动把一个内容变成菜单。需要独占输入的页面仍由 `ULyraActivatableWidget` 和 CommonUI Layer 管理输入映射、焦点与 Back Action。
- `Priority` 只用于同一插槽多条 extension 的顺序，不应被当作页面导航、互斥显示或输入优先级机制。
- 输入动作保持 `UInputAction + UInputMappingContext` 配置，不在 C++ 写键盘或手柄分支。Menu Widget 激活时使用其继承自 `UCommonActivatableWidget` 的 `InputMapping` 与 `InputMappingPriority` 管理映射。

## 验收清单

- 插件模块可加载：`/Script/UIExtension.UIExtensionPointWidget` 和 `/Script/UIExtension.UIExtensionSubsystem` 均不是 `None`。
- HUD layout 已推入正确 LocalPlayer 的 `UI.Layer.Game`。
- 目标扩展点的 Tag、匹配规则、EntryBox 和布局位置正确。
- 注册使用目标 LocalPlayer 或与扩展点一致的 ContextObject。
- 注册后出现一次内容实例；注销后实例移除，反复注册/注销不累计残留。
- 本地双人 PIE 中，两名玩家只看见、操作自己的内容。
- CommonUI 返回和输入映射在开启/关闭其他菜单后恢复正常。

## 当前项目状态

- `/Game/UI/Hud/W_DefaultHUD` 继续作为 HUD layout，但不再包含 `ExtensionPoint_PoseLibrary`。
- `HUD.Slot.PoseLibrary` 已随姿势库迁移删除；其他 `HUD.Slot.*` 可以按本说明独立增加。
- `/Game/UI/Menu/WBP_GameMenu` 在 `PoseLibraryLyraScrollBox -> PoseLibraryUniformGrid` 直接生成姿势条目；旧 `/Game/UI/Pose/W_PoseLibrary` 不再存在。
- 姿势数据仍来自 `/Game/Blueprints/Animations/DA_PoseLibrary` 的 `PoseEntries`；新增条目配置展示名、图标、性别、动画、Slot 和播放参数，不需要在 C++ 数组中硬编码。

## 常见错误

- 错误：将菜单、衣柜或姿势选择器注册到 HUD Slot。
- 正确：使用 CommonUI Layer 或已有菜单内容容器，并让 Back Action 退栈。

- 错误：`CreateWidget + AddToViewport` 创建玩家私有 HUD 片段。
- 正确：通过目标 LocalPlayer 的 layout 与 `RegisterExtensionAsWidgetForContext`。

- 错误：只保存 Widget 指针，关闭时 `RemoveFromParent()`。
- 正确：保存并注销 `FUIExtensionHandle`。

- 错误：删除功能后只删蓝图控件，留下 Tag、注册字段、handle 或模块依赖。
- 正确：按“Tag、扩展点、注册代码、handle、资产、文档、测试”完整清理。
