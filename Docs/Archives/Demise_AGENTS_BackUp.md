# 项目快速指南（AGENTS）

你正在处理Unreal Engine 5项目NewWorldOrder。

## 环境判定（任何操作前第一步）

- 会话可能跑在 Windows（Codex）、WSL/Linux（deepseek-harness）或 macOS，三种环境本项目都会用。先用 `uname -a`（Linux/macOS）或 `$env:OS`（Windows）判断，不要默认某一种。
- 路径一律使用相对项目根目录的写法（如 `NewWorldOrder/Docs/...`），不写盘符绝对路径；确实需要绝对路径时，用 `$PWD` / `$(pwd)` 等运行时展开。
- Windows 用 PowerShell/pwsh 与 `curl.exe`；WSL/bash 与 macOS 用对应 shell 与 `curl`，不要照抄其他环境的命令示例。
- WSL 会话全程使用 Linux/bash 命令；即使仓库位于 Windows 盘挂载点，也不切换回 Windows 工具链。

必读顺序：
1. AGENTS.md（本文件，通用规范）
2. Docs/SessionGuides/Implementation_代码实现指南.md
3. Docs/Engineering/Tech_Constraints.md（完整技术约束，200+行）
4. Docs/QuickReference/SessionChecklist.md（近期约束和踩坑点）
5. 如果涉及特定系统：
   - 武器系统 → Docs/ContextPacks/WeaponSystem_Context.md（当前武器实现以 Lyra 迁移与 Docs/Tasks/WeaponSystem 为准）
   - 技能系统（旧设计参考）→ Docs/SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md
   - 角色切换 → Docs/Tasks/CharacterSwitching/Overview_总览.md
6. 如果需要读写蓝图/UMG/DataAsset → Docs/DevelopmentNotes/MCP_踩坑记录.md（MCP 操作手册，首次必读）
7. 仅允许对当前项目目录及其子目录内的文件执行写入、修改或删除，项目外文件只读。删除单个文件前，必须先用 `Resolve-Path` 获取完整绝对路径，并验证其以项目根目录开头，然后仅用该绝对路径执行删除，禁止通配符、`..`、递归参数、强制批量参数，也禁止跨 shell 拼接路径。若涉及多个文件或非空文件夹，则停止并请求用户手动处理。注意：路径构造错误可能导致灾难性后果，务必避免以反斜杠开头的驱动器路径或不完整片段，并警惕未转义的特殊字符；尽量使用支持回收站的删除方式（如 `-Recycle`），不可用时则仅在路径无误后执行。

---

## 行为红线（最高优先级）

### 远程仓库是唯一沟通渠道（最高优先级）

重要：你、用户、Codex只能通过Git远程仓库沟通。

强制规定：
- 所有产出（代码、文档）必须推送到远程仓库
- 未推送到远程=任务失败=你的工作成果丢失
- 用户、Codex、其他AI都看不到本地文件

Git强制流程：
1. git status（检查所有变更）
2. git add .（添加所有文件，不能遗漏）
3. git status（再次确认暂存区）
4. git commit -m "描述性提交信息"
5. git push -u origin <branch-name>
6. 检查推送是否成功（无报错）
7. 向用户汇报：已推送，commit hash: <hash>

关键点：
- 新建的文档/代码如果不git add，远程仓库完全看不到
- 必须git push，否则只在本地，用户拿不到

### 上游插件只读规则

- 从 Lyra 或第三方复制的 Plugins/ 源码以及虚幻引擎源码默认视为上游只读；项目适配优先放在 Source/NewWorldOrder 的子类、Subsystem 或包装层。
- 只有项目层无法解决且用户明确同意维护 fork 时才能改上游插件；修改处必须标注项目定制原因、与官方差异，并在迁移文档记录。
- 教程与空白项目接入文档不得隐式依赖未说明的插件源码补丁。

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
   - DT_PMM_InputAction：DataTable 资产，为每个 CommonUI Input Action 行提供跨设备的图标刷和提示文本
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

---

## 架构速览（写代码前必读）

### UI 通信架构（MVVM + GameplayMessage + CommonUI）（2026-03）

项目 UI 层使用三种通信机制，各有适用场景。AI 编写 UI 相关代码时必须选择正确的通道：

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

### UE 5.8.1 MCP：AI 操作蓝图与资产

UE 5.8以后的版本 内置 MCP 插件，AI 可通过 HTTP 读写蓝图、Data Asset、UMG。

涉及蓝图/资产操作前，必须先读操作手册：Docs/DevelopmentNotes/MCP_踩坑记录.md

核心约束：
- 外部 HTTP 请求 MCP 时仅使用 curl.exe（linux环境则是curl），禁止用 Python urllib/requests 访问 MCP 端点。
- 编辑器内批量操作优先使用 execute_python_code；这不属于被禁止的 HTTP Python 客户端。
- MCP 请求必须串行发送并等待返回，不再固定 sleep 0.5-1 秒，不并发堆叠请求。
- UE 5.8.1 已修复 `tools/call` 在有进度和无进度路径下的响应 framing；客户端仍应按 HTTP `Content-Type` 兼容 JSON 与 SSE，不再把旧版 framing 异常当作业务失败。
- UE 5.8.1 在工具脚本执行期间禁用事务，单次脚本修改不再合并成一个可依赖的 Undo 事务。所有写资产脚本必须逐项打印变更路径、保持幂等，并在每个可验证阶段编译、保存和复读资产。
- UE 5.8.1 已修复向没有 `SimpleConstructionScript` 的 Blueprint 添加组件时的致命崩溃；不再保留针对该 5.8.0 缺陷的重建资产或绕过方案。
- 具体参数、已知问题和验证方法以 Docs/DevelopmentNotes/MCP_踩坑记录.md 为准。

---

## 协作流程

开始任务前：
- 阅读本文件（AGENTS.md）
- 根据任务类型阅读对应的SessionGuides
- 查看SessionChecklist.md确认近期约束
- 执行任何修改前，先输出实施计划与将修改的文件列表

实现过程中：
- 若在实现中发现缺失规则或踩坑案例，先回写到相关Docs/文件再继续开发
- 涉及数学公式、向量换算等复杂逻辑时，必须在C++中加入逐行中文注解
- 发现既有代码与文档不符时，在修正实现的同时更新对应文档
- 涉及复杂功能改造时，先做"主线 / 兼容 / 遗留 / 候删"判断，再决定是否删旧逻辑
- 如果某段 C++ 是为了解决"文档可能滞后、蓝图不可见、多人协作易误解"的问题，必须在关键入口、状态字段、调用链节点旁补中文注释，直接说明：
  - 这段逻辑的职责
  - 调用链从哪里来、往哪里去
  - 哪些蓝图/父类属性需要配合设置
- 注释只要求"使用中文"，禁止写成"中文注解："这类无信息量前缀；注释内容必须直接进入主题。

任务完成后：
- 更新文档（DevelopmentNotes、CHANGELOG等）
- Git提交推送（git add . && git commit && git push）
- 执行维护检查（见Maintenance_Rules.md）
- 向用户汇报（包含Git提交状态和维护检查结果）

---

## 通用规范

### 输出格式（强制执行）

仅保留结构性符号：
- 标题（#）
- 列表（- * 1.）
- 代码块（```）

禁止任何装饰符号：
- 粗体（*）
- 斜体（_）
- 下划线
- Emoji表情符号

### 文档维护规则（强制执行）

- 中文文档只能用安全的 UTF-8 方式修改。
- 允许的方式：
  - `apply_patch`
  - 明确指定 UTF-8 的文件读写
- 禁止使用未经验证的批量重写链路处理中文文档，避免把中文写成 `?`。
- 修改文档前，必须先完整阅读目标文档和关联文档，再决定如何修改。
- 修改文档时，必须先判断三类内容：
  - 哪些应保留
  - 哪些应局部更新
  - 哪些才应该完全覆盖
- 看到"必须遵守""权威架构""相关必读"这类章节时，默认保留；只有在代码事实和需求文档都明确证明其错误时才能删除或重写。
- 不允许因为"状态过时"就整页覆盖掉仍然有效的架构约束。
- 文档归属：通用规范写 AGENTS.md 或 Docs/Engineering；任务相关结论写 Docs/Tasks/<Task>/ 或对应 DevelopmentNotes；禁止把任务细节混进 AGENTS.md 的通用规则。
- 文档修改完成后，必须检查文件是否正常显示中文，不能出现 `?`、乱码或整页语义丢失。
- 说明代码或蓝图属性时，必须写清"属性定义在哪个类"。如果属性来自父类，必须明确写成"某类继承自某父类，使用的是某父类上的某属性"，避免出现"走基类 InputMapping"这类有歧义的表述。
- 写代码、文档、注释时，必须站在三类接手者视角审视信息是否足够：
  - 用户验收功能时，先看文档和蓝图配置步骤，不会先逐行审错误代码
  - 后续维护者会先看注释和文档，再决定是否深入读实现
  - 其他AI上下文丢失后，只能依赖仓库里的代码注释和文档恢复思路
- 因此凡是"C++ 逻辑依赖蓝图子类设置""依赖父类已有属性""依赖编辑器资产配置"的实现，必须同时满足：
  - 代码里有中文注释直接写清楚
  - 文档里有操作说明
  - 命名和描述不带歧义

### 需求与既有实现审查规则（强制执行）

- 改代码前，先查需求文档，再查任务包文档，再查当前代码调用链。
- 先理解"为什么这样写"，再决定"要不要改"。
- 不允许在未审查需求来源的情况下，凭猜测重构复杂系统。
- 修改复杂系统（如角色切换、技能、库存、武器、存档）前，至少完成：
  - 阅读对应 Requirements / Overview / STATUS
  - 搜索调用点和引用点
  - 判断当前代码属于主线、兼容、遗留还是候删
- 删除旧代码前，必须先确认：
  - 没有蓝图引用
  - 没有别的系统依赖
  - 已有替代链路
- 如果不确定，先收集证据并整理结论，再一次性找用户确认；不要遇到一点疑问就反复打断用户。
- 询问用户时，必须带上已核对的需求来源、代码证据和风险点，减少用户决策成本。

### 文档命名规范

文件命名格式：英文_中文.md
- 正确：Overview_总览.md、MaleSkills_男主技能.md
- 错误：Overview.md（缺少中文）、总览.md（缺少英文）

文件夹命名：只使用英文
- 正确：SystemDesign/、QuickReference/
- 错误：SystemDesign_系统设计/

详见：Docs/Engineering/Style.md

### 编译与引擎版本

引擎版本判定：
- 以 NewWorldOrder.uproject 的 EngineAssociation 为准
- EngineAssociation 变更后，编译脚本路径也要同步调整

Windows 编译（本 Windows 工作站优先使用项目脚本）：
```
powershell -ExecutionPolicy Bypass -File .\Scripts\Build_Windows.ps1
```

说明：
- 以下 PowerShell 示例只用于 Windows 会话；WSL/macOS 会话使用对应 shell，不要照抄。
- `Scripts/Build_Windows.ps1` 会读取 `NewWorldOrder.uproject` 的 `EngineAssociation`
- 查找顺序：`UE_ENGINE_DIR` 环境变量 → HKCU/HKLM Unreal Engine Builds 注册表 → 常见安装盘目录。引擎路径由脚本运行时解析，不要写死绝对路径。
- 如果脚本找不到引擎，优先设置 `UE_ENGINE_DIR`，不要只依赖 HKCU 注册表

Windows 手动编译（脚本不可用时才使用）：
```
$uproject = Get-Content .\NewWorldOrder.uproject | ConvertFrom-Json
$engineAssoc = $uproject.EngineAssociation
$engineDir = (Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds").$engineAssoc

& "$engineDir\Engine\Build\BatchFiles\Build.bat" `
  NewWorldOrderEditor Win64 Development `
  -Project="$(Resolve-Path .\NewWorldOrder.uproject)" `
  -WaitMutex
```

macOS 编译（示例）：
```
UE_ENGINE_DIR="$(ls -d /Users/Shared/Epic\ Games/UE_5.* | tail -n 1)"
"$UE_ENGINE_DIR/Engine/Build/BatchFiles/Mac/Build.sh" \
  NewWorldOrderEditor Mac Development \
  -Project="$PWD/NewWorldOrder.uproject" \
  -WaitMutex
```

注意：
- 若注册表中找不到 EngineAssociation，可在常见路径中手动定位 UE_5.x
- `Scripts/Build_Windows.ps1` 只用于 Windows；macOS 电脑需要使用 macOS 对应的引擎路径和 `Build.sh`
- 编译失败需记录关键错误日志，并修复后再次编译验证

### 知识资产管理原则

核心理念：聊天记录跟会议一样，必须产出可持久化的知识资产。

强制要求：
- 每次对话结束前必须提交代码（git commit && git push）
- 文档必须保持最新（设计理念、开发笔记、踩坑记录）
- 单一数据源（SSOT）：当前实现与任务包优先（Docs/Tasks/<System>/STATUS）；Docs/SystemDesign 为设计稿，数值部分可能过时
- 快速恢复上下文：新会话开始时只需阅读本文件和相关Docs/
- Token使用优化：优先引用Docs/下的现有章节，避免粘贴整段聊天记录

知识沉淀流程：
1. 发现新问题 → 解决问题
2. 更新文档（SystemDesign、DevelopmentNotes）
3. 提交Git
4. 后续会话直接受益

---

## 领域架构规则

### 库存与武器架构强制规则（2025-11-15）
- PlayerState 上的 UResourceInventoryComponent 只负责可堆叠资源（材料/货币/徽章/设计图），所有数据默认 Persistent，需要参与 SaveGame；战斗内临时回血包等即时效果请绕过该组件，直接通过能力或临时系统处理。
- PlayerState 上的 UShootInventoryManagerComponent 管理有身份的物品/武器实例，QuickBar、EquipmentManager、武器/装备 GA 只能与该组件打交道，不允许直接从 SaveGame 或蓝图数据表创建武器实例。
- Inventory ItemInstance 必须带 EShootItemLifetime（Persistent / RuntimeOnly）枚举，提供 AddPersistentItem / AddRuntimeItem 两套接口；副本内拾取必须走 RuntimeOnly，退出副本或返回 Hub 时统一清空，且绝不写入 SaveGame 或 QuickBar 配置。
- SaveGame 序列化时仅保存 Lifetime=Persistent 的 ItemInstance 和 QuickBar 绑定；RuntimeOnly 物品只存在于当前副本，不进入账号仓库。
- 进入副本时用 SaveGame 中的 QuickBar 配置从 InventoryManager 取出 Persistent 物品填充槽位；副本内拾取替换当前武器时不得修改账号 QuickBar，只更新战斗期实例引用。
- 编写库存/武器相关 C++ 代码时必须添加中文注释，说明本文件如何区分"ResourceInventory = 数量型仓库""InventoryManager = 有身份背包""QuickBar/Equipment 仅引用 InventoryManager"，保证 AI/同事读代码即可理解架构。
- HUD/QuickBar/UI 只能通过 QuickBar 消息或 `UShootWeaponInstance` 读取武器数据；禁止直接引用 `ARangedWeaponInstance/AHitscanWeaponInstance` 获取弹药或展示数值，Actor 仅作为表现壳。

### 蓝图、Data Asset 与 C++ 的分工（MCP 时代，2026-06）

UE 5.8.1 MCP 让 AI 可以直接创建和编辑蓝图资产。以下规则取代旧版"C++优先"惯例，明确三种资产类型的职责边界。

三类资产的职责：

1. C++ 类（游戏逻辑，不可替代）：
   - 网络复制、GAS 能力激活、武器射击/换弹、伤害计算
   - 组件创建（CreateDefaultSubobject）
   - 接口定义、委托声明
   - BlueprintNativeEvent / BlueprintImplementableEvent 的函数声明
   - 规则：C++ 构造函数只做组件创建和默认值设定。禁止在构造函数中加载资源（ConstructorHelpers::FObjectFinder）、禁止硬编码数据数组（如 AddUnique 循环）

2. 蓝图子类（配置与调优，通过 MCP 创建）：
   - 视觉参数调优（灯光位置/强度、网格缩放、材质颜色、UI 尺寸）
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
- 禁止使用 VisibleAnywhere（蓝图可见但只读）+ C++ 硬编码组合
- 禁止使用 ConstructorHelpers::FObjectFinder 加载资源 → 改为 EditDefaultsOnly 的 TObjectPtr，在蓝图子类中设置
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

### 交互系统责任划分（2025-11-15）
- 玩家交互与 AI 交互分离：`UShootGA_Interact` 只授予 PlayerControlled 角色；AI 使用 AutoOverlap 或脚本/行为树 GA，绝不依赖 WaitInputPress/交互 UI。
- 可交互物统一提供 `EShootInteractionTriggerMode`（None/AutoOverlap/PressToInteract/AIScripted）与 `EShootInteractionUserFilter`（PlayerOnly/AIOnly/PlayerAndAI）；`PressToInteract` 的目标仅由玩家交互能力扫描，`AutoOverlap/AIScripted` 可分别对玩家或 AI 生效。
- 交互效果需数据驱动：拾取物根据 Instigator 阵营 Tag（Faction.Player、Faction.Enemy 等）查表应用 GE，禁止在 C++ 中硬编码 if(IsPlayer) 分支；同一交互可对玩家扣血、对 AI 回血。（另见上方「阵营与敌我识别」章节）
- 拾取能力必须根据配置分流：账号资源写入 `UResourceInventoryComponent`，Persistent 物品走 `UShootInventoryManagerComponent::AddPersistentItem`，副本临时武器走 `AddRuntimeItem/EquipTemporaryPickupWeapon`，并确保仅服务器修改账号数据。
- `AShootResourcePickup` 的 AutoOverlap 流程与交互 GA 并存，可通过 `TriggerMode` 切换；实现 `IInteractableTarget` 时只负责提供交互选项与提示，真正增减资源或装备必须交由对应 Ability/组件处理。

---

## 关键文档索引

### 工程规范
- Docs/Engineering/Style.md - 代码和文档命名规范
- Docs/Engineering/Tech_Constraints.md - 完整技术约束（代码AI必读）
- Docs/Engineering/Maintenance_Rules.md - 自动维护规则
- Docs/Engineering/DocumentStructure.md - 文档体系说明
- Docs/Engineering/Build_编译指南.md - 编译与引擎版本说明

### 系统设计（设计稿，数值可能与当前实现不一致）
- Docs/SystemDesign/GameDesign/ - 游戏设计主文档与核心理念（含 游戏设计完整文档 v7.0 Final）
- Docs/SystemDesign/GameDesign/NumericalDesign/Attributes/ - 属性系统（旧版数值）
- Docs/SystemDesign/GameDesign/NumericalDesign/Skills/ - 技能系统（旧版数值）
- Docs/SystemDesign/GameDesign/NumericalDesign/Weapons/ - 武器系统（旧版数值；当前武器实现以 Docs/Tasks/WeaponSystem 与 Lyra 迁移为准）
- Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md - 剧情世界观
- Docs/SystemDesign/GameFlow/Overview_总览.md - 游戏流程

### 快速参考（Token优化）
- Docs/QuickReference/Attributes.md - 常用属性表（旧版）
- Docs/QuickReference/Skills.md - 常用技能表（旧版）
- Docs/QuickReference/Weapons.md - 常用武器表（旧版；当前武器以 Lyra 迁移为准）
- Docs/QuickReference/SessionChecklist.md - 会话检查清单

### 上下文包（快速启动）
- Docs/ContextPacks/WeaponSystem_Context.md - 武器系统上下文
- Docs/SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md - 技能系统（旧设计参考）

### 任务包（自包含）
- Docs/Tasks/CharacterSwitching/ - 角色切换完整实现指南

### 会话启动指南
- Docs/SessionGuides/Implementation_代码实现指南.md
- Docs/SessionGuides/GameDesign_游戏策划指南.md

### UI 系统（必读，AI 经常忽略）
- Docs/Engineering/Notes/UI/CommonUI_架构与用法.md - CommonUI 层级、输入管理、Widget 生命周期
- Docs/Engineering/Notes/UI/RegisterUIActionBinding_学习笔记.md - UIActionBinding 与长按实现

### 开发笔记（知识积累）
- Docs/DevelopmentNotes/INDEX.md - 笔记索引
- Docs/DevelopmentNotes/README.md - 笔记规范
- Docs/DevelopmentNotes/MCP_踩坑记录.md - UE 5.8.1 MCP 操作蓝图（含崩溃规避）

### 项目管理
- Docs/PROJECT_PHASE.md - 项目阶段管理
- Docs/Engineering/ADR/ - 架构决策记录

---

## 注意事项

归档文档：
- Docs/Archives/中的文档已废弃，请勿引用

权威模块：
- Docs/Engineering/Tech_Constraints.md中标注的权威模块不可直接改动
- 请使用子类或扩展

补充文档：
- Docs/GASDocumentation_Chinese/ - UE4.26社区GAS指南（参考）
- Mutable官方Wiki - 角色定制系统（参考；本机路径以用户告知为准，不写进命令）

---

最后更新：2026-08-16
版本：4.6（环境判定与相对路径规则；标注旧版设计参考；放宽 Live Coding 与代理规则）

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
  (`AgentSkillToolset`) and for the few Epic tools whose **result the MCP layer must surface for
  you** (image returns like `CaptureViewport`). **Everything else from Epic's engine toolsets is
  also reachable from inside `execute_python_code`** via `unreal.ToolsetRegistry.execute_tool(...)`
  (see §2), so batch engine-toolset calls with your Python instead of spending a round-trip. Don't
  use `call_tool` for work `execute_python_code` can batch.

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
- `deep_research` — web search / page fetch / geocode (see §5).
- `terrain_data` — real-world heightmaps + water splines (see §5).

**VibeUE services (call from Python inside `execute_python_code`):** `unreal.<Name>Service.<method>()`
— Blueprint, BlueprintGraph (via BlueprintService), Material(+Node), Widget, Skeleton, AnimSequence,
AnimMontage, AnimGraph, Landscape(+Material), Foliage, MetaSound, SoundCue, Niagara(+Emitter,
+ScratchPad), StateTree, Input, EnumStruct, UVMapping, RuntimeVirtualTexture, MapBlockout,
GameplayTag, Viewport, Actor, Engine/ProjectSettings, **Performance** (`unreal.PerformanceService.frame_timing()`).
These overlap-trimmed services keep only what the engine lacks — for plain asset/actor/blueprint
basics the engine's own tools may be simpler (below).

**Calling Epic's engine toolsets from Python (`execute_tool`).** Epic's engine toolset *classes*
exist as `unreal.*` (e.g. `unreal.EditorAppToolset`) but their AICallable functions are **not**
exposed as Python methods — `unreal.EditorAppToolset.get_selected_assets()` fails. Invoke them
through the registry instead (same dispatch `call_tool` uses, but in-process and batchable):
```python
import unreal, json
res = unreal.ToolsetRegistry.execute_tool(
    "EditorToolset.EditorAppToolset",   # registered (namespaced) toolset name
    "GetSelectedAssets",                # tool name
    "{}")                               # args as a JSON string
assert res.is_complete and not res.error, res.error
out = json.loads(res.get_value_as_json_string())     # -> {"returnValue": ...}
```
Discover exact names/schemas from Python: `unreal.ToolsetRegistry.get_all_toolset_json_schemas()`
(all of them) or `get_toolset_json_schema("EditorToolset.EditorAppToolset")` (one). Names are
namespaced — `EditorToolset.EditorAppToolset`, `NiagaraToolsets.NiagaraToolset_System`, etc. (a bare
`"EditorAppToolset"` returns "Toolset not found"). Note `execute_tool` returns an **async** result:
the editor tools above complete synchronously (`is_complete=True`), but for a long-running tool check
`is_complete` / bind `on_completed` rather than assuming `value` is ready.

**Use the engine's native tools for these (VibeUE intentionally doesn't duplicate them):**
- **Assets** (find / save / move / delete / duplicate / metadata): native Python
  `unreal.EditorAssetLibrary` / `EditorAssetSubsystem` inside `execute_python_code` (batchable), or
  Epic's `AssetTools` toolset (via `execute_tool` in-Python, or `call_tool`).
- **Screenshots / vision**: Epic's `EditorAppToolset` — `CaptureViewport` (returns a PNG, and can
  overlay a world grid + actor labels), `CaptureEditorImage`, `CaptureAssetImage`. **Use `call_tool`
  for these** so the MCP layer surfaces the image for you to view (`execute_tool` would only hand
  back a base64 string).
- **PIE**: `EditorAppToolset.StartPIE` / `StopPIE` / `IsPIERunning` — batchable via `execute_tool`
  (`"EditorToolset.EditorAppToolset"`), or `call_tool`.
- **Logs**: `LogsToolset.GetLogEntries` — via `execute_tool` or `call_tool` (or read the `.log` file).
- **DataTables / DataAssets / enum-struct basics**: Epic's `DataTableTools` / `DataAssetTools` /
  `ObjectTools` (via `execute_tool` or `call_tool`). (VibeUE keeps only `EnumStructService` for
  create/edit of user enums & structs.)

---

## 3. Skills — native `AgentSkill` (lazy domain knowledge)

VibeUE's ~36 skill packs are registered as Unreal **AgentSkills** and served by the engine's
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
call_tool(tool_name="CaptureViewport", toolset_name="EditorToolset.EditorAppToolset")
```
It returns a PNG (base64) and can overlay a world-space grid + actor labels for spatial awareness. For
a running game, `StartPIE` first. **Open/read the image, judge it against the request, fix, re-capture.**

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

---

## 8. Build & launch

When asked to rebuild / relaunch / test, use the project script — not manual `Build.bat`/editor commands:
- `./Plugins/VibeUE/BuildAndLaunchGame.ps1` (stops the editor, builds, relaunches).
- `-StrictRebuild` for a full plugin recompile under warnings-as-errors; `-Clean` to wipe artifacts;
  `-SkipBuild` to relaunch only.
- On Linux or macOS: `./Plugins/VibeUE/BuildAndLaunchGame.sh --engine /path/to/UE5`.
  Use `--strict-rebuild`, `--clean`, or `--skip-build` for the corresponding operations.

**Readiness gate (required after launch, both platforms):**
- Parse `Editor-PID=<pid>` from the launch script's output.
- Check once, then watch `<ProjectDir>/Saved/VibeUE/Signals/editor-<pid>-true.json` using filesystem events.
- Wait at most 180 seconds; do not poll MCP. Fail if that Editor process exits or the timeout expires.
- Ignore signal files for other or dead PIDs. The signal only means `RegisterToolsets()` reached its end;
  Python, World, and level readiness remain separate checks.
- The file is JSON (`signal`, `pid`, `createdUtc`, `sessionStartUtc`, `pluginVersion`) and is written
  atomically, so it is complete as soon as it appears. PIDs get recycled: the launch scripts clear a
  stale same-PID signal on start, but if you launch the Editor yourself, check that `sessionStartUtc`
  is later than your launch time before trusting it.

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
  <!-- END VibeUE -->

