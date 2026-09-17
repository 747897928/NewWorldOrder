# AdventurersInventoryKit Tooltip 蓝图实现调研

日期：2026-07-03
状态：[可用]

## 调研目标

用户希望确认 AdventurersInventoryKit 的 GearHoverInfo 是否使用了 UE 内置 Tooltip 组件，或者是否有比当前衣柜系统手动显示/隐藏 `W_ClothItemTooltip` 更可复用的做法。

本次通过 UE 5.8 MCP 连接 AdventurersInventoryKit 的 16000 端口，只读检查以下资产：

- `/Game/InventoryKit/Widgets/InventoryKit/Widget_GearHoverInfo`
- `/Game/InventoryKit/Widgets/InventoryKit/Widget_ItemSelectionPage`
- `/Game/InventoryKit/Widgets/InventoryKit/Widget_SingleGear`

## 关键结论

AdventurersInventoryKit 的 GearHoverInfo 不是挂在单个 item 上的 UE 原生 ToolTip Widget。

它的核心模式是：

1. 父页面 `Widget_ItemSelectionPage` 常驻持有详情卡实例：
   - `LeftHoverWidget`
   - `RightHoverWidget`
2. 装备格子 `Widget_SingleGear` 持有自己的 `ItemInfo`，并通过事件分发器向父页面报告交互：
   - `OnHovered`
   - `OnUnHovered`
   - `OnClickedGear`
   - `UpdateWidgetReference`
3. 父页面收到 hover/click/focus 事件后，选择左侧或右侧详情卡，调用详情卡的更新事件：
   - `Widget_GearHoverInfo.Update_Hover_Widget(ItemInfo)`
4. 详情卡只负责把传入的 `ItemInfo` 渲染到自己的文本、图标、词条列表和说明区域。
5. 父页面负责播放详情卡动画：
   - hover 时正向播放 `HoverInfo`
   - unhover 时反向播放 `HoverInfo`

因此它更接近“可嵌入的物品详情卡组件”，不是“跟随鼠标的 Tooltip 浮层组件”。

## Widget 结构

### Widget_GearHoverInfo

父类：

- `/Script/UMG.UserWidget`

根节点：

- `Container`，类型 `/Script/UMG.Border`

主要层级：

- `Container`
  - `SizeBox_11`
    - `Overlay_24`
      - 外层 Border
      - 内容 Border
      - `VerticalBox_30`
        - 头部区域
          - `Header`
          - `HeaderDirt`
          - `ItemName`
          - `Rarity`
        - 等级区域
          - `RequiredLevel`
          - `LevelSizer`
        - 主属性区域
          - `MainEffectValue`
          - `MainEffect`
        - `PerksList`
          - `Widget_SingleGearPerk` x3
        - `EngraveInfoContainer`
          - `PerkIcon`
          - `EngraveItemsList`
          - `Widget_SingleRequiredItem` x3
        - `ItemDescription`

变量：

- `RarityColors`

事件：

- `Update_Hover_Widget(ItemInfo)`

已读到的外层逻辑：

```text
Update_Hover_Widget(ItemInfo)
  -> SetDefaultTexts(ItemInfo)
  -> SetMainEffectTexts(ItemInfo)
```

注意：`SetDefaultTexts` 和 `SetMainEffectTexts` 是 `K2Node_Composite`，UE 5.8 的 `BlueprintTools.read_graph_dsl` 读取时会报 `K2Node_Composite` 无法 cast 到 Blueprint。这是 MCP 工具限制，不代表蓝图本身有问题。

### Widget_SingleGear

父类：

- `/Script/UMG.UserWidget`

变量中与通信相关的部分：

- `ItemInfo`
- `OnClickedGear`
- `UpdateWidgetReference`
- `OnHovered`
- `OnUnHovered`
- `IsSelected`
- `Empty?`
- `Locked?`

关键函数：

```text
HoverAction()
  -> PlayAnimation(HoverEffect)
  -> CallOnHovered(self)

UnHoverAction()
  -> PlayAnimation(HoverEffect, Reverse)
  -> CallOnUnHovered()

ClickAction()
  -> CallUpdateWidgetReference(self)
  -> CallOnClickedGear()

SetSelected(Selected)
  -> SetIsSelected(Selected)
  -> SelectedBorder visibility = SelfHitTestInvisible / Collapsed
```

### Widget_ItemSelectionPage

父类：

- `/Script/UMG.UserWidget`

它是 GearHoverInfo 的直接引用者。

树中关键节点：

- `LeftGearOverlay`
  - `LeftGear`
  - `LeftSideSwitcher`
  - `LeftHoverWidget`，类型 `Widget_GearHoverInfo`
- `RightGearOverlay`
  - `RightHoverWidget`，类型 `Widget_GearHoverInfo`
  - `RightSideSwitcher`
  - `RightGear`

关键函数：

```text
OnHoverToGear(GearWidget)
  -> Update_Hover_Widget(UsingLeftSide ? LeftHoverWidget : RightHoverWidget, GearWidget.ItemInfo)
  -> PlayAnimation(HoverInfo)
  -> UpdateHoveredGearForGamepad(GearWidget)

OnUnHoverGear()
  -> PlayAnimation(HoverInfo, Reverse)
  -> ClearHoveredGearForGamepad()

UpdateHoveredGearForGamepad(GearWidget)
  -> GetGameWidgets(GameInstance)
  -> Widget_InGameMenu.Widget_Inventory.OnHoveredToGearSlot(GearWidget)

ClearHoveredGearForGamepad()
  -> GetGameWidgets(GameInstance)
  -> Widget_InGameMenu.Widget_Inventory.OnUnHoveredGearSlot()
```

生成格子时，父页面会创建 `Widget_SingleGear`，加入 UniformGrid，并绑定它的事件分发器：

```text
PopulateGears(...)
  -> CreateWidget(Widget_SingleGear)
  -> GenerateGrid(...)
  -> BindEventToOnHovered(...)
  -> BindEventToOnUnhovered(...)
  -> BindEventToOnClickedGear(...)
  -> BindEventToUpdateWidgetReference(...)
```

## 对 NewWorldOrder 衣柜系统的启发

### 推荐方向

衣柜系统的详情展示也应当分成两层：

1. `W_Cloth_Item` 或对应 C++ item widget 只负责：
   - 持有服装条目数据
   - 发出 hover、unhover、focus、selected、clicked 事件
   - 自己播放格子 hover/selected 动画
2. `W_Cloth` 作为父页面负责：
   - 持有一个常驻的详情卡实例
   - 鼠标 hover 时更新详情卡
   - 手柄焦点变化时更新详情卡
   - 决定详情卡是嵌入右侧栏、浮在格子旁边，还是进入“详情模式”
3. `W_ClothItemTooltip` 只负责：
   - 渲染服装详情
   - 暴露一个数据更新入口
   - 不负责自己 AddToViewport
   - 不负责自己推算屏幕位置
   - 不在 C++ 中强制尺寸、边距、根节点类型

### 不建议继续的方向

不建议每次 hover 都创建一个新的 Tooltip，再手动 AddToPlayerScreen、移动位置、隐藏/显示。

原因：

- 鼠标模式勉强可用，但容易出现第一帧左上角闪烁。
- 手柄模式没有鼠标位置，详情显示应跟焦点或选中项走。
- 父页面无法稳定管理 Tooltip 生命周期。
- 如果后续武器、材料、技能都需要详情卡，会重复写一套浮层定位代码。

### UE 原生 Tooltip 属性的适用性

UE 的 Widget 确实有 ToolTip / ToolTipWidget 相关能力，但 AdventurersInventoryKit 没有采用这种方式。

对本项目来说，原生 Tooltip 可以用于简单鼠标提示，但不适合作为衣柜主详情系统的唯一方案：

- 手柄焦点和 CommonUI 导航需要“当前焦点项”的详情展示。
- 衣柜详情卡需要装备状态、稀有度、适用角色、来源、未来属性词条等复杂内容。
- 详情卡可能需要固定在右侧面板，或者进入单独详情模式，不一定跟随鼠标。

因此更推荐“父页面常驻详情卡 + item 事件分发器/焦点事件”的架构。

## 可迁移到 W_Cloth 的实现草案

1. 保留 `W_ClothItemTooltip` 作为可复用详情卡 Widget，但不要把它理解成只能跟随鼠标的 Tooltip。
2. `UShootWardrobeItemTooltipBase` 应该只保留数据入口和最少的可选 BindWidget 字段：
   - 不强制创建布局
   - 不强制设置 SizeBox 尺寸
   - 不强制设置 Border padding
   - 不在 C++ 中覆盖蓝图 Designer 里的视觉设置
3. `W_Cloth` 内常驻一个 `W_ClothItemTooltip` 实例，或在右侧/浮层容器中创建一次后复用。
4. `W_Cloth_Item` 需要向父页面发事件：
   - 鼠标进入：更新详情卡并播放出现动画
   - 鼠标离开：按当前策略隐藏或保留选中项详情
   - 获得焦点：手柄模式下更新详情卡
   - 点击/确认：装备或卸下
5. 如果仍需要“浮在鼠标附近”的 PC 体验，浮层位置应该由 `W_Cloth` 统一计算，不应由详情卡自身计算。
6. 手柄模式下应优先显示“当前焦点项”或“当前选中项”的详情，而不是依赖鼠标 hover。

## 与当前衣柜代码的关系

当前 `W_ClothItemTooltip` 已经做成 GearHoverInfo 风格的外观，但仍存在运行态布局与 Designer 不一致的问题。结合本次调研，优先减少 `UShootWardrobeItemTooltipBase` 对布局的干预，让蓝图完全控制视觉结构。

2026-07-03 已开始迁移：

1. `UShootWardrobeWidgetBase` 不再运行时 `CreateWidget` 后 `AddToPlayerScreen` 显示 `W_ClothItemTooltip`。
2. `UShootWardrobeWidgetBase` 不再保存服装格子锚点，不再每帧调用 `PositionItemTooltip`。
3. `W_Cloth` 应内嵌一个继承自 `UShootWardrobeItemTooltipBase` 的详情卡，并命名为 `ItemTooltipPanel`。
4. `UShootWardrobeWidgetBase` 只通过 `ItemTooltipPanel->SetWardrobeItem` 写入当前服装数据。
5. 鼠标 hover、CommonUI focus、右键详情请求都更新同一个常驻详情卡。
6. `HandleItemWidgetUnhovered` 不再隐藏详情卡，避免手柄焦点移动时详情卡反复折叠导致焦点链断开。

仍需蓝图侧配合：

1. `/Game/UI/Mutable/W_Cloth` 需要把 `/Game/UI/Mutable/W_ClothItemTooltip` 放进自己的布局树。
2. 该实例必须命名为 `ItemTooltipPanel`，否则 C++ 的 `BindWidgetOptional` 不会绑定到它。
3. `ItemTooltipPanel` 的尺寸、边框、背景、动画、显隐初始状态都在蓝图 Designer 中设置，不应再由 C++ 改写。

## MCP 操作记录

本次使用 16000 端口的 UE 5.8 原生 MCP，未安装 VibeUE 也能完成只读调研。

已验证可用工具：

- `editor_toolset.toolsets.asset.AssetTools.find_assets`
- `editor_toolset.toolsets.asset.AssetTools.get_referencers`
- `UMGToolSet.UMGToolSet.GetWidgets`
- `editor_toolset.toolsets.blueprint.BlueprintTools.list_functions`
- `editor_toolset.toolsets.blueprint.BlueprintTools.list_events`
- `editor_toolset.toolsets.blueprint.BlueprintTools.list_event_dispatchers`
- `editor_toolset.toolsets.blueprint.BlueprintTools.read_graph_dsl`

踩坑：

- `BlueprintTools.read_graph_dsl` 读取 `K2Node_Composite` 子图时会失败，错误核心是 `K2Node_Composite` 无法 cast 到 `Blueprint`。遇到这种情况，优先读外层调用链、变量列表和普通函数图，不要反复重试同一个 composite 子图。

# 2026-08 落地结论（悬停回归修复·最终）

现象：`W_ClothItemTooltip` 不再随鼠标悬停弹出。

真正根因（经 MCP 读 MVVM 绑定确认，非简单显隐）：
1. `W_Cloth` 里有一条坏掉的 MVVM 绑定 `WardrobeVM.SelectedItem -> ItemTooltipPanel.TooltipItemVM`，编译报 “The field for source 'ItemTooltipPanel' exists but is not accessible at runtime”（fatal），W_Cloth 无法编译。
2. 迁移后没有任何“悬停 -> 详情卡”的蓝图层事件与定位逻辑；参考实现（AdventurersInventoryKit `Widget_GearHoverInfo` / `Widget_ItemSelectionPage`）本应有左/右卡 + 跟随鼠标 + 出现/收起动画。

修复（对齐参考实现，视觉/定位放蓝图）：
1. C++ 只留管道：`UShootObjectEntryButtonBase` 增加与点击对称的 `EntryHovered` / `EntryUnhovered` 语义委托，`UShootWardrobeScreen` 透传给蓝图事件 `BP_OnWardrobeItemHovered(Item, EntryWidget)` / `BP_OnWardrobeItemUnhovered`；C++ 不写显隐、不算坐标、不做设备分支。
2. 删除 `W_Cloth` 与 `W_ClothItemTooltip` 上共 4 条坏掉的 / 被替代的 MVVM 绑定。
3. `W_ClothItemTooltip` 新增 `Update_Hover_Widget(ItemVM)` 自定义事件，直接写 `NameLabel` / `DescLabel` / `IconImage`（对应 GearHoverInfo 的 `Update_Hover_Widget(ItemInfo)`）。
4. `W_Cloth` EventGraph：`BP_OnWardrobeItemHovered -> ItemTooltipPanel.Update_Hover_Widget(Item) + SetVisibility(SelfHitTestInvisible)`；`BP_OnWardrobeItemUnhovered -> SetVisibility(Collapsed)`。

# 2026-08-14 闪烁修复（最终状态）

现象升级：a10441f 的 Tick 平滑跟随（X/Y 都 FInterpTo 速度 10.0）导致所有格子悬停都闪烁。经 MCP 读 W_Cloth EventGraph 实锤根因：

1. 卡片 X 跟随鼠标（+20/-320）且每帧 SetRenderTranslation——卡片移动扫过相邻格子，触发 unhover -> Collapsed -> 再 hover -> 循环。
2. 卡片可见性 SelfHitTestInvisible 只让自身穿透，子控件（文本/图标）仍拦截命中——鼠标在卡片上时格子收不到 hover。

修复（对齐参考实现 Widget_ItemSelectionPage 的防闪三要素）：
1. X 不跟随鼠标：hover 时按半屏一次性定 TooltipTargetX（左半屏 X=鼠标X+20，右半屏 X=鼠标X-320），Tick 不再动 X。
2. Y 跟随但偏移 300 且 Clamp：Tick 里 Y = Clamp(FInterpTo(CurY, 鼠标Y-300, 3.0), 0, 1000)，卡片出现在鼠标上方，不盖鼠标所在格子。
3. 卡片整体 HitTestInvisible：W_ClothItemTooltip 根 Container 设 HitTestInvisible（自身+子控件都不拦截命中），鼠标穿透卡片，hover 永不丢失。
4. 显隐：hover -> PlayShowAnimation（播放已有 ShowTooltipAnimation 正向动画）；unhover -> PlayHideAnimation -> SetVisibility(Collapsed)。动画资产 W_ClothItemTooltip:ShowTooltipAnimation 确认存在（0.14s，绑定 1 控件），此前从未被接线。

MCP 踩坑补充：
- BlueprintService.build_graph 任一条连接失败即整体返回 None（无逐项错误），需单独创建节点 + connect_nodes 排查；节点 ID 必须是完整 GUID（截断 8 位会连接失败返回 False）。
- 蓝图自定义事件作为函数调用时，function_call 的 class 需用完整资产路径（/Game/UI/Mutable/W_ClothItemTooltip.W_ClothItemTooltip_C），短类名会创建失败。
