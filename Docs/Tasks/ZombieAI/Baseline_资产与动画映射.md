# Zombie 资产与动画回归基线

日期：2026-09-07

状态：资产审计完成，五类正式 AnimBP 已创建并写入 archetype Class Defaults；近战 Montage、Notify、GAS 近战链路、统一 AbilitySet 与接敌槽位已接入；2026-09-07 已完成站立近战动画、攻击距离和伤害调优，Listen Server 已回归

## 审计范围

- 通过 Unreal AssetRegistry、VibeUE BlueprintService、AnimGraphService 和 SkeletonService 读取资产与 CDO。
- 未读取或解析 `.uasset` 二进制文件。
- 五个 archetype 的权威路径为 `/Game/AI/Zombie/Archetypes`；旧文档中的
  `/Game/Testing/Enemies/Archetypes` 已确认过时。

## Archetype 映射

| 类型 | Skeletal Mesh | Mesh Skeleton | 正式 AnimBP | Idle | Move | Attack | Attack Montage | Aggro | Range | Interval | Damage | Status |
|---|---|---|---|---|---|---|---|---:|---:|---:|---:|---|
| Walker | `Warzombie_F_Pedroso` | `Warzombie_F_Pedroso_Skeleton` | `ABP_Zombie_Walker` | `zombie_idle__4_` | `1_walk_chase_Story_clip_export` | `zombie_attack` | `AM_Zombie_Common_Melee` | 1700 | 145 | 1.10 | 15 | 无 |
| Runner | `Parasite_L_Starkie` | `Parasite_L_Starkie_Skeleton` | `ABP_Zombie_Runner` | `zombie_idle__3_` | `zombie_run_1` | `zombie_attack` | `AM_Zombie_Common_Melee` | 1900 | 140 | 0.80 | 12 | 无 |
| Bruiser | `copzombie_l_actisdato` | `copzombie_l_actisdato_Skeleton` | `ABP_Zombie_Bruiser` | `Zombie_Idle` | `Zombie_Walk` | `zombie_attack` | `AM_Zombie_Common_Melee` | 1500 | 160 | 1.50 | 28 | 无 |
| Bleeder | `Yaku_J_Ignite` | `Yaku_J_Ignite_Skeleton` | `ABP_Zombie_Bleeder` | `zombie_scratch_idle` | `zombie_run_1` | `zombie_attack` | `AM_Zombie_Common_Melee` | 1750 | 150 | 1.15 | 12 | `ShootEffect_EnemyBleed`, magnitude 3 |
| Elite | `Zombiegirl_W_Kurniawan` | `Zombiegirl_W_Kurniawan_Skeleton` | `ABP_Zombie_Elite` | `zombie_idle_6` | `zombie_run_1` | `zombie_attack` | `AM_Zombie_Common_Melee` | 2100 | 155 | 0.95 | 22 | 无 |

表中资产的完整路径均位于以下目录：

- 网格：`/Game/Characters/Zombie/...`
- 动画序列：`/Game/Characters/Zombie/Animations/...`
- AnimBP：`/Game/AI/Zombie/Animation/Archetypes/...`

## 动画技术属性

本轮将五类正式 Attack 统一收敛到直立挥击 `zombie_attack`，避免 Runner 的 `zombie_biting` 和 Bleeder 的
`zombie_attack_4` 在攻击过程中突然进入爬行姿态。该序列为 30 FPS、无 Root Motion；由它创建的
`AM_Zombie_Common_Melee` 配置一个 `UShootAnimNotify_ZombieMeleeHit`，触发时间为 1.04s，命中只在这个
可见挥击窗口发生。不同敌人复用同一近战动作是有意选择，差异保留在网格、待机、移动、属性、攻击节奏和伤害配置。

序列帧数和 Montage 时长如下：

| 类型 | 序列 | Montage | 序列时长 | Montage 时长 | 帧数 | 骨骼轨道 | Notify |
|---|---|---|---:|---:|---:|---:|---:|
| Walker/Runner/Bruiser/Bleeder/Elite | `zombie_attack` | `AM_Zombie_Common_Melee` | 2.600000s | 2.600000s | 79 | 85 | 1（1.04s） |

五套网格的 Skeleton 之间，兼容关系存储在 `Warzombie_F_Pedroso_Skeleton` 的 Compatible Skeletons 上，
包含另外四套 Skeleton。所有 Zombie 动画序列当前都关联到 Warzombie Skeleton，因此同一套状态图可以复用，
但为了保留每类动作资产的独立调优，当前创建了五个同构的具体 AnimBP。

## AnimBP 配置契约

- 每个 AnimBP 的父类为 `UShootZombieAnimInstance`。
- `Locomotion` 状态机包含 `Idle`、`Move`、`Attack`、`Dead`。
- `Idle -> Move` 使用 `GroundSpeed > 5`，`Move -> Idle` 使用 `GroundSpeed <= 5`。
- `Idle/Move -> Attack` 使用 `bIsAttacking`；`Attack -> Idle` 使用源序列自动结束。
- `Idle/Move/Attack -> Dead` 使用 `bIsDead`。
- 主 AnimGraph 按顺序连接状态机、`DefaultGroup.DefaultSlot` 攻击槽、`DefaultGroup.HitReact` 受击槽和 Output Pose。
- `bIsAttacking` 是 `UShootZombieAnimInstance` 从敌人当前近战 GA 阶段读取的表现状态；它只负责让正式 AnimBP 进入 Attack 状态，不负责决定伤害时刻。
- 近战 GA 通过 archetype 的 Attack Montage 播放动作，Montage 内的 `UShootAnimNotify_ZombieMeleeHit` 才是伤害/状态 GE 的权威触发点；Montage 结束保护计时器只负责 Notify 缺失时结束能力，不会补发伤害。
- 近战 GA CDO 的 `AbilityTags` 和 `ActivationBlockedTags` 使用 `Config/DefaultGameplayTags.ini` 中提前注册的配置 Tag；构造阶段通过 `FGameplayTag::RequestGameplayTag(FName(...), false)` 读取。修改时不能改回依赖 `UShootAssetManager::StartInitialLoading` 才填充的 `FShootGameplayTags` 字段，否则会再次出现“Ability Spec 存在但按 `Ability.Skill.Zombie.Melee` 找不到”的问题。
- 五个正式 archetype 统一配置 `/Game/AI/Zombie/AbilitySets/DA_AbilitySet_ZombieCombat`，由 `AEnemyBotCharacter::PossessedBy` 通过 `UShootAbilitySet::GiveToAbilitySystem` 授予 `UShootGA_ZombieMelee`；`UnPossessed/EndPlay` 使用 `FShootAbilitySet_GrantedHandles` 回收，旧 `StartupAbilities` 不再作为正式 Zombie 的第二授予入口。

`CharacterMesh0` 是 `AEnemyBotCharacter` 的继承组件，直接覆盖其 `AnimClass` 的服务调用已实测返回失败。
因此正式 AnimBP 通过 `AEnemyBotCharacter::TestAnimClass` 配置，`ApplyTestSkeletalMesh` 在换网格后设置该类；
`ApplyTestAnimationState` 遇到正式 AnimBP 时直接返回，避免回到 Single Node `PlayAnimation`。

## 本轮动画清理与调参结论

- 已删除 `/Game/AI/Zombie/Animation/Montages/AM_Zombie_Runner_Melee`，以及
  `/Game/Characters/Zombie/Animations/running_crawl`、`zombie_biting`、`zombie_biting__`、`zombie_crawl`。
  删除前已重新检查 AssetRegistry 和 Montage 内部引用；Runner 旧 Montage 的唯一动画依赖已先迁移到
  `AM_Zombie_Common_Melee`。
- `zombie_scream` 没有被强行塞进待机或攻击。资产审计截图显示它包含明显下蹲姿态，当前直接用于战斗会重现“站着突然爬下”的问题；暂时保留，未来若增加独立 Alert 状态再单独评估。
- `zombie_idle__3_`、`zombie_idle__4_` 已分别用于 Runner、Walker；Bruiser、Bleeder、Elite 继续使用各自已验证的直立待机。移动也保留 Walker 慢走、Bruiser 重走和 Runner/Bleeder/Elite 追跑的差异。
- `Attack Range` 是角色中心距，不是 Aggro Range，也不是胶囊表面距。五类当前分别为 145/140/160/150/155cm；这是在保留胶囊碰撞和接敌槽位的前提下，让 Notify 命中更接近手部可见接触的战斗包络。
- 伤害从 10/7/20/8/16 小幅调整为 15/12/28/12/22；仍通过原有 GAS/GE 入口结算，Bleeder 的周期流血仍为 3。

## 下一步与回归边界

- 五个 Attack 序列对应的 Montage、`UShootAnimNotify_ZombieMeleeHit` 和 Zombie 近战 GA 已完成；Notify 时会重新验证死亡状态、Team Attitude、距离、朝向和视线，再应用既有伤害/流血 GE。
- 敌我判断统一使用 `ILyraTeamAgentInterface` 的 `ETeamAttitude`，而不是 `if (Player)` 或角色类硬编码；TeamId 无法解析时才回退到 `Faction.Player` / `Faction.Enemy` 标签。
- 行为树在无 Hostile 目标时才进入 Patrol；接近攻击时会停止移动并转向目标，`UShootBTService_AttemptMeleeAttack` 按固定间隔重试，避免一次 GAS 激活失败让丧尸永久退出接敌分支。
- `UShootMeleeEngagementSubsystem` 为每个目标分配有限环形槽位；`TargetApproachLocation` 是站位，`TargetActor` 是目标，攻击 Service/GA 是事件链，三者不合并。
- 接敌 `MoveTo` 的 `AcceptableRadius` 为 20cm；Controller 默认只给同一目标分配 4 个近战槽位，
  有槽位的接近环半径为 `max(80cm, AttackRange * 0.55)`，槽位满的 Zombie 改在外围排队。
  该组合让胶囊碰撞、导航容差和攻击中心距不再把已经面向目标的 Zombie 推出命中包络。
- 2026-09-07 PIE 调优后的攻击参数为 Walker 15/1.10s/145cm、Runner 12/0.80s/140cm、
  Bruiser 28/1.50s/160cm、Bleeder 12/1.15s/150cm 加 3 点周期流血、Elite 22/0.95s/155cm；
  这些值仍属于五个 archetype 的 Class Defaults，Range 是中心距。
- 在 Montage/Notify/GAS 通过 Listen Server 与 SplitScreen 验收前，不删除旧枚举和回退字段；它们是迁移期回滚边界，不得再成为正式 AnimBP 的运行时驱动。
- Listen Server 与独立测试进程已验证统一 `IncomingDamage` / 护盾 / 生命值链；SplitScreen、高密度接敌和玩家可见的 Bleeder 数值仍需回归。
