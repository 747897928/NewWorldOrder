---
task_id: CharacterSwitching
status: in_progress
assigned_to: Codex
progress: 95%
started: 2025-11-23
last_updated: 2026-03-21
---

# 角色切换任务状态

当前状态：in_progress（切换后端已落地，统一请求入口已收口；菜单长按骨架已落地；玩家交互能力授予链已在 C++ 收口；世界入口当前退回 PressToInteract 占位方案；另一位主角 NPC 入口的 C++ 骨架已落地）
负责人：Codex
进度：95%

建议阅读：
- 人类先看：`Overview_总览.md`、`CallFlow_调用链路.md`、`STATUS.md`
- 深挖交互再看：`LyraInteraction_研究笔记.md`、`Implementation_实现指南.md`

正史要求（游戏设计完整文档 v7.0 Final.md）
- 仅在 Hub/安全点切换角色，副本战斗中禁止切换
- 双主角共享存档级武器池，但 QuickBar/Loadout 独立
- Hub 中切换角色时保存当前主角 QuickBar/外观并加载目标主角配置
- 最终体验参考《刺客信条：枭雄 / 影》：靠近另一主角切换，菜单内也可切换

当前临时对齐结论（未正式回写权威需求，当前以 `DecisionDraft_角色切换需求临时对齐.md` 为准）
- 主入口：主菜单长按切换
- 次入口：世界里的切换点 / 交互物，点击交互后直接切换
- 当前不采用“靠近另一位主角 NPC 后直接切换”作为主线方案
- 当前先移除“仅 Hub 可切换”的硬限制，后续按战斗状态、特殊剧情状态等真实约束再收口
- 未来仍预留“另一位主角 NPC -> 同伴交互对象”扩展，但不进入本轮实现

当前实现（代码事实）
- `AShootPlayerState::SwitchToCharacter` 完成快照保存/加载（Inventory/QuickBar/外观）
- `BeginPlay/EndPlay` 通过 SaveGameSubsystem 恢复/保存账号库存与 QuickBar
- `ApplyGenderAbilityKit` 按 KitTag 移除/授予性别套件
- 快照恢复通过 `ShootEffect_SnapshotRestore` 写回 Health/ShieldCapacity/UltimateCharge
- Hub/安全区判定优先读取 SafeAreaVolume + GameState（GameMode 写入后复制）
- 当前世界内入口为 `AShootCharacterSwitchStation`（开发期占位）
- 当前代码里保留一条世界入口占位链：
  - `UShootGA_Interact` 已回归通用交互样例 / 参考
  - `UShootGA_WorldCharacterSwitchRequest` 负责把世界入口请求桥接到共享切换后端
  - `AShootCharacterSwitchEntryActorBase` 负责世界入口 Actor 的共性骨架
  - `AShootCharacterSwitchNPCBase` 负责另一位主角 NPC 入口的最小 C++ 骨架
  - 世界入口对象当前通过 `IShootCharacterSwitchEntry` 提供目标性别
  - 当前世界入口暂时是 `PressToInteract` 占位方案，不再把角色切换长按逻辑写在 `UShootGA_Interact` 中
- 玩家交互能力授予链已在 C++ 收口：
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
  - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
  - 当前只显式兜底授予 `UShootGA_Interact`
  - `UShootGA_Interaction_Collect`、`UShootGA_Interaction_Revive`、`UShootGA_WorldCharacterSwitchRequest` 由附近目标动态授予
  - `UShootGA_Interact` 构造函数已补 `StartupInputTag = InputTag.Ability.Interact`
  - 因此当前不再依赖蓝图 `StartupAbilities` 或旧存档分支偷偷授予交互能力
- `AShootPlayerController` 已新增长按进度广播与统一请求入口：
  - `OnCharacterSwitchHoldProgressChanged`
  - `BeginCharacterSwitchHold / CancelCharacterSwitchHold / CompleteCharacterSwitchHold`
  - `RequestSwitchCharacter / ServerRequestSwitchCharacter / TrySwitchCharacter`
- 2026-03-21：当前切换链里的“仅 Hub/安全区可切换”硬限制已临时移除：
  - `AShootPlayerController::TrySwitchCharacter`
  - `AShootPlayerState::CanSwitchCharacter`
  - 当前先保留 TODO，后续改为战斗状态 / 剧情状态 / 特殊任务状态等更细粒度门禁
- `UShootGA_WorldCharacterSwitchRequest` 已不再发起确认框，而是直接走服务器权威切换
- 世界内旧确认框 C++ 主线已删除
- 2026-08：`UShootCharacterSwitchWidgetBase` 已删除（全仓蓝图与 C++ 零引用，git 可恢复）。
  - 菜单切换的委托绑定已由 `UShootWardrobeViewModel::SwitchCharacter` / `HandleCharacterSwitchResult` 直连 `AShootPlayerController` 完成。
  - `IA_SwitchCharacter`、`IMC_CharacterSwitchMenu`、`IMC_Confirm_Select_Character` 资产保留；其中 `IMC_CharacterSwitchMenu` 按用户要求刻意保留。
  - 后续菜单长按 UI 若需要基类封装，按“语义委托 + ViewModel”模式重新收敛，不再恢复旧 Widget 基类。
- `UMutableAppearanceComponent` 收口为表现层，不再直接写 PlayerState 的 `CharacterGender`
- `AShootPlayerState::LoadCharacterFromSnapshot` 改为统一走 `ServerSetAppearanceTags`，服务端本地也触发外观刷新回调
- `AShootPlayerState::OnRep_CharacterGender` 已广播当前性别外观标签，避免切换后表现不同步
- 2026-06-13：衣柜第一版复用角色切换外观快照链
  - `AShootPlayerState::ServerEquipWardrobeItem` 会更新当前性别的 AppearanceTags
  - `ServerSaveCurrentWardrobe` 与 `PersistCurrentWardrobeState` 会把当前性别外观写回角色快照并触发 SaveGameSubsystem 保存
  - W_Cloth 的 SwitchCharacterButton 通过 `UShootWardrobeViewModel::SwitchCharacter` 调用 `AShootPlayerController::RequestSwitchCharacter` 请求切到另一性别
  - 这部分不改变既有角色切换入口架构，只把衣柜装备结果接入现有 PlayerState 外观同步链
- 2026-02-28：修复 Mutable 外观同步链路（去重绑定回调、按当前性别过滤标签应用、同步期间阻止反向回写、补充切换与标签调试日志）
- 2026-02-28：恢复 `AShootPlayerState::IsInHubOrSafeArea` 正式判定（移除调试硬返回 true），并将 Hub 标记统一为 GameMode 配置 -> GameState 复制
- 2026-02-28：补充蓝图可执行的 Hub 配置路径（`BP_ShootGameMode` 中设置 `bHubOrSafeAreaMap`）
- 2026-03-13：复核任务包与当前代码，确认 Hub 判定职责边界维持为“GameMode 决策，GameState 复制，PlayerState 统一读取”
- 2026-03-13：按《刺客信条：枭雄 / 影》参考重新明确需求，确认最终 UX 应包含“Hub 内靠近另一主角切换”与“CommonUI / ESC 菜单切换”两种入口；当前 `AShootCharacterSwitchStation` 仅算占位实现
- 2026-03-13：重新核对官方 Enhanced Input 文档与项目输入实现，确认当前世界内长按方案采用 `GAS WaitDelay + WaitInputRelease`，不直接改 Interact 输入资产为 Hold Trigger
- `BP_ShootCharacter` 启动能力已清理：
  - `StartupAbilities` 只保留仍有效的 `GA_Hero_Jump`
  - `StartupPassiveAbilities` 保留承担 GameplayEvent 转 GameplayEffect 的 `GA_ListenForEvent`
  - `GA_Skill1~4` 已从当前启动数组移除但保留为后续扩展资产；正式男女主技能继续由 `AShootPlayerState::ApplyGenderAbilityKit` 授予
- 当前交互架构：
  - 主交互能力 `UShootGA_Interact` 作为玩家预设常驻能力存在
  - `GrantNearbyInteraction` 负责按附近目标动态授予和回收具体执行能力
  - 交互物只提供选项与执行能力，不授予交互扫描主能力
- 2026-03-18：根据用户补充的《刺客信条：影》参考与 CommonUI 约束，进一步纠偏：
  - 菜单入口明确为 CommonUI 长按，且需要支持手柄
  - 菜单入口不是交互，不应依赖 `UShootGA_Interact`
  - `UShootGA_Interact` 只能算交互样例 / 通用交互参考，不应继续承载角色切换专属长按主逻辑
- 2026-03-18：补查 CommonUI + Enhanced Input 官方实现与引擎源码，确认菜单层长按可直接在 C++ Widget 中通过 `RegisterUIActionBinding(FBindUIActionArgs)` 或 `UCommonButtonBase` 完成，不需要经过 `UShootInputComponent -> InputTag -> ASC`
- 2026-03-18：同日补上 `AShootPlayerController::RequestSwitchCharacter / ServerRequestSwitchCharacter`，让世界入口与后续 CommonUI 菜单入口都能走同一个“本地请求 -> 服务器执行”后端
- 2026-03-18：继续核对 CommonUI 与 Enhanced Input 的组合方式，确认：
  - `UCommonActivatableWidget::InputMapping / InputMappingPriority` 会在 Widget 激活/反激活时自动切入和移除 Input Mapping Context
  - 角色切换菜单可在 `IMC_FrontEnd` 基础上叠加 `IMC_Confirm_Select_Character`
  - 菜单长按第一版不建议把 `IA_Confirm` 改成 Enhanced Input 的 `Hold` / `Hold And Release`
  - 更稳的做法是让 `IA_Confirm` 保持普通确认语义，再由 CommonUI 的 hold 机制处理进度和完成
  - 若菜单 Widget 走 `RegisterUIActionBinding(FBindUIActionArgs)`，确认动作更适合作为 UI Generic Action；否则 CommonUI 可能优先注入 Enhanced Input，而不是直接执行 Widget 绑定委托
- 2026-03-18：用户补充前端现状后，进一步确认：
  - `IA_Confirm` 已映射手柄 A，且 `DT_PMM_InputAction` 已提供对应跨设备图标数据
  - `IMC_FrontEnd` 已服务于前端地图流程，因此角色切换菜单应在此基础上叠加专用菜单 IMC，而不是改写玩法层输入
  - `RegisterUIActionBinding(FBindUIActionArgs)` 只是 Widget 监听动作的方式，不是 `UInputAction / IMC / DT` 的替代品
  - 如果菜单角色切换需要独立键位，例如手柄 X、键盘 E，则应新增 `IA_SwitchCharacter` 与 `IMC_CharacterSwitchMenu`
- 2026-03-18：继续核对 UE 5.7 CommonUI 源码后，进一步确认：
  - `RegisterUIActionBinding` 不是全局监听，而是绑定到具体 Widget，并且会检查当前 Widget 是否可达、当前输入模式是否匹配
  - 如果走 `UInputAction` 绑定，CommonUI 会通过 `QueryKeysMappedToAction` 查询当前激活 IMC 里的键位，所以切换 IMC 会直接影响它是否收到输入
  - 纯 `FBindUIActionArgs(UInputAction*)` 路线不应默认假设就能完整提供 CommonUI 风格的长按进度和松手回落
  - 当前更稳的菜单长按方案是：独立 `IA_SwitchCharacter` + 独立 `IMC_CharacterSwitchMenu` + Enhanced Input 自己的 `Hold` Trigger + C++ 手动进度与回落
- 2026-03-18：用户进一步澄清后，确认：
  - `DT_PMM_InputAction` 在当前项目里只用来给 `UCommonActionWidget` 提供跨设备图标与提示，不承载角色切换长按逻辑
  - `IA_SwitchCharacter` 当前不应继续保留 `Pressed + Released` 这种短按组合
  - 当前建议把 `IA_SwitchCharacter` 改成 `Hold` Trigger；用户已将 `HoldTimeThreshold` 设为 3.0 秒
  - 回落时间由 C++ 自己控制，初始值建议 0.25 秒
  - `IA_SwitchCharacter` 当前不建议改成 `Hold And Release`
  - `bAffectedByTimeDilation` 当前应保持 `false`
  - 需要新增一份 `RegisterUIActionBinding_学习笔记.md`，明确它在当前项目中不作为菜单独立切换键长按的核心监听方案
- 2026-03-18：同日已在 `UShootCharacterSwitchWidgetBase` 落地菜单长按输入骨架：
  - `IA_SwitchCharacter` 作为蓝图可配的 C++ 成员
  - `UShootCharacterSwitchWidgetBase -> ULyraActivatableWidget -> UCommonActivatableWidget` 这条继承链上，直接复用 `UCommonActivatableWidget::InputMapping`
  - 蓝图子类同时需要把 `UShootCharacterSwitchWidgetBase::SwitchCharacterInputAction` 设成 `IA_SwitchCharacter`
  - Widget 自己绑定 `Started / Ongoing / Triggered / Canceled / Completed`
  - `Triggered` 调 `AShootPlayerController::RequestSwitchCharacter`
  - `Canceled` 触发本地 0.25 秒回落
  - 纯 C++ Widget 若没有蓝图输入事件，激活时需兜底手动创建 `UEnhancedInputComponent`
  - C++ 会根据当前主角自动推导默认目标角色，蓝图只在需要覆盖默认目标时再调用 `SetMenuSwitchTargetGender`
  - 同日晚些时候重新编译通过：`Scripts/Build_Windows.ps1` -> `Result: Succeeded`
- 2026-03-19：继续收口世界入口职责：
  - `UShootGA_Interact` 已删除角色切换专属长按门禁、目标锁定、松手取消与延时读条逻辑
  - `AShootCharacterSwitchStation` 的默认提示改为普通交互文案，不再误导为“长按切换”
  - `UShootGA_WorldCharacterSwitchRequest` 当前明确视为世界入口桥接类
  - 新增 `IShootCharacterSwitchEntry`，让未来“另一位主角 NPC 入口”可复用同一条桥接能力
  - 新增 `AShootCharacterSwitchEntryActorBase`，让站点 / 切换点这类 Actor 入口可复用同一套交互提示与事件包装骨架
  - 当前世界入口先使用 `PressToInteract` 占位，后续若恢复长按，必须放在入口层自己实现
  - 同日重新执行 `Scripts/Build_Windows.ps1`，接口改造、Actor 基类抽取与世界入口收口均编译通过（Result: Succeeded）
- 2026-03-20：完成世界入口桥接类改名收口：
  - 旧 `UShootGA_Interaction_CharacterSwitch` 已正式重命名为 `UShootGA_WorldCharacterSwitchRequest`
  - 源文件路径同步调整为 `ShootGA_WorldCharacterSwitchRequest.h/.cpp`
  - 文档统一改成“世界入口请求桥接类”表述，后续不再把这项工作描述成“修正错误命名”
  - 同日重新执行 `Scripts/Build_Windows.ps1`，改名与引用修正编译通过（Result: Succeeded）
- 2026-03-20：同日继续补齐“另一位主角 NPC”入口的 C++ 骨架：
  - 新增 `AShootCharacterSwitchNPCBase`
  - 该基类直接实现 `IInteractableTarget` 与 `IShootCharacterSwitchEntry`
  - 蓝图子类只需要设置 `RepresentedGender`、外观和待机表现，就能复用当前世界入口桥接链
  - 同日重新执行 `Scripts/Build_Windows.ps1`，新增 NPC 入口骨架编译通过（Result: Succeeded）
- 2026-03-20：同日补齐玩家交互能力的 C++ 显式授予链：
  - `AShootCharacter` 新增 `CoreInteractionAbilities`，默认包含 `UShootGA_Interact`、`UShootGA_Interaction_Collect`、`UShootGA_Interaction_Revive`、`UShootGA_WorldCharacterSwitchRequest`
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded` 会在服务器 `PossessedBy -> LoadProgress` 之后统一兜底授予
  - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing` 负责查重，避免 PlayerState ASC 在重复附身时叠加重复 AbilitySpec
  - `UShootGA_Interact` 构造函数已补 `StartupInputTag = InputTag.Ability.Interact`
  - 同日重新执行 `Scripts/Build_Windows.ps1`，交互授予链改造编译通过（Result: Succeeded）
- 2026-03-20：同日根据用户补充与外部参考，把需求文档进一步收口：
  - 菜单入口明确按《刺客信条：影》的“当前操控角色切换”理解，属于单选型操控权切换
  - 世界入口明确按《刺客信条：枭雄》的安全屋/火车切换理解，属于上下文交互切换
  - 明确记录未来扩展：另一位主角 NPC 后续属于“同伴交互对象”，切换角色只是其中一个交互选项
  - 但本轮继续按方案一执行，不把亲密度、跳舞、送礼、约会等内容混入当前验收范围
- 2026-03-17：收口 CharacterSwitching 文档目录：
  - `DesignSpec_设计规格.md` 的架构原则并入 `Implementation_实现指南.md`
  - `BlueprintNodeFlow_蓝图节点流程图.md` 并入 `BlueprintIntegration_蓝图接线指南.md`
  - `EnhancedInput_Hold_Notes.md` 并入 `LyraInteraction_研究笔记.md`
  - `CodeAudit_代码审计.md` 并入 `CallFlow_调用链路.md`
  - `TransitionScene_过渡切换方案.md` 的关键结论并入 `Implementation_实现指南.md`
  - 目标是减少文件数，但不丢信息密度

文档状态：
- [x] Overview_总览.md
- [x] Requirements_需求.md
- [x] Implementation_实现指南.md
- [x] BlueprintIntegration_蓝图接线指南.md
- [x] Checklist_检查清单.md
- [x] CallFlow_调用链路.md
- [x] LyraInteraction_研究笔记.md

实现状态：
- [x] 运行时切换入口与快照骨架接入（`AShootPlayerState::SwitchToCharacter`）
- [x] 存档结构升级：共享仓库 + per-gender QuickBar/外观 + LastActiveGender
- [x] SaveGameSubsystem 读档恢复 QuickBar/外观并应用当前性别技能套件
- [x] Hub Loadout C++ 入口：`CommitCurrentGenderLoadoutToSave`
- [x] 基础 Hub/安全区校验：`IsInHubOrSafeArea`（SafeAreaVolume + GameState 标记）
- [x] 用安全区 Volume 替换 MapName 兜底判断
- [x] 含 SetByCaller 的 GE 快照策略（保存/恢复 SetByCaller 标签数值）
- [x] 快照中包含基础属性（Health/ShieldCapacity/UltimateCharge）
- [x] 快照中包含主动效果/冷却等高级战斗状态（持续型 GE，含剩余时间/层数/等级）
- [x] 首次进入性别的默认装备/QuickBar 预设策略完善（从共享背包中自动填充）
- [x] 角色切换后端流程（服务器权威切换、快照保存/恢复、落盘）
- [x] 当前存在一条世界入口占位链（仅证明入口与后端已接通，不等于最终架构正确）
- [x] 默认主角调整为男主（可被存档 LastActiveGender 覆盖）
- [ ] 宿舍区蓝图 Widget 接线（长按进度条与双角色展示）
- [ ] 过渡黑屏 / 过渡场景协调层（当前仅完成概念设计与文档）
- [ ] “另一位主角 NPC 交互切换”入口替代当前 `SwitchStation`
- [x] “另一位主角 NPC 交互切换”最小 C++ 骨架
- [x] 需求文档已记录未来“同伴交互对象”扩展方向，但不进入本轮实现
- [x] 把角色切换专属长按逻辑从 `UShootGA_Interact` 中拆出
- [~] CommonUI / ESC 菜单长按切换入口（CommonUI + 增强输入，支持手柄）
- [x] 统一角色切换请求入口（`RequestSwitchCharacter -> ServerRequestSwitchCharacter -> TrySwitchCharacter`）
- [x] 将旧 `UShootGA_Interaction_CharacterSwitch` 重命名为 `UShootGA_WorldCharacterSwitchRequest`
- [x] 玩家交互主能力与执行交互能力的 C++ 显式授予链
- [ ] 继续评估世界入口桥接层是否需要再拆分
- [x] `DA_ShootInputConfig` 已录入 `IA_Interact -> InputTag.Ability.Interact`

下一步：
- 对接给用户的直接下一步只有蓝图与资产配置：
  - 菜单蓝图子类（旧 `UShootCharacterSwitchWidgetBase` 已删除，直接绑定 Controller 委托）：
    - 把 `UCommonActivatableWidget::InputMapping` 设为 `IMC_CharacterSwitchMenu`
    - 绑定 `OnCharacterSwitchHoldProgressChanged` 到进度条
    - 绑定 `OnCharacterSwitchResult` 到结果提示
  - 世界入口蓝图子类：
    - 优先新建蓝图子类继承 `AShootCharacterSwitchNPCBase`
    - 设置 `RepresentedGender`、外观、待机与 Hub 内摆放
  - 然后按 `Checklist_检查清单.md` 做一次完整验收
- C++ 侧当前只剩可选优化：
  - 继续评估世界入口桥接层是否需要再拆成更窄的请求类 / 展示类
  - 再决定旧蓝图 `GA_Interact` 是否退休或改造成继承 `UShootGA_Interact` 的薄包装

注意：
- 角色切换事实以 `Source/NewWorldOrder/Private/Player/ShootPlayerState.cpp` 为准
- 世界入口占位事实以 `Source/NewWorldOrder/Private/Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.cpp` 为准
- 菜单长按/切换事实以 `Source/NewWorldOrder/Private/UI/ViewModel/WardrobeViewModel.cpp` 的 `SwitchCharacter` / `HandleCharacterSwitchResult` 与 `Source/NewWorldOrder/Public/Player/ShootPlayerController.h` 为准（旧 `ShootCharacterSwitchWidgetBase` 已删除）
- 规则来源：`Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md`
- 严格遵守：WeaponInstance/ItemInstance/Fragments/StatTags 为唯一数据源；Actor 仅视觉
