# Gameplay 产品、玩法与架构总规格 v0.3

## 1. 项目目标

项目采用 UE 5.8、Lyra 派生架构与 Listen Server 网络模式，最终需要支持多种 Gameplay Experience，共享角色、能力、属性、武器、伤害、死亡、复活、AI、UI、经济与进度基础设施。

长期玩法范围包括：

- HomeMap / 非战斗场景。
- 传统 PvP。
- CF / CSOL 风格生化感染模式。
- CF 风格挑战模式。
- CSOL 风格大灾变模式。
- Boss 副本、求生、推进、歼灭、守卫、护送、逃生等 PvE Objective。
- 局内金币、等级、属性强化、随机技能、商店和 Build 构筑。
- 原创 Roguelite / Build 型合作 PvE。

当前 Experience 系统来自 Lyra，但已经经过裁剪和修改。真实职责、类关系和可复用程度以现有项目代码为准。

## 2. 需求状态

文档中的需求按以下状态理解：

- **已确定**：当前架构需要支持的长期能力。
- **当前方案**：准备优先尝试，允许根据实机体验调整。
- **候选玩法**：用于验证玩法和架构扩展能力。
- **未来扩展**：当前阶段保留数据和职责边界，后续按需求实现。
- **现状待审计**：项目已经存在相关实现，需要结合真实代码确认质量和职责。

## 3. Gameplay 架构总原则

### 3.1 Experience 作为玩法装配入口

Experience 表达“当前这一局由哪些 Gameplay 模块组成”。

一个 Experience 可以组合：

- PawnData。
- AbilitySet。
- Gameplay Feature。
- Input。
- UI Layout / UI Feature。
- Gameplay Rule。
- Progression Definition。
- Encounter Definition。
- Difficulty Definition。
- Join Policy。
- Reward Definition。

Experience 负责组合，具体 Gameplay 行为由相应系统执行。

### 3.2 Gameplay Capability、Rule、Presentation 分层

系统按三个维度组织：

**Gameplay Capability**

角色或玩家拥有的数据和基础能力，例如：

- Health。
- MaxHealth。
- Armor。
- MoveSpeed。
- ASC。
- AttributeSet。
- Inventory。
- Equipment。
- Weapon。
- Interaction。
- Currency。
- Match XP。
- Skill Slot。

**Gameplay Rule**

当前 Experience 对这些能力采用的规则，例如：

- Damage 是否参与当前玩法。
- Health 到 0 后进入哪一种状态。
- Respawn 方式。
- Infection 方式。
- Friendly Fire。
- Shop Eligibility。
- Skill Grant 规则。
- Match / Round 生命周期。

**Presentation**

当前本地玩家看到的表现，例如：

- Health Bar。
- Ammo。
- Skill Bar。
- Currency。
- Attribute Upgrade。
- Round Timer。
- Team Count。
- Objective。
- Boss Health。
- Respawn UI。

三层通过数据和事件连接。UI 展示范围由当前 Experience 的 Presentation 配置决定。

### 3.3 生命周期分层

Gameplay 数据至少区分以下生命周期：

- **Persistent**：跨 Match 保存，例如账号解锁、永久成长。
- **Session**：当前服务器 Session。
- **Match**：当前比赛或副本，例如大灾变局内金币、原创 Build 技能。
- **Round**：当前 Round，例如生化士气或特定挑战回合资源。
- **Life**：当前一次生命，例如部分临时 Buff。
- **Ability Duration**：GameplayEffect 或 Ability 自身持续时间。

每一种 Currency、Level、Buff、Skill、Reward 都应明确 Scope、Reset Policy 和复制需求。

## 4. 核心类职责

### 4.1 GameMode

GameMode 负责服务器权威的 Match 流程与裁决，包括：

- Match Start / End。
- Round Start / End。
- 玩家加入资格。
- Spawn / Respawn 流程入口。
- 胜负裁决。
- Experience 相关服务器流程协调。

玩法细节由专门系统承担，例如 Infection、Encounter、Shop、Skill、Boss、Damage、Progression。

### 4.2 GameState

GameState 或 replicated GameState Component 同步全局比赛状态，例如：

- Current Round。
- Round Phase。
- Remaining Time。
- Current Objective。
- Current Stage。
- Human Count。
- Zombie Count。
- Boss State。
- Match Result。
- Team-wide Buff / Morale。

### 4.3 PlayerState

PlayerState 或其 Component 承担跨 Pawn 生命周期的玩家状态，例如：

- Team。
- Role。
- Match Currency。
- Match XP / Match Level。
- Skill Build。
- Match Statistics。
- Infection Count。
- Kill / Assist。
- Round Score。

### 4.4 Pawn / Character

Pawn 表达玩家当前控制的生命形态。

同一个 PlayerState 可能依次拥有：

```text
Human Pawn
-> Zombie Pawn
-> Zombie Respawn Pawn
```

或：

```text
Player Pawn
-> Dead
-> Respawned Pawn
```

角色形态可通过 PawnData、AbilitySet、GameplayEffect、GameplayTag、Cosmetic 等方式配置。

### 4.5 ASC / AttributeSet / AbilitySet

GAS 作为技能、Buff、Debuff、部分状态与属性修改的公共机制。

AbilitySet 可以提供：

- Gameplay Ability。
- Gameplay Effect。
- AttributeSet。

属性按照职责拆分，例如：

```text
CoreHealthSet
CoreCombatSet
PvEProgressionSet
InfectionSet
```

各 Experience 加载自身需要的集合。

### 4.6 UI

UI 从 replicated Gameplay State、PlayerState、ASC 和其他客户端可用状态读取数据。

Experience 决定当前加载的 HUD Layout 和 UI Feature。

示例：

**HomeMap**

- Interaction。
- Navigation。
- Menu。
- Social。

**大灾变**

- Health。
- Ammo。
- Match Currency。
- Attack / Health Upgrade。
- Skill Bar。
- Objective。
- Boss Health。

**生化模式**

- Health。
- Round Time。
- Human / Zombie Count。
- 当前角色技能。
- 特殊角色提示。

## 5. Listen Server 网络要求

项目以 Listen Server 为目标。

房主同时运行服务器和本地客户端，远程玩家仅运行客户端逻辑。Gameplay 权威仍然位于服务器。

服务器负责：

- Damage。
- Death / Infection Result。
- Team Change。
- Spawn / Respawn。
- Currency Reward。
- XP / Progression。
- Skill Grant / Upgrade Result。
- AI。
- Encounter。
- Boss。
- Match Result。

客户端负责：

- 输入。
- 本地 UI。
- 动画、音效、VFX 表现。
- GAS 允许的客户端预测。

Host 和 Remote Client 都需要独立验收，尤其检查 LocalPlayer / IsLocallyControlled 与 Authority 语义是否混用。

## 6. 通用属性与伤害框架

角色属性需要支持多来源叠加：

```text
Base Character
+ PawnData
+ Equipment
+ Weapon
+ Persistent Progression
+ Match Progression
+ Difficulty Modifier
+ GameplayEffect
+ Skill
+ Team Buff
+ Map Modifier
```

候选属性包括：

- Health / MaxHealth。
- Armor / MaxArmor。
- MoveSpeed。
- AttackPower。
- DamageMultiplier。
- DamageResistance。
- CriticalChance。
- CriticalDamage。
- KnockbackPower。
- KnockbackResistance。
- SkillPower。
- CooldownReduction。
- LifeSteal。
- Match Currency。
- Match Experience。
- Match Level。
- Skill Points。

武器负责产生攻击信息和基础伤害规格；最终伤害由统一 Damage Pipeline、GameplayEffect、目标状态、抗性和当前规则共同处理。

## 7. Health、Death、Death Consequence、Respawn

Health 到达 0 表示生命值状态满足阈值。随后由当前 Gameplay Rule 决定结果。

典型结果：

```text
Classic PvP:
Health <= 0 -> Death -> Timed Respawn

Infection Human:
Health <= 0 -> Infection -> Zombie

Challenge:
Health <= 0 -> Consume Life -> Respawn

Zombie Scenario:
Health <= 0 -> Respawn Timer

Hardcore PvE:
Health <= 0 -> Spectator

Downed PvE:
Health <= 0 -> Downed -> Revive / Death
```

Respawn 规则需要容纳：

- TimedRespawn。
- LimitedLives。
- SharedTeamLives。
- ReviveToken。
- DownedState。
- CheckpointRespawn。
- RespawnAtTeammate。
- RespawnAtStageStart。
- SpectateUntilEncounterEnd。

## 8. HomeMap

HomeMap 复用共享 Player、ASC、Attribute、Inventory、Equipment、Interaction、Movement 和 Cosmetic 基础设施。

当前表现目标：

- 常规战斗入口关闭。
- HUD 以交互、导航、菜单、社交为主。
- Health 等底层数据可继续存在。
- Health Bar、Combat HUD、Skill Bar 根据 Experience Presentation 配置隐藏。
- HomeMap 角色与战斗模式角色保持兼容的基础 Gameplay Contract。

---

# 9. 生化感染模式

## 9.1 模式定位

生化模式参考 CF 与 CSOL 的感染玩法。

Round 开始时玩家首先处于 Human 阵营。准备阶段结束后，服务器从有效玩家中选择少量 Initial Zombie / Mother Zombie。随后形成 Human 与 Zombie 的动态人数对抗。

Zombie 的目标是扩大感染。

Human 的目标根据模式规则包括：

- 生存到 Round Time 结束。
- 消灭可永久死亡的 Zombie。
- 完成逃生 Objective。
- 完成特殊事件。

## 9.2 Match 与 Round

Match 可以包含多个 Round。

每个 Round 独立处理：

- Human / Zombie 身份。
- Initial Zombie Selection。
- Round Buff。
- Round Winner。

Match Scope 数据和 Round Scope 数据分别定义保存策略。

典型流程：

```text
RoundWaiting
-> RoundPreparation
-> InitialInfection
-> RoundActive
-> FinalPhase（可选）
-> RoundEnding
```

## 9.3 RoundPreparation

准备阶段：

- 所有有效玩家先以 Human 身份进入。
- 玩家寻找防守位置。
- 初始化 Pawn、Ability、Equipment 和 UI。
- 显示病毒扩散倒计时。

## 9.4 Initial Infection

准备阶段结束后，服务器根据当前玩家数量计算 InitialZombieCount，并随机选择符合条件的 Human。

Initial Zombie 可以拥有：

- 更高 MaxHealth。
- 更高 Armor。
- 更高抗击退。
- 特殊 AbilitySet。
- 特殊技能。
- 特殊外观。

InitialZombieCount 通过数据配置与玩家数量映射。

## 9.5 阵营与角色

逻辑阵营：

- Human Team。
- Zombie Team。

角色身份：

- Human.Normal。
- Human.Hero。
- Human.Hunter。
- Zombie.Normal。
- Zombie.Initial。
- Zombie.Terminator。
- 未来扩展角色。

Hero、Hunter、Terminator 等仍属于对应基础 Team，Role 用于表达属性、技能和规则差异。

## 9.6 Infection 是独立 Gameplay 事件

感染流程拥有独立语义：

```text
Infection Attempt
-> Infection Validation
-> Infection Success / Resist
-> Team / Role Change
-> Pawn / PawnData / Ability Change
```

Gameplay Event 可以采用类似：

- Event.Infection.Attempt。
- Event.Infection.Success。
- Event.Infection.Resisted。

## 9.7 感染条件

系统需要支持多种 Infection Policy。

### 接触 / 有效攻击立即感染

普通 Zombie 对普通 Human 命中后，满足感染判定即完成感染。

### HP 归零后感染

特殊模式或特殊角色通过正常 Damage 先消耗 Health，Health 到 0 后进入 Infection Consequence。

适用对象例如：

- Hero。
- Hunter。
- 特殊 Human。
- 拥有多段防护的角色。

### 抵抗与延迟感染

Gameplay Rule、装备和 Ability 可以参与：

- 一次感染免疫。
- 护盾。
- 防化服。
- Infection Resistance。
- Delayed Infection。
- Infection Pending。
- Cleanse / Antidote。

## 9.8 普通感染结果

感染成功后：

- Human Team 成员减少。
- Zombie Team 成员增加。
- PlayerState 保持同一个玩家身份和 Match 数据。
- 当前 Pawn 按项目架构完成形态转换、PawnData 切换或 Respawn。
- Zombie Ability / Attribute / Cosmetic 生效。

## 9.9 Zombie Death Policy

生化变种需要支持不同 Zombie Death Policy。

### PermanentDeath

Zombie 死亡后退出本 Round。全部 Zombie 永久死亡时 Human 获胜。

### RespawnUnlessSpecialKilled

普通击杀后 Zombie 延迟复活；特殊终结方式产生 Permanent Death。

特殊终结条件可以包括：

- Melee Finisher。
- Headshot。
- Hero Weapon。
- 指定 GameplayTag / DamageType。

### InfiniteRespawn

Zombie 持续复活。胜负由 Time、Escape、Objective 等条件决定。

## 9.10 Zombie 恢复、硬直与击退

Zombie 可以具有：

- 脱战回血。
- Knockback Resistance。
- Hit Stun Resistance。
- Slow Resistance。
- Movement Modifier。

Human 武器除 Damage 外还可以产生：

- Knockback。
- Hit Stun。
- Slow。
- Break / Stagger。

空间控制属于生化模式的核心体验之一。

## 9.11 基础胜负

Zombie Victory：

- Alive Human Count 归零。
- 或模式定义的 Human Objective 失败。

Human Victory：

- Round Time 结束且仍有 Human 存活。
- 或全部 Zombie 满足 Permanent Death。
- 或完成 Escape / Special Objective。

## 9.12 Last Human / Final Phase

Human 数量达到阈值时可以触发 FinalPhase。

可触发内容：

- Human -> Hunter。
- 特殊武器。
- Health 强化。
- AbilitySet 切换。
- Zombie 强化。
- Final Phase UI / 音乐 / VFX。

触发条件支持：

- Player Count。
- Remaining Time。
- Objective。
- Gameplay Event。

## 9.13 终结者模式

终结者模式在基础 Infection 规则上增加特殊角色。

Zombie 方：

- Initial Infection 阶段生成 Terminator。
- Terminator 拥有高 Health、特殊攻击、特殊 Ability 和更高抗击退。

Human 方：

- Human 数量下降到阈值后，剩余玩家可以转为 Hunter。
- Hunter 使用 Health 机制承受 Zombie Damage。
- Hunter Health 到 0 后按当前规则进入 Infection 或 Permanent Death。

典型 Zombie 终结规则：

- 枪械击杀 -> Zombie Respawn。
- Melee / Finisher -> Permanent Death。

## 9.14 Hero / 救世主模式

Round 开始后从 Human 中选择一个或多个 Hero。

Hero 可以获得：

- 特殊武器。
- 特殊 AbilitySet。
- MaxHealth / MoveSpeed 强化。
- 团队支援能力。

Hero 仍属于 Human Team。

## 9.15 Human Morale

Human Team 可以拥有 Round Scope Morale。

Morale 来源示例：

- Permanent Kill Zombie。
- 完成特殊事件。

Morale 可以提供全队：

- DamageMultiplier。
- Reload / Move / Defense 等 Buff。

Round 结束后按配置清理。

## 9.16 Zombie Evolution

Zombie 可以通过感染、造成伤害、承受伤害、存活、击杀特殊 Human 等事件累计 Evolution / Rage。

示例：

```text
Normal Zombie
-> Evolved Zombie
-> Mother / Elite Zombie
```

升级可以改变：

- MaxHealth。
- Ability Level。
- Damage。
- Resistance。
- Cosmetic。

## 9.17 生化 Z / Progression Infection

生化模式可以叠加 Match Progression：

- Match Level。
- Match XP。
- Skill Selection。
- Currency。
- Weapon Unlock。
- Zombie Unlock。
- Shop。
- Mutation。

Progression 作为附加系统运行，感染核心规则保持独立。

## 9.18 生化逃亡 / 追击

Human 沿地图路线持续推进：

```text
Start
-> Stage A
-> Stage B
-> Stage C
-> Escape Zone
```

地图 Gameplay Actor 可以包含：

- Door。
- Elevator。
- Breakable Gate。
- Bridge。
- Defense Point。
- Checkpoint。
- Escape Vehicle。

Human 胜利规则可配置为：

- Team Escape：至少一名 Human 完成逃生即团队成功。
- Individual Escape：逐个记录成功，所有玩家状态结算后结束 Round。

Zombie 通常采用 Infinite Respawn 或 Checkpoint 后方 Respawn。

## 9.19 补给

Supply Crate 可以提供：

- Ammo。
- Weapon。
- Grenade。
- Armor。
- Temporary Buff。

Spawn Interval、Supply Table、拾取资格由规则和数据配置。

## 9.20 Join In Progress

生化模式需要独立 Join Policy，例如：

- Spectate Until Next Round。
- Join As Zombie。
- 指定阶段允许加入。

Player Disconnect 后服务器重新计算 Team Count 与胜负状态。

---

# 10. CF 风格挑战模式

## 10.1 模式定位

挑战模式属于多人合作 PvE。玩家与服务器控制的 Zombie、Monster、Elite、Boss 等 AI 战斗。

核心玩法由 Encounter、Objective、AI、Boss、Reward、Respawn 与 Difficulty 组合形成。

## 10.2 Match Flow

典型流程：

```text
Lobby / Waiting
-> Load Experience
-> Player Preparation
-> Stage / Encounter
-> Elite Encounter
-> Boss Encounter
-> Dungeon Complete
-> Reward / Result
```

地图可以采用纯 Wave、线性推进、守卫、Boss Rush 或多目标组合。

## 10.3 Encounter

Encounter 是 PvE 流程的基本组织单位。

支持类型：

- Wave Survival。
- Kill Target Count。
- Defend Target。
- Protect NPC。
- Escort。
- Reach Destination。
- Destroy Object。
- Activate Device。
- Collect Items。
- Timed Survival。
- MiniBoss。
- Boss。
- Hold Area。
- Escape。
- Continuous Spawn。
- MultiStage Objective。

Encounter Definition 可配置：

- EncounterId。
- ObjectiveType。
- SpawnDefinition。
- SpawnGroup。
- SpawnCount。
- SpawnInterval。
- EnemyPool。
- ElitePool。
- BossDefinition。
- TimeLimit。
- CompletionCondition。
- FailureCondition。
- RewardDefinition。
- NextEncounter。
- Checkpoint。
- DifficultyModifier。

## 10.4 Spawn / Encounter Director

PvE 流程需要承担以下职责的系统：

- 决定刷怪时间。
- 选择 Enemy Pool。
- 控制 Spawn Count / Budget。
- 根据玩家数缩放。
- 根据 Difficulty 调整。
- 生成 Elite。
- 切换 Boss Encounter。
- 推进 Stage。

具体由现有项目最合适的 Component、Subsystem 或 Director 承担。

## 10.5 Enemy Archetype

Enemy 可以通过数据和 GameplayTag 表达：

- Normal。
- Fast。
- Tank。
- Ranged。
- Exploder。
- Support。
- Elite。
- Boss。

Enemy 共享 Combat Contract：

- Health。
- Damage。
- GameplayEffect。
- GameplayTag。
- Ability。
- Death。
- Team Attitude。
- Combat Target。

## 10.6 Boss

Boss 继续使用统一 Combat Framework，并叠加 Boss 机制：

- Phase。
- Invulnerability。
- Weak Point。
- Breakable Part。
- Groggy / Stagger。
- Summon Adds。
- Area Attack。
- Target Selection。
- Enrage。
- Timed Mechanic。
- Environment Mechanic。

Phase 触发来源可以是：

- Health Threshold。
- Time。
- Gameplay Event。
- Objective Completion。
- Breakable Part。

Encounter 系统负责判断 Boss Encounter 是否完成；Boss 行为由 Boss 自身 Ability、AI、StateTree / BehaviorTree 或 Component 执行。

## 10.7 传统挑战

典型结构：

- 大量 AI 分波出现。
- Enemy 类型和强度逐步提升。
- Elite 出现。
- 最终 Boss。
- Boss 击败后 Dungeon Success。

## 10.8 推进型挑战

典型结构：

```text
Area A Clear
-> Door Open
-> Area B Objective
-> Defense
-> MiniBoss
-> Area C
-> Boss
```

地图通过 Area Trigger、Objective、Checkpoint、Gameplay Event 推进。

## 10.9 守卫挑战

玩家保护 Defense Target：

- Generator。
- Vehicle。
- NPC。
- Gate。
- Core。

Objective 可以拥有独立 Health 和 Failure Condition。

## 10.10 试炼挑战

试炼玩法可组合：

- Character Class。
- Deployable / Defense Device。
- Active Skill。
- Passive Skill。
- Skill Point。
- PvE Level。
- Attribute Growth。
- Round Objective Rotation。

## 10.11 Lives / Respawn

挑战模式可以采用有限生命：

```text
Player Death
-> Consume Life
-> Life Remaining > 0 -> Respawn
-> Life Remaining == 0 -> Spectator / Other Rule
```

也可以使用 Revive Token、Timed Respawn、Checkpoint Respawn 等规则。

---

# 11. CSOL 风格大灾变

## 11.1 模式定位

大灾变属于多人合作 PvE。玩家清理大量 AI、获得局内资源、强化自身、完成地图 Objective、击败 Elite 与 Boss。

主要玩法类型包括：

- 求生。
- 追击 / 推进。
- 歼灭。
- Boss / MultiStage。

## 11.2 求生

核心目标是在持续出现的敌人中维持团队战斗能力。

推进条件可以使用：

- Wave。
- Time。
- Enemy Count。
- Stage。

成功条件：完成全部 Stage / Survival Objective。

失败条件：全队进入无法继续战斗的状态或关键 Objective 失败。

## 11.3 追击 / 推进

玩家沿地图持续推进：

```text
Spawn
-> Clear Obstacle
-> Area A Combat
-> Activate Device
-> Area B Defense
-> Area C
-> Boss
```

Map Gameplay 可以包含：

- Door。
- Breakable Wall。
- Generator。
- Elevator。
- Vehicle。
- NPC。
- Trigger。
- Checkpoint。
- Boss Arena。
- Objective Actor。

## 11.4 歼灭

Objective 可以要求：

- Kill N Normal Enemies。
- Kill N Elites。
- Kill Commander。
- Destroy Spawn Nest。
- Kill Boss。

多个 Objective 完成后 Scenario Complete。

## 11.5 Match Currency

玩家通过以下行为获得 Match Currency：

- Kill。
- Damage / Assist。
- Objective。
- Elite / Boss Reward。
- Event Reward。

Match Currency 在本次 Match 内持续存在，并在 Match End 按定义清理。

Persistent Currency 与 Match Currency 使用独立数据定义。

## 11.6 局内属性强化

玩家可以使用 Match Currency 强化：

- Health Level。
- Attack Level。
- Armor Level。
- MoveSpeed Level。
- Skill Level。

属性强化通过 Progression Data、GameplayEffect 和 Attribute Modifier 作用到统一属性系统。

## 11.7 Respawn 与 Checkpoint

Respawn 可以受到以下因素影响：

- Difficulty。
- Map。
- Item。
- Checkpoint。
- Current Encounter。

Checkpoint 可以提供：

- Player Respawn Point。
- AI Spawn Boundary。
- Join In Progress Spawn。
- Objective Resume Point。
- Map Progress Anchor。

## 11.8 中途加入

PvE Experience 可以允许 Join In Progress。

新玩家进入后根据当前 Stage 获得：

- 合适 Spawn Point。
- 当前副本必要的基础装备。
- 与进度匹配的初始状态或补偿资源。

---

# 12. 原创 PvE Build Experience

## 12.1 玩法定位【候选玩法】

四人合作 PvE，使用 CF 挑战的 Round 感和 CSOL 大灾变的 Match 成长感，加入随机技能 Build、商店、保护目标和轻度 Roguelite 选择。

## 12.2 Story

玩家保护一名老人 NPC，在 Zombie / Monster 攻击下完成多个 Round / Encounter。

老人可以同时承担：

- Protect Objective。
- 普通商店。
- 技能导师。
- 技能升级。
- 治疗 / 补给。
- 剧情节点。

部分地图可以采用：

- 老人死亡 -> Match Fail。
- 老人 Health 影响商店价格。
- 老人 Health 影响技能升级功能。
- 老人状态影响后续 Encounter。

## 12.3 Match 循环【当前方案】

```text
进入副本
-> Round / Encounter
-> Kill / Objective 获得 Match Gold
-> Round Clear
-> Skill / Upgrade 三选一
-> 老人商店补给与稳定成长
-> 神秘商人事件（可选）
-> 下一 Round
-> Elite
-> Boss
-> Match End
-> Match Gold / Skill Build / Match Level 清理
```

## 12.4 Gold 生命周期【当前方案】

原创 Build Experience 默认使用 Match Scope Gold。

Gold 来源：

- Kill Zombie。
- Elite / Boss。
- Assist。
- Objective。
- Round Reward。
- 特殊事件。

Gold 用途：

- Weapon。
- Ammo。
- Heal。
- Armor / Consumable。
- Skill Upgrade。
- Attribute Upgrade。
- Reroll。
- 神秘商人交易。

框架同时保留 Round Scope Currency 的能力，供其他 Challenge Experience 使用。

## 12.5 四个主动技能槽【当前方案】

玩家同时装备最多 4 个主动技能。

默认键位：

```text
C / Q / E / X
```

Input Mapping 支持玩家改键。

技能槽数据至少包含：

- Skill Definition。
- Skill Level。
- Slot Index。
- Input Action。
- Cooldown。
- Charge / Stack（按技能需要）。
- Icon / Presentation Metadata。

## 12.6 技能获取【当前方案】

每完成一个 Round，或达到指定 Stage / Objective 条件时，玩家获得一次随机技能选择。

基础规则：

```text
Generate 3 Candidates
-> Player Select 1
-> Grant / Upgrade / Replace
```

可扩展控制：

- Gold Reroll。
- Elite / Boss 特殊候选。
- 商人 Skill Pool。
- Character / Equipment 权重。
- Rare Event。

## 12.7 技能槽未满

候选可以提供新的主动技能。

获得后分配到空闲槽。

## 12.8 四槽已满

后续 Round 继续提供构筑选择：

- 重复 Skill -> Upgrade。
- 新 Skill -> Replace Existing Slot。
- Passive Upgrade。
- Mutation。
- Gold / Rare Reward。

## 12.9 Skill Level【当前方案】

主动技能最高三级：

```text
Lv1
-> 获得同一 Skill / 稳定升级
-> Lv2
-> 再次升级
-> Lv3
```

升级方式可由随机抽取、老人商店或特殊奖励提供。

## 12.10 定身术示例

**Lv1**

- 定住附近 1 个敌人。

**Lv2**

- 定住附近多个敌人。

**Lv3**

- 最多定住 5 个敌人。
- 目标选择优先 Elite Zombie。

实际范围、Duration、Boss CC Resistance 由平衡数据决定。

## 12.11 技能风格示例

玩法可以使用具有强识别度和轻松感的技能：

- 宠物鸡哥。
- 肌肉鸡哥。
- 铁山靠。
- 定身术。
- 召唤。
- 位移。
- 治疗。
- 控制。
- 范围爆发。

## 12.12 Build 协同【候选玩法】

技能之间可以产生组合关系，例如：

```text
定身术 + 铁山靠
-> 对控制目标增加破甲 / Damage / Stagger

冰冻 + 爆炸
-> 冰冻目标死亡产生范围爆炸

鸡哥 + 召唤强化
-> 召唤物继承玩家部分属性
```

Build 协同用于形成每局差异和队伍分工。

## 12.13 普通商店

普通商店承担稳定成长：

- Weapon。
- Ammo。
- Heal。
- Armor。
- Consumable。
- Skill Upgrade。
- Attribute Upgrade。

老人可以作为普通商店和技能导师的世界内载体。

## 12.14 神秘商人【候选玩法】

神秘商人承担高风险、高收益选择：

- Rare Skill。
- 随机升级。
- 未知奖励。
- 特殊融合。
- 带代价的强力效果。

风险应对应潜在收益，并形成多人合作中的讨论和记忆点。

## 12.15 技能融合与 Mutation【未来扩展】

未来可以支持：

- Skill Fusion。
- Lv3 Branch Evolution。
- Skill Mutation。
- Random Modifier。
- 同名 Skill 的不同终局形态。

第一阶段只需要 Skill Definition、Grant、Remove、Slot、Level、Upgrade、Replacement 具备清晰的数据与职责边界。

## 12.16 可玩性目标

重点验证：

- 基础射击与打怪反馈。
- Zombie 数量压力和空间控制。
- Skill Feedback。
- Build 协同。
- Round 后选择期待。
- 四人团队分工。
- Randomness 与玩家控制的平衡。
- Mid / Late Match 持续成长。
- Boss 对 Build 的检验。

建议 Vertical Slice 范围：

- 4 Player。
- 约 10 个 Round / Encounter。
- 1 个老人 Objective。
- 5 种普通 Enemy。
- 2 种 Elite。
- 1 个 Boss。
- 15~20 个 Skill。
- 4 主动槽。
- 3 级 Skill。
- Round Clear 三选一。
- Match Gold。
- 普通商店。
- 1 类神秘商人事件。

---

# 13. Progression、Currency 与 Shop

## 13.1 Progression

项目可能同时存在：

- Account Level。
- Character Level。
- Weapon Level。
- PvE Permanent Progression。
- Match Level。
- Skill Level。
- Attribute Upgrade Level。

每一种进度拥有独立 Definition 和生命周期。

## 13.2 Currency

Currency 按 Scope 区分，例如：

- Persistent Currency。
- Match Currency。
- Round Currency。

Currency System 提供通用账户和交易能力；具体 Reward Source 与 Shop Eligibility 由 Experience / Rule 配置。

## 13.3 Shop

Shop Definition 可以定义：

- Currency Type。
- Item Pool。
- Skill Pool。
- Unlock Condition。
- Team / Role Eligibility。
- Level Requirement。
- Round / Stage Requirement。
- Price Rule。

同一 Shop Framework 可以服务 HomeMap 永久商店、大灾变局内商店、生化 Human / Zombie 商店和原创 PvE 商人。

---

# 14. Objective、Encounter、Checkpoint 与 Difficulty

## 14.1 Objective

统一 Objective Framework 支持：

- Start。
- Progress。
- Complete。
- Fail。
- Cancel。
- Replication。

Objective 类型：

- Kill。
- Survive。
- Defend。
- Escort。
- Interact。
- Reach。
- Escape。
- Collect。
- Destroy。
- Boss。
- Capture。
- MultiStage。

UI 订阅 Objective 状态并显示目标、进度、剩余时间和相关 Boss 信息。

## 14.2 Checkpoint

Checkpoint 可以服务：

- Respawn。
- Join In Progress。
- AI Spawn Boundary。
- Objective Progress。
- Map Progress。

Checkpoint 是否保存完整副本状态由具体 Experience 定义。

## 14.3 Difficulty

Difficulty 作为数据修改层，可以影响：

- Enemy Health。
- Enemy Damage。
- Enemy Count。
- Spawn Rate。
- Elite Chance。
- Boss Mechanic。
- Objective Time。
- Player Respawn。
- Revive Cost。
- Reward。
- Friendly Fire。

---

# 15. 当前 PvE Zombie 实现【已审计：联网战斗 POC】

2026-09-02 已结合 C++、Blueprint Class Defaults、Blackboard 和 Behavior Tree 真实资产完成审计。

当前主线由以下资产和类组成：

- `AEnemyBotCharacter` / `BP_EnemyBotCharacter`。
- `AEnemyBotController` / `BP_EnemyBotController`。
- `BB_Zombie_Melee`。
- `BT_Zombie_Melee`。
- `UShootBTService_RefreshHostileTarget`、`UShootBTTask_FindPatrolLocation`、`UShootBTTask_MeleeAttack`。

已经具备、可以保留的基础：

- `AEnemyBotController` 继承 `AModularAIController`，负责 AI Perception、Team Attitude、黑板初始化和 Behavior Tree 生命周期。
- Blackboard 已定义 `TargetActor`、`HomeLocation`、`PatrolLocation`，目标只接受统一阵营接口判定出的 Hostile。
- Behavior Tree 已形成两条实际分支：有目标时 `MoveTo -> MeleeAttack -> Wait`，无目标时在出生点附近随机巡逻。
- `AEnemyBotCharacter` 拥有 Character 自身 ASC、AttributeSet、敌方 Faction、服务器权威攻击距离与冷却校验，并通过 GAS/GE 结算伤害。
- 动画状态枚举会复制到模拟端，因此该 POC 已覆盖基础 Listen Server 表现同步，不是纯本地假人。

仍属于 POC、不能作为正式敌人或伙伴 AI 交付的部分：

- `BP_EnemyBotCharacter` 的 Mesh 使用 `Animation Blueprint` 模式，但 `Anim Class=None`，项目内 `/Game/AI/Zombie` 也不存在 AnimBP。
- `AEnemyBotCharacter::ApplyTestAnimationState` 通过 `PlayAnimation` 把 Mesh 切入 Single Node，只有 Idle、Moving、Attacking 三态，没有 Locomotion StateMachine、BlendSpace、Montage Slot、Hit React 和 Death 动画链。
- 近战伤害在行为树任务执行时立即结算，攻击动画没有 Notify 驱动的 Damage Window，动作与命中时序无法可靠对应。
- 目标选择只有视野内最近 Hostile；没有威胁值、目标迟滞、路径失败恢复、包围站位、特殊状态、技能决策或复杂战术。
- 行为树节点存在不等于 AI 已达到生产级；当前树适合验证感知、追击、GAS 伤害与网络复制，不适合直接复制成机器人伙伴、Elite 或 Boss。

因此用户观察到的动画突变不是误判，而是 Single Node 回退路径的预期缺陷。正式实现必须以 AnimBP、动画状态机、Montage/Notify 和 AI 行为共同驱动，不能继续扩充这条回退路径。

## 15.1 Zombie 动画目标

正式 PvE Enemy 至少具备连贯的：

```text
Idle
<-> Walk / Run
-> Attack
-> Hit React
-> Death
```

动画状态需要和 AI 行为、Movement、Attack Timing、Damage Window、Hit Reaction 与网络表现保持一致。

生化模式可先复用现有 Zombie 抓人资产完成玩法验证；动画资产不足属于后续内容生产问题，Gameplay 架构应允许替换 AnimBP、Montage 和 Animation Set。

机器人伙伴的完整设计、Lyra 对照和强制验收标准见 `RobotCompanion_机器人伙伴技能设计与验收.md`。

---

# 16. UI 与视觉验收

## 16.1 技能栏

四技能栏参考用户提供的紧凑 HUD 方向。

单个 Slot 表达：

- Icon。
- Key。
- Cooldown。
- Skill Level。
- Charge / Stack。
- Disabled / Available。

默认输入：C / Q / E / X。

最终位置、尺寸、图标可读性、动效和遮挡由用户在实际分辨率下验收。

## 16.2 用户视觉验收

涉及动画、VFX、UMG 和 Gameplay Feel 的任务，在技术验证之后提供明确的 PIE 验收项。

典型验收：

- Idle / Walk / Run Blend。
- Zombie Turn / Acceleration。
- Attack 进入与退出。
- Attack Damage Timing。
- Hit React 打断关系。
- Death Transition。
- Skill VFX 可读性。
- UI 视野遮挡。
- Skill Icon / Key / Cooldown 可读性。
- 不同分辨率布局。
- Host / Remote Client 表现一致性。

编译、自动测试和日志负责技术层验证；实机观感由用户验收。

---

# 17. 架构验收场景

现有架构需要能够自然表达以下场景：

### HomeMap

Health 和 ASC 可以继续存在，HUD 只显示 Home Presentation。

### 生化感染

同一 PlayerState 从 Human Pawn 转换为 Zombie Pawn，Match Statistics 和玩家身份连续存在。

### 生化特殊角色

普通 Human 使用接触感染；Hunter 使用 Health Damage，Health 到 0 后进入 Infection Consequence。

### Challenge Respawn

同一 Health / Death 基础框架可以切换 Limited Lives、Timed Respawn、Checkpoint Respawn。

### 大灾变成长

Match Currency、Attack Level、Health Level 在 Match 内持续，Match End 后按规则清理。

### Original PvE Build

Round Clear 触发 Skill Choice；Skill Slot 最多 4 个；重复 Skill 升级；四槽满后可替换；Match End 清理 Build。

### UI Presentation

同一个 Gameplay Capability 在不同 Experience 下展示不同 HUD，而底层 Gameplay 状态保持一致的数据契约。

### Boss

Boss Phase、Weak Point、Adds 和 Objective 通过 Boss / Encounter 系统协作，Match Flow 只接收 Encounter Completion。

---

# 18. 当前开发阶段的架构目标

当前阶段优先完成现有 Experience、GameMode、GameState、PlayerState、Pawn、ASC、Health、Death、Respawn、GameFeature、UI 与 PvE POC 的真实代码审计。

后续实现以现有项目可复用基础为起点，并确保以下长期能力具有明确扩展位置：

- Infection。
- PvE Encounter。
- Objective。
- Boss。
- Match / Round Progression。
- Currency。
- Skill Slot / Skill Choice / Skill Upgrade。
- Shop。
- Respawn Policy。
- UI Presentation。

系统名称和具体类结构以项目真实代码为准。
