# 项目快速指南（AGENTS）

Unreal Engine 5 项目 NewWorldOrder。

## 环境判定

- 先判断会话环境：Windows（Codex）用 PowerShell、`$env:OS`；WSL/Linux（deepseek-harness）用 bash；macOS 用对应 shell。不要默认某一种。
- 路径一律用相对项目根目录（如 `Docs/...`），不写盘符绝对路径。
- 仅允许对当前项目目录及其子目录内的文件执行写入、修改或删除，项目外文件只读。删除单个文件前，必须先用 `Resolve-Path` 获取完整绝对路径，并验证其以项目根目录开头，然后仅用该绝对路径执行删除，禁止通配符、`..`、递归参数、强制批量参数，也禁止跨 shell 拼接路径。若涉及多个文件或非空文件夹，则停止并请求用户手动处理。注意：路径构造错误可能导致灾难性后果，务必避免以反斜杠开头的驱动器路径或不完整片段，并警惕未转义的特殊字符；尽量使用支持回收站的删除方式（如 `-Recycle`），不可用时则仅在路径无误后执行。

### Windows 启动 Unreal Editor

- 重启编辑器时优先使用本文后文约定的项目启动脚本。手动启动只用作脚本不可用时的备选方案。
- 启动前必须检查已有 `UnrealEditor.exe` 进程的命令行，以 `.uproject` 完整路径识别具体项目，避免重复启动同一项目或误关其他 Unreal 项目。
- 项目路径包含空格。PowerShell 的 `Start-Process -ArgumentList` 会重新组装参数；不得直接传入未加引号的 `.uproject` 路径，否则编辑器会收到被拆分的参数并打开 Project Browser。手动启动时必须把项目路径作为带引号的单一参数：

```powershell
$projectFile = (Resolve-Path -LiteralPath '.\NewWorldOrder.uproject').Path
$projectArgument = '"' + $projectFile + '"'
Start-Process -FilePath $editorExecutable -ArgumentList $projectArgument
```

- 启动后必须核对目标进程的命令行、窗口标题或 VibeUE readiness signal 确实指向 `NewWorldOrder.uproject`。只看到 `UnrealEditor.exe` 进程不代表项目启动成功；出现 Recent Projects / Project Browser 即为启动失败。

### 远程仓库是唯一沟通渠道

所有产出必须推送远程仓库，未推送=失败。流程：git add . → git status → git commit → git push -u origin <branch> → 检查推送成功 → 向用户汇报 commit hash。

### 上游插件只读

Lyra 或第三方 Plugins/ 源码和Unreal Engine源码默认只读。项目适配放 Source/NewWorldOrder 子类或包装层。只有用户明确同意才能改，且必须标注定制原因。

### 多输入设备支持（CommonUI + Enhanced Input）（2026-03）

项目使用 CommonUI + Enhanced Input 架构，天然支持键盘、鼠标、手柄、触摸等多种输入设备。

核心原则（AI 必须遵守）：

- 禁止在 C++ 中 hardcode 键盘/手柄分支判断（如 `if (Key == EKeys::E)`、`if (IsGamepad())` 切换逻辑）
- 输入映射统一通过 UInputAction + UInputMappingContext 配置，一个 IA 可同时绑定键盘和手柄键位
- CommonUI 的 UCommonButtonBase、RegisterUIActionBinding、FBindUIActionArgs 已内置跨设备支持
- 菜单 Widget 通过 `UCommonActivatableWidget::InputMapping` + `InputMappingPriority` 自动管理 IMC 叠加/移除，不需要手动 AddMappingContext/RemoveMappingContext

输入架构分层：

1. 游戏玩法输入（角色技能/武器，基于 GAS）：
   - 项目输入最终汇入 GAS（Gameplay Ability System），角色技能和武器操作统一走 ASC
   - AShootCharacter::SetupPlayerInputComponent
   - UShootInputComponent（继承 UEnhancedInputComponent）绑定 InputTag → 输入事件
   - UShootInputConfig 数据资产（InputTag → UInputAction 映射）
   - UShootAbilitySystemComponent::AbilityInputTagPressed/Released/Held 将输入事件分发给对应 GA
   - 流程：按键 → Enhanced Input → InputTag → ASC → GA 激活

2. 菜单/UI 输入：
   - ULyraActivatableWidget（继承 UCommonActivatableWidget）
   - Widget 激活时自动叠加 InputMapping，反激活时自动移除
   - RegisterUIActionBinding(FBindUIActionArgs) 监听 UI 动作
   - UCommonButtonBase 子类（ULyraButtonBase 等）处理按钮交互

3. 跨设备图标与提示（UCommonActionWidget / ULyraActionWidget）：
   - UCommonInputSubsystem 查询当前输入设备类型（键盘/手柄/触摸）
   - UCommonActionWidget：CommonUI 原生控件，根据当前输入设备自动切换显示对应按键图标（键盘显示"E"、手柄显示"X"按钮图标）
   - ULyraActionWidget（继承 UCommonActionWidget）：项目改进版，重写 GetIcon() 从 Enhanced Input 查询当前 IMC 中 IA 的实际绑定键位并获取对应图标刷，支持玩家自定义键位后的图标同步更新
   - DT_UniversalActions：以 Lyra 主表为基础的 DataTable 资产，为 CommonUI Input Action 行提供跨设备的图标刷和提示文本；项目缺失行只在此表中增量补充
   - 典型用法：交互提示 Widget（如 ShootInventoryGrantActor 上的 InteractionPromptWidgetClass）内嵌 ULyraActionWidget，绑定 IA_Interact，自动显示"按 E 拾取" / "按 X 拾取"等跨设备提示
   - ULyraButtonBase::OnInputMethodChanged 响应设备切换

关键类与文件：

- Source/NewWorldOrder/Public/Input/ShootInputComponent.h
- Source/NewWorldOrder/Public/Input/ShootInputConfig.h
- Source/NewWorldOrder/Public/UI/LyraActivatableWidget.h
- Source/NewWorldOrder/Public/UI/Foundation/LyraButtonBase.h
- Source/NewWorldOrder/Public/UI/Foundation/LyraActionWidget.h
- Docs/Engineering/Notes/UI/CommonUI_架构与用法.md
- Docs/Engineering/Notes/UI/RegisterUIActionBinding_学习笔记.md

### 阵营与敌我识别（ILyraTeamAgentInterface）（2026-03）

项目从 Lyra 复制了 ILyraTeamAgentInterface 作为阵营识别标准接口，禁止在 C++ 中 hardcode "IsPlayer" / "IsEnemy" 分支判断。

接口定义（Source/NewWorldOrder/Public/Teams/LyraTeamAgentInterface.h）：

- 继承自 IGenericTeamAgentInterface（UE 原生阵营接口）
- 添加 FOnLyraTeamIndexChangedDelegate 阵营变更广播
- 提供 ConditionalBroadcastTeamChanged 静态方法

当前阵营分配：

- 人类玩家（AShootPlayerState、ULyraLocalPlayer）：TeamId = 1
- 丧尸 AI（AEnemyBotController）：TeamId = 2

使用方式：

```cpp
// 判断目标是否友方——通过 ILyraTeamAgentInterface，不通过 IsPlayer()
if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Target->GetController()))
{
    ETeamAttitude::Type Attitude = TeamAgent->GetTeamAttitudeTowards(*this);
    if (Attitude == ETeamAttitude::Friendly) { /* ... */ }
}
```

AI 禁止直接 IsPlayer 判断的场合：

- 伤害过滤：通过 GetTeamAttitudeTowards 而非 Cast<AShootCharacter>
- 技能目标筛选：通过 ILyraTeamAgentInterface 查询阵营
- 交互效果：通过 Faction Tag（Faction.Player / Faction.Enemy）查表，禁止硬编码

实现 ILyraTeamAgentInterface 的类：

- AShootPlayerState（玩家）
- ULyraLocalPlayer（本地玩家）
- AEnemyBotController（AI）

### 类型转换约束（Cast）

- 仅在需要子类特有功能时才 Cast 到子类；共用行为请使用基类（如 ACharacter）指针，避免无谓开销。
- Cast 表达意图要清晰，避免误导维护者；频繁调用场景优先使用基类或接口判断。
- 修改旧代码时发现不必要的子类 Cast，应重构为基类访问，保留必要的 Cast 场景需有明确理由。

## UI 通信架构（MVVM + GameplayMessage + CommonUI）（2026-03）

项目 UI 层使用三种通信机制，各有适用场景：

1. UMVVMViewModelBase（FieldNotify 属性绑定）：
   - 适用于：本地 UI 需要自动刷新标量/轻量数据（血量、等级、属性值）
   - 用法：继承 UMVVMViewModelBase，用 UE_MVVM_SET_PROPERTY_VALUE 宏更新字段
   - UMG Widget 通过 FieldNotify 自动绑定，无需手动 Push 数据
   - 限制：TArray/TMap 复杂容器只支持整体刷新，不提供元素级通知
   - 示例：UAttributeViewModel（Source/NewWorldOrder/Public/UI/ViewModel/AttributeViewModel.h）

2. UGameplayMessageSubsystem（跨系统广播）：
   - 适用于：服务器 OnRep / 状态变更后广播给多个松耦合订阅者（UI、组件、其他系统）
   - 用法：`UGameplayMessageSubsystem::Get(this).BroadcastMessage(Tag, Message)`
   - 订阅：`UGameplayMessageSubsystem::Get(this).RegisterListener(Tag, Callback)`
   - 典型消息标签：Msg_Quickbar_SlotsChanged、Msg_Quickbar_ActiveIndexChanged、Msg_UI_Reticle_HitNotify、Inventory_Message_StackChanged、Inventory_Resource_Message_Changed
   - 推荐流程：服务器更新 → OnRep → 广播消息 → ViewModel 监听 → FieldNotify 更新 UMG

3. CommonUI 层级系统（Widget 生命周期管理）：
   - UPrimaryGameLayout：每个玩家的根布局，管理多个 GameplayTag 驱动的 UI Layer
   - PushContentToLayerForPlayer：异步/同步推送 Widget 到指定层级（UI.Layer.HUD / Menu / Modal / Game）
   - DeactivateWidget()：正确关闭 Widget（恢复输入配置、触发层级栈清理），禁止直接 RemoveFromParent
   - 继承链：UCommonActivatableWidget → ULyraActivatableWidget → 具体 Widget 基类
   - 本项目计划支持本地多人分屏。菜单、HUD、衣柜、角色切换等玩家私有 UI 必须通过目标 LocalPlayer 对应的 UPrimaryGameLayout 和 PushContentToLayerForPlayer 推入对应层级，禁止用 AddToViewport 创建全局 UI。
   - AddToPlayerScreen 只允许用于已经明确绑定到目标 OwningPlayer、且不会进入 CommonUI 层级栈的临时玩家私有浮层。使用前必须确认本地分屏下不会显示到其他玩家屏幕、不会抢错输入焦点、不会绕过 CommonUI 的输入配置恢复。衣柜、主菜单、暂停菜单、角色切换菜单等页面不走 AddToPlayerScreen。

4. UIExtension HUD 插槽系统（Lyra 模式，2026-07）：
   - 稳定 HUD 片段（准星、快捷栏、资源提示、触摸按钮、模式状态等）优先通过 UUIExtensionPointWidget 暴露 `HUD.Slot.*` 插槽，再用 UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext 按 LocalPlayer 注册 Widget。
   - HUD layout 自身仍通过 CommonUI 推到 `UI.Layer.Game`；具体 HUD 片段不要为了扩展性反复改主 HUD 蓝图。
   - 菜单、衣柜、角色切换、姿势选择这类需要焦点、滚动、确认或返回的页面，继续走 CommonUI 层级栈，并提供可见关闭入口调用 DeactivateWidget()。
   - 新增目录条目、姿势条目、HUD 片段等可配置内容，优先落到 DataAsset / Widget 蓝图 / GameplayTag 插槽配置，不要写进 C++ 构造函数或硬编码数组。
   - 相关笔记：Docs/Engineering/Notes/UI/UIExtension_HUD插槽学习笔记.md

UI 组件归属原则：

- UI 组件（如 UShootHUDReticleComponent）应附加在 APlayerController 上，不是 ACharacter 上
- Character 可能被销毁/重新生成，PlayerController 在玩家会话期间持久存在
- 涉及本地分屏时，任何 UI 创建入口都必须显式说明目标 PlayerController / LocalPlayer。不要用 GetFirstPlayerController、GetPlayerController(0) 或无 OwningPlayer 的 CreateWidget 偷懒，否则第二个本地玩家会看到、操作或关闭错误的 UI。

UMG 视觉状态复用规则：

- 做按钮、格子、槽位选中态时，先参考项目已有 Lyra/CommonUI 资产的控件树、材质参数和 Widget Animation，例如 `/Game/UI/Weapon/WBP_ActionTouchButton`、`/Game/UI/Weapon/WBP_WeaponSlot`。
- 页面内部自适应布局优先使用 `HorizontalBox` / `VerticalBox` 的 Slot `Fill Value` 和 `Spacer`；`CanvasPanel` / `Overlay` 用于顶层锚点、覆盖层和大区域分组。
- 布局、导航、显隐、样式等凡是 Widget 蓝图 Details 面板能配置的，一律放蓝图配置；只有动态创建控件、蓝图无法跨 UserWidget 解析目标等少数运行时桥接才允许 C++，且必须在注释写清蓝图为何做不到。
- UI 颜色、尺寸、背景、装饰层、兜底视觉和布局默认值优先放在控件蓝图或预览 Actor 蓝图；C++ 只处理数据、状态、输入绑定和必要的显隐切换。
- 固定按钮的文案、图标、尺寸、位置和可见样式由 Widget 蓝图维护。C++ 只给运行时动态生成的按钮写数据文本，或绑定必须进入 C++ 业务逻辑的动作。
- 固定页面按钮的 OnClicked 默认保留在 Widget Blueprint EventGraph，并调用语义明确的 BlueprintCallable 业务函数。C++ 新增控件绑定前先判断：控件是文档化的稳定结构、且显式绑定能明显提升代码可读性，才用 BlueprintReadOnly + BindWidgetOptional；否则优先蓝图配置或运行时查找。绑定是少数例外，不是随手写的默认。
- 蓝图里难以发现的设置（隐藏按钮接 IA、特殊 Visibility、导航配置等）在 EventGraph 放注释节点说明用途；C++ 侧注释随代码一起更新，不让注释变成旧描述。
- 业务状态由 C++/ViewModel/消息驱动，UMG EventGraph 只保留必要的视觉转场。禁止为了兼容旧蓝图图表，在 C++ 增加库存、Mutable、角色 Cast 或旧 DataTable 兜底路径。
- 确认不再需要的 UI 控件、旧图表、兼容分支必须删除；不要用 `Collapsed`、`Hidden` 或空函数把废弃路径留在资产或 C++ 里。只有仍有明确运行时切换需求的控件才允许折叠保留。
- 对列表格子这类可复用控件，优先用 `InactiveToActive`、`ActiveToInactive`、材质参数或同类动画表达 hover/focus/selected；不要在子 Widget 图表里直接发装备 RPC 或修改角色外观。

- 改代码前先查需求文档、任务包文档、当前代码调用链
- 修改复杂系统前先阅读对应 Requirements/Overview/STATUS，搜索调用点，判断主线/兼容/遗留/候删
- 删除旧代码前确认无蓝图引用、无系统依赖、已有替代链路
- 任务完成后：更新文档 → git 提交推送 → 向用户汇报

## 通用规范

### 输出格式

仅保留结构性符号，只用标题、列表、代码块。禁止任何装饰符号，禁止粗体、斜体、下划线、Emoji表情符号。

### 文档维护

- 中文文档只能用安全的 UTF-8 方式修改。允许 apply_patch 或明确指定 UTF-8 的文件读写。禁止未经验证的批量重写链路处理中文文档，避免把中文写成 `?`。

- 修改文档前，必须先完整阅读目标文档和关联文档，再决定如何修改。

- 修改文档时，必须先判断三类内容：哪些应保留、哪些应局部更新、哪些才应该完全覆盖。

- 看到"必须遵守""权威架构""相关必读"这类章节时，默认保留；只有在代码事实和需求文档都明确证明其错误时才能删除或重写。不允许因"状态过时"就整页覆盖仍然有效的架构约束。

- 文档归属：通用规范写 AGENTS.md 或 Docs/Engineering；任务相关结论写 Docs/Tasks/<Task>/ 或对应 DevelopmentNotes；禁止把任务细节混进 AGENTS.md 的通用规则。

- 文档修改完成后，必须检查文件是否正常显示中文，不能出现 `?`、乱码或整页语义丢失。

- 说明代码或蓝图属性时，必须写清属性定义在哪个类。如果属性来自父类，必须明确写成"某类继承自某父类，使用的是某父类上的某属性"。

- 写代码、文档、注释时，站在三类接手者视角审视：用户验收先看文档和蓝图配置步骤，不会逐行审代码；后续维护者先看注释和文档再深入实现；其他 AI 上下文丢失后只能靠仓库里的注释和文档恢复思路。

  判断标准：凡是你觉得"这件事不交代，别人接手时可能不知道"的隐性知识，必须主动留下注解。不要面面俱到的流水账，要捕捉反直觉的、隐式的、跨文件/跨资产的心智负担点。例如蓝图中覆盖了 C++ 的 EditDefaultsOnly 值、EventGraph 里藏着影响流程的隐藏逻辑、某段逻辑依赖父类已有属性或编辑器资产配置。这类信息无法靠读单一文件发现，不写注解，接手者会误以为功能未实现，另起炉灶，最终两套冲突实现并存，代码不可用。

  接手者发现前人代码有错误时可以纠正，但必须先了解前人做了什么：读懂设计意图、检查蓝图配置和 C++ 注解，确认真的写错后再改。修改时同样遵守本规则，留下注解说明纠正了什么、为什么。AI 会犯错，前人的工作不一定对，但了解清楚前人的工作，是判断对错的前提。

  遇到已有注解时，把它当成线索去核实，不要直接忽略或盲目信任。注解与代码不一致时，先深入代码弄清实际行为，判断是代码错了还是注解过时了。确定注解过时，修正注解，与本次代码修改放在同一个 commit。无法确定时，带上代码和注解的证据询问用户，让用户介入，不要擅自处理。

  上述隐性知识场景，必须同时满足：代码里有中文注释直接写清楚、文档里有操作说明、命名和描述不带歧义。

- 注释用中文，禁止"中文注解："这类无信息量前缀，直接进入主题。

### 需求与既有实现审查规则

- 改代码前，先查需求文档，再查任务包文档，再查当前代码调用链。先理解"为什么这样写"，再决定"要不要改"。不允许在未审查需求来源的情况下，凭猜测重构复杂系统。

- 修改复杂系统（角色切换、技能、库存、武器、存档）前，至少完成：阅读对应 Requirements / Overview / STATUS；搜索调用点和引用点；判断当前代码属于主线、兼容、遗留还是候删。

- 删除旧代码前，必须确认：没有蓝图引用、没有别的系统依赖、已有替代链路。

- 不确定时，先收集证据并整理结论，再一次性找用户确认；不要遇到一点疑问就反复打断用户。询问用户时，必须带上已核对的需求来源、代码证据和风险点，减少用户决策成本。

### 命名规范

文件名：英文_中文.md（如 Overview_总览.md）。文件夹只英文。

### 编译

- 引擎版本以 NewWorldOrder.uproject 的 EngineAssociation 为准
- Windows 编译优先用：`powershell -ExecutionPolicy Bypass -File .\Scripts\Build_Windows.ps1`
- WSL/macOS 用对应 shell 和 Build.sh
- 编译失败记录错误日志，修复后再次验证

## 知识资产管理

- 每次会话结束前必须 git commit && git push
- 文档保持最新
- 单一数据源：当前实现与任务包优先（Docs/Tasks/<System>/STATUS）；Docs/SystemDesign 为设计稿，数值可能过时
- 新会话只需读本文件和相关 Docs/

## 领域架构规则

### 库存与武器架构强制规则（2025-11-15）

- PlayerState 上的 UResourceInventoryComponent 只负责可堆叠资源（材料/货币/徽章/设计图），所有数据默认 Persistent，需要参与 SaveGame；战斗内临时回血包等即时效果请绕过该组件，直接通过能力或临时系统处理。
- PlayerState 上的 UShootInventoryManagerComponent 管理有身份的物品/武器实例，QuickBar、EquipmentManager、武器/装备 GA 只能与该组件打交道，不允许直接从 SaveGame 或蓝图数据表创建武器实例。
- Inventory ItemInstance 必须带 EShootItemLifetime（Persistent / RuntimeOnly）枚举，提供 AddPersistentItem / AddRuntimeItem 两套接口；副本内拾取必须走 RuntimeOnly，退出副本或返回 Hub 时统一清空，且绝不写入 SaveGame 或 QuickBar 配置。
- SaveGame 序列化时仅保存 Lifetime=Persistent 的 ItemInstance 和 QuickBar 绑定；RuntimeOnly 物品只存在于当前副本，不进入账号仓库。
- 进入副本时用 SaveGame 中的 QuickBar 配置从 InventoryManager 取出 Persistent 物品填充槽位；副本内拾取替换当前武器时不得修改账号 QuickBar，只更新战斗期实例引用。
- 编写库存/武器相关 C++ 代码时必须添加中文注释，说明本文件如何区分"ResourceInventory = 数量型仓库""InventoryManager = 有身份背包""QuickBar/Equipment 仅引用 InventoryManager"，保证 AI/同事读代码即可理解架构。
- HUD/QuickBar/UI 只能通过 QuickBar 消息或 `UShootWeaponInstance` 读取武器数据；禁止直接引用 `ARangedWeaponInstance/AHitscanWeaponInstance` 获取弹药或展示数值，Actor 仅作为表现壳。

### 主角技能与属性数据生命周期（2026-08-23 架构调整）

- 技能（男主/女主主动+被动、手雷等)属于副本数据：进入副本由 GameMode 的 Experience 数据（AbilitySet DA）授予，出副本/Experience 卸载时取回；存档不持有技能。
- 属性（Strength/Vitality/Agility/Perception 及派生 MaxHealth 等）同样属于副本数据：每次进副本统一默认初始化（AShootCharacterBase::InitializeDefaultAttributes，GE 由角色蓝图配置）；回合制副本由 GameMode::InitializeRoundForAll 重置玩家与敌人属性并重授技能套件。
- 存档仅保留账号级战利品：Persistent 物品、资源（材料/货币等）、双主角外观与 QuickBar；等级/XP/属性点/技能字段已从存储结构移除（SaveVersion=3）。
- 角色切换（男/女）只切换外观/装备快照，不再即时移除/授予技能；技能授予与取回由 Experience 生命周期管理。副本内是否允许切换由 UI 控制，C++ 不做“是否副本”硬判断。

### 蓝图、Data Asset 与 C++ 的分工（MCP 时代，2026-06）

UE 5.8以后 MCP 让 AI 可以直接创建和编辑蓝图资产。以下规则取代旧版"C++优先"惯例，明确三种资产类型的职责边界。

三类资产的职责：

1. C++ 类（游戏逻辑，不可替代）：
   - 网络复制、GAS 能力激活、武器射击/换弹、伤害计算
   - 组件创建（CreateDefaultSubobject）
   - 接口定义、委托声明
   - BlueprintNativeEvent / BlueprintImplementableEvent 的函数声明
   - 规则：C++ 构造函数只做组件创建和默认值设定。禁止在构造函数中加载资源（ConstructorHelpers::FObjectFinder）、禁止硬编码数据数组（如 AddUnique 循环）
   - 复杂逻辑

2. 蓝图子类（配置与调优，通过 MCP 创建）：
   - 视觉参数调优（灯光位置/强度、网格缩放、材质颜色、UI 尺寸）等
   - 引用具体资产（StaticMesh、Material、SkeletalMesh、AnimBP）
   - 覆盖 C++ 暴露的 EditDefaultsOnly 属性
   - Widget 的 BindWidget 绑定和样式调整
   - 规则：AI 在写 C++ 之前，先用 MCP 的 search_subclasses 检查是否已有蓝图子类。如果某项参数需要频繁调整或所见即所得编辑（灯光、旋转、颜色、UI 布局），暴露为 EditDefaultsOnly 并在蓝图子类中设置，不要硬编码在 C++ 构造函数

3. Data Asset（目录与配置表，通过 MCP 或 C++ 创建）：
   - 物品目录（武器列表、服装列表、技能列表）
   - GameplayTag 到属性的映射表
   - GameplayEffect 的 Modifier 数组
   - 规则：任何 AddUnique/Add/Push 超过 3 条的数据列表，创建 Data Asset 或暴露 EditDefaultsOnly 的 TArray，不要在 C++ 构造函数中逐条硬编码

MCP 工作流（AI 写代码前先执行）：

```
1. search_subclasses(base_class) → 找现有蓝图子类
2. 如果有蓝图子类 → get_default_object(blueprint) → 查看已有配置
3. 如果没蓝图子类但需要配置 → BlueprintTools.create() → 创建蓝图子类
4. 配置数据 → DataAssetTools.create() → 创建 Data Asset
```

UPROPERTY 规范（强制）：

- 需要在蓝图中调整的参数 → EditDefaultsOnly 或 EditAnywhere
- 不推荐使用 ConstructorHelpers::FObjectFinder 加载资源 → 改为 EditDefaultsOnly 的 TObjectPtr，在蓝图子类中设置，然后你可以在c++的构造方法留下注解，告知其他协同者你在蓝图子类设置或者覆盖了这个值。
- 材质、网格等资源引用统一用 TObjectPtr<UMaterialInterface> 等蓝图可设类型

反模式清单（AI 禁止）：

- C++ 构造函数中写 AddUnique 循环填充物品目录
- C++ 构造函数中设置灯光位置/强度/颜色等视觉参数
- C++ 中硬编码 UI 尺寸、颜色、偏移量
- 创建 NotBlueprintable 的调试数据类用作目录条目
- 从 C++ 引用 /Game/ 下的具体资产路径

### 蓝图可读调用链（2026-07）

- UI 表现、布局、固定控件和可直观表达的数据装配优先保留在 Widget Blueprint；不要只为减少连线把简单流程搬进 C++。
- 复杂、网络权威或跨系统逻辑可放 C++，但蓝图必须保留语义明确的入口节点调用 BlueprintCallable / NativeEvent；关键入口、状态和蓝图配置依赖须有中文注释说明调用方向。
- 只删除已确认废弃的控件、图表和兼容逻辑；不以 Collapsed、Hidden 或空函数伪装废弃路径。
- 不得把自动布局当成修改蓝图后的固定步骤。已有布局清晰且节点没有重叠时，保持用户原布局，不得为了风格统一或“顺手整理”制造无关资产差异。
- 只有本次新增或修改的相关节点出现重叠、连线明显难以阅读时，才使用 `Plugins/BlueprintAutoLayout`（编辑器显示名 `Blueprint Anti-Pasta`）。先编译确认图表语义正确，只选中本次改动影响的节点后使用 `Ctrl+Shift+K`，整理后再次编译并复查连线。
- 修改 `FunctionA` 时禁止格式化 `FunctionB`、其他 EventGraph、Macro 或未触及的节点区域。只有本次任务确实重构了整张当前图，且整图存在重叠或严重不可读时，才允许使用 `Ctrl+Shift+L`；提交前必须确认布局差异没有扩散到任务范围之外。
- 默认只使用普通 `Auto Layout Selected/Graph`。`Group` 和 `Route Wires` 模式会新增注释框或 reroute 节点，只有任务明确需要且新增节点属于本次范围时才使用；自动布局只负责可读性，不能替代编译、调用链检查和运行验证。
- 该插件只处理 Blueprint/Animation Blueprint 的 K2 图，不适用于 Material、Niagara、Behavior Tree；这些图继续使用各自编辑器或 MCP 服务整理。

### 交互系统

- 玩家交互与 AI 交互分离：UShootGA_Interact 只授予 PlayerControlled 角色；AI 用 AutoOverlap 或脚本/行为树 GA
- 交互效果数据驱动：根据 Instigator 阵营 Tag 查表应用 GE，禁止硬编码 if(IsPlayer)
- 拾取必须分流：资源走 UResourceInventoryComponent，Persistent 物品走 AddPersistentItem，副本临时武器走 AddRuntimeItem
- AShoootResourcePickup 的 AutoOverlap 与交互 GA 并存，通过 TriggerMode 切换

## 本地化语言策略（2026-09）

- 开发源文案统一使用英文；Game 本地化目标 `NativeCulture=en`，稳定 namespace/key 不随翻译改变。
- 首启与发行优先识别 Steam 应用语言，再识别系统语言；匹配 11 个 Culture（`zh-Hans`、`en`、`ru`、`es`、`pt-BR`、`de`、`ja`、`fr`、`pl`、`ko`、`zh-Hant`）时使用对应语言，否则回退 `zh-Hans`。
- `zh-Hans` 是首启回退默认显示语言；设置页明确 Apply 的 Culture 写入 `GameUserSettings.ini`，`System Default` 清除覆盖并重新执行自动识别。
- UMG 与代码固定用户文案使用稳定可 Gather 的 `FText`、`LOCTEXT` 或 `NSLOCTEXT`；新增文案先对齐英文源文本，再更新 11 个 PO 和 `Game.locres`。

## 文档索引

- Docs/DevelopmentNotes/ - 开发笔记（含 MCP 操作手册、踩坑记录）
- Docs/Tasks/ - 任务包（自包含实现指南）

涉及 MCP 操作蓝图/DataAsset/UMG 前，必须先读 Docs/DevelopmentNotes/MCP_踩坑记录.md。

最后更新：2026-09-06

<!-- BEGIN VibeUE (v5.0) — generated by VibeUE.GenerateAgentConfig; re-run to refresh -->
# VibeUE — AI agent guide (Unreal Engine 5.8)

VibeUE **extends Unreal 5.8's native AI toolset system** — its services, tools, and skills register
into the engine's `ToolsetRegistry` and are reachable through the MCP tools you already have.

**ALWAYS use the MCP tools / Python API for Unreal operations — NEVER read `.uasset` files from disk.**

---

## 1. The efficient interaction model (read this first)

There are two ways to act on the editor. Pick the cheap one:

- **`execute_python_code` — your workhorse.** Runs an arbitrary Python script in the editor in **one
  round-trip**. Every VibeUE service is exposed to Python (`unreal.BlueprintService.build_graph(...)`)
  and sits next to the whole native `unreal.*` API in the same script. **Batch aggressively** — do a
  whole multi-step task (create + edit + compile + verify) in a single call, and `print()` only what
  you need back.
- **`call_tool` — one tool per round-trip.** Genuinely needed only for **skills**
  (`AgentSkillToolset`). **Everything else from Epic's engine toolsets is also reachable from
  inside `execute_python_code`** via `unreal.ToolsetRegistry.execute_tool(...)` (see §2), so batch
  engine-toolset calls with your Python instead of spending a round-trip. Don't use `call_tool`
  for work `execute_python_code` can batch.
- **`capture_image` — screenshots you can actually SEE.** Returns a real MCP image block the
  client renders (never a base64 text blob). `source="game"` captures the PIE viewport
  **including the Slate/UMG HUD** — the thing `CaptureViewport` cannot see (see §6).

**Speed + tokens:** **avoid `describe_toolset` as a habit** — it dumps the full JSON schema of every
tool in a toolset (the most token-heavy thing here); reach for a **skill** plus a narrow
`discover_python_class('unreal.BlueprintService', method_filter='variable')` instead.

---

## 2. Tool roster — what's where

**VibeUE MCP tools (call directly):**
- `execute_python_code` — run Python (must start with `import unreal`). The workhorse.
- `discover_python_module` / `discover_python_class` / `discover_python_function` — inspect the API
  (use `unreal` lowercase; narrow with `name_filter` / `method_filter`).
- `list_python_subsystems` — list editor subsystems.
- `capture_image` — screenshot as a real MCP image (see §6): `source` = `game` (PIE incl. UMG HUD),
  `window` (whole editor window), or `editor` (viewport scene). Also saves a PNG under
  `Saved/VibeUE/Captures` and returns the path.
- `deep_research` — web search / page fetch / geocode (see §5).
- `terrain_data` — real-world heightmaps + water splines (see §5).

**VibeUE services (call from Python inside `execute_python_code`):** `unreal.<Name>Service.<method>()`
— Blueprint, BlueprintGraph (via BlueprintService), Material(+Node), Widget, Skeleton, AnimSequence,
AnimMontage, AnimGraph, Landscape(+Material), Foliage, MetaSound, SoundCue, Niagara(+Emitter,
+ScratchPad), StateTree, BehaviorTree, Blackboard, Input, EnumStruct, UVMapping,
RuntimeVirtualTexture, MapBlockout, GameplayTag, Viewport, Actor, Engine/ProjectSettings,
**Performance** (`unreal.PerformanceService.frame_timing()`).
These overlap-trimmed services keep only what the engine lacks — for plain asset/actor/blueprint
basics the engine's own tools may be simpler (below).

**Calling Epic's engine toolsets from Python (`execute_tool`).** Epic's engine toolset *classes*
exist as `unreal.*` (e.g. `unreal.EditorAppToolset`) but their AICallable functions are **not**
exposed as Python methods — `unreal.EditorAppToolset.get_selected_assets()` fails. Invoke them
through the registry instead (same dispatch `call_tool` uses, but in-process and batchable):
```python
import unreal, vibeue   # vibeue ships in the plugin's Content/Python — always importable

out = vibeue.exec_tool("EditorToolset.EditorAppToolset", "GetSelectedAssets")
# exec_tool fills missing optional params from the tool's schema (so bare StartPIE/CaptureViewport
# calls just work), reports ALL missing required params in ONE error, and returns fully-decoded
# Python values (the raw execute_tool "returnValue" is sometimes double-encoded JSON).
```
Discover schemas by NAME with `vibeue.get_toolset_schema("EditorToolset.EditorAppToolset")` /
`vibeue.list_toolset_names()`. (The engine's own
`unreal.ToolsetRegistry.get_toolset_json_schema(...)` takes a **class object** like
`unreal.EditorAppToolset`, NOT a name string; `get_all_toolset_json_schemas()` dumps everything —
token-heavy.) Names are namespaced — `EditorToolset.EditorAppToolset`,
`NiagaraToolsets.NiagaraToolset_System`, etc. (a bare `"EditorAppToolset"` returns "Toolset not
found"). Raw `unreal.ToolsetRegistry.execute_tool(name, tool, args_json)` remains available; it
returns an **async** result — the editor tools above complete synchronously (`is_complete=True`),
but for a long-running tool check `is_complete` / bind `on_completed` rather than assuming `value`
is ready.

**Use the engine's native tools for these (VibeUE intentionally doesn't duplicate them):**
- **Assets** (find / save / move / delete / duplicate / metadata): native Python
  `unreal.EditorAssetLibrary` / `EditorAssetSubsystem` inside `execute_python_code` (batchable), or
  Epic's `AssetTools` toolset (via `execute_tool` in-Python, or `call_tool`).
- **Screenshots / vision**: use VibeUE's **`capture_image` MCP tool** (see §6) — it is the only
  path that returns a renderable image. Epic's `CaptureViewport`/`CaptureEditorImage`/
  `CaptureAssetImage` return base64 **inside a text block** on every route (including `call_tool`),
  which you cannot view and which can blow the client token limit; reach for `CaptureViewport` only
  when you need its world-grid/actor-label annotations, via `vibeue.exec_tool` (which fills its
  mandatory param shape for you).
- **PIE**: `EditorAppToolset.StartPIE` / `StopPIE` / `IsPIERunning` — via `vibeue.exec_tool`
  (bare `StartPIE` works; the raw path demands the full options shape), or `call_tool`.
- **Logs**: `LogsToolset.GetLogEntries` — via `execute_tool` or `call_tool` (or read the `.log` file).
- **DataTables / DataAssets / enum-struct basics**: Epic's `DataTableTools` / `DataAssetTools` /
  `ObjectTools` (via `execute_tool` or `call_tool`). (VibeUE keeps only `EnumStructService` for
  create/edit of user enums & structs.)

---

## 3. Skills — native `AgentSkill` (lazy domain knowledge)

VibeUE's ~88 skill packs are registered as Unreal **AgentSkills** and served by the engine's
`AgentSkillToolset`. Skills tell you **what to do and why**; they do **not** replace discovery of exact
signatures.

**Discover + load (both are `call_tool` on `ToolsetRegistry.AgentSkillToolset`):**
```
call_tool(tool_name="ListSkills", toolset_name="ToolsetRegistry.AgentSkillToolset")
  → { "/VibeUE/Python/init_unreal_PY.VibeUE_blueprints": "Create and modify Blueprint assets…", … }

call_tool(tool_name="GetSkills", toolset_name="ToolsetRegistry.AgentSkillToolset",
          arguments={"skillPaths": ["/VibeUE/Python/init_unreal_PY.VibeUE_blueprints"]})
  → full markdown for that pack
```
- `ListSkills` returns **summaries only** (cheap) — call it once per session to see what exists. VibeUE
  packs are `/VibeUE/Python/init_unreal_PY.VibeUE_<name>`; the engine's own skills appear alongside.
- `GetSkills` returns full instructions **lazily** — request only the packs you need.
- **Sub-docs are their own skill entries** (e.g. `…VibeUE_blueprint_graphs__build_graph`,
  `…VibeUE_state_trees__api_reference`) — load them by path the same way; no `skill/section` argument.

**When to load a skill:** the user names a domain ("create a blueprint", "build a state tree"), or you
hit a non-obvious workflow. Then: read the pack → `discover_python_class` the classes it names → write
the Python. Don't reload a pack you already loaded this session.

---

## 4. Python basics

```python
import unreal  # lowercase

# Editor subsystems:
sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# VibeUE services are static classes, called directly:
info = unreal.BlueprintService.get_blueprint_info("/Game/MyBP")

# Batch a whole task in ONE execute_python_code call, printing evidence as you go.
# Blueprint create / variables / compile are ENGINE-side (native libs + BlueprintTools toolset),
# NOT BlueprintService (it owns graphs/components/timelines — see discover_python_class):
import json
factory = unreal.BlueprintFactory(); factory.set_editor_property("ParentClass", unreal.Actor)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_Enemy", "/Game/Blueprints", unreal.Blueprint, factory); print("CREATED:", bp)
unreal.ToolsetRegistry.execute_tool("editor_toolset.toolsets.blueprint.BlueprintTools", "add_variable",
    json.dumps({"blueprint": {"refPath": "/Game/Blueprints/BP_Enemy.BP_Enemy"},   # refPath = full object path
                "name": "Health", "type_name": "float"})); print("ADDED: Health")
unreal.BlueprintEditorLibrary.compile_blueprint(bp); print("COMPILED: BP_Enemy")
```

---

## 5. When to use `deep_research` and `terrain_data`

**`deep_research`** — when you need information that isn't in the editor:
- `action="search"` / `action="fetch_page"` — research a UE topic, API, or technique before writing code.
- `action="geocode"` / `action="reverse_geocode"` — turn a place name into lat/lng (feeds `terrain_data`).

**`terrain_data`** — when the user wants terrain from a **real-world location**:
- `preview_elevation` → use the suggested `base_level`/`height_scale` → `generate_heightmap`
  (`resolution` MUST match the landscape) → import via `unreal.LandscapeService` → `get_water_features`
  for rivers/lakes.

**The real-world-terrain chain:** `deep_research(geocode "Mount Fuji")` → `terrain_data(generate_heightmap, lng/lat)`
→ `LandscapeService` import → `terrain_data(get_water_features)` → landscape splines. Load the
`terrain-data` and `landscape` skills for the resolution formulas and water workflow.

---

## 6. See what you built (screenshots)

After any **visible** change, capture and actually look before claiming success:
```
capture_image                          # editor viewport scene (default when PIE is off)
capture_image {"source": "game"}       # PIE game viewport INCLUDING the Slate/UMG HUD
capture_image {"source": "window"}     # the whole editor window (any editor UI/panel)
```
`capture_image` returns a **real MCP image block** — you see the picture directly, no base64
decoding, no token blowout — and saves a PNG under `Saved/VibeUE/Captures` (path in the result).
`source="game"` is the ONLY way to screenshot the running game's HUD: Epic's `CaptureViewport`
sees the editor camera scene, never PIE or Slate UI. For a running game, `StartPIE` first.
**Look at the image, judge it against the request, fix, re-capture.** Reach for Epic's
`CaptureViewport` (via `vibeue.exec_tool`) only when you need its world-grid/actor-label overlay.

---

## 7. Diagnose performance

`PerformanceService` is VibeUE's net-new capability (the engine has no perf tooling). **STEP 0 is
always CPU-bound vs GPU-bound** — optimising the GPU does nothing on a CPU-bound frame:
```python
import unreal, json
print(unreal.PerformanceService.frame_timing())            # game/render/gpu ms + bound verdict — RUN FIRST
unreal.PerformanceService.start_trace("cap", "")           # Unreal Insights trace
# … reproduce the workload (ideally under PIE / standalone) …
unreal.PerformanceService.stop_trace()
print(unreal.PerformanceService.analyse("both", ""))       # frame stats + worst frames + log hitches
```
Load the `profiling` and `frame-rate` skills for the full drill-down.

**Unfocused editor = ~3 FPS.** Editor background throttling makes unattended PIE runs useless and
is NOT controlled by `t.IdleWhenNotInForeground` or `Slate.bAllowThrottling`. Disable it for the
session before automated verification — no window-focus (AppActivate) hacks needed:
```python
unreal.PerformanceService.set_background_throttling(False)   # full rate while unfocused/minimized
# ... run your PIE verification ...
unreal.PerformanceService.set_background_throttling(True)    # restore when done
```
To drive gameplay in PIE without OS focus or input-asset remapping:
`unreal.InputService.inject_action("/Game/Input/IA_Fire")` (Enhanced Input, one tick per call) and
`unreal.InputService.inject_key("SpaceBar")` (raw key via Slate).

---

## 8. Build & launch

When asked to rebuild / relaunch / test, use the project script — not manual `Build.bat`/editor commands:
- `./Plugins/VibeUE/BuildAndLaunchGame.ps1` (stops the editor, builds, relaunches).
- `-StrictRebuild` for a full plugin recompile under warnings-as-errors; `-Clean` to wipe artifacts;
  `-SkipBuild` to relaunch only.
- **`-Map /Game/Maps/YourMap`** — open a specific map. Without it the editor opens the project
  DEFAULT map; after a mid-task relaunch that is usually the wrong level, and world-edit scripts
  that grab "the current world" then modify the wrong map. Always pass `-Map` when your task
  targets a non-default level, and verify `currentMap` from the readiness signal before editing.
- On Linux or macOS: `./Plugins/VibeUE/BuildAndLaunchGame.sh --engine /path/to/UE5`.
  Use `--strict-rebuild`, `--clean`, `--skip-build`, or `--map` for the corresponding operations.

**Readiness gate (required after launch, both platforms):**
- Parse `Editor-PID=<pid>` from the launch script's output.
- Check once, then watch `<ProjectDir>/Saved/VibeUE/Signals/editor-<pid>-true.json` using filesystem events.
- Wait at most 180 seconds; do not poll MCP. Fail if that Editor process exits or the timeout expires.
- Ignore signal files for other or dead PIDs. The signal only means `RegisterToolsets()` reached its end;
  Python, World, and level readiness remain separate checks.
- The file is JSON (`signal`, `pid`, `createdUtc`, `sessionStartUtc`, `pluginVersion`, `currentMap`)
  and is written atomically, so it is complete as soon as it appears. It is re-published on every map
  open, so `currentMap` (the loaded map's package name, e.g. `/Game/Maps/TrainingPool`) stays fresh —
  gate world edits on it. PIDs get recycled: the launch scripts clear a stale same-PID signal on
  start, but if you launch the Editor yourself, check that `sessionStartUtc` is later than your
  launch time before trusting it.

**Health heartbeat (is the editor alive?):** `Signals/editor-<pid>-health.json` is rewritten every
~5s by a background thread and carries `updatedUtc` + `gameThreadStallSeconds`. Epic's MCP endpoint
runs on the game thread with NO request timeout, so a dead or wedged editor hangs MCP calls for the
client's full timeout (300s observed). Before waiting on a suspect call — or whenever results come
back empty/strange — read the health file instead: file missing or `updatedUtc` older than ~15s →
the process is gone (relaunch); `gameThreadStallSeconds` > ~10 → alive but wedged (modal dialog /
crash handler — MCP will hang; relaunch); fresh and small → the editor is fine, look elsewhere.

---

## 9. Critical rules (evergreen)

- **Log every change for rollback.** Python has no auto-rollback — `print("CREATED:/ADDED:/MODIFIED:/DELETED:", path)`
  after each op so a mid-script failure can be undone.
- **Idempotent: check before create.** Use the service `*_exists()` (or `unreal.EditorAssetLibrary.does_asset_exist`)
  before creating, to avoid duplicates.
- **Compile after structure changes.** `unreal.BlueprintEditorLibrary.compile_blueprint(unreal.EditorAssetLibrary.load_asset(path))`
  after adding variables/functions/components (there is no `BlueprintService.compile_blueprint`;
  `build_graph`'s compile flag also works for graph edits).
- **Verify success with evidence.** For Blueprint/Widget/Material/AnimGraph/StateTree edits, a successful
  tool call isn't proof — re-read the asset (`get_nodes_in_graph`, `get_connections`, compile result)
  and report brief evidence.
- **Non-destructive.** Never remove-and-recreate to change a value, clear data to make a write succeed,
  or replace a whole object to change one field. Discover the supported setter; if none exists, report
  the gap. (StateTree reparenting: `move_state`, never remove+add.)
- **Loop prevention.** Track *outcomes*. Never repeat the same call with the same args >2× when output
  is unchanged; after 2 failed attempts at a goal, stop and report — don't try a 3rd variation.
- **Never** use modal dialogs, `input()`, blocking ops, long `time.sleep()`, or infinite loops.
- **Full asset paths** (`/Game/Blueprints/BP_Name`). **Colors are 0.0–1.0** (`{"R":1.0,"G":0.5,"B":0.0,"A":1.0}`).
- **`unreal.EditorLevelLibrary` is deprecated** — use `EditorActorSubsystem` (`get_all_level_actors()`
  + `isinstance` filtering; `get_all_level_actors_of_class` does not exist).

---

## 10. Communication & working style

- **Be concise** — this is an IDE tool. Before each tool call, one sentence on what/why; after, 1–2 on
  the result. Execute multi-step tasks straight through — don't pause for "continue".
- **Discover before you call.** Method signatures come from `discover_python_class`, not memory or skill
  prose. Skills say *which* class and *why*; discovery gives the exact call shape.
- **Commit at milestones** if the project is a git repo, so a bad experiment reverts cleanly.
- **Living gotchas:** when you solve a real problem, append a one-line gotcha+fix to this file so the
  next session doesn't relearn it.
- Rocket Launcher reload gotcha: a one-round launcher must gate fresh held-ammo creation on `Reserve > 0`, not `CurrentAmmo > 0`; Mutable-generated runtime meshes may not inherit newly added base-mesh sockets, so use a bone plus configurable relative transform unless the generated mesh is rebuilt.
- Windows launch gotcha: always invoke `Scripts/Build_Windows.ps1` with `-RestartEditor -WaitForReady -Map`; verify the quoted full `.uproject` argument and readiness signal before using PIE.
<!-- END VibeUE -->


