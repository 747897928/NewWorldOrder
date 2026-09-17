# 僵尸行为树与敌人类型

## 状态

- 建立日期：2026-08-29
- 当前状态：正式 Behavior Tree、五类 AnimBP 与 Zombie 近战 Montage/Notify 驱动的 GAS 能力已落地；Controller 持有行为树、AbilitySet 授予、死亡清理与近战接敌槽位已接入；2026-09-07 已完成站立近战动画、接敌中心距和伤害调优，并在 Listen Server PIE 中观察到新的统一攻击 Montage 与护盾扣减
- 适用地图：`/Game/Maps/TestMap_ListenServer`、`/Game/Maps/TestMap_SplitScreen`

## 权威架构

调用链：

```text
Experience / Map
  -> BP_EnemyTestSpawner：只生成、补足数量、按权重选 archetype、暂停/恢复 Brain
  -> BP_EnemyBotController：TeamId + AI Perception + Blackboard + BehaviorTree
  -> BT_Zombie_Melee：目标刷新、追击/巡逻分支、近战任务编排
  -> AEnemyBotCharacter：再次验证敌我/距离/冷却，转交 Zombie 近战 GA
  -> Zombie 近战 GA + AnimNotify：服务器命中窗口再次校验后创建攻击 GE Spec
  -> UShootAttributeSet::IncomingDamage：护盾、友伤倍率、受击 Cue、死亡
```

- `AShootEnemyTestSpawner` 不是 AI 大脑。禁止在其中增加寻敌、移动、攻击、仇恨、冷却或动画驱动；它的定时器只管理人口和测试暂停开关。
- `AEnemyBotController` 拥有 AI Perception，并在 `OnPossess` 初始化黑板、写入出生点、启动 Controller Class Defaults 上的 `BehaviorTreeAsset`。Character 不再保存第二份 Behavior Tree 引用。
- Behavior Tree 只编排决策，不直接修改 Health。`UShootBTService_AttemptMeleeAttack` 按间隔调用 `AEnemyBotCharacter::TryActivateMeleeAttack`，只按 `Ability.Skill.Zombie.Melee` 请求 Zombie 近战 GA；服务器在 Notify 命中窗口再次验证并通过 GAS/GE 结算，瞬时激活失败不会让接敌 Sequence 失败。
- 僵尸近战 GA 使用的 `Ability.Skill.Zombie.Melee` 和 `Cooldown.AI.ZombieMelee` 由 `Config/DefaultGameplayTags.ini` 提前注册；GA CDO 和运行时查找统一使用 `FGameplayTag::RequestGameplayTag(FName(...), false)`。不要把这两个配置 Tag 再放回依赖 `UShootAssetManager::StartInitialLoading` 时序的 `FShootGameplayTags` 字段，否则可能出现 Spec 存在但按标签无法找到近战能力。
- `AEnemyBotCharacter` 拥有 ASC 和 AttributeSet。敌人类型的网格、正式 AnimBP、动画回退资产、移动速度、属性 GE、攻击 GE、距离、节奏和数值配置在各自蓝图 Class Defaults；Zombie 近战 GA 由五个 archetype 共同选择的 `DA_AbilitySet_ZombieCombat` 授予。
- 群体 RVO 配置属于 `AEnemyBotCharacter` 的移动能力，放置敌人和 Spawner 生成敌人行为一致；不能由某张测试地图的 Spawner 临时注入。
- Zombie 移动使用 `bOrientRotationToMovement=true`、`bUseControllerDesiredRotation=false`；近战开始时由 Character 停止移动并对准目标。当前 `bUseRVOAvoidance=false`，虽然接敌槽位已经落地，但在 CrowdFollowing 与真实尸群回归完成前仍不恢复 RVO，避免移动避让再次把近战者推离目标。
- `AEnemyBotController` 对 Hostile 目标增加短暂记忆和切换迟滞：目标短暂丢失时保留 2 秒，新的 Hostile 只有明显更近才抢占；参数位于 Controller Class Defaults，不按玩家身份写分支。
- `TargetApproachLocation` 是 Controller 根据 `UShootMeleeEngagementSubsystem` 分配的环形槽位生成的接近点，Behavior Tree 的追击 `MoveTo` 使用该 Vector，攻击 Service 仍使用 `TargetActor` 做最终目标和 GAS 校验。槽位满时保留目标记忆但把站位放到外围，释放后自动重新申请。

## Blackboard 与 Behavior Tree

资产：

- `/Game/AI/Zombie/BB_Zombie_Melee`
- `/Game/AI/Zombie/BT_Zombie_Melee`

黑板键：

- `TargetActor`：当前最近且仍被感知的 Hostile Actor。
- `TargetApproachLocation`：Controller 为当前 Hostile 生成的环形接近点，供追击 MoveTo 分散站位；不是攻击目标替代物。
- `HomeLocation`：Controller 接管时记录的出生位置。
- `PatrolLocation`：NavMesh 上围绕 HomeLocation 选出的巡逻点。

树结构：

```text
Root
  Service: Refresh Perceived Hostile Target
  Selector: Zombie Decision
    Sequence: Engage Hostile
      Blackboard(TargetActor Is Set, Observer Aborts Both)
      MoveTo(TargetApproachLocation)
      Service: Attempt Melee Attack
      Wait
    Sequence: Patrol Around Home
      Blackboard(TargetActor Is Not Set, Observer Aborts Both)
      Find Patrol Location
      MoveTo(PatrolLocation)
      Wait
```

- `Observer Aborts Both` 保证感知到敌人时立即打断巡逻，丢失/死亡时立即退出接敌分支；接敌 Sequence 内的持续攻击 Service 不会因单次 GAS 冷却或能力条件失败而退出。
- 感知组件可以接收三种 affiliation，但 `RefreshPerceivedHostileTarget` 只把 Team Attitude 为 Hostile 的目标写入黑板。
- 近战距离、攻击冷却与伤害配置属于 archetype；树不按 Walker/Runner 等分类硬编码分支。攻击槽位数量由 Controller 的 `MeleeEngagementSlotCount` 统一控制，站位由 `UShootMeleeEngagementSubsystem` 管理。

## 阵营规则

- 人类玩家当前 TeamId 为 1，Zombie Controller 当前默认 TeamId 为 2。
- 所有判断统一调用 `UShootAbilitySystemLibrary::GetTeamAttitudeForActors` 或 `ILyraTeamAgentInterface`，禁止 Cast 成玩家/敌人后硬编码。
- `ULyraTeamAgentInterface` 是 UE 反射用的 UInterface 壳，C++ 实际通过 `ILyraTeamAgentInterface` 读取 `FGenericTeamId`；Helper 会依次检查 Actor、Pawn 的 PlayerState、Controller，再回退到 ASC Faction Tag。
- Friendly：默认不主动选择。
- Neutral：默认不主动选择；NoTeam 或无法可靠解析归属时也是 Neutral，不能猜成 Hostile。
- Hostile：可进入黑板并被追击、攻击。
- GameMode 的友伤开关只影响伤害倍率，不应偷偷改变 AI 的默认攻击欲望。
- “Neutral 被攻击后积累仇恨”“友伤开启时 Zombie 反击 Zombie”属于后续 Story。实现时应增加独立仇恨/记忆数据与可审计的目标评分，不修改上述默认 Team Attitude 语义。

## 首期敌人类型

| 类型 | 网格 | 移速 | 主动范围 | 攻击中心距 | 攻击间隔 | 直接伤害 | 设计定位 |
|---|---|---:|---:|---:|---:|---:|---|
| Walker | Warzombie | 240 | 1700cm | 145cm | 1.10s | 15 | 常规基础单位 |
| Runner | Parasite | 400 | 1900cm | 140cm | 0.80s | 12 | 快速、脆弱、低单次伤害 |
| Bruiser | Cop Zombie | 180 | 1500cm | 160cm | 1.50s | 28 | 慢速、高生命、高单次伤害 |
| Bleeder | Yaku | 290 | 1750cm | 150cm | 1.15s | 12 | 命中附带 3 点周期流血 GE |
| Elite | Zombie Girl | 330 | 2100cm | 155cm | 0.95s | 22 | 高属性低权重精英 |

- 2026-09-07 根据“站在目标面前但不攻击、单次伤害过低”的 PIE 验收重新收紧攻击中心距并调高伤害：
  Controller 候选视野仍为 `SightRadius=2200cm`、`LoseSightRadius=2500cm`，五种敌人再由上表各自的
  `AEnemyBotCharacter.TestAggroRange` 二次过滤。`TestAttackInterval` 仍由 Character 在服务器权威校验，
  Behavior Tree 的重试频率不能绕过该冷却。`攻击中心距`使用 Pawn Actor Location 的二维中心距，
  不是胶囊表面距；过大的历史值（220–320cm）会使手还没有碰到目标就进入 Notify 命中窗口。
- 接敌分支的 `MoveTo` 使用 `TargetApproachLocation`，`AcceptableRadius=20cm`，而近战任务继续用
  `TargetActor` 做距离、阵营、朝向、视线和 GAS 校验；Controller 默认只分配 4 个近战槽位，
  接近环半径为 `max(80cm, AttackRange * 0.55)`，槽位满的 Zombie 使用 `AttackRange + 75cm` 以上的外围站位。
  这样胶囊碰撞与导航容差不会把 Zombie 停在 `AttackRange` 外仍面朝目标。
- 这些数值保存在五个 archetype 蓝图 Class Defaults，并写入
  `NewWorldOrder.ZombieCombatTuning` 资产元数据。后续 Experience 做回合平衡时应调整蓝图或正式敌人
  DataAsset，不要在 Spawner、Behavior Tree Task 或 GameMode 里复制第二套数值。

蓝图：

- `/Game/AI/Zombie/Archetypes/BP_Enemy_Zombie_Walker`
- `/Game/AI/Zombie/Archetypes/BP_Enemy_Zombie_Runner`
- `/Game/AI/Zombie/Archetypes/BP_Enemy_Zombie_Bruiser`
- `/Game/AI/Zombie/Archetypes/BP_Enemy_Zombie_Bleeder`
- `/Game/AI/Zombie/Archetypes/BP_Enemy_Zombie_Elite`

- 五类网格使用不同 Skeleton，但项目已配置 Compatible Skeletons，可以复用兼容动作；这不等于随机换皮，同一蓝图类的表现和玩法配置保持固定。
- Bleeder 使用 `UShootEffect_EnemyBleed`。周期伤害继续进入统一 `IncomingDamage`，不另写扣血路径。
- 五个正式 AnimBP 现在保留不同的 Idle/Move 组合（Walker 慢走、Bruiser 重走、Runner/Bleeder/Elite 追跑，
  Walker/Runner 使用新增直立 Idle 变体），Attack 统一复用站立挥击 `zombie_attack`，用战斗参数和网格差异
  保持敌人定位。这样复用不会把 Runner/Bleeder 的爬行姿态带入攻击。
- 后续特殊感染者应通过独立蓝图子类、GA/GE 与必要的 Behavior Tree 子树扩展，例如冲锋、抓取、远程喷吐；不要在基类或 Spawner 写 `if (ZombieType)` 大分支。

## 动画边界

- 五个 archetype 当前使用 `/Game/AI/Zombie/Animation/Archetypes/ABP_Zombie_Walker`、
  `ABP_Zombie_Runner`、`ABP_Zombie_Bruiser`、`ABP_Zombie_Bleeder` 和 `ABP_Zombie_Elite`。
  五个 AnimBP 均以 `Warzombie_F_Pedroso_Skeleton` 为目标骨架；Warzombie Skeleton 的 Compatible Skeletons
  已覆盖 Parasite、Cop、Yaku 和 ZombieGirl 四套网格。
- 每个 AnimBP 的 `Locomotion` 状态机包含 `Idle`、`Move`、`Attack`、`Dead`，主 AnimGraph 串接
  `DefaultGroup.DefaultSlot` 攻击槽和 `DefaultGroup.HitReact` 受击槽。
- `UShootZombieAnimInstance` 只提供 `GroundSpeed`、`bIsMoving`、`bIsFalling`、`bIsDead` 和过渡期的
  `bIsAttacking` 状态；序列与 Slot 由 AnimBP 配置。`bIsAttacking` 当前读取 Character 的复制状态，
  直到 Montage/GA 替代链通过多人验证后再随 P4 删除。
- `CharacterMesh0` 是继承组件，直接写组件 `AnimClass` 无法持久化。因此每个 archetype 在 Class Defaults
  使用 `AEnemyBotCharacter` 上的 `Animation Blueprint`（`TestAnimClass`）属性；基类应用网格后设置 AnimClass。
  配置该属性时 `ApplyTestAnimationState` 不再调用 `PlayAnimation`，无 AnimBP 的旧类才继续走回退路径。
- 正式 AnimBP 用 `GroundSpeed` 驱动 Idle/Move，用复制的 `bIsAttacking` 驱动攻击状态；CharacterMovement 更新事件只为迁移期枚举与无 AnimBP 回退服务，Spawner 不驱动动画，也没有服务器 Tick AI。
- 五类攻击 Montage 统一为 `/Game/AI/Zombie/Animation/Montages/AM_Zombie_Common_Melee`，来源序列是
  `/Game/Characters/Zombie/Animations/zombie_attack`，并配置一个 `UShootAnimNotify_ZombieMeleeHit`，
  触发时间为 1.04s。GA 只在 Notify 帧重新验证目标并应用攻击/流血 GE；Montage 时长 Timer 仅用于
  Notify 漏配时安全结束能力，不补发伤害。
- 2026-09-07 已完成五个 archetype 回读：正式 AnimBP 的父类均为 `UShootZombieAnimInstance`，
  状态机校验无错误；旧 `TestIdleAnimation`、`TestMoveAnimation`、`TestAttackAnimation` 仍保留，
  只作为无 AnimBP 时的迁移期回退字段，不再引用已清理的爬行/咬人资产。
- 已在确认引用迁移后删除 `/Game/AI/Zombie/Animation/Montages/AM_Zombie_Runner_Melee`、
  `/Game/Characters/Zombie/Animations/running_crawl`、`zombie_biting`、`zombie_biting__` 和
  `zombie_crawl`。`zombie_scream` 虽然存在，但审计姿态包含明显下蹲，不用于当前战斗状态，保留给未来
  独立 Alert 状态评估。
- 生产化的分批步骤、删除前引用检查和多人验收标准见
  `Docs/Tasks/ZombieAI/RefactorPlan_生产化重构计划.md`。在正式 AnimBP、Montage/Notify 与 Zombie GA 替代链完成前，
  不先删除当前回退，避免把仍可玩的基线变成无动画状态。

## 测试开关

- `AShootEnemyTestSpawner::SetEnemyBehaviorEnabled` 是 BlueprintCallable 的服务器权威入口。
- 关闭：维持场内人口，暂停 Brain，停止 PathFollowing 与 CharacterMovement，回到 Idle 表现。
- 开启：恢复原 Behavior Tree。
- 2026-09-07 PIE 卡住排查：`TestMap_ListenServer` 中 `BP_EnemyTestSpawner` 的实例覆盖值曾为 `Enemy Behavior Enabled=false`，导致首帧 `ApplyBehaviorEnabledState` 主动调用 `PauseLogic`；因此会出现目标和 Focus 已设置，但所有 Brain 都是 `running=false、paused=true`。该地图实例已恢复为 `true` 并保存。Spawner 蓝图 CDO 与 C++ 默认值为 `true`，但地图实例覆盖值优先，后续排查必须同时回读三层配置。
- UI/按键必须由测试地图蓝图通过 Enhanced Input/CommonUI 配置；C++ 不硬编码键盘或手柄键位。

## 已验证与已知边界

- 两玩家 Listen Server 产生一个服务器世界和一个远端客户端世界。
- 服务器上的所有 Zombie 均有 Controller、BehaviorTreeComponent、BlackboardComponent；黑板能选择 Hostile，也能生成巡逻点。
- 远端客户端没有权威 AIController/BehaviorTree，只接收五类网格、移动和表现复制。
- Listen Server 运行时已观察到多个 Zombie 使用 `AM_Zombie_Common_Melee`，且近处 Runner 在中心距约 100cm
  的命中窗口使玩家 `Shield` 从 102 降至 78；近战任务经过 Zombie GA/GE 链，而不是 Character 直接扣血。
- 原“面朝玩家后退/平移离开”由两条链路叠加：Controller Desired Rotation 让朝向锁定玩家，RVO 在无接敌容量时把速度推离玩家；同时旧的一次性攻击 Task 失败后会让接敌 Sequence 退出。当前已统一移动朝向、关闭 RVO、关闭追击 MoveTo 的 `Allow Strafe`，接入持续攻击 Service、正式接敌槽位，并给巡逻分支增加 `TargetActor Is Not Set`。
- 2026-09-07 清理了 `TestMap_ListenServer` 和 `TestMap_SplitScreen` 中放置的旧 `BP_EnemyBotCharacter` 实例。该基类实例没有 `StartupAbilities`、Attack Montage 或正式 AnimBP，却会启动 Zombie Behavior Tree，正是“能追近但永远不能攻击”的无效测试对象；两张地图现在只由 Spawner 生成五类正式 Zombie 子类。
- Zombie 近战视线会忽略其他 `ACharacter` 的拥挤遮挡，只把世界几何视为遮挡；这样队列中的尸体不会让所有近战任务失效，墙体仍会阻挡命中。
- 同一目标默认最多分配 4 个近战槽位；未获得槽位的 Zombie 保留 Hostile 目标和外围追击位置，不会反复在攻击范围内请求 GA。死亡、切换目标、UnPossess 和 EndPlay 会释放槽位。
- 2026-09-07 独立测试进程加载 `TestMap_ListenServer` 后，在没有玩家输入的情况下观察到连续 `IncomingDamage`、`Shield` 和 `Health` 属性变化；
  干净 PIE 复核到近处 Runner/Walker 的接近点半径约 80cm、Pawn 中心距约 121–125cm，
  `AM_Zombie_Common_Melee` 正在播放且玩家护盾持续扣减。仍需玩家对攻击姿态、SplitScreen 和难度做最终验收。
- 2026-09-07 连接 PIE 时确认：游戏时间继续增长并不代表 AI Brain 正在运行；必须同时检查 `BehaviorTreeComponent::IsRunning/IsPaused`、PathFollowing 状态和 Pawn 速度。此次实际根因是测试 Spawner 的地图实例暂停开关，而不是感知、Team Attitude 或 GAS 攻击链。
- 仍需玩家侧验收 SplitScreen 画面、Bleeder 周期伤害数值和 AI 开关 UI。
