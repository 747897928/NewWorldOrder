# 姿势库检查清单

最后更新：2026-07-24

# 2026-07-24 GameMenu 迁移

- [x] 更新 UIExtension Point Tag 完整生命周期笔记。
- [x] 删除姿势库 UIExtension C++ 注册、handle、Tag 和模块依赖。
- [x] 删除 `W_DefaultHUD.ExtensionPoint_PoseLibrary`。
- [x] 删除 `W_PoseLibrary.CloseButton` 和图表链路。
- [x] 删除 `W_PoseLibrary`，避免保留横向条与重复列表生命周期。
- [x] 在 `WBP_GameMenu.PoseLibraryLyraScrollBox` 直接创建 `PoseLibraryUniformGrid`，并用蓝图按三列生成卡片。
- [x] 删除 `IA_PoseLibrary` 和 `IMC_Default` 中的映射。
- [x] 复核 `IMC_FrontEnd` 未添加姿势库专用输入。
- [x] Widget 编译、层级校验与 PIE 启停 smoke。
- [ ] 单人/双人实际交互验收。

# 需求阶段

- [x] 阅读 AGENTS.md。
- [x] 阅读 Implementation 会话指南。
- [x] 阅读 Tech_Constraints。
- [x] 阅读 SessionChecklist。
- [x] 扫描 CharacterSwitching 任务包，确认当前输入与网络分层。
- [x] 使用 VibeUE 扫描 `/Game/Assets/Animations/Girl`。
- [x] 使用 VibeUE 扫描 `/Game/Blueprints/Input`。

# 实现阶段

- [x] 新增姿势库 DataAsset C++ 类型。
- [x] 新增姿势库输入动作属性。
- [x] 新增 PlayerController 打开姿势库 UI 蓝图事件。
- [x] 新增服务器姿势播放请求。
- [x] 新增角色多播播放动态 Montage。
- [x] 新增 `IA_PoseLibrary`。
- [x] 更新 `IMC_Default` 映射键盘和手柄。
- [x] C++ 编译通过。
- [x] 创建姿势库 DataAsset。
- [x] 设置 `BP_ShootCharacter.OpenPoseLibraryAction`。
- [x] 设置 `BP_ShootPlayerController.PoseLibrary`。
- [x] 增加 `BP_ShootPlayerController.PoseLibraryWidgetClass` 配置入口。
- [x] 实现姿势库 Widget。
- [x] 设置 `BP_ShootPlayerController.PoseLibraryWidgetClass = W_PoseLibrary`。
- [x] 检查女主 AnimBP Slot 节点，并把 `DA_PoseLibrary` SlotName 对齐到 `UpperBodyAdditive`。
- [x] 确认当前 `DA_PoseLibrary` 条目均为 `FEMALE`，男主暂无姿势条目。

# 验收阶段

- [x] 2026-07-03 VibeUE/MCP 资产级验收：当时 `DA_PoseLibrary` 有 20 个条目，均有动画，性别均为 `FEMALE`，Slot 均为 `UpperBodyAdditive`；2026-07-07 已补齐为 26 条并配置图标。
- [x] VibeUE/MCP 资产级验收：`BP_ShootCharacter.OpenPoseLibraryAction = IA_PoseLibrary`。
- [x] VibeUE/MCP 资产级验收：`BP_ShootPlayerController.PoseLibrary = DA_PoseLibrary`。
- [x] VibeUE/MCP 资产级验收：`BP_ShootPlayerController.PoseLibraryWidgetClass = W_PoseLibrary`，`PoseLibraryLayerTag = UI.Layer.Menu`。
- [x] VibeUE/MCP 资产级验收：`IMC_Default` 中 `IA_PoseLibrary` 同时映射键盘 `P` 和手柄 `Gamepad_DPad_Up`。
- [x] VibeUE/MCP 资产级验收：`W_PoseLibrary` 父类为 `ULyraActivatableWidget`，控件树包含 `PoseListPanel`。
- [x] VibeUE/MCP 资产级验收：`W_PoseLibraryEntry` 父类为 `ULyraButtonBase`，生成类存在 `PoseIndex` 与 `PoseDisplayName`。
- [x] VibeUE/MCP 资产级验收：相关蓝图状态均为 `BS_UP_TO_DATE`。
- [x] UE 重启后补做非 PIE 验收：DataAsset、输入映射、控制器配置、角色输入动作、条目变量检查错误数为 0。
- [x] UE 重启后补做非 PIE 验收：VibeUE `BlueprintService.get_graph_definition` 确认 `W_PoseLibrary` 和 `W_PoseLibraryEntry` 主链路连接完整。
- [x] macOS `NewWorldOrderEditor Mac Development` C++ 编译通过。
- [x] 修复 `W_PoseLibraryEntry` 无本地 root 的问题：新增 `ButtonTextBlock`，并实现 `UpdateButtonText(InText) -> ButtonTextBlock.SetText(InText)`。
- [x] 修复后 `W_PoseLibraryEntry` 编译状态为 `BS_UP_TO_DATE`，`WidgetService.validate` 通过。
- [x] 修复后 macOS `NewWorldOrderEditor Mac Development` C++ 编译通过。
- [x] 2026-07-06 视觉补强：`W_PoseLibrary` 改为右下角 Canvas/Overlay 半透明深色面板，`PoseListPanel` 改为横向 ScrollBox。
- [x] 2026-07-06 视觉补强：`W_PoseLibraryEntry` 改为固定尺寸姿势卡片，保留 `ButtonTextBlock` 变量和点击图表。
- [x] 2026-07-06 非 PIE 资产级验收：`WidgetService.validate` 检查 `W_PoseLibrary` 与 `W_PoseLibraryEntry` 均通过。
- [x] 2026-07-06 非 PIE 预览：生成 `Saved/WidgetPreviews/W_PoseLibrary.png` 与 `Saved/WidgetPreviews/W_PoseLibraryEntry.png`，确认面板锚点、标题、提示与卡片层级。
- [x] 2026-07-07 资产扩展复核：参考 `DA_WardrobeCatalog` 和 `BP_Item_Wardrobe_Female_BobHair`，确认姿势库新增条目应继续走 `DA_PoseLibrary.PoseEntries`，不需要新增 C++。
- [x] 2026-07-07 UI 可用性修正：删除 `PoseHintText`，不再显示 AI 生图误导出的 `Q E / Enter` 操作提示。
- [x] 2026-07-07 UI 可用性修正：新增 `CloseButton` / `CloseButtonText`，并把 `CloseButton.OnClicked` 接到 `DeactivateWidget()`。
- [x] 2026-07-07 Lyra 学习落地：通过 Lyra MCP 读取 `W_ShooterHUDLayout`、`LAS_ShooterGame_StandardHUD` 和 `GameFeatureAction_AddWidgets` 相关代码，新增 `Docs/Engineering/Notes/UI/UIExtension_HUD插槽学习笔记.md`。
- [x] 2026-07-07 非 PIE 图表清理：删除 `W_PoseLibrary` 中空的 `Event PreConstruct`、`Event Construct`、`Event Tick` 节点，保留 `BP_OnActivated` 和 `CloseButton.OnClicked` 主链路。
- [x] 2026-07-07 Windows PIE smoke：通过 `EditorToolset.EditorAppToolset.StartPIE` 进入当前地图 PIE，warmup 3 秒后 `StopPIE` 成功退出。
- [x] 2026-07-07 数据补全：`/Game/Assets/Animations/Girl` 下 26 个 AnimSequence 已全部加入 `/Game/Blueprints/Animations/DA_PoseLibrary`。
- [x] 2026-07-07 资产整理修正：`DA_PoseLibrary` 已放到 `/Game/Blueprints/Animations/DA_PoseLibrary`，`BP_ShootPlayerController.PoseLibrary` 已指向新路径。
- [x] 2026-07-07 图标补全：为 26 个姿势导入 `/Game/Assets/Animations/PoseIcons/T_PoseIcon_*`，并填入每个 `PoseEntries[*].Icon`。
- [x] 2026-07-07 UI 图标接线：`W_PoseLibrary` 将 `FShootPoseLibraryEntry.Icon` 传给 `W_PoseLibraryEntry.PoseIcon`，`W_PoseLibraryEntry` Construct 调用 `PoseIconImage.SetBrushFromTexture(PoseIcon)`。
- [x] 2026-07-07 UI 名称修正：`W_PoseLibraryEntry` 的 `PoseIconImage` 预留底部空间，`ButtonTextBlock` 底部显示姿势名称，旧 `PoseGlyphText` 占位字已隐藏。
- [x] 2026-07-07 工具沉淀：新增 `Scripts/Generate_PoseLibraryIcons.ps1` 和 `Docs/Tasks/GestureLibrary/IconGeneration_图标生成.md`，记录图标生成方法。
- [x] 2026-07-08 架构修正：`AShootPlayerController::OpenPoseLibrary` 改为注册 `HUD.Slot.PoseLibrary`，不再把 `W_PoseLibrary` 推到 `UI.Layer.Menu`。
- [x] 2026-07-08 架构修正：`W_DefaultHUD` 根节点已从 Overlay 包装为 CanvasPanel，原 HUD Overlay 保留为全屏内容层。
- [x] 2026-07-08 架构修正：`W_PoseLibrary` 已去掉根 CanvasPanel，改为内容面板根 `Overlay_0`。
- [x] 2026-07-08 Lyra 重新复核：`W_ShooterHUDLayout` 是 `SafeZone -> Overlay -> CanvasPanel`，HUD 插槽放在 layout 的 CanvasPanel 或局部容器内，不放在被注册进去的内容 Widget 自己内部。
- [x] 2026-07-08 Lyra 重新复核：`LAS_ShooterGame_StandardHUD.actions[0].layout` 把 `W_ShooterHUDLayout_C` 推到 `UI.Layer.Game`，`widgets` 数组按 `widgetClass + slotId` 注册到 `HUD.Slot.*`。
- [x] 2026-07-08 插件修复：修复 `Plugins/UIExtension/UIExtension.uplugin` 重复 `Plugins` 字段，`CommonUI` 和 `CommonGame` 已在同一依赖数组。
- [x] 2026-07-08 编译验证：Windows `Scripts/Build_Windows.ps1` 通过。
- [x] 2026-07-08 用户重启 NewWorldOrder 编辑器后，MCP 确认 `/Script/UIExtension.UIExtensionPointWidget` 和 `/Script/UIExtension.UIExtensionSubsystem` 可加载。
- [x] 2026-07-08 已在 `W_DefaultHUD.CanvasPanel_0` 右下角添加 `ExtensionPoint_PoseLibrary`，控件类为 `UUIExtensionPointWidget`，`ExtensionPointTag = HUD.Slot.PoseLibrary`，`ExtensionPointTagMatch = ExactMatch`。
- [x] 2026-07-08 已设置 `ExtensionPoint_PoseLibrary` 的 CanvasPanelSlot：anchors 1,1，alignment 1,1，offsets -40,-40,760,220，zOrder 50。
- [x] 2026-07-08 已补 `W_PoseLibrary.Event Construct -> Clear Children`，UIExtension 创建路径也会生成姿势列表。
- [x] 2026-07-08 已把 `CloseButton.OnClicked` 改为 `GetOwningPlayer -> Cast To ShootPlayerController -> ClosePoseLibrary()`。
- [x] 2026-07-08 `W_DefaultHUD` 与 `W_PoseLibrary` 均通过 `WidgetService.validate`，编译状态均为 `BS_UP_TO_DATE`。
- [ ] 单人 PIE 按键能打开姿势库。
- [ ] 单人 PIE 点击姿势能播放并恢复 locomotion。
- [ ] Listen Server 双人 PIE 中 Host 播放姿势，Client 能看见。
- [ ] Listen Server 双人 PIE 中 Client 播放姿势，Host 能看见。
- [ ] 手柄 `Gamepad_DPad_Up` 能打开姿势库。
- [ ] UI 关闭和返回走 CommonUI，不破坏角色移动输入恢复。

# 当前阻塞

- [ ] Windows 环境已完成基础 PIE 启停 smoke，但还没有自动完成按键打开、点击播放、手柄打开、CommonUI 返回和 Listen Server 双端可见性验收；以上未勾选项仍需要可交互运行态验收。
