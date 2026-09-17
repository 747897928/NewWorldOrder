# 姿势库实现指南

最后更新：2026-07-24

# 主线调用链

1. `BP_ShootCharacter.ShowMenuWidget` 将 `/Game/UI/Menu/WBP_GameMenu` 推入目标 LocalPlayer 的 CommonUI GameMenu 层。
2. `WBP_GameMenu.PoseLibraryLyraScrollBox` 持有唯一的 `PoseLibraryUniformGrid`。
3. `WBP_GameMenu.Event Construct` 从 OwningPlayer 的 `AShootPlayerController::GetPoseLibrary()` 读取 `/Game/Blueprints/Animations/DA_PoseLibrary`。
4. `WBP_GameMenu` 遍历 `PoseEntries`，创建 `W_PoseLibraryEntry`，以 `Index % 3` 为列、`Index / 3` 为行加入 `PoseLibraryUniformGrid`。
5. 玩家点击条目，`W_PoseLibraryEntry` 调用 `AShootPlayerController::RequestPlayPoseByIndex(PoseIndex)`。
6. 客户端只发送索引；服务器校验 DataAsset、角色性别和动画资源。
7. 服务器调用 `AShootCharacter::MulticastPlayPose`，所有机器的 AnimInstance 使用 `PlaySlotAnimationAsDynamicMontage` 播放，Blend Out 后回到 locomotion。

# C++ 边界

`AShootPlayerController`

- `PoseLibrary` 属性定义在 `AShootPlayerController`，由 `BP_ShootPlayerController` 指向 `DA_PoseLibrary`。
- `GetPoseLibrary()` 是 UMG 读取数据源的唯一入口。
- `RequestPlayPoseByIndex`、Server RPC、客户端结果通知保留为网络和提示链路。
- 不再拥有 `PoseLibraryWidgetClass`、`PoseLibrarySlotTag`、`FUIExtensionHandle`、`OpenPoseLibrary`、`ClosePoseLibrary` 或 `BP_OpenPoseLibrary`。

`AShootCharacter`

- 保留 `MulticastPlayPose`，只负责所有客户端可见的动画表现。
- 不再拥有 `OpenPoseLibraryAction`、输入绑定和 `OpenPoseLibrary` 函数。

`NewWorldOrder.Build.cs`

- 姿势库不再直接依赖 UIExtension；只有其它确有 HUD extension 需求的模块才应声明该依赖。

# UMG 配置

`/Game/UI/Menu/WBP_GameMenu`

- 父类 `UShootMainMenuScreen` 继承自 `ULyraActivatableWidget`，由 CommonUI 管理 GameMenu 的输入、焦点与 Back Action。
- 右侧 `PoseLibraryBorder -> PoseLibraryLyraScrollBox` 是姿势库区域。
- 将 `PoseLibraryUniformGrid` 作为 `PoseLibraryLyraScrollBox` 的唯一内容。ScrollBox 负责纵向滚动和裁剪，UniformGridPanel 以三列填充可用区域；不保留横向条或嵌套 ScrollBox。
- `PoseLibraryUniformGrid` 的 `MinDesiredSlotWidth = 230`、`MinDesiredSlotHeight = 180`、`SlotPadding = 5px`。这是 WBP 的表现层配置，用来保证短名称和长名称的姿势卡片占用一致的网格单元。
- 不新增姿势库专用关闭按钮；`ReturnButton` 和 CommonUI Back Action 关闭整个 GameMenu。

`/Game/UI/Pose/W_PoseLibrary`

- 已删除。它原有的横向 `PoseListPanel` 不适合 GameMenu 的大面积内容区，避免保留重复的数据生成生命周期。
- `W_PoseLibraryEntry` 保留为可复用的姿势卡片；其点击蓝图入口继续调用 C++ 的 `RequestPlayPoseByIndex`。

`/Game/UI/Hud/W_DefaultHUD`

- 删除 `ExtensionPoint_PoseLibrary`，不保留隐藏控件或无效 Tag。
- 血条与蓝条 HUD 内容保持原位置。

# 输入资产

- 删除 `/Game/Blueprints/Input/Actions/IA_PoseLibrary`。
- 从 `/Game/Blueprints/Input/IMC_Default` 移除其 P 和 Gamepad DPad Up 映射。
- `/Game/Blueprints/Input/IMC_FrontEnd` 不添加姿势库专用 Action；它仍由激活的 `WBP_GameMenu` 使用，负责菜单的真实导航、确认和返回。

# UIExtension 迁移清单

- 删除 `Config/DefaultGameplayTags.ini` 中 `HUD.Slot.PoseLibrary`。
- 删除 HUD Widget 中同名 `UUIExtensionPointWidget`。
- 删除控制器注册、注销、handle、软类和 Tag 字段。
- 删除姿势库关闭按钮及其“注销 handle”图表。
- 验证仓库中不再有 `HUD.Slot.PoseLibrary`、`OpenPoseLibrary`、`ClosePoseLibrary`、`PoseLibraryExtensionHandle`、`IA_PoseLibrary` 引用。
- 通用 UIExtension 原理见 `Docs/Engineering/Notes/UI/UIExtension_HUD插槽学习笔记.md`。

# 验收

- 打开 GameMenu 后能看到姿势库区域和所有可用姿势条目。
- 鼠标、键盘和手柄可在现有 CommonUI 菜单中导航；返回关闭整个菜单。
- 选择女主可用姿势后，Host 与 Client 都能看到播放和 Blend Out。
- 男主选择不兼容条目时由服务器拒绝，且不会播放。
- HUD 不再有姿势库浮层，P/DPad Up 不再单独打开姿势库。
