# 机器人伙伴技能设计与验收

日期：2026-09-04

状态：P4 首版垂直切片已落地并通过用户 PIE 验收；下一阶段是表现调优、配置治理与 Zombie 生产化重构

## 1. 目标

为原创 PvE Build Experience 建立第一个正式可玩的召唤技能垂直切片。

玩家获得技能后可以召唤 RadicalMike 机器人伙伴。机器人不是跟随玩家移动的展示 Actor，也不是在 Character Tick 中写几段寻敌代码的假 AI。它必须拥有完整的 AI、动画、战斗、网络和 Match 生命周期，并为后续召唤物技能提供可复用契约。

本任务同时验证四槽 Match Skill Loadout、Experience AbilitySet 授予与回收、技能升级以及伙伴模式切换的真实架构。

## 2. RadicalMike 资产审计

资产位置：`/Game/Characters/RadicalMike`

当前已确认存在：

- `SKM_MegaMikeZ` Skeletal Mesh。
- `SK_MegaMikeZ` Skeleton。
- `PA_MegaMikeZ` Physics Asset。
- 42 个 AnimSequence。
- 左右武器与 PowerPod 静态网格。
- Idle、Walk、Run、RunChase、JumpStart、JumpApex、JumpEnd。
- 左右 Fire、Reload、Buster Equip、Buster Idle。
- 多方向 Hit Register、Death、Punch、Claw、Uppercut 等动作。

当前未发现：

- AnimBlueprint。
- AnimMontage。
- 生产用 Character/Pawn 蓝图。
- AIController、Blackboard、Behavior Tree。
- GAS AbilitySet、攻击 GA、生命与伤害配置。

2026-09-04 通过 Unreal MCP 复核后的精确配置事实：

- 目录共 106 个资产，其中 42 个 AnimSequence；所有动画均使用 `SK_MegaMikeZ` Skeleton、关闭 Root Motion、
  非 Additive，并且没有任何 Notify Event。
- `SKM_MegaMikeZ` 使用 `SK_MegaMikeZ` 与 `PA_MegaMikeZ`，材质槽为 `MikeyZ=MI_ZMike_default`、
  `glass=MI_ZMike_Lens_Blue`。
- Lv1 右武器使用 `SM_RadicalBuster` 挂到 `Buster_RSocket`；Lv3 左武器再挂同一网格到
  `Buster_LSocket`；`SM_PowerPod` 挂到 `PowerPod`。
- SkeletalMesh 只有上述三个 Socket，没有枪口 Socket。正式机器人蓝图必须为左右武器各提供可调的
  Muzzle Scene/Arrow 组件，后续若给静态网格补枪口 Socket，再迁移引用。
- Locomotion 推荐映射：`Idle`/`IdleAggro`，`Walk`/`Walk_Back`/`WalkLeft`/`WalkRight`，
  `Run`/`Run_Faster`，`RunChase`/`RunChase_Faster`，`JumpStart` -> `JumpApex` 或 `Jump` -> `JumpEnd`。
- 武器动画推荐映射：右手 `BusterEquip_R`、`BusterIdle_R`、`FireR`、`ReloadR`，左手使用对应 `_L`；
  受击使用 Front/Back/L/R，死亡主用 `DeathState`。
- 当前没有 AimOffset、Turn-in-place、高速侧跑/后退跑、BlendSpace、Montage、曲线或 BlendProfile；这些是
  AnimBP 实施与 PIE 调姿项，不能把 42 个 AnimationSequence 的存在误写成动画系统已完成。

这些动画都使用 `SK_MegaMikeZ` Skeleton，当前检查的移动、跳跃、射击、换弹、受击和死亡动画均关闭 Root Motion，适合先由 CharacterMovement 驱动位移，再由 AnimBP 根据速度和移动状态驱动表现。

## 3. 现有 Zombie 实现的可复用边界

可以借鉴：

- `AModularAIController` 基类选择。
- AI Perception 与统一 Team Attitude 过滤。
- Controller 负责初始化 Blackboard 和启动 Behavior Tree。
- Character 服务器权威校验，伤害通过 GAS/GE 结算。
- ASC、阵营和关键表现状态的网络复制思路。

不能复制：

- `EShootEnemyTestAnimationState` 三态表现模型。
- `PlayAnimation` Single Node 回退路径。
- 行为树任务执行时立即结算伤害的攻击时序。
- 只有最近目标、追击、近战、随机巡逻的两分支行为树。
- `Test*` archetype 字段和测试 Spawner 生命周期。

2026-09-04 对运行资产的复核确认 `/Game/AI/Zombie` 只有 `BP_EnemyBotCharacter`、
`BP_EnemyBotController`、`BB_Zombie_Melee`、`BT_Zombie_Melee` 四项。行为树只有“最近敌对目标追击近战”与
“出生点附近随机巡逻”两条分支；五个 Zombie Archetype 的 `AnimClass` 都为空，继续由 C++ `PlayAnimation`
切换 Single Node。它是可工作的服务器权威近战 POC，但缺少威胁评分、Owner/指令模式、射程维持、LOS 射击位、
换弹、枪口、动画 Notify 时序和死亡后显式停止 Brain，不能继承为正式机器人。

旧 Zombie 是联网战斗 POC，不是机器人伙伴模板。机器人可以复用底层契约，但必须建立自己的正式 Pawn、AI 和动画资产。

## 4. Lyra ShooterCore/Bot 对照结论

Lyra 的 `/ShooterCore/Bot` 不只是一个 Controller。它包含：

- Bot Controller Blueprint。
- Blackboard 与 Behavior Tree。
- `EQS_AIPerceptionEnemy`、`EQS_FindTarget`、`EQS_MoveAgainstEnnemy`、`EQS_FindWeapon` 等 EQS。
- `BTS_CheckAmmo`、`BTS_ReloadWeapon`、`BTS_SetFocus`、`BTS_Shoot` 等行为树 Service。
- Bot Creation Component、PlayerState、PawnData、ASC、武器和正式角色动画栈的协作。

Lyra 的玩家 Bot 能复用玩家 Pawn 与动画，是因为 AIController 只替代玩家输入和决策来源，Pawn、ASC、武器及 AnimBP 契约本来就是完整的。不能据此推导出“只要换 AIController，任何 NPC 就自动拥有完整 AI 和动画”。

本项目应借鉴 Lyra 的职责分层和数据装配，不直接复制 ShooterCore 的 PvP 选枪、控制点和射击 EQS。机器人伙伴需要围绕 Owner、指令模式、协同目标、召唤生命周期和 PvE 敌我关系重新设计决策树。

## 5. 强制运行时架构

### 5.1 召唤与生命周期

调用链目标：

```text
Match Skill Definition
-> Experience / Match AbilitySet 授予召唤 GA
-> 玩家激活 GA
-> Server Spawn Robot Character
-> Server Spawn / Possess Robot AIController
-> 初始化 Owner、Team、ASC、Blackboard、Behavior Tree、AnimBP
-> Match End / Skill Replace / Experience Unload / Owner Leave 时统一回收
```

要求：

- 生成、模式变更、目标选择、伤害和销毁以服务器为权威。
- 客户端只负责输入请求、UI 与预测允许范围内的表现。
- 召唤物必须归属明确的 Owner PlayerState 或稳定 Owner Actor，不通过 `GetFirstPlayerController` 查找主人。
- 机器人属于局内技能数据，不写入 SaveGame；Match 结束或离开副本后必须清理。
- AbilitySet、GA 内无限 GE、召唤 Actor 和委托都要进入可撤销句柄或明确的清理链。
- Owner 死亡、Pawn 重生、玩家掉线、技能替换和 Experience 卸载都要定义结果，不能依赖 Actor 恰好被地图销毁。
- 机器人被击毁后立即进入本技能冷却；冷却结束后才允许再次召唤。
- Owner 死亡时机器人执行服务器权威的自爆/销毁流程，不跨死亡保留到新 Pawn。
- 自爆可以复用正式技能表现与伤害链，但无论表现资源是否就绪，都必须保证 Actor、Controller、委托和目标引用被清理。

实施前核实到的两个强制修复点：

- `AShootCharacterBase` 当前没有广播死亡委托；`ICombatInterface` 虽声明了 `FOnDeathSignature`，但获取接口仍被注释。
  首步应恢复幂等的 DeathStarted 委托，并只在 `NotDead -> DeathStarted` 成功跃迁时广播一次。禁止轮询 Health 或 OnRep 猜死亡。
- `AShootGameModeBase::PlayerDied` 当前只完整处理 `AEnemyBotCharacter` 或玩家 Controller。机器人若只调用基类
  `Die()` 会停在 DeathStarted 而不销毁。因此 Robot Character 必须拥有独立的停止 Brain、FinishDeath、尸体计时与销毁链，
  不能进入 Zombie 重生分支，也不能依赖地图销毁。

### 5.2 Character 与战斗

正式机器人 Character 至少负责：

- CharacterMovement 与 NavMesh 移动。
- 自身 ASC、Health、Damage、Death 和必要的 AttributeSet。
- 通过 `ILyraTeamAgentInterface` 与项目统一敌我识别链工作。
- 武器挂点、枪口位置和服务器射击入口。
- 受击、死亡和禁用状态。
- 将战斗事实暴露给 AnimBP，不把行为树决策写入动画蓝图。

攻击必须通过 GA 或同等服务器权威的 GAS 链结算。射击动画 Notify 只负责动作时序和表现窗口，最终命中、敌我过滤与伤害仍由服务器验证。

### 5.3 AIController、Blackboard 与 Behavior Tree

必须创建独立的机器人 AIController、Blackboard 和 Behavior Tree。

Blackboard 至少需要表达以下概念，最终键名在实施前结合项目现有命名审计：

- Owner Actor。
- Command Mode。
- Command Location。
- Combat Target。
- Last Known Target Location。
- Desired Follow Location。
- Target In Attack Range。
- Has Line Of Sight。
- Is Disabled / Is Dead。

行为树至少覆盖：

```text
Dead / Disabled
-> 停止战斗与移动

Mode Command
-> 计算当前允许活动区域与目标偏好

Acquire Target
-> Team Attitude 过滤
-> 距离、视线、威胁和 Owner 指令评分

Combat
-> 保持有效射程
-> 重新站位
-> 转向与瞄准
-> 射击 / 换弹 / 冷却

No Target
-> 跟随 Owner 或守卫 Command Location
-> 距离过远时执行可靠回归
-> Idle
```

EQS 不是为了显得复杂而强制添加。需要在“保持射程、寻找有视线射击位、绕开 Owner、选择定点防守站位”等空间决策中使用时再加入；如果普通 NavMesh 查询足以满足某一节点，就不额外建立空壳 EQS。

### 5.4 三种模式最终语义

2026-09-05 根据实际 PIE 将旧“跟随/突击/守点”替换为三种有明确攻击差异的模式：

1. 远程压制：只使用枪械，单发伤害低、攻击频率高，主动从机器人周围搜索 Hostile。
2. 近战强袭：主动贴近目标，进入 250cm 后用左爪攻击，每第三次近战改用咬击；单次伤害高、攻击间隔长。
3. 均衡护卫：从 Owner 周围选择威胁，距离合适时射击、贴身时近战，追出 1400cm 后优先回到 Owner。

权威标签分别为 `Ability.Mode.Robot.Ranged/Melee/Balanced`。旧 Follow/Assault/Guard 标签不再用于机器人资产，避免 UI 名称与实际攻击策略相互误导。敌我筛选仍统一使用 Team Attitude，不建立第二套阵营系统。

`FShootSkillSlot::CurrentModeTag` 已存在并 OwnerOnly 复制，但尚无 Set/Cycle API。正式实现以 SkillLoadout 槽位为玩家技能状态的
单一来源：服务器校验目标 Tag 必须属于 Definition 的 `ModeTags`，更新槽位并广播；机器人只复制一份镜像给世界表现与 Blackboard。
Blackboard 不能反过来成为权威状态。

### 5.5 AnimBP 与动画链

必须为 `SK_MegaMikeZ` 创建正式 AnimBP。最低要求：

- Idle / Walk / Run Blend。
- Acceleration、Deceleration、Direction 和 Turn 表现。
- JumpStart / JumpLoop or Apex / JumpEnd。
- 左右武器上半身或全身射击 Slot。
- Reload、Equip。
- 多方向 Hit React。
- Death，死亡后不回到 Locomotion。
- 攻击、换弹、受击和死亡的打断优先级。
- CharacterMovement 速度与动画播放速度匹配。

射击、换弹、受击和死亡应通过 AnimMontage 或明确的 AnimGraph 状态组织，关键时序使用 Notify/NotifyState。禁止继续用 Character C++ 每次调用 `PlayAnimation` 切换整个 Mesh。

### 5.6 UI 与输入

- 机器人召唤技能作为四槽 Match Skill 的一个 Skill Definition。
- 召唤 GA 与机器人射击 GA 都必须提供 `/Game/Blueprints/Skills/Abilities` 下的蓝图子类入口；对应 AbilitySet
  授予蓝图生成类。网络权威、GAS Commit、伤害与清理规则保留在 C++ 父类，模型、动画、特效、音效、冷却和可调参数
  由蓝图 Class Defaults 配置，禁止再次只把原生 C++ Class 塞进 AbilitySet 让编辑器侧无处覆写。
- 技能栏显示召唤状态、冷却、当前等级和当前机器人模式。
- 默认技能键位仍由 Input Action、Input Mapping Context 与 InputTag 链驱动，支持玩家改键。
- 模式切换不能在 C++ 中硬编码键盘按键。优先实现 CommonUI 三模式指令轮盘；若第一版轮盘成本或稳定性不满足要求，则采用“再次按机器人技能键循环切换模式”的降级方案。
- 使用重复技能键降级方案时，技能槽图标与模式文案必须即时变化，让玩家明确知道当前模式。
- 本地分屏下 UI 与输入必须绑定目标 LocalPlayer，不能使用玩家 0 兜底。

## 6. 技能等级建议

第一版按以下已确认规则验证三级 Skill，而不提前引入随机变异系统：

- Lv1：使用右侧武器的基础机器人。
- Lv2：提高生命、伤害或射击效率，保持单武器形态。
- Lv3：升级为双武器形态，并继续提高战斗能力。

三种指令模式不与技能等级绑定；技能一旦可用就应能切换模式。具体数值需要 PIE 后按玩法反馈调整。等级差异放 Skill Definition、AbilitySet 或数据资产，不在 Character 构造函数中硬编码资产列表和成长数组。

## 7. 不可降级验收门槛

以下任一项缺失，都不能把任务标记为“机器人技能完成”：

- 正式 Robot Character/Pawn。
- 独立 AIController。
- Blackboard。
- Behavior Tree。
- 正式 AnimBP 与 Locomotion StateMachine。
- 射击、换弹、受击、死亡的动画链与关键 Notify。
- GAS/GE 伤害和统一敌我过滤。
- 服务器权威 Spawn、Possess、模式、目标和伤害。
- Listen Server Host 与 Remote Client 一致表现。
- Owner 重生、掉线、技能替换与 Match End 清理。
- 四槽技能系统中的授予、升级、替换和 UI 状态。

不能接受的替代品：

- 只有模型跟随玩家。
- Character Tick 内硬编码找最近敌人。
- 有 Behavior Tree 资产但运行时未真正启动。
- 只有 Idle/Run/Attack 三段 `PlayAnimation`。
- 客户端本地 Spawn 或每端各生成一个机器人。
- 直接修改敌人 Health 绕过 GAS/GE。
- 用 `IsPlayer`、固定 TeamId 分支或 `GetFirstPlayerController` 代替项目统一接口。

## 8. 分阶段实施与验证

### 阶段 A：架构对账

- 修正 Experience AbilitySet 授予、被动自动激活和完整撤销生命周期。
- 明确四槽 Match Skill Loadout 的 Owner、复制和清理位置。
- 审计现有 GameplayTag、InputTag、属性和 UIExtension，避免第二套系统。

验证：退出 Experience 后能力、无限 GE、动态 AttributeSet 和召唤 Actor 全部按定义清理。

### 阶段 B：机器人 Pawn 与动画垂直切片

- 创建 Robot Character、AIController、Blackboard、Behavior Tree 和 AnimBP。
- 先完成跟随、索敌、保持射程、射击、受击、死亡。

验证：Idle/Walk/Run/Attack/HitReact/Death 连贯，Host 与 Remote Client 一致，伤害只由服务器产生一次。

实施代码边界已经收敛为：

- `UShootRobotCompanionComponent` 位于 `AShootPlayerState`，持有唯一机器人实例、等级、清理原因和绑定句柄；负责
  Skill Replace、Experience Unload、Owner Logout、Owner Pawn Death/Respawn，不把机器人生命属性放进玩家 ASC。
- `AShootRobotCompanionCharacter : AShootCharacterBase` 自有 ASC/AttributeSet，复制 Owner PlayerState、模式和指令位置，
  管理武器挂点、枪口与自身死亡终结，不承载寻敌决策。
- `AShootRobotCompanionController : AModularAIController, ILyraTeamAgentInterface` 自有 Perception/Blackboard/BT，
  Team 从 Owner PlayerState 派生并监听变化，禁止固定 Team 1 或复用 Zombie 默认 Team 2。
- 召唤 GA 只负责服务器请求召唤或模式切换；组件通过 Deferred Spawn 注入 Owner/等级/模式后再 FinishSpawning、
  SpawnDefaultController 并校验 Possess，避免一帧 Neutral、BT 无 Owner 或重复 Controller。
- 首版 Robot Fire GA 使用服务器权威 hitscan，先验证 Hostile、目标、距离与 LOS，再用机器人 ASC 创建并应用现有
  Damage GE。玩家武器 GA 强依赖 WeaponInstance、Inventory、Equipment、准星与本地预测，不能直接移植。
- 若以后必须使用投射物，应单独把 `AShootProjectileBase` 对 `UShootRangedWeaponInstance` 的强依赖泛化并回归七把正式武器；
  不在机器人首版顺手扩大风险面。

### 阶段 C：三模式与技能等级

- 接入模式指令、目标标记、定点防守。
- 接入 Lv1-Lv3 与四槽技能 UI。

验证：模式能改变行为树决策而不重生机器人；升级不产生重复 Ability、GE、Controller 或 Actor。

### 阶段 D：玩法与压力验收

- 多玩家各自召唤机器人。
- 大量 Zombie、导航阻塞、Owner 死亡/重生、掉线和中途加入。

验证：无串 Owner、串目标、重复伤害、遗留 Actor、导航永久卡死或动画状态卡死。

## 9. 当前未决产品项

- CommonUI 指令轮盘留到三模式数值与行为验收稳定后评估；当前使用已确认的重复技能键方案。
- 机器人外观固定使用哪一套 RadicalMike 材质与左右武器组合。
- Owner 死亡自爆是否造成范围伤害，以及该伤害是否会影响 Owner 的友方单位。
- 已确认存续期随等级成长：Lv1/Lv2/Lv3 为 30/35/40 秒。冷却仍是到期自爆或战斗击毁后开始的 20 秒。
  未来减冷却构筑若允许召唤物并存，应另行定义实例上限，而不是依赖硬编码时间关系。

## 10. 本批编码与单次编译边界

本批按以下边界一次性完成后才触发项目编译：

1. 基类幂等死亡委托与机器人独立死亡终结。
2. PlayerState 伙伴组件、Robot Character、Robot AIController 及服务器 Spawn/Possess/统一清理。
3. Blackboard 与最小 Behavior Tree：跟随、敌对目标、保持射程、射击、无目标回归。
4. 服务器权威 Robot Fire GA/GE 与一次伤害链。
5. RadicalMike AnimBP、Locomotion、Fire/Death Montage 与 Notify 时序；Reload/HitReact 留到 P4.1 表现增强。
6. 三模式槽位 Set/Cycle、机器人复制镜像与技能栏当前模式表现。
7. Lv1-Lv3 数据配置；升级沿用唯一机器人实例，权威更新血量、伤害、射速与 Lv3 左武器，不重复授予 Ability 或生成 Controller。
8. 文档与一次性 PIE 验收矩阵。

本阶段所有正式 GA 在接入 AbilitySet 前必须先创建并编译蓝图子类，并在状态文档登记“蓝图资产 -> C++ 父类 ->
源文件”的映射。这样用户可从 Content Browser 直接定位入口，同时不会把复杂网络逻辑复制到蓝图中。

不得在上述链路中间为单个类反复编译或逐项 PIE。资产/代码/文档先批量完成，阶段末只做一次构建，再交由用户统一验收。

## 11. 2026-09-04 P4 首版实际落地

### 11.1 可维护资产入口

| 职责 | 蓝图或数据资产 | C++ 父类或实现 |
|---|---|---|
| 玩家召唤与模式切换 GA | `/Game/GameFramework/Skills/Abilities/Robot/GA_RobotCompanion` | `UShootGA_RobotCompanion` |
| 机器人射击 GA | `/Game/GameFramework/Skills/Abilities/Robot/GA_RobotFire` | `UShootGA_RobotFire` |
| 机器人 Character | `/Game/AI/Robot/Blueprints/BP_RobotCompanionCharacter` | `AShootRobotCompanionCharacter` |
| 机器人 AIController | `/Game/AI/Robot/Blueprints/BP_RobotCompanionController` | `AShootRobotCompanionController` |
| 玩家四槽 AbilitySet | `/Game/GameFramework/Skills/AbilitySets/DA_AbilitySet_Skill_RobotCompanion` | 授予 `GA_RobotCompanion` |
| 机器人自身 AbilitySet | `/Game/GameFramework/Skills/AbilitySets/DA_AbilitySet_RobotCombat` | 授予 `GA_RobotFire` |
| 技能目录 | `/Game/GameFramework/Skills/Definitions/DA_Skill_RobotCompanion` | `UShootSkillDefinition` |
| 固定交互来源 | `/Game/Gameplay/Interactables/SkillAcquisition/BP_RobotSkillPickup` | `AShootRandomSkillPickup`，使用 `OfferedSkillDefinition` |

- 机器人蓝图 Class Defaults 是网格、武器、PowerPod、蒙太奇、行为树、机器人 AbilitySet、数值和挂点的可视化调优入口。
- 两个 GA 都有蓝图子类。网络权威、GAS、伤害和生命周期留在 C++，具体机器人类、冷却 GE 和 Cue Tag 可从蓝图定位和覆盖。
- `Scripts/RobotCompanion_CreateAssets.py` 是本批资产装配的可重复执行脚本；它不启动 PIE，也不会重复放置同名固定拾取物。

### 11.2 AI、动画与战斗链

- `/Game/AI/Robot/BB_RobotCompanion` 包含 Owner、Target、移动点、守点位置、模式、目标/射程/LOS 和死亡状态。
- `/Game/AI/Robot/BT_RobotCompanion` 为 Selector：攻击序列、移动、空闲等待；根节点挂
  `UShootBTService_RobotRefreshContext`，射击节点只激活机器人自身 GAS Ability。
- 感知候选必须实现 `IAbilitySystemInterface`，敌我关系统一通过 `ILyraTeamAgentInterface` 的 Team Attitude 解析；
  `ICombatInterface` 只用于通用死亡状态过滤，没有玩家类或 Zombie 类硬编码判断。
- `/Game/Characters/RadicalMike/Animations/ABP_RobotCompanion` 继承 `UShootRobotAnimInstance`。父类
  `UShootRobotAnimInstance` 在 Native Update 中维护 GroundSpeed、bIsMoving、bIsFalling、bIsDead；AnimBP 消费这些事实，
  使用 Idle/Walk/Run/Airborne/Death 状态机，并通过 `DefaultSlot` 接入 Fire/Death Montage。EventGraph 为空不是 Tick 缺失。
- `AM_Robot_FireR` 与 `AM_Robot_FireL` 在 0.25 秒放置 `UShootAnimNotify_RobotFire`。命中、伤害 GE 和 Cue 只在服务器执行；
  0.22 秒计时器仅用于 Notify 被错误移除时防止 GA 永久挂起。
- Fire、SelfDestruct 与 SummonImpact GameplayCue 位于 `/Game/Effects/GameplayCues/Abilities/RobotCompanion`，分别复用
  `P_ky_shotShockwave`、`P_ky_explosion` 和落地组合表现，均有 1.5 秒强制清理。项目在
  `Config/DefaultGame.ini` 的 `AbilitySystemGlobals` 与 UE 5.8 `GameplayAbilitiesDeveloperSettings` 两个兼容配置段中都把
  `/Game/Effects/GameplayCues` 加入 `GameplayCueNotifyPaths`，因此迁移后的生产目录属于运行时有效扫描路径。

### 11.3 Match 生命周期与三模式

- `UShootRobotCompanionComponent` 固定创建在 `AShootPlayerState`，每名玩家最多持有一个机器人。
- 第一次按技能槽生成机器人；已有机器人时再次按同一技能键，按远程压制、近战强袭、均衡护卫循环切换，槽位名称和图标随
  `FShootSkillSlot::CurrentModeTag` 的 OwnerOnly 复制结果更新。
- 机器人 Lv1 默认存在 30 秒，每级增加 5 秒，Lv2/Lv3 分别为 35/40 秒。到期执行与 Owner 死亡同一套服务器权威范围自爆表现与伤害，然后施加 20 秒重召冷却。
  冷却从到期时开始，而不是召唤时开始：同一技能键在存续期间还承担模式切换，若召唤时直接挂普通 GAS 冷却，GA 会被
  `ActivationBlockedTags` 拦截而无法下达模式指令。以后若模式指令迁到独立 CommonUI 轮盘，再评估召唤即计时模型。
- 战斗击毁通过 Character 死亡链明确通知 PlayerState 组件，施加 20 秒 `Cooldown.Skill.RobotCompanion`；
  冷却期间不能重新召唤，结束后允许再次生成。
- Owner 死亡走显式 `OwnerDeath` 清理原因并播放自爆表现，默认不开始冷却；技能被移除和 Experience 卸载分别走
  `SkillRemoved`、`ExperienceUnload`，直接清理且不产生跨地图冷却。`OnDestroyed` 只收尾引用，不猜销毁原因。
- Lv1 为右武器；Lv2 增加生命、伤害并缩短射击间隔；Lv3 显示左武器并交替枪口。升级更新唯一实例，不生成第二个机器人。
- `AShootRobotCompanionCharacter` 定义 `RightWeaponSocket`、`LeftWeaponSocket`、`PowerPodSocket`，蓝图子类
  `BP_RobotCompanionCharacter` 分别配置为 SkeletalMesh 自带的 `Buster_RSocket`、`Buster_LSocket`、`PowerPod`。PowerPod 位于背部，Lv1/Lv2 使用右手
  Buster 与 `AM_Robot_FireR`；Lv3 显示左手 Buster，并按实际枪口在 `AM_Robot_FireR` 与 `AM_Robot_FireL` 间交替。
  三个组件的相对 Transform 仍由该蓝图调优，不把美术偏移硬编码到 C++。

### 11.4 获取与 UI

- 机器人 Definition 已加入 `DA_SkillLoadoutConfig_PVE.RandomSkillPool`，普通随机技能球可以抽到。
- `TestMap_ListenServer` 与 `TestMap_SplitScreen` 还各放置一个 `RobotSkillPickup`，用于无需控制台即可稳定获得或升级机器人。
- 远程压制、近战强袭、均衡护卫首版临时复用 TacticalScan、TacticalAssault、SteelBulwark 图标；这是可替换美术配置，不是最终图标。
- `W_SkillSlot.CooldownFrame` 的 `M_UI_RadialProgress_Ability` 继续保留。C++ 从同一个 GAS ActiveGE 的
  Remaining/Duration 推导 `TimerStart` 与 `Duration`，与 `RemainingCooldownTime` 共用权威时间，不维护第二套 UI Timer。

### 11.5 本批尚未伪装为完成的部分

- 2026-09-04 已使用 `Scripts/Build_Windows.ps1` 对本节 11.7 的攻击、范围伤害、Airborne 和装备修正完成一次
  `NewWorldOrderEditor Win64 Development` 冷构建，结果为 `Succeeded`，耗时 587.90 秒。
- 编辑器冷启动后已重新读取并确认：行为树校验无错误，攻击 Decorator 为 `LowerPriority`；Airborne 回落条件为
  `!bIsFalling`；机器人蓝图使用 `Buster_RSocket`、`Buster_LSocket`、`PowerPod`；四个范围伤害默认值为
  45/350cm 与 120/450cm；机器人 Character、Controller、两项 GA、三个 GameplayCue 蓝图均为最新编译状态。
- 上述结论只证明编译、序列化和静态装配正确。机器人能否在真实 NavMesh、多人网络和尸群遮挡下持续索敌、射击、结算伤害，
  仍以第 12 节 PIE 为准，未把静态验证伪装成运行验收。
- 首版按已确认的降级方案使用“重复技能键循环模式”，尚未制作 CommonUI 指令轮盘。
- RadicalMike 已有 Fire/Death 正式 Montage；Reload、HitReact 和最终机器人专用图标属于 P4.1。
- Zombie 仍保留旧单节点动画/Character Tick。机器人 PIE 通过后再抽取可复用 Team、黑板和 AnimBP 契约，替换 Zombie 旧实现。
- `AShootCharacterBase` 与 `ICombatInterface` 的 Aura 历史残留不在本批清理。机器人只恢复了所有战斗角色都需要的死亡委托；
  P4 验收后对照玩家、Zombie、机器人三方再决定哪些上提父类，避免边做功能边大改基类。

### 11.6 首次 PIE 输入故障与修复

- 2026-09-04 用户确认机器人技能无论进入哪个槽位都无法激活，而其他技能在同一槽位正常，因此故障边界在机器人技能装配，
  不在 IMC、Q/E/C/X 顺序或通用 SkillLoadout 输入分发。
- 编辑器资产回读显示 `DA_AbilitySet_Skill_RobotCompanion` 的 `GA_RobotCompanion` 条目 `InputTag` 为空；旧授予逻辑却把
  “条目已有固定 InputTag”当作允许槽位覆盖的隐式标记，导致该 GA 没有获得实际槽位标签。
- 第一次修复曾给机器人条目写入 `InputTag.Q` 作为占位。该方案虽会被运行时覆盖，却错误表达了机器人固定属于 Q，且无法清晰
  区分混合 AbilitySet 中的主动与被动条目，已按用户审查意见撤销。
- 最终修复不增加额外字段：`UShootSkillLoadoutComponent` 是唯一传入 `InputTagOverride` 的调用方，
  `UShootAbilitySet::GiveToAbilitySystem` 收到有效覆盖值时直接用于本次授予。机器人和其他 Match Skill 条目保持空 InputTag，
  技能实际进入任意槽位时才获得 Q/E/C/X；其他调用方不传覆盖值，固定输入与无输入能力均不受影响。
- `Scripts/RobotCompanion_CreateAssets.py::make_ability_entry` 同步保持机器人玩家 AbilitySet 的 InputTag 为空，防止重跑脚本倒退；
  `DA_AbilitySet_RobotCombat` 的 `GA_RobotFire` 同样为空标签，但它由 BehaviorTree 按 Ability Tag 激活。
- 第一次召唤成功后不进入冷却；只有机器人被敌人击毁才施加 20 秒冷却。若只看到“召唤后没有冷却”，不是故障。

### 11.7 首次 PIE 攻击故障与召唤反馈修正

- 用户首次攻击验收确认机器人只跟随 Owner，在近距离尸群中没有攻击。BehaviorTree、Blackboard、RobotCombat AbilitySet 和
  `GA_RobotFire` 资产均通过静态回读，故障边界在感知候选与 Fire 分支之间。
- 直接行为树原因是攻击 Sequence 的 `TargetInAttackRange` Blackboard Decorator 原为 `FlowAbortMode=None`：机器人一旦进入
  低优先级 MoveTo，目标随后进入射程也不会抢占移动任务。现改为 `LowerPriority`，让射程条件变真时立即中止移动并进入 Fire。
- `AShootRobotCompanionController::SetGenericTeamId` 在继承 Owner PlayerState TeamId 后调用
  `UAIPerceptionComponent::RequestStimuliListenerUpdate`。这是对齐 Lyra PlayerBotController 的必要处理，因为 Perception Listener
  会缓存队伍信息；只改 Controller 的 TeamId 而不刷新 Listener，感知过滤可能继续使用旧 NoTeam。
- `AEnemyBotCharacter` 增加 `UAIPerceptionStimuliSourceComponent` 并显式注册 Sight，机器人不再依赖引擎项目级
  “自动注册全部 Pawn”隐式配置。
- `GetCurrentlyPerceivedActors(UAISense_Sight)` 返回的候选已经代表当前 Sight 成立；黑板不再额外调用 `LineOfSightTo` 形成
  第二套可能冲突的可见性门槛。最终是否能命中仍由服务器 `GA_RobotFire` 的 Visibility Trace 决定，不会穿墙。
- 尸群遮挡时，射线首先命中的另一只 Team Hostile 现在承担伤害；若命中环境、友方或无 ASC 对象则仍不结算。
- 首次召唤从蓝图可调的 `SummonDropHeight` 高处生成，默认 500cm。机器人自然落地后用 ImpactPoint 执行
  `GameplayCue.Skill.RobotCompanion.SummonImpact`；Cue 蓝图同时配置 `NS_ImpactConcrete` 和缩放后的
  `P_ky_explosion`，资产引用仍在蓝图而非 C++ 构造函数。
- `AShootRobotCompanionCharacter` 定义蓝图可调的 `SummonImpactDamage/Radius` 与 `SelfDestructDamage/Radius`；默认分别为
  45/350cm 与 120/450cm。落地与 Owner 死亡自爆都由服务器球形查询，统一用 Team Attitude 仅筛选 Hostile，并通过机器人
  ASC 创建现有 Damage GE；不直接扣血，也不按玩家/Zombie 具体类型分支。
- AnimBP 已补 Airborne 状态，使用 `Anim_ZMIKE_JumpApex`；Idle/Walk/Run 在 bIsFalling 时进入，落地时通过
  `!bIsFalling` 回到 Idle，死亡仍可优先进入 Death。`DefaultSlot` 保留给左右射击和死亡 Montage。
- 2026-09-05 攻击模式与数值调整：旧三模式只改变活动范围、近战由统一距离分支偶发触发，玩家无法感知模式差异。
  现在远程模式永不自动切近战，近战模式在贴近前只追击不射击，均衡模式才按距离择招；均衡目标评分以 Owner 为中心，
  远程/近战以机器人为中心。三者继续共用现有 AIController、Blackboard、BehaviorTree 和两项机器人 GA，没有新增 Tick 寻敌。
- 远程基础单发由 12/17/22 调整为 8/10/12，基础射击间隔为 0.7/0.65/0.6 秒；远程模式再乘 0.6，均衡模式乘 0.9。
  近战左爪为 32/38/44，间隔 1 秒；近战模式每第三击咬击为 1.35 倍伤害、间隔 1.35 秒。目标是让玩家主武器继续承担主输出，
  机器人远程提供稳定副输出，主动承担贴身风险的近战才获得更高单次伤害。最终平衡必须用玩家正式武器实际 DPS 做同场回归，
  不能只比较伤害飘字，因为玩家 GE、命中部位、射速和霰弹弹丸数都会改变最终输出。
- 机器人先以 18 秒完成了有限存续链验证，现按用户体验反馈调整为 30 秒基础存续、每级增加 5 秒；到期自爆后开始 20 秒冷却。战斗击毁仍立即开始同一冷却，Owner 死亡默认自爆但不额外惩罚冷却；
  SkillRemoved 与 ExperienceUnload 仍静默回收。`UShootRobotCompanionComponent` 用显式 `LifetimeExpired` 原因管理计时与清理，
  `OnDestroyed` 只释放引用和 Timer，不猜销毁原因。

### 11.8 2026-09-05 构建、资产回读与 PIE 证据

- 正式 `NewWorldOrderEditor Win64 Development` 构建第一次被项目的 warning-as-error 规则拦截：局部变量 `Tags` 遮蔽
  `AActor::Tags`。改名为 `GameplayTags` 后增量构建 5/5 成功，最终 DLL 链接完成；这次失败与修复均已保留在构建记录中。
- 重启编辑器后重新执行 `Scripts/RobotCompanion_CreateAssets.py`。MCP 外层在 30 秒超时，但编辑器日志确认脚本完整执行到
  `Robot companion asset pipeline completed`，且相关 BehaviorTree、AnimBP、Montage、GA、GameplayCue、AbilitySet、Definition、
  固定拾取物全部通过 AssetCheck。
- 资产回读确认 Definition 的模式顺序为 Ranged、Melee、Balanced，中文名分别为远程压制、近战强袭、均衡护卫；机器人蓝图
  的三组 Aggro/DesiredRange、远程与近战数值均与 11.7 一致，召唤 GA 蓝图的 `CompanionLifetime=18`。
- 上一轮同一攻击链的自然 PIE 已测得：机器人在 1007cm 目标前的朝向点积约 1.0，目标护盾从 78.12 降至 18.12；将已感知
  Hostile 放到约 150cm 后目标被近战链销毁。该证据证明朝向门槛与服务器伤害链工作，但不能替代本轮三模式手感验收。
- 本轮 PIE 单独验证了 18 秒到期链：机器人到期后被清理，槽位查询返回约 7.39/20 秒剩余冷却，证明 `LifetimeExpired ->
  SelfDestruct -> Cooldown` 已执行。敌人攻击关闭与技能自动测试准备不稳定，因此没有把本轮三模式攻击选择伪装成已通过；仍交由
  第 12 节集中复验。

### 11.9 配置归属与编辑入口

- 机器人相关内容不全部放在 `/Game/AI`。`/Game/AI/Robot` 只保存 Pawn、Controller、Blackboard、BehaviorTree、AnimBP 和 Montage；
  玩家技能装配放在 `/Game/GameFramework/Skills`，这是按运行时职责分域，不是资产遗漏。
- `/Game/GameFramework/Skills/Definitions/DA_Skill_RobotCompanion`：名称、说明、技能图标、等级上限、模式名称/图标与 AbilitySet 引用。
- `/Game/GameFramework/Skills/Abilities/Robot/GA_RobotCompanion`：继承 `UShootGA_RobotCompanion`，配置机器人类、基础存续 30 秒、每级增加 5 秒、召唤高度、冷却 GE 与 Cue Tag。
- `/Game/GameFramework/Skills/AbilitySets/DA_AbilitySet_Skill_RobotCompanion`：把玩家侧召唤 GA 装入任意 Match Skill 槽；AbilitySet 条目不写固定 Q/E/C/X。
- `/Game/AI/Robot/Blueprints/BP_RobotCompanionCharacter`：继承 `AShootRobotCompanionCharacter`，配置 Mesh、AnimBP、武器、生命、远程/近战伤害、攻击间隔、模式活动范围和自爆/落地伤害。
- `/Game/GameFramework/Skills/Abilities/Robot/GA_RobotFire` 与 `GA_RobotMelee`：机器人 ASC 使用的攻击 GA 蓝图壳；复杂网络与伤害逻辑在各自 C++ 父类中，Montage/Cue 等资产引用留在蓝图。
- `UShootEffect_RobotCompanionCooldown` 是 `Source/NewWorldOrder` 中的原生 GE，因此 Content Browser 不会出现同名 `.uasset`；当前固定 20 秒。
- `UShootEffect_RobotCompanionVitals` 同样是 `Source/NewWorldOrder` 中的原生 GE，定义在
  `Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_RobotCompanion.h`，实现位于对应 Private 目录；它负责把机器人
  Character 蓝图上的生命配置初始化到 ASC，所以 `/Game/AI/Robot` 下也不会出现同名 GE 资产。
- 当前没有把存续、攻击、生命和 UI 合并成一张总表。GA 蓝图负责召唤生命周期，Character 蓝图负责战斗体，Definition 负责玩家技能目录；
  只有出现多个机器人 archetype 或多套 Experience 平衡预设时，才建立专用机器人配置 DataAsset，并一次性迁移权威字段，禁止新增一份重复表格。
- 当前完整编辑入口按职责分为：`/Game/AI/Robot` 的 AI 脑与角色壳、`/Game/GameFramework/Skills` 的玩家技能装配、
  `/Game/Effects/GameplayCues/Abilities/RobotCompanion` 的表现 Cue，以及 `Source/NewWorldOrder` 的网络/GAS 原生实现。
  因此不能以“机器人”为名把 GA、GE、Definition 和 Cue 全部搬回 `/Game/AI/Robot`；若以后为了可发现性调整技能目录，必须先讨论
  目标树并在同一批完成引用更新、Redirector 清理与 PIE。

### 11.10 用户验收结论

- 2026-09-05 用户确认机器人伙伴首版 PIE 验收通过，包括三种攻击模式、远近程攻击、召唤/到期/自爆、范围伤害与冷却主链。
- 本次把默认存续从历史验证值 18 秒改为 30/35/40 秒；该数值变化需要按第 12 节第 6 项重新做一次定时回归，
  但不会推翻已经通过的 AI、攻击和生命周期架构验收。

## 12. P4 首版统一 PIE 验收

1. 在 `TestMap_ListenServer` 对准 `RobotSkillPickup` 按 F，技能槽显示“机器人：远程压制”和对应图标，等级为 1。
2. 按该槽实际改键召唤 RadicalMike；确认只有一台机器人、存在 AIController、BehaviorTree 正在运行，下落显示 Airborne，
   落地后回到 Idle/Walk/Run，落点 350cm 内 Zombie 受到一次 45 基础伤害，友方不受伤。
3. 让 Zombie 在机器人跟随或移动期间进入射程：机器人应立即中止 MoveTo，转向并只攻击 Team Hostile 目标；
   `AM_Robot_FireR/L` 的 Notify 每枪只产生一次服务器伤害和一次可见 Cue。
4. 机器人存在时连续按同一技能键，槽位依次显示远程压制、近战强袭、均衡护卫；远程只开枪，近战主动贴近后使用左爪且每第三击咬击，均衡在近身/远程间择招并优先保护 Owner，切换过程不重生机器人。
5. 再与固定拾取物交互升级：Lv2 仍是同一实例且数值增强；Lv3 左手 Buster 显示，射击时左右枪口与
   `AM_Robot_FireR`/`AM_Robot_FireL` 对应交替；PowerPod 始终位于背部，等级不超过 3。
6. 不击毁机器人并按当前等级等待 30/35/40 秒：机器人自动自爆，450cm 内 Hostile 受到一次范围伤害，随后技能进入 20 秒冷却；冷却期间不重召，结束后可以重新召唤。
7. 另一次召唤中让敌人提前击毁机器人：播放死亡表现并立即进入同一 20 秒冷却，且到期 Timer 不得再次触发自爆或重复冷却。
8. 玩家死亡：机器人立即自爆，450cm 内 Zombie 受到一次 120 基础伤害、友方不受伤，并在约 2 秒内销毁；
   默认不附加 20 秒战斗击毁冷却，玩家重生后可重新召唤。
9. 执行 `Shoot.Skill.Clear 0` 或切换/退出 Experience：机器人立即清理、槽位 AbilitySet 撤销、返回时不从 SaveGame 恢复。
10. 在 Listen Server 使用 Host 与 Remote Client 分别获得并召唤，确认各自只有自己的机器人、Team 随各自 PlayerState、目标和 HUD 不串 Owner。
11. 在 `TestMap_SplitScreen` 让两个本地玩家分别获得、切模式和清理，确认输入、槽位、机器人与冷却互不污染。
12. 如果地图提示 NavMesh 需要重建，先重建 NavMesh 后再判断 MoveTo；这不影响原地索敌与服务器射击链的核验。
