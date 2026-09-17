# 战斗纵切与测试副本后续

## 状态

- 建立日期：2026-08-29
- 当前状态：CameraMode、伤害公式静态审计、正式 Zombie Behavior Tree 与五类 AnimBP/GAS 近战纵切已落地；Listen Server 已验证感知、黑板、移动复制、Montage/Notify 与统一伤害链；音效依赖已迁移，等待逐枪听感回归
- 本文是跨 WeaponSystem、GEDamageRegression、CombatSystem 与测试地图的后续入口，不覆盖各任务包已经验收的历史结论。

## P1 伤害模型审计

### Sniper

- 当前产品意图：狙击枪在其有效射程内不发生距离伤害损失，并维持高基础伤害。
- 超出有效射程后的行为需要结合 WeaponInstance 射程与命中 Trace 规则审计，不能把“射程内无衰减”误写成无限距离必中。
- 部分未来 Sniper 可以穿甲。穿甲应作为显式逐武器能力或伤害数据，不按 Sniper 分类硬编码给全部狙击枪。
- 审计入口：`UShootDamageExecution`、Sniper WeaponInstance 的 `DistanceDamageFalloff`、Sniper Damage GE、Armor/DamageReduction 后处理。

审计结论：

- `ID_Sniper_Rifle_A` 的 `MaxRange=50000cm` 是 hitscan Trace 的硬端点；超出端点不会命中，也不会应用 Damage GE。
- `UShootRangedWeaponInstance::GetDistanceAttenuation` 在曲线为空时返回 `1.0`，正好表达“有效射程内不损失伤害”；以后需要某把 Sniper 衰减时只在对应 `B_WeaponInstance_*` 配曲线。
- 穿甲尚未实现，继续作为显式逐武器数据/能力待办；不能把全部 Sniper 分类硬编码为穿甲。

### Rocket Launcher 与 Grenade Launcher

- 投射物从枪口飞到爆点的旅行距离不参与命中扫描武器的 `DistanceDamageFalloff`。
- 爆炸伤害只使用爆心到目标的径向距离衰减。Rocket/Grenade 可以拥有不同内外半径与曲线。
- 必须审计 `AShootProjectileBase` 的爆炸 EffectContext 与 `UShootDamageExecution`，确认没有先乘旅行距离曲线、再乘爆炸半径曲线造成双重衰减。
- 现有文档记录 Rocket 无径向衰减、Grenade 为 200/500cm 线性衰减；最终数值仍由产品平衡决定，本任务先保证公式职责正确。

审计结论：

- `AShootProjectileBase::ApplyRadialDamage` 为每个目标把 `HitResult.TraceStart` 写成爆心、`ImpactPoint` 写成目标位置；Execution 计算的是爆心距离。
- `UShootDamageExecution` 的径向分支和 hitscan 分支互斥。径向分支不会调用 `Weapon->GetDistanceAttenuation`，因此不存在先乘飞行距离、再乘爆炸半径的双重衰减。
- Rocket 当前 300/600cm、`DamageFalloff=0`，半径内不衰减；Grenade 当前 200/500cm、`DamageFalloff=1`，内半径外线性降到 0。该结论只说明公式职责正确，数值仍可在 Projectile Fragment 调整。

## P2 武器开火音效映射

- 审计 `/Game/Weapons/Pistol`、`/Game/Weapons/Rifle`、`/Game/Weapons/Shotgun` 与 `/Game/Weapons/Shotgun_A` 的角色 Montage、武器 Montage、Sequence Notify、`B_WeaponFire` 和 GameplayCue 音效来源。
- 只读对照 Lyra `/ShooterCore/Weapons` 的 Pistol/Rifle/Shotgun 配置，确认项目迁移时是否串用了枪型声音。
- 玩家已经明确听出 `Shotgun_A` 当前开火声不像霰弹枪，列为首个验证样本。
- 修复前先排除同一发射击由 Sequence、Montage 与 GameplayCue 重复播放多份声音；不能只换一个 Sound 资产而保留重复 Notify。

当前审计结论：

- Pistol、Rifle、旧 Shotgun 的角色/武器 Fire Sequence 与 Montage 没有 `AnimNotify_PlaySound`，三个目录下也没有本地 Sound 资产。
- `GCN` 是 Gameplay Cue Notify。`GCN_Weapon_*_Fire` 接收 GameplayCue 后播放一次开火音频/特效表现；它不承担命中、弹药或伤害权威。三把枪的 GCN 继承项目 `UShootGameplayCueNotify_WeaponFire`，该适配器继续调用迁入的 `B_Weapon.Fire` 表现图，因此不能在 Montage 上再补一份声音。
- Lyra 的完整 Pistol/Rifle/Shotgun 音频依赖已迁入项目。项目 `UShootGameplayCueNotify_WeaponFire` 现先调用 `B_Weapon.TriggerFireAudio`，再调用 `B_Weapon.Fire`；三个 GCN 的 `FireSound` 已分别配置 `/Game/Audio/Sounds/Weapons/Pistol/MSS_Weapons_Pistol_Fire`、`/Game/Audio/Sounds/Weapons/Rifle/MSS_Weapons_Rifle_Fire`、`/Game/Audio/Sounds/Weapons/Shotgun/MSS_Weapons_Shotgun_Fire`。该接法复用 Lyra 的连续射击 AudioComponent/Trigger Parameter 链，不在 Montage 叠加第二份声音。
- `Shotgun_A` 自带 `ShotgunA_Fire_Cue` 与四组 Wave，但当前正式 Fire Ability 是共享的 `UShootGA_Weapon_Fire_Shotgun`，所以它和旧 Shotgun 都发出 `GameplayCue.Weapon.Shotgun.Fire` 并使用同一正式霰弹枪 MetaSound；本轮不再并行播放商城 Cue。原先误用的 `/Game/NiagaraExamples/Gallery/Weapons/Rifle/Sounds/Rifle_Load01` 属于孤立商城示例资产，不应作为任何枪型的开火声。
- CDO 回读已确认三个 GCN 指向各自 MetaSound，Windows Editor 冷编译通过。最终听感仍需玩家在编辑器中逐枪开火，确认 `Pistol/Rifle/Shotgun/Shotgun_A` 各自只播放一条正确链路；资产引用与自动回读不能替代听觉验收。

## P3 可玩的 Zombie 副本纵切

- 目标地图：`/Game/Maps/TestMap_ListenServer`、`/Game/Maps/TestMap_SplitScreen`。
- 当前架构与资产操作说明见 `Docs/Tasks/ZombieAI/Overview_僵尸行为树与敌人类型.md`，该文档是后续 AI/同事接手入口。
- Zombie 多样性按“敌人蓝图子类”表达，不在 `AEnemyBotCharacter` 上随机换网格。Compatible Skeleton 只解决动画复用，不代表不同外观属于同一种敌人；每类应独立配置移动速度、Primary Attribute GE、攻击距离/节奏/伤害、动作和可选状态 GE。
- 测试地图增加明确的“AI 行动开关”：关闭时停止感知、移动和攻击，专用于相机、输入、分屏与武器验收。它是测试夹具，不进入正式 Experience 玩法规则。
- 测试记录必须区分“输入没有触发”和“Pawn 正处于死亡/重生、不能接收输入”；不能再次把后者误判为 Enhanced Input 故障。

本轮纵切实现：

- 正式黑板与行为树位于 `/Game/AI/Zombie/BB_Zombie_Melee`、`/Game/AI/Zombie/BT_Zombie_Melee`。根服务从 AI Perception 按 Team Attitude 刷新 Hostile，并保留短暂目标记忆/切换迟滞；接敌分支使用 `TargetActor` 做攻击目标、`TargetApproachLocation` 做分散追击点、Observer Aborts Both、MoveTo 与 GAS 近战任务；无目标时在 `HomeLocation` 周围选择 `PatrolLocation`。
- `AEnemyBotController` 负责 TeamId、AI Perception、黑板和行为树生命周期。它通过 `UShootAbilitySystemLibrary::GetTeamAttitudeForActors` 统一判断 Friendly/Neutral/Hostile，只会把 Hostile 写入黑板；NoTeam 不再被猜成敌人。
- `AShootEnemyTestSpawner` 现在只负责服务器生成、加权选型、维持数量和暂停/恢复 Brain。类中已删除寻敌、移动、站位、攻击、攻击冷却与直接驱动动画的旧实现，不存在需要以后再拆除的第二套 Tick AI。
- `AEnemyBotCharacter` 负责自身 ASC、AttributeSet、archetype 配置与服务器权威攻击入口。Behavior Tree 近战任务必须再次验证敌我、距离和冷却，随后用配置的 GE 写入 `SetByCaller.Damage`；最终进入统一 `IncomingDamage`、护盾、友伤、死亡与受击 Cue 链，不直接修改 Health。
- 五类敌人已经分别配置正式 `ABP_Zombie_*`。AnimBP 父类为 `UShootZombieAnimInstance`，通过 `GroundSpeed`、`bIsMoving`、`bIsFalling`、`bIsDead` 与 `bIsAttacking` 驱动 Idle/Move/Attack/Dead 状态机；`Idle/Moving/Attacking` 枚举和 `MovementUpdated` 只保留为没有正式 AnimBP 时的迁移回退层，不再由 Spawner Tick 驱动，也不决定 GAS 伤害。
- 近战入口由 Behavior Tree 调用 `TryActivateMeleeAttack`。该入口确认目标与自身的 `ETeamAttitude::Hostile`、距离、朝向和视线后停止移动并转向目标，再按 archetype 配置激活 `Ability.Skill.Zombie.Melee`。GA 播放对应 `AM_Zombie_*_Melee`，只有 `UShootAnimNotify_ZombieMeleeHit` 会应用伤害/流血 GE；结束保护计时器只处理 Notify 丢失。
- 丧尸移动使用 `bOrientRotationToMovement=true`、`bUseControllerDesiredRotation=false`，暂时关闭 `bUseRVOAvoidance`。在建立接敌槽位或切换 CrowdFollowing 前，不让避障速度向量把已锁定目标的丧尸推成“面朝主角后退/平移”。
- 首期 Walker、Runner、Bruiser、Bleeder、Elite 已是五个独立蓝图子类，不是随机换皮。每类固定网格、动作、移动速度、Primary Attribute GE、攻击距离/节奏/伤害；Bleeder 额外应用短时流血 GE，Elite 用低权重控制数量。
- `UShootEffect_EnemyBleed` 每秒把 `SetByCaller.Damage` 写入 `IncomingDamage`、持续 3 秒；同一来源重复命中刷新持续时间，不无限叠层。
- `AShootEnemyTestSpawner::SetEnemyBehaviorEnabled` 是服务器权威测试开关。关闭后继续维持人口，但暂停各敌人 Brain 并停止移动；恢复后继续同一棵 Behavior Tree。入口保持 BlueprintCallable，不在 C++ 硬编码键盘、手柄或 UI。

验证边界：

- `NewWorldOrderEditor Win64 Development` 冷编译成功，产物是基础 `UnrealEditor-NewWorldOrder.dll`。
- 两玩家 Listen Server 运行时存在服务器世界与远端客户端世界。服务器 Spawner 生成的 20 只正式敌人均启动 `BP_EnemyBotController`、`BehaviorTreeComponent` 与 `BlackboardComponent`；黑板能通过 `ILyraTeamAgentInterface`/`ETeamAttitude` 选中 Hostile 目标，无目标时才写入巡逻点并移动。
- 五类网格在客户端世界正确复制，客户端不运行权威 Controller/Behavior Tree，只接收移动与动画表现。日志持续出现 `IncomingDamage` 变更，证明近战任务进入正式 GAS 伤害链。
- 20 个近战单位同时围攻时出现过一次 CharacterMovement 互相阻挡警告；当前已关闭所有 `AEnemyBotCharacter` 的 RVO，并用 `TargetApproachLocation` 做轻量环形分散。后续如增加更高密度副本，应在 Behavior Tree/EQS 增加带容量的正式接敌槽位或升级 CrowdFollowing，再评估重新启用群体避障，而不是把环形站位写回 Spawner。
- 已验证行为树无 Hostile 目标时才进入 Patrol，拥挤 Pawn 不会被视线 Trace 当成墙；SplitScreen 画面、Bleeder 流血数值和测试 UI 按钮仍需玩家验收，AI 开关业务入口已经可用，是否增加按钮由测试地图蓝图配置。

## P4 最终玩家上手回归

- 完成上述纵切后，以正常玩家路径体验拾枪、切枪、第一/第三人称、Sniper Scope、换弹、伤害、死亡重生与敌人战斗。
- 至少覆盖 Standalone、Listen Server 远端拥有客户端和双本地玩家；测试时禁止用直接调用业务函数替代真实输入。

## 外部依赖与 Story

- 用户后续迁移 Lyra `/Game/UI/Menu/Experiences/W_ExperienceTile` 后，再接 HomeMap 副本交互物与 Experience 选择 UI。
- 生化模式默认第一人称属于后续 Story；当前 CameraMode 只提供 Experience 可选择默认视角并允许/禁止切换的基础设施。
