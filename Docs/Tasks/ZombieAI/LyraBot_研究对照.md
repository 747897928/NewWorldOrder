# Lyra ShooterCore Bot 研究对照

日期：2026-09-07

状态：已完成 Lyra 编辑器 11000 端口的只读审计；未修改 Lyra 资产、蓝图或源码。

## 审计范围

通过 VibeUE 读取 Lyra 项目的 `/ShooterCore/Bot` 资产和相关原生源码，重点核对：

- `/ShooterCore/Bot/BT/BT_Lyra_Shooter_Bot` 及 `BB_Lyra_Shooter_Bot`
- Bot Controller、Spawner、EQS Context/Generator、EQS 查询
- `BTS_CheckAmmo`、`BTS_ReloadWeapon`、`BTS_SetFocus`、`BTS_Shoot`
- `LyraPlayerBotController`、`LyraBotCreationComponent`、`LyraTeamAgentInterface`、`LyraTeamSubsystem`

该目录下共发现 21 个资产。除上述资产外，还包括被动 Controller、性能 Spawner、EQS Tester、`BTDecorator_AllowedToFire` 和 `EQS_AIPerceptionEnemy` 等配套资产。行为树校验结果为空，未发现结构错误。

## 行为树的真实决策链

Lyra 的核心不是“靠近目标就攻击”，而是把目标、战斗位置和开火事件拆成三层：

1. `TargetEnemy`：目标是谁。
2. `MoveGoal`：为了射击应该站在哪里。
3. `InputTag.Weapon.Fire`：当前武器是否满足条件，满足时才发送开火 Gameplay Event。

行为树的主要结构如下：

```text
Root Selector
├─ TargetEnemy 已设置时
│  ├─ Shoot And Move
│  │  ├─ AllowedToFire 装饰器
│  │  ├─ BTS_Shoot：检查武器后发送 InputTag.Weapon.Fire
│  │  ├─ BTS_SetFocus：对 TargetEnemy 设置 Focus
│  │  ├─ EQS_MoveAgainstEnnemy：寻找射击位置，写入 MoveGoal
│  │  └─ MoveTo MoveGoal：AcceptableRadius=25，允许 Strafe
│  └─ NoVisiblePlayer
│     ├─ 清除 Focus
│     ├─ 换弹
│     ├─ 搜索武器或搜索玩家位置
│     └─ Wait 1 秒
└─ 没有目标或没有弹药时
   ├─ OutOfAmmo 时搜索武器
   └─ Wait 1 秒
```

`Shoot And Move` 上的 `TargetEnemy` Blackboard 装饰器使用 `FlowAbort=Both`。目标刚设置时可以立即打断搜索分支进入战斗；目标失效或被清空时也能立即退出战斗分支。`BTS_Shoot` 的服务间隔为 0.1 秒，随机偏差为 0.3 秒，因此它不是“进入一次节点只开一枪”。

## 目标识别与敌我判断

Lyra 没有用 `if (Player)` 作为通用敌我判断。

- `LyraPlayerBotController::GetTeamAttitudeTowards` 通过目标 Pawn 的 Controller 查询 `ILyraTeamAgentInterface`，再比较 Generic Team ID。
- `EQS_GetAllEnemy` 使用 `LyraTeamSubsystem.CompareTeams` 筛选 `DifferentTeams`，并用 `LyraHealthComponent.IsDeadOrDying` 排除死亡目标。
- `EQS_Context_ControlPoints` 也通过 Lyra Team Subsystem 排除己方控制点。
- `EQS_AIPerceptionEnemy` 的感知查询限定为 `LyraCharacter`，这是 Lyra Shooter Bot 的类型边界，不等于“玩家判断”。

需要注意：Lyra 的 `EQS_GetAllEnemy` 虽然使用了 Team Subsystem，但仍把候选类型限制为 `LyraCharacter`。当前 NewWorldOrder 的通用敌我规则仍应以 `ILyraTeamAgentInterface` 和 `ETeamAttitude` 为准，不能照搬这个类型限制。

## 感知、追击与射击位置

Lyra Shooter Bot 的 Controller 蓝图配置了 Sight 和 Damage 两种感知：

| 配置 | 已核对值 |
| --- | --- |
| SightRadius | 5000 |
| LoseSightRadius | 6000 |
| PeripheralVisionAngleDegrees | 90 |
| Sight MaxAge | 1 秒 |
| Damage MaxAge | 1 秒 |
| 检测敌对阵营 | 开启 |
| 检测中立/友方 | 关闭 |

`EQS_MoveAgainstEnnemy` 是 400 单位网格、点间距 150 的导航查询，先投影到导航网格，再用到目标的距离和 Camera Trace 结果评分，倾向于选择既能到达又能看见目标的位置。它服务于远程射击站位，不应直接作为丧尸近战攻击位置的实现。

`EQS_Context_TargetEnemy` 只负责从 Blackboard 读取 `TargetEnemy` 的位置。这个分工很重要：寻找“谁”和寻找“站在哪里”不能共用一个会不断覆盖目标的 Blackboard 键，否则移动查询失败、目标短暂丢失或位置评分变化时容易出现绕圈、退离和目标抖动。

Controller 的 AIPerception 蓝图组件 CDO 上读取到 `bAutoActivate=False`，但其 Sight/Damage 配置有效，Controller 在阵营变化时会调用 `RequestStimuliListenerUpdate`。仅凭这个 CDO 值不能断言感知没有启动，后续若排查运行时感知，必须结合组件实际激活状态和 Perception 调试数据验证。

## 攻击事件与伤害的边界

`BTS_Shoot` 不直接扣血，也不在行为树里实现伤害。它会：

1. 从 Pawn 的 Equipment Manager 和 Inventory Item 读取当前武器。
2. 检查弹匣弹药和备用弹药。
3. 必要时发送 `InputTag.Ability.Quickslot.SelectSlot`，让武器流程处理换弹/切槽。
4. 满足条件时发送 `InputTag.Weapon.Fire` Gameplay Event。

真正的射击、目标数据提交、Gameplay Ability 激活和伤害在 GAS/武器能力链路中完成。`BTDecorator_AllowedToFire` 只调用 `Can Player Bots Attack`，本质是开发设置闸门；本次读取的 Lyra CDO 中 `bAllowPlayerBotsToAttack=True`，因此它不是当前“完全不攻击”的直接解释。

这个设计对丧尸的启示是：行为树只负责决定“现在尝试攻击”，近战距离、攻击能力是否可激活、冷却、蒙太奇和 AnimNotify 应由 Zombie Gameplay Ability 与伤害链路共同决定。不能因为行为树分支执行了，就假设伤害一定已经产生。

## 生命周期与状态清理

`B_AI_Controller_LyraShooter` 的关键生命周期是：

- BeginPlay 等待 Experience Ready 后才 Run Behavior Tree。
- OnPossess 调用父类逻辑、启动 Brain，并注册阵营观察。
- OnDeathStarted 清空 Blackboard 的 `TargetEnemy`，再停止 Brain Logic。
- OnUnPossess 复用死亡清理路径，避免旧 Pawn 的目标残留。
- 阵营变化时请求 AIPerception 更新。

这解释了为什么 Lyra 不容易把上一具 Pawn 的目标状态带到下一具 Pawn。NewWorldOrder 的丧尸重生、死亡、重新 Possess 和回合重置也应保持同样的清理边界。

## 与当前 NewWorldOrder 实现的对照

当前 NewWorldOrder 的 ZombieAI 已把 Lyra 研究中最重要的几项原则落到代码和资产：

- 使用 `ILyraTeamAgentInterface`/`ETeamAttitude`，没有用 `if (Player)` 做敌人判断。
- 目标记忆和迟滞，避免感知瞬断导致目标抖动。
- Controller 持有 Behavior Tree，Character 只提供 Avatar、表现和战斗能力，避免 Pawn 与 Controller 各维护一份决策资产。
- 将目标 Actor 与 `TargetApproachLocation` 分开，并由服务器近战接敌槽位子系统管理有限站位，避免把目标位置和攻击站位混成一个状态。
- 近战 `MoveTo` 保持 `Allow Strafe=false`，接受半径为 40cm。
- 五个正式 Zombie archetype 通过 `DA_AbilitySet_ZombieCombat` 统一授予 Zombie Gameplay Ability，AbilitySet 句柄在 UnPossess/EndPlay 时回收；Tag 继续由 `Config/DefaultGameplayTags.ini` 提前注册。
- 行为树使用持续的 `UShootBTService_AttemptMeleeAttack` 尝试能力激活，失败不会让接敌 Sequence 失败；真正伤害仍由 Zombie Gameplay Ability、Gameplay Tag、Cooldown 和 AnimNotify 决定。

Lyra 对当前丧尸最有价值的补充不是复制远程 Bot 的 EQS，而是验证以下结构是否在运行时真的成立：

1. 目标 Blackboard 键设置后，攻击分支是否能通过 Observer Aborts 立即抢占搜索分支；当前 `BT_Zombie_Melee` 已验证为 `TargetActor Is Set + Observer Aborts Both`。
2. 攻击分支是否存在持续的“尝试攻击”服务，而不是只在接近完成时调用一次；当前已由 `UShootBTService_AttemptMeleeAttack` 按 0.15 秒节奏重试。
3. 攻击尝试失败时，是否能明确区分距离不足、能力冷却、蒙太奇未播放、目标无效和阵营不合法；当前行为树不再把瞬时激活失败升级为分支失败，GA/Notify 仍保留服务器复核边界。
4. 攻击分支失败或目标死亡时，是否清除 Focus、攻击状态和旧目标；当前 Controller 已监听 `DeathStarted`，并在死亡、UnPossess、EndPlay 路径清理 Blackboard、Focus、移动、Brain 和槽位。
5. 多个丧尸同时接近时，是否需要攻击槽位、环形站位和目标评分；当前已落地有限环形槽位和外围等待，最近目标/切换迟滞仍是第一版评分，最近伤害来源与威胁权重留作下一批。

## 本轮已落地

- `AEnemyBotController` 的 `BehaviorTreeAsset` 成为唯一行为树配置入口，`AEnemyBotCharacter` 不再保存重复的 Behavior Tree 引用。
- `UShootBTService_AttemptMeleeAttack` 替代一次性近战 Task；`TryActivateMeleeAttack` 使用 GAS 原生 `TryActivateAbilitiesByTag`，不再手工遍历 Ability Spec。
- `UShootMeleeEngagementSubsystem` 为每个目标提供有限环形站位；没有槽位的 Zombie 只追到外围等待，获得释放后再申请攻击位。
- `AEnemyBotCharacter` 在 PossessedBy 中通过 `UShootAbilitySet` 授予能力，在 UnPossessed/EndPlay 中通过 `FShootAbilitySet_GrantedHandles` 取消能力并回收句柄。
- 五个 archetype 已指向 `/Game/AI/Zombie/AbilitySets/DA_AbilitySet_ZombieCombat`，旧 `StartupAbilities` 已清空。
- 服务器测试进程加载 `TestMap_ListenServer` 后，在无玩家输入的情况下出现 `IncomingDamage` 到 `Shield/Health` 的连续属性变化，证明攻击链进入运行时；这不替代玩家对动画姿态、SplitScreen 和难度的最终验收。
- 2026-09-07 连接 PIE 排查到一个容易误判的运行时开关问题：`BP_EnemyTestSpawner` 的 C++ 默认值和蓝图 CDO 都是启用，但 `TestMap_ListenServer` 的 Actor 实例曾将 `Enemy Behavior Enabled` 覆盖为关闭。Spawner 首帧会按实例值调用 `PauseLogic`，所以游戏时间仍增长、感知仍能写入 `TargetActor`，但所有 Brain 都处于 `running=false、paused=true`，表现为僵尸锁定目标后完全不移动。地图实例已修正并保存。以后检查 AI“卡住”时，必须把测试开关实例值、Brain 状态、PathFollowing 状态和 Pawn 速度一起观测，不能只看 Blackboard 目标。

## 不应直接照搬的部分

- `bAllowStrafe=true` 是远程射击 Bot 的站位策略；丧尸近战应继续禁止平移绕射。
- `AcceptableRadius=25`、5000 感知半径、400 网格等数值服务于远程武器和测试场景，不能作为丧尸调参结论。
- `LyraCharacter` 候选类型限制不能替代阵营接口判断。
- `SetFocus` 只能解决朝向目标，不能保证近战能力激活或伤害产生。
- EQS 寻找“最佳位置”不能替代攻击能力的距离、扇区、冷却和动画通知校验。

## 后续验证顺序

若继续处理“站着看主角不攻击”“靠近后平移离开”“攻击欲望低”，建议按以下顺序检查可观测证据：

1. 记录每个丧尸的 `TargetEnemy`、行为树当前分支、距离、Team Attitude、攻击能力激活结果和 Cooldown Tag。
2. 确认攻击尝试服务/Task 的调用频率，以及失败后是否立刻重新尝试。
3. 确认近战 Gameplay Event 的 Tag 与能力 CDO `AbilityTags`、激活条件和输入/事件触发方式完全一致。
4. 确认攻击蒙太奇中的 AnimNotify 是否到达伤害窗口，且伤害目标仍通过 Team Attitude 过滤。
5. 最后再调整伤害值、攻击间隔、攻击距离和人群槽位；不要先用提高伤害掩盖攻击根本未触发的问题。

若 Blackboard 已有目标但 Pawn 不移动，先检查测试 Spawner 是否把对应实例的 `Enemy Behavior Enabled` 覆盖为关闭，再检查 `BehaviorTreeComponent::IsRunning/IsPaused` 和 PathFollowing；不要把“游戏时间在增长”误当成 AI 决策正在执行。

本记录既保存 Lyra 对照结论，也记录已经落地的适配边界；后续实现应继续沿用目标、站位、攻击事件、GAS 和生命周期各自单一职责的结构。
