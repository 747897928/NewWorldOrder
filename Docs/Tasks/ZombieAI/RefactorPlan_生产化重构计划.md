# Zombie AI 生产化重构计划

日期：2026-09-05

状态：现状审计、正式 AnimBP、Zombie 近战 GAS Montage/Notify 时序、AbilitySet 授予、站立近战动画、接敌槽位和首轮 Listen Server 回归已完成；正在进行 SplitScreen 与高密度尸群回归

## 1. 本轮审计结论

- Zombie 已经不是纯 Spawner POC：`AEnemyBotController`、AI Perception、Blackboard、Behavior Tree、Team Attitude 与服务器 GAS 伤害链均已存在，应保留并逐步加固。
- 当前最大技术债是迁移期兼容层。正式 AnimBP 已接管五类 Zombie；`EShootEnemyTestAnimationState` 与无 AnimBP 时的 `PlayAnimation` Single Node 回退仍保留，等待多人回归后删除。
- `/Game/AI/Zombie/Archetypes` 下五个现有 archetype 已配置正式 AnimBP；旧 Idle、Move、Attack `UAnimSequence` 仍作为迁移期回退和 Montage 制作来源保留。
- 当前伤害已从旧的 `TryPerformMeleeAttack` 即时结算迁移到 `UShootGA_ZombieMelee`；攻击 Montage 的 `UShootAnimNotify_ZombieMeleeHit` 进入命中窗口后，服务器才创建攻击/状态 GE。Montage 时长 Timer 只在 Notify 漏配时结束能力，不补发伤害。
- `OnCharacterMovementUpdated` 不是每帧 Actor Tick；正式 AnimBP 使用 `GroundSpeed` 驱动移动，它目前只为迁移期攻击枚举和无 AnimBP 回退服务。多人回归后应删除整条枚举复制、MovementUpdated 绑定和 Single Node 回退。
- 当前 Controller 按 Team Attitude 选择主动范围内的最近 Hostile，并已加入短暂目标记忆、目标切换迟滞和服务器侧近战接敌槽位；槽位满时使用外围等待位置。仍缺少仇恨/最近伤害来源和更完整的目标评分，大规模尸潮前应继续补充威胁权重与 CrowdFollowing 回归。
- 五类敌人的战斗参数目前分散在 archetype 蓝图 Class Defaults。单一模式下仍可维护；当生化、大灾变或多 Experience 需要不同难度倍率、波次权重和共享敌人目录时，再建立敌人 Definition/DataAsset。现在立即复制一套表格会产生两个数据源。

## 2. 不推翻的权威链

```text
Experience / Encounter
  -> Spawner：人口、波次、权重与暂停
  -> AIController：TeamId、Perception、目标评分
  -> Blackboard / BehaviorTree：巡逻、追击、攻击编排
  -> Zombie GA：Montage、攻击窗口、可中断状态
  -> GameplayEffect：服务器伤害与附加状态
  -> AnimBP：Locomotion、受击、死亡和 Montage Slot 表现
```

- 敌我识别继续调用 `UShootAbilitySystemLibrary::GetTeamAttitudeForActors` / `ILyraTeamAgentInterface`。
- Spawner 不接管寻敌、移动、攻击或动画。
- AnimNotify 只发出“进入命中窗口”的时机信号；服务器 GA 必须再次验证目标、距离、死亡状态和 Team Attitude 后才创建 GE Spec。
- 玩家、Zombie、机器人都走 ASC/GE 伤害，但不会为了减少文件数量强行共用同一 Character 子类或同一 AnimBP。

## 3. 优先级与执行批次

### P0 资产与调用链清点

- 已完成：五个 archetype 的 Mesh、Skeleton、兼容 Skeleton、Idle/Move/Attack/HitReact/Death 可用资产清点。
- 已完成：确认本批选用 Attack 序列均为 30 FPS、无 Root Motion、无现有 Notify，并记录时长、帧数和轨道数。
- 已完成：搜索蓝图和 C++ 对 `TestIdleAnimation`、`TestMoveAnimation`、`TestAttackAnimation`、`SetArchetypeAnimationState` 和 `PlayAttackAnimation` 的引用；旧回退仍保留到替代链验收。
- 已完成：记录当前 BT/BB、攻击 GE、Bleeder 周期 GE 和各 archetype 数值，详见 `Baseline_资产与动画映射.md`。

### P1 正式 AnimBP 与状态契约

- 已完成：新建 `UShootZombieAnimInstance`，公开速度、移动、空中、死亡和迁移期攻击状态；具体序列与 Slot 留在 AnimBP。
- 已完成：在 `/Game/AI/Zombie/Animation/Archetypes` 建立五个按 archetype 配置的 AnimBP，提供 Idle/Move/Attack/Dead、攻击 Slot 和受击 Slot。
- 已完成：五个 archetype 通过 `AEnemyBotCharacter::TestAnimClass` 明确配置对应 AnimClass；正式 AnimBP 运行时不再进入 `PlayAnimation` Single Node。
- 进行中：正式 AnimBP 的移动/攻击/死亡多人验收通过后，删除 `EShootEnemyTestAnimationState` 复制、`ApplyTestAnimationState`、MovementUpdated 绑定和 Single Node 回退；当前 Timer 已不再承担伤害时序。

### P2 GAS 攻击时序

- 已完成：近战从 Character 立即扣血迁为 Zombie 近战 GA；持续 BT Service 只按 `Ability.Skill.Zombie.Melee` 请求激活，不直接结算伤害，单次激活失败不会让接敌分支失败。
- 已完成：五个 Zombie archetype 通过 `DA_AbilitySet_ZombieCombat` 统一授予近战 GA，`FShootAbilitySet_GrantedHandles` 在 UnPossessed/EndPlay 回收；旧 `StartupAbilities` 已从五个正式 archetype 清空。
- 已完成：GA 播放蓝图可配置 Montage，在服务器权威 Notify 命中窗口中再次验证 Team、距离、朝向、视线和目标状态，然后施加已有伤害 GE/状态 GE。
- 已完成：攻击冷却由 `Cooldown.AI.ZombieMelee` 与 archetype `Attack Interval` 统一管理；BT 的 Wait 只负责决策重试，不成为第二套冷却。
- 已完成：Bleeder 继续复用统一 `SetByCaller.Damage` 与周期 IncomingDamage，不增加直接写 Health 的分支。

### P3 受击、死亡与多人验证

- 已完成：接入通用受击事件和死亡状态，验证 Montage/AnimGraph 不会被 Locomotion 覆盖；近战 Notify 命中后能看到 IncomingDamage/Shield/Health 变化。
- 已完成：修正 Zombie 移动朝向为 `bOrientRotationToMovement=true`、`bUseControllerDesiredRotation=false`，关闭追击 MoveTo 的 `Allow Strafe`，并在接敌槽位完成前关闭 RVO；攻击起始停止移动并对准目标。
- 已完成：Controller 对 Hostile 目标增加 2 秒记忆、250cm 切换迟滞、`TargetApproachLocation` 和 `UShootMeleeEngagementSubsystem` 槽位；该点只负责站位，`TargetActor` 仍是攻击 Service/GA 的唯一实际目标。
- 已完成：为巡逻分支增加 `TargetActor Is Not Set`、Observer Aborts Both，并让近战视线忽略其他角色的拥挤遮挡，避免攻击失败后面朝目标滑回巡逻；死亡、UnPossess 和 EndPlay 会清理 Blackboard、Focus、Brain 和槽位。
- 进行中：Listen Server、Remote Client 与 SplitScreen 验证；独立测试进程已观察到无玩家输入时的 `IncomingDamage`、`Shield`、`Health` 变化，但客户端不运行权威 BT/GA 结算，仍需检查一致动画、受击、死亡和 Cull/销毁。
- 待验收：禁用 AI 开关暂停 Brain/PathFollowing 后 AnimBP 回 Idle，恢复后不重复初始化 ASC 或 Behavior Tree。
- 2026-09-07 已修正 `TestMap_ListenServer` 的 Spawner 实例覆盖值：`Enemy Behavior Enabled` 曾为 `false`，会在首帧暂停全部 Brain；该配置问题已在 PIE 中复现并通过恢复开关验证，后续测试需区分“测试开关暂停”和“行为树自身停止”。
- 2026-09-07 接敌站位回归确认：旧的 8 槽位、100cm 固定环和 40cm 导航接受半径会在胶囊碰撞后把攻击者推到命中范围外；当前改为 4 个近战槽位、`AcceptableRadius=20cm`、有槽位接近半径 `max(80cm, AttackRange * 0.55)`，槽位满时使用外围等待位置。干净 PIE 已观察到近处 Zombie 播放共同站立挥击 Montage 并持续扣减玩家护盾。

### P4 尸群生产化增强

- 已完成第一版服务器接敌槽位和环形外围等待；当前首期容量为每个目标 4 个槽位，未获得槽位的 Zombie 保留 Hostile 目标并在 `AttackRange + 75cm` 以上排队。在真实尸群压力下继续评估槽位容量、EQS 或 CrowdFollowing，目标评分可包含距离、攻击 Owner/队友的威胁和最近伤害来源。
- 特殊感染者通过独立 AbilitySet、GA/GE 和 BT 子树扩展，不在基类加入 `if (ZombieType)` 大分支。
- 只有多 Experience 确实需要共享目录和难度变体时，才引入 Zombie Definition/DataAsset，并一次性迁移蓝图参数，禁止表格与蓝图双写。

## 4. 与机器人可复用的边界

- 立即复用：现有 Team Attitude 工具、ASC/GE 伤害入口、AI Perception 的 Hostile 过滤原则、死亡状态接口。
- 重构后再抽取：目标有效性/评分的纯函数或小型组件；必须先由 Zombie 与机器人两条已验证调用链证明参数确实相同。
- 不共用：机器人三模式决策、Owner 保护语义、双枪/爪击动画、召唤寿命；Zombie 群体追击、感染者 archetype、波次与仇恨语义。
- `AShootCharacterBase` 与 `ICombatInterface` 的 Aura 历史残留另立清理批次。只有玩家、Zombie、机器人三方都需要且语义一致的能力才上提父类。

## 5. 完成标准

- 五类 Zombie 全部使用正式 AnimBP，不存在运行时 `PlayAnimation`/Single Node 切换。
- 移动、攻击、受击、死亡过渡连续，服务器伤害发生在可见攻击窗口内。
- 所有伤害仍通过 Team Attitude、ASC 与 GE；友方、Neutral 和死亡目标不被攻击。
- BT、GA、AnimBP 各自只有一个职责源；不存在 Character Timer 与 Montage/Notify 双重伤害时序，GA Timer 仅是 Notify 漏配时的安全结束机制。
- 能力授予来自 AbilitySet 且可通过 GrantedHandles 回收；同一正式 archetype 不得同时配置 ZombieAbilitySet 和 StartupAbilities。
- 近战接敌槽位由服务器 World Subsystem 统一分配与释放，目标 Blackboard、站位 Vector 和攻击事件不得重新合并成一个状态。
- 两玩家 Listen Server 和 SplitScreen 中无重复伤害、动画卡死、目标串线或 ASC 重复初始化。
- 文档、蓝图中文注释和 C++ 中文注释能让后续协作者在不读取历史会话的情况下恢复设计边界。
