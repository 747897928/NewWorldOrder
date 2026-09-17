---
task_id: GestureLibrary
status: validation_pending
assigned_to: Codex
progress: 95%
started: 2026-06-30
last_updated: 2026-07-24
---

# 姿势库任务状态

当前状态：validation_pending

# 2026-07-24 迁移状态

## 2026-07-24 网格布局修正

- 用户验收截图确认旧的横向条没有利用 `PoseLibraryBorder` 的可用空间，因此删除 `/Game/UI/Pose/W_PoseLibrary`，不保留兼容容器。
- `WBP_GameMenu.Event Construct` 直接读取 `AShootPlayerController.GetPoseLibrary()`，清空 `PoseLibraryUniformGrid` 后遍历 `PoseEntries`，以 `Index % 3` 和 `Index / 3` 写入 `InColumn`、`InRow`。
- 数据装配、三列布局和 `W_PoseLibraryEntry` 创建流程保留为带中文注释框的蓝图节点；C++ 只保留 DataAsset 入口与服务器权威播放请求。
- `WBP_GameMenu` 与 `W_PoseLibraryEntry` 已重新编译，`PoseLibraryLyraScrollBox -> PoseLibraryUniformGrid` 层级和蓝图图表布局检查均通过。
- 追加网格视觉修正：`PoseLibraryUniformGrid` 使用 `230 × 180` 最小单元，并为每个单元设置四边 `5px` 间距，避免短名称条目缩小卡片或相邻卡片贴边。
- 生成蓝图同时把每个 `UniformGridSlot` 显式设为 `HAlign_Fill` 与 `VAlign_Fill`，使卡片 Border 填满单元而非仅包住图标和名称。

新需求已将姿势库从 HUD extension 改为 `WBP_GameMenu` 内容区。当前已完成：

- 更新 `UIExtension_HUD插槽学习笔记.md`，补齐 Point Tag 的配置、注册、实例化、清理、分屏、输入和验收生命周期。
- 删除 C++ 中 `OpenPoseLibrary`、`ClosePoseLibrary`、`PoseLibraryWidgetClass`、`PoseLibrarySlotTag`、`FUIExtensionHandle`、角色输入绑定和 UIExtension 模块依赖。
- 删除 `HUD.Slot.PoseLibrary` 配置，并更新本任务包的需求、实现与蓝图接线指南。

已完成资产迁移与验证：

- `/Game/UI/Pose/W_PoseLibrary` 已删除。`WBP_GameMenu.PoseLibraryLyraScrollBox` 的唯一内容是 `PoseLibraryUniformGrid`，Event Construct 在蓝图中按三列创建 `W_PoseLibraryEntry`，充分使用右侧内容区并保持纵向滚动。
- `/Game/UI/Hud/W_DefaultHUD.ExtensionPoint_PoseLibrary` 已删除；HUD 控件树只保留血条和蓝条。
- `W_PoseLibrary.CloseButton`、`CloseButtonText`、事件绑定和 `ClosePoseLibrary` 图表残留均已删除。
- `/Game/Blueprints/Input/Actions/IA_PoseLibrary` 已删除，`IMC_Default` 的 P、Gamepad DPad Up 映射已删除。
- `IMC_FrontEnd` 已复核，仅保留 `IA_Confirm -> Gamepad_FaceButton_Bottom`，未引入姿势库专用输入。
- `BP_ShootPlayerController`、`BP_ShootCharacter`、`WBP_GameMenu` 和 `W_DefaultHUD` 已编译；迁移后的 Widget 层级均通过校验。
- 已完成标准 PIE 启动、运行和退出 smoke。

剩余运行态验收：

- 单人打开 GameMenu 后确认姿势卡片出现、可滚动、点击播放并自然恢复 locomotion。
- Listen Server 双人 PIE 中分别选择姿势，确认双方可见。
- 用键盘、鼠标和手柄确认 CommonUI 焦点与返回关闭整个 GameMenu 后输入恢复。

已完成：
- 扫描 `/Game/Assets/Animations/Girl`，确认当前资源类型是 `AnimSequence`，不是 `AnimMontage`。
- 新增 `UShootPoseLibraryDataAsset` 与 `FShootPoseLibraryEntry`，姿势列表由 DataAsset 配置，不在 C++ 写死动画路径。
- 新增 `AShootPlayerController::OpenPoseLibrary`、`RequestPlayPoseByIndex` 与服务器校验链路。
- 新增 `AShootCharacter::OpenPoseLibraryAction`，输入动作由 `IMC_Default` 管理。
- 新增 `AShootCharacter::MulticastPlayPose`，服务器确认后多播播放 Slot 动态 Montage。
- 通过 VibeUE 新增 `/Game/Blueprints/Input/Actions/IA_PoseLibrary`。
- 通过 VibeUE 给 `/Game/Blueprints/Input/IMC_Default` 增加姿势库键位：键盘 `P`、手柄 `Gamepad_DPad_Up`。
- 已创建并迁移 `/Game/Blueprints/Animations/DA_PoseLibrary`，填入 `/Game/Assets/Animations/Girl` 下全部 26 个 AnimSequence。
- 已设置并保存 `BP_ShootCharacter.OpenPoseLibraryAction = IA_PoseLibrary`。
- 已设置并保存 `BP_ShootPlayerController.PoseLibrary = /Game/Blueprints/Animations/DA_PoseLibrary`。
- 已为 `AShootPlayerController` 增加 `PoseLibraryWidgetClass`，配置后会通过 CommonUI 推送到 `UI.Layer.Menu`。
- 已创建并编译 `/Game/UI/Pose/W_PoseLibrary`，父类为 `ULyraActivatableWidget`，不依赖新增 C++ Widget 基类。
- 已创建并编译 `/Game/UI/Pose/W_PoseLibraryEntry`，父类为项目已有 `ULyraButtonBase`。
- `W_PoseLibrary` 在 `BP_OnActivated` 中读取 `AShootPlayerController::GetPoseLibrary()`，遍历 `GetPoseEntries()`，动态创建条目按钮并添加到 `PoseListPanel`。
- `W_PoseLibraryEntry` 持有蓝图变量 `PoseIndex` 与 `PoseDisplayName`，点击时调用 `AShootPlayerController::RequestPlayPoseByIndex(PoseIndex)`。
- `W_PoseLibraryEntry` 已补齐本地 UMG root：`ButtonTextBlock`，并实现 `UpdateButtonText(InText) -> ButtonTextBlock.SetText(InText)`，避免条目按钮没有可显示文本控件。
- 已设置并保存 `BP_ShootPlayerController.PoseLibraryWidgetClass = W_PoseLibrary`。
- 已删除上一轮错误加入的 `UShootPoseLibraryWidgetBase` / `UShootPoseLibraryEntryWidgetBase` C++ Widget 类，姿势库 UI 回到 UMG 蓝图负责。
- 已通过 MCP 确认 `BP_ShootAnimInstance_F` 的 AnimGraph 中存在 Slot 节点：`UpperBodyAdditive`。
- 已把 `/Game/Blueprints/Animations/DA_PoseLibrary` 的 26 个女主姿势条目 SlotName 统一为 `UpperBodyAdditive`。
- 已确认 `DA_PoseLibrary` 的 26 个条目均为 `FEMALE`，因此男主当前会被服务器性别校验拒绝，不需要改 `BP_ShootAnimInstance_M`。
- C++ 已通过 macOS `NewWorldOrderEditor Mac Development` 编译。
- `BP_ShootCharacter` 和 `BP_ShootPlayerController` 已通过 BlueprintTools 编译。
- 2026-07-03 通过 VibeUE/MCP 重新做资产级验收，当时 DA 仍在旧单数目录，`PoseEntries` 数量为 20：
  - 20 个条目均有 `Animation`，`CompatibleGender` 均为 `FEMALE`，`SlotName` 均为 `UpperBodyAdditive`。
  - `/Game/Blueprints/Character/BP_ShootCharacter` 的 `OpenPoseLibraryAction` 指向 `/Game/Blueprints/Input/Actions/IA_PoseLibrary`。
  - `/Game/Blueprints/Player/BP_ShootPlayerController` 的 `PoseLibrary` 指向当时的旧 DataAsset 路径。
  - `/Game/Blueprints/Player/BP_ShootPlayerController` 的 `PoseLibraryWidgetClass` 指向 `/Game/UI/Pose/W_PoseLibrary`，`PoseLibraryLayerTag` 为 `UI.Layer.Menu`。
  - `/Game/Blueprints/Input/IMC_Default` 中 `IA_PoseLibrary` 当前映射为键盘 `P` 和手柄 `Gamepad_DPad_Up`。
  - `/Game/UI/Pose/W_PoseLibrary` 父类为 `ULyraActivatableWidget`，控件树包含 `PoseLibraryRoot`、`TitleText`、可变量 `PoseListPanel`。
  - `/Game/UI/Pose/W_PoseLibraryEntry` 父类为项目已有 `ULyraButtonBase`，生成类上存在 `PoseIndex` 与 `PoseDisplayName` 变量。
  - `BP_ShootCharacter`、`BP_ShootPlayerController`、`W_PoseLibrary`、`W_PoseLibraryEntry`、`BP_ShootAnimInstance_F`、`BP_ShootAnimInstance_M` 状态均为 `BS_UP_TO_DATE`。
- 2026-07-03 UE 重启后补做非 PIE 验收：
  - 通过 MCP 编译并检查 `BP_ShootCharacter`、`BP_ShootPlayerController`、`W_PoseLibrary`、`W_PoseLibraryEntry`、`BP_ShootAnimInstance_F`、`BP_ShootAnimInstance_M`。
  - 姿势库 DataAsset、输入映射、控制器配置、角色输入动作、条目变量检查均通过，错误数为 0。
  - `W_PoseLibrary` 编译状态为 `BS_UP_TO_DATE_WITH_WARNINGS`，当前未拿到 UE Message Log 展开的具体警告正文；图表连接经 VibeUE `BlueprintService.get_graph_definition` 检查，主链路完整。
  - macOS `NewWorldOrderEditor Mac Development` C++ 编译通过。
- 2026-07-03 继续修复非 PIE 验收残留：
  - 发现 `W_PoseLibraryEntry` 直接继承 `ULyraButtonBase` 但没有本地 root，`WidgetService.validate` 报 `Widget Blueprint has no root widget`。
  - 已通过 MCP 给 `W_PoseLibraryEntry` 增加本地 root `ButtonTextBlock`，并用 `BlueprintService.build_graph` 增加 `UpdateButtonText` 事件图。
  - 修复后 `W_PoseLibraryEntry` 编译状态为 `BS_UP_TO_DATE`，`WidgetService.validate` 通过。
  - `W_PoseLibrary` 仍为 `BS_UP_TO_DATE_WITH_WARNINGS`，但 `WidgetService.validate` 通过；SlateInspector 未能在当前界面抓到具体 Compiler Results 文本。
  - 修复后再次执行 macOS `NewWorldOrderEditor Mac Development` C++ 编译，通过。
- 2026-07-06 根据用户提供的概念图右下角验收目标补强姿势库 UI：
  - `W_PoseLibrary` 从基础 `VerticalBox` 根改为 `CanvasPanel_0` 根，右下角锚定 `Overlay_0` 主面板。
  - `Overlay_0` 包含 `PosePanelBackdrop` 半透明深色底板和原 `PoseLibraryRoot`，保留 `PoseListPanel` 变量与现有 `BP_OnActivated` 图表链路。
  - `PoseListPanel` 改为横向 `ScrollBox`，与概念图的横向姿势卡片列表一致。
  - 当时新增过 `PoseHintText` 小型按键提示；该提示并非真实输入需求，已在 2026-07-07 删除。
  - `W_PoseLibraryEntry` 从单文本根升级为 `SizeBox_0 -> Overlay_0` 卡片结构，新增 `PoseEntryBackdrop`、`PoseGlyphText`、`PoseStarText`，保留 `ButtonTextBlock` 变量和点击图表。
  - 通过 `WidgetService.validate` 确认 `W_PoseLibrary` 与 `W_PoseLibraryEntry` 层级有效。
  - 通过 `WidgetService.capture_preview` 生成 `Saved/WidgetPreviews/W_PoseLibrary.png` 与 `Saved/WidgetPreviews/W_PoseLibraryEntry.png` 做非 PIE 视觉检查。
- 2026-07-07 修正姿势库 UI 可用性和 Lyra HUD 扩展笔记：
  - 已通过 MCP 确认 `/Game/Blueprints/Mutable/Wardrobe/DA_WardrobeCatalog` 使用 `WardrobeItems` 配置 18 个 ItemDefinition 蓝图，新增衣柜条目应走 DataAsset，不应写 C++。
  - 已通过 MCP 确认当时的旧 DataAsset 使用 `PoseEntries` 配置 20 个姿势条目，新增姿势应继续加 DataAsset 条目，不需要改 C++。
  - 已删除 `W_PoseLibrary` 中 AI 生图误导出的 `PoseHintText`，不再显示 `Q E 切换姿势 Enter 确认`。
  - 已新增 `CloseButton` 与 `CloseButtonText`，并把 `CloseButton.OnClicked` 接到 `DeactivateWidget()`，用户可关闭姿势面板。
  - 已将 `CloseButton` 的 OverlaySlot 设为右上角对齐，避免按钮覆盖姿势列表。
  - 已用 Lyra MCP 端口 11000 读取 `W_ShooterHUDLayout` 与 `LAS_ShooterGame_StandardHUD`，确认 Lyra 的 HUD 模式是 `layout -> UI.Layer.Game` 加 `widgets -> HUD.Slot.*`。
  - 已新增工程笔记 `Docs/Engineering/Notes/UI/UIExtension_HUD插槽学习笔记.md`，记录 `UUIExtensionPointWidget`、`UUIExtensionSubsystem` 和 `RegisterExtensionAsWidgetForContext` 的调用链。
- 2026-07-07 继续收尾未完成待办：
  - 已删除 `W_PoseLibrary` EventGraph 中空的 `Event PreConstruct`、`Event Construct`、`Event Tick` 节点，避免后来维护者误以为这些事件承担逻辑。
  - 删除空节点后再次检查 `W_PoseLibrary`：`WidgetService.validate` 通过，EventGraph 剩余 13 个节点，主链路为 `BP_OnActivated` 动态生成条目和 `CloseButton.OnClicked -> DeactivateWidget()`。
  - `W_PoseLibrary` 编译状态仍为 `BS_UP_TO_DATE_WITH_WARNINGS`；MCP 与 LogsToolset 仍未暴露具体 warning 正文，因此不能在文档中臆测原因。
  - 已在 Windows 编辑器通过 `EditorToolset.EditorAppToolset.StartPIE` 进入当前地图 PIE，warmup 3 秒后 `StopPIE` 成功退出。
  - 本次 PIE 仅证明当前地图能进入和退出 PIE，不等价于姿势库按键打开、点击播放、手柄输入、CommonUI 返回或 Listen Server 双端可见性验收。
- 2026-07-07 根据用户追问补齐姿势 DataAsset 与图标：
  - 复查 `/Game/Assets/Animations/Girl`，实际共有 26 个 `AnimSequence`；此前 `DA_PoseLibrary` 只有 20 条，漏了 `kicking`、`kicking_1`、`mma_kick`、`mma_kick_1`、`mma_kick_3`、`standing_melee_attack_kick`。
  - 已按用户要求将 `DA_PoseLibrary` 放到 `/Game/Blueprints/Animations/DA_PoseLibrary`，避免姿势库 DataAsset 散落在动画资产目录。
  - 已把 26 个 Girl 动画全部写入 `PoseEntries`，所有条目 `CompatibleGender = FEMALE`、`SlotName = UpperBodyAdditive`。
  - 已通过 `EditorToolset.EditorAppToolset.CaptureAssetImage` 导出 26 张动画资产图，导入为 `/Game/Assets/Animations/PoseIcons/T_PoseIcon_*`。
  - 已将 26 张 Texture2D 填入对应 `PoseEntries[*].Icon`，复查结果为 26 条均有 `Animation` 和 `Icon`。
  - 已新增 `Scripts/Generate_PoseLibraryIcons.ps1` 与 `Docs/Tasks/GestureLibrary/IconGeneration_图标生成.md`，后续可以复跑生成图标。
  - 已确认 `BP_ShootPlayerController.PoseLibrary` 指向 `/Game/Blueprints/Animations/DA_PoseLibrary`。
  - 已为 `W_PoseLibraryEntry` 增加 `PoseIcon : Texture2D` ExposeOnSpawn 变量和 `PoseIconImage` 控件。
  - 已将 `W_PoseLibrary` 的 `Break Shoot Pose Library Entry.Icon` 接到 `Create W Pose Library Entry Widget.PoseIcon`。
  - 已将 `W_PoseLibraryEntry` 的 Construct 接到 `PoseIconImage.SetBrushFromTexture(PoseIcon)`。
  - 已调整 `W_PoseLibraryEntry` 为上方预览图、下方 `ButtonTextBlock` 名称显示，隐藏旧 `PoseGlyphText` 占位字。
  - `W_PoseLibraryEntry` 编译状态为 `BS_UP_TO_DATE`，`WidgetService.validate` 通过；`W_PoseLibrary` 仍为 `BS_UP_TO_DATE_WITH_WARNINGS`，但 `WidgetService.validate` 通过。
- 2026-07-08 重新复核 Lyra UIExtension 链路并修复本项目插件描述：
  - 通过 Lyra MCP 11000 读取 `/ShooterCore/UserInterface/W_ShooterHUDLayout`，确认根结构为 `SafeZone_0 -> Overlay_0 -> CanvasPanel_0`，`UUIExtensionPointWidget` 插槽放在 HUD layout 的 CanvasPanel 或局部布局容器里。
  - 通过 Lyra MCP 11000 读取 `/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD`，确认 `actions[0]` 为 `GameFeatureAction_AddWidgets_0`，其 `layout` 使用 `W_ShooterHUDLayout_C -> UI.Layer.Game`，其 `widgets` 使用 `widgetClass + slotId` 注册到 `HUD.Slot.*`。
  - 通过 Lyra C++ 复核 `UGameFeatureAction_AddWidgets::AddWidgets`：layout 走 `UCommonUIExtensions::PushContentToLayer_ForPlayer`，HUD 片段走 `UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext`。
  - 修复 `Plugins/UIExtension/UIExtension.uplugin` 中重复的 `Plugins` 字段，把 `CommonUI` 和 `CommonGame` 合并到同一个依赖数组。
  - Windows `Scripts/Build_Windows.ps1` 编译通过。
  - 当前已打开的 NewWorldOrder 编辑器仍无法热加载 `UIExtension`，`/Script/UIExtension.UIExtensionPointWidget` 和 `/Script/UIExtension.UIExtensionSubsystem` 返回 `None`；需要重启 NewWorldOrder 编辑器后才能在 `W_DefaultHUD` 里添加真实插槽。
- 2026-07-08 用户重启 NewWorldOrder 编辑器后完成 HUD 插槽落地：
  - MCP 验证 `/Script/UIExtension.UIExtensionPointWidget` 和 `/Script/UIExtension.UIExtensionSubsystem` 均可加载。
  - 已在 `/Game/UI/Hud/W_DefaultHUD` 的 `CanvasPanel_0` 下添加 `ExtensionPoint_PoseLibrary`。
  - `ExtensionPoint_PoseLibrary` 控件类为 `/Script/UIExtension.UIExtensionPointWidget`。
  - 已设置 `extensionPointTag = HUD.Slot.PoseLibrary`、`extensionPointTagMatch = ExactMatch`、`dataClasses = []`、`entryBoxType = Overlay`。
  - 已设置 CanvasPanelSlot 为右下角锚定：anchors 1,1，alignment 1,1，offsets left -40、top -40、right 760、bottom 220，zOrder 50。
  - 已给 `/Game/UI/Pose/W_PoseLibrary` 增加 `Event Construct -> Clear Children` 连接，UIExtension 创建普通 Widget 时也会生成姿势列表；原 `BP_OnActivated` 路径保留。
  - 已把 `CloseButton.OnClicked` 从 `DeactivateWidget()` 改为独立链路：`GetOwningPlayer -> Cast To ShootPlayerController -> ClosePoseLibrary()`，由控制器注销 UIExtension handle。
  - `W_DefaultHUD` 和 `W_PoseLibrary` 均通过 `WidgetService.validate`，编译状态均为 `BS_UP_TO_DATE`。

待完成：
- PIE 验收 Host/Client 双端可见性。
- 单人 PIE 按键打开姿势库、点击姿势播放并恢复 locomotion。
- 手柄 `Gamepad_DPad_Up` 打开姿势库。
- UI 关闭和返回走 CommonUI，不破坏角色移动输入恢复。

风险：
- `WidgetService.capture_preview` 是离屏预览，不会完整模拟游戏场景背景和 `BP_OnActivated` 动态填充列表；最终视觉仍需在 PIE 或独立运行中对照概念图右下角验收。
- 如果后续加入男主姿势，需要为男主 DataAsset 条目配置男主 AnimBP 中真实存在的 SlotName，或先给 `BP_ShootAnimInstance_M` 添加对应 Slot 节点。
- 当前扫描到的是女角色动画，男主是否能复用要看骨架兼容性；不兼容时需要单独男主姿势条目。
- 当前 Windows 编辑器已通过基础 PIE 启停 smoke，但资产级验收和 PIE 启停不能替代真实 Host/Client 运行态验收；不要把本任务标为 completed。

相关 Commit：
- 以 Git 历史中的姿势库 Slot 对齐提交为准。
