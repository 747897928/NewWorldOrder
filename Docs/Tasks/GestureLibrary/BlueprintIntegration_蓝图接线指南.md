# 姿势库蓝图接线指南

最后更新：2026-07-24

# 迁移步骤

1. 打开 `/Game/UI/Hud/W_DefaultHUD`。

- 删除 `ExtensionPoint_PoseLibrary`。
- 确认根 `CanvasPanel_0` 只保留原有 HUD 内容，不留下隐藏或空插槽。

2. 删除 `/Game/UI/Pose/W_PoseLibrary`。

- 它的 `CloseButton`、横向 `PoseListPanel` 和双初始化路径不再保留。
- 保留 `/Game/UI/Pose/W_PoseLibraryEntry`，它仍是姿势卡片和点击入口。

3. 打开 `/Game/UI/Menu/WBP_GameMenu`。

- 在 `PoseLibraryBorder` 内的 `PoseLibraryLyraScrollBox` 添加 `UniformGridPanel`，命名为 `PoseLibraryUniformGrid`，并使其成为唯一内容。
- 设置 `PoseLibraryUniformGrid.MinDesiredSlotWidth = 230`、`MinDesiredSlotHeight = 180`，`SlotPadding = (5, 5, 5, 5)`；这是卡片网格的一致尺寸与四周间距，不要依赖文字或图标的期望大小。
- 在 EventGraph 的 `Event Construct` 保留可见链路：`Clear Children -> Get Owning Player -> Cast To ShootPlayerController -> Get Pose Library -> Get Pose Entries -> For Each Loop -> Create W_PoseLibraryEntry -> Add Child to Uniform Grid`。
- 将数组索引同时接到 `PoseIndex`、`% (Integer)` 和 `int / int`；两者除数为 3，分别接 `InColumn`、`InRow`。展示名和图标从 `Break Shoot Pose Library Entry` 接入创建卡片节点。
- `Add Child to Uniform Grid` 返回的 `UniformGridSlot` 依次调用 `Set Horizontal Alignment(HAlign_Fill)`、`Set Vertical Alignment(VAlign_Fill)`。此蓝图步骤显式保证卡片背景填满固定网格单元。
- 外层保持 GameMenu 的样式、边距和裁剪；不要把姿势库移回 HUD，也不要用 `AddToViewport`。
- `WBP_GameMenu` 继承 `UShootMainMenuScreen`，其 `CloseScreen()` 和 `NativeOnHandleBackAction()` 已调用 `DeactivateWidget()`；姿势库关闭跟随菜单退栈。

4. 打开 `/Game/Blueprints/Player/BP_ShootPlayerController`。

- 保留 `PoseLibrary = /Game/Blueprints/Animations/DA_PoseLibrary`。
- C++ 字段删除后，清除旧的 `PoseLibraryWidgetClass` 和 `PoseLibrarySlotTag` 覆盖值。

5. 打开 `/Game/Blueprints/Character/BP_ShootCharacter`。

- C++ 字段删除后，清除旧的 `OpenPoseLibraryAction` 覆盖值。

6. 输入资产。

- 删除 `/Game/Blueprints/Input/Actions/IA_PoseLibrary`。
- 从 `/Game/Blueprints/Input/IMC_Default` 删除它的 P 和 Gamepad DPad Up 映射。
- 不向 `/Game/Blueprints/Input/IMC_FrontEnd` 添加姿势库动作。该映射上下文仍由 GameMenu 的 CommonUI 输入配置使用。

# 保留的数据与动画接线

- `/Game/Blueprints/Animations/DA_PoseLibrary` 继续保存 `PoseEntries`。
- 每个条目配置 `DisplayName`、`Icon`、`CompatibleGender`、`Animation`、`SlotName`、播放和混合参数。
- 当前女主条目使用 `UpperBodyAdditive`；女主 AnimBP 必须有同名 Slot 节点。
- `W_PoseLibraryEntry.BP_OnClicked` 继续调用 `RequestPlayPoseByIndex(PoseIndex)`，不要直接播放任意动画资源。

# 验收步骤

1. 单人 PIE 打开 GameMenu，确认右侧姿势库按三列填充并可纵向滚动。
2. 点击姿势卡片，确认角色播放后回到 locomotion。
3. 用 Back Action 或 ReturnButton 关闭菜单，确认输入恢复。
4. Listen Server 双人 PIE：分别选择姿势，双方都能看到正确角色播放。
5. 确认 HUD 中没有姿势库浮层，P 和 DPad Up 不再单独开启姿势库。
