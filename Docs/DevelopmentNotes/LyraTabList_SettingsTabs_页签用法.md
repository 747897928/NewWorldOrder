# LyraTabList SettingsTabs 页签用法

日期：2026-06-26
状态：[可用] [11000 MCP 已复核]

## 来源

- Lyra MCP 11000：
  - `/Game/UI/Settings/W_LyraSettingScreen.W_LyraSettingScreen`
  - `/Game/UI/Settings/W_SettingsPanel.W_SettingsPanel`
  - `/Game/UI/Settings/W_GameSettingsDetailView.W_GameSettingsDetailView`
- 本轮 MCP 请求与结果：
  - `Saved/CodexMCP/2026-06-26-wardrobe/lyra_11000_getwidgets_W_LyraSettingScreen.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/lyra_11000_getwidgets_W_SettingsPanel.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/lyra_11000_getwidgets_W_GameSettingsDetailView.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/lyra_11000_read_graph_dsl_W_LyraSettingScreen_RegisterTopLevelTab.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/lyra_11000_read_graph_dsl_W_LyraSettingScreen_EventGraph.response.json`
- Lyra 本地源码：
  - `G:/Documents/Unreal Projects/LyraStarterGame/Source/LyraGame/UI/Common/LyraTabListWidgetBase.cpp`
  - `G:/Documents/Unreal Projects/LyraStarterGame/Plugins/GameSettings/Source/Private/Widgets/GameSettingScreen.cpp`
  - `G:/Documents/Unreal Projects/LyraStarterGame/Plugins/GameSettings/Source/Private/Widgets/GameSettingPanel.cpp`
- 本项目对应源码：
  - `Source/NewWorldOrder/Private/UI/Common/LyraTabListWidgetBase.cpp`
  - `Source/NewWorldOrder/Private/UI/Wardrobe/ShootWardrobeWidgetBase.cpp`

## 结论

- `TopSettingsTabs` 应该继续作为衣柜一级分类入口，注册 `服装 / 发型 / 容貌 / 体型 / 预设`。
- `WardrobePrimaryTabsHBox` 不需要保留。它和 `TopSettingsTabs` 职责重复，容易造成两套一级分类不同步。
- `WardrobeSubTabsHBox` 仍然需要，但它不是一级分类，也不是 `TopSettingsTabs` 的替代品。它是当前一级分类内容区里的二级筛选条，例如：
  - 服装：全部、上装、下装、连衣裙、鞋子、配饰
  - 发型：全部、整发、前发、后发、发色
  - 容貌：全部、眉毛、眼妆、瞳色、口红、腮红、面纹
  - 体型：身高、胸部、腰部、臀部、腿部、肌肉
- Lyra Settings 页签没有把设置列表作为每个一级页签的 `TabContentType`。它注册的是按钮页签，切换后由 `Settings_Panel` 根据 TabId 过滤内容。
- `BP_OnExecuteNamedAction` 不是页签切换链路。它只处理具体设置项上的命名动作，例如打开安全区、亮度、HDR 校准弹窗。

## Lyra Settings 的 Widget 树

`W_LyraSettingScreen`：

- 父类：`/Script/LyraGame.LyraSettingScreen`
- 根：`Overlay`
- 关键层级：
  - `HeaderBorder -> TabSZ -> TopSettingsTabs`
  - `Settings_Panel`
  - `W_BottomActionBar`

`W_SettingsPanel`：

- 父类：`/Script/GameSettings.GameSettingPanel`
- 根：`HorizontalBox`
- 关键层级：
  - `ListView_Settings`
  - `Details_Settings`

`W_GameSettingsDetailView`：

- 父类：`/Script/GameSettings.GameSettingDetailView`
- 只负责显示当前设置项详情，不参与顶层页签切换。

## Lyra 蓝图链路

`W_LyraSettingScreen.RegisterTopLevelTab(SettingDevName)` 做三件事：

1. 通过 `GetSettingCollection(SettingDevName)` 拿设置集合。
2. 用集合显示名构造 `FLyraTabDescriptor`。
3. 调用 `TopSettingsTabs.RegisterDynamicTab(Descriptor)`。

MCP 读到的图表链路要点：

- `Construct` 中依次调用 `RegisterTopLevelTab("GameplayCollection")` 等。
- `OnTabSelected(TopSettingsTabs)` 只调用 `NavigateToSetting(TabId)`。
- `OnExecuteNamedAction(Settings_Panel)` 用于具体设置项动作，不参与 TabList 切换。

## C++ 底层机制

`ULyraTabListWidgetBase::RegisterDynamicTab` 的关键点：

- 先把 `FLyraTabDescriptor` 放进 `PendingTabLabelInfoMap`，供按钮创建时设置文本与图标。
- 然后调用 `RegisterTab(TabId, TabButtonType, CreatedTabContentWidget)`。
- 如果 `TabContentType` 为空，`CreatedTabContentWidget` 也为空，但按钮仍然会注册。

`ULyraTabListWidgetBase::SetupTabs` 只在 `TabContentType` 不为空时创建内容 Widget。也就是说：

- `TabContentType` 是可选项。
- 没有 `TabContentType` 的 TabList 仍然可以作为纯按钮页签使用。
- 内容切换可以由 `OnTabSelected` 外部处理。

`UGameSettingScreen::NavigateToSetting` 的关键点：

- 根据 `TabId` 找到对应设置集合。
- 构造 `FGameSettingFilterState`。
- 调用 `Settings_Panel->SetFilterState(FilterState)`。

`UGameSettingPanel::SetFilterState` 的关键点：

- 保存过滤状态。
- 清理导航栈。
- 刷新列表。

所以 Lyra Settings 的真实模式是：

1. Top Tab 只负责选择一个分类 ID。
2. 内容面板常驻。
3. 选择变化后，把分类 ID 转成过滤状态，再刷新常驻内容面板。

## 对 W_Cloth 的落地建议

当前衣柜应按 Lyra Settings 的模式收敛：

1. `TopSettingsTabs` 注册一级分类，不要再保留 `WardrobePrimaryTabsHBox`。
2. 一级分类切换时调用 C++ 的 `HandleTopLevelTabSelected`，更新 `ActiveAppearanceSection`。
3. `ActiveAppearanceSection` 决定 `WardrobeSubTabsHBox` 里的二级筛选按钮集合。
4. 二级筛选变化时更新 `ActiveSubCategory`，刷新 `ClothUniformGrid`。
5. `WardrobeSubTabsHBox + ClothUniformGrid` 是当前常驻内容面板，不需要为了短期修复拆成每个一级分类一个 `TabContentType`。
6. 如果以后要做更完整的 MVVM 或复杂页面，可新增独立内容 Widget，例如 `W_WardrobeCategoryPanel`，再考虑 `TabContentType + LinkedSwitcher`。这属于后续重构，不是当前修 UI 的必要前置。

## 当前 W_Cloth 风险点

- 如果同时保留 `TopSettingsTabs` 和 `WardrobePrimaryTabsHBox`，用户会看到两套一级分类，状态也容易不同步。
- 如果把二级分类误塞成 `TopSettingsTabs` 的动态 Tab，会让“服装、发型、容貌、体型、预设”的一级信息丢失。
- 如果在没有 `LinkedSwitcher` 的情况下依赖 `TabContentType` 展示内容，TabList 可能能注册按钮，但内容不会自然出现在预期位置。
- 如果二级分类按钮仍由 C++ 创建原始按钮而不统一使用项目按钮样式，就会继续出现按钮无边框、未 Hover 看不清、摆放不稳定的问题。
- 动态注册页签时不要在 Widget 激活路径里调用 `RemoveAllTabs`。`ULyraTabListWidgetBase` 和 `UCommonTabListWidgetBase` 自己维护按钮、选中态和输入监听，外部清空会破坏关闭重开后的生命周期。
- 不要再给顶层页签额外加 `bTopLevelTabsRegistered` 这类外部缓存状态。正确做法是注册前查询 TabList 当前是否已有对应 `TabId`，没有才调用 `RegisterDynamicTab`。
- `RegisterDynamicTab` 不能放进衣柜 C++ 构造函数。`TopSettingsTabs` 是 UMG `BindWidget`，构造阶段尚未绑定；应在 `NativeConstruct`、`NativeOnInitialized` 或 `NativeOnActivated` 这类 Widget 已经拥有子控件的阶段做幂等注册。

## 当前推荐判断

- `WardrobePrimaryTabsHBox`：候删。
- `TopSettingsTabs`：主线。
- `WardrobeSubTabsHBox`：主线，但应只承载二级分类。
- `ClothUniformGrid`：主线，承载当前过滤后的物品。
- `BP_OnExecuteNamedAction`：不用于页签切换，不应被衣柜页签逻辑依赖。

## MCP 正式版调用补充

UE 5.8 正式版 MCP 的 `call_tool` 是一层包装。调用 toolset 工具时参数结构必须是：

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "call_tool",
    "arguments": {
      "toolset_name": "UMGToolSet.UMGToolSet",
      "tool_name": "GetWidgets",
      "arguments": {
        "widgetBlueprint": {
          "refPath": "/Game/UI/Settings/W_LyraSettingScreen.W_LyraSettingScreen"
        }
      }
    }
  }
}
```

PowerShell 里不要把函数参数命名为 `$args`，它容易和 PowerShell 自动变量混淆，导致请求里 `"arguments":[]`，MCP 会报输入参数为空。
