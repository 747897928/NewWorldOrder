我理解你现在真正想告诉 Codex 的不是“再加两个 GameMode”，而是：

**这个项目最终会同时包含差异非常大的玩法，但它们仍然共享角色、属性、技能、伤害、死亡、复活、经济、武器、UI 等基础能力。架构必须允许某个模式启用、禁用、隐藏、替换这些能力，而不是围绕模式名称不断追加条件判断。**

这个要求应该在 Codex 开始继续抄 Lyra 之前就钉死。尤其 Lyra 本身把 AbilitySystem 放在 PlayerState 一侧，Pawn 更换后能力系统仍可持续存在，这对你以后“人类→僵尸”“PVE 死亡→重生”“切换角色形态”都很有价值。([Epic Games Developers](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/abilities-in-lyra-in-unreal-engine?utm_source=chatgpt.com))

下面这份我建议和上一份生化模式规则**一起交给 Codex**。

# 项目最终玩法目标与通用 Gameplay 架构要求

## 1. 本文目的

不要把当前任务理解为：

“参考 Lyra 实现一个新的 GameMode。”

也不要认为当前只需要让某一张地图成功开始、生成玩家、结束比赛，就已经完成玩法框架。

本项目最终会同时存在多种差异很大的游戏模式。

至少包括：

1. HomeMap / 非战斗场景
2. 生化感染类 PvP/PvPvP 模式
3. CF 风格挑战模式
4. CSOL 风格大灾变模式
5. 未来增加的原创 PvE 副本
6. 带局内等级、属性强化、随机技能、金币和商店的模式
7. 不公开属性和技能系统，但仍然复用统一底层角色系统的模式

因此现在设计 Experience、GameMode、GameState、PlayerState、Pawn、AbilitySystem、UI 时，必须以最终项目为目标。

不要针对当前地图做一次性实现。

------

# 2. 最重要的设计原则

本项目不是：

Mode A 有一套角色代码。

Mode B 再复制一套角色代码。

Mode C 再修改一套伤害代码。

正确方向应该是：

通用 Gameplay 能力
+
不同 Gameplay Rule
+
不同 Progression Rule
+
不同 Respawn Rule
+
不同 Objective Rule
+
不同 UI Presentation
+
不同 Encounter Content

最终组合成为一个具体 Experience。

可以概括成：

# Experience

Gameplay Capabilities
+
Rule Set
+
Progression Set
+
Presentation Policy
+
Map / Encounter Content

Experience 是装配玩法。

Experience 本身不应该成为所有玩法逻辑的巨大代码容器。

------

# 3. 不允许以模式名称驱动底层代码

避免出现大量类似：

```cpp
if (GameMode == Home)
{
}
else if (GameMode == Infection)
{
}
else if (GameMode == Challenge)
{
}
else if (GameMode == ZombieScenario)
{
}
```

尤其禁止这种判断逐渐扩散到：

Character

Weapon

HealthComponent

Ability

PlayerState

HUD

Respawn

AI

Inventory

Shop

Damage

Interaction

之中。

这种代码短期实现最快，但随着模式增加会形成大量耦合。

以后加入一个模式，可能需要修改十几个已有系统。

这是本项目需要避免的架构。

------

# 4. 模式决定“规则组合”，而不是底层能力是否存在

必须区分三个概念：

## Capability Exists

角色是否具有某种基础能力或数据。

例如：

Health

MaxHealth

Armor

MoveSpeed

AttackPower

DamageMultiplier

Currency

Experience

Level

SkillPoints

AbilitySystem

Inventory

WeaponSystem

Interaction

DeathState

这些属于通用 Gameplay Capability。

------

## Rule Enabled

当前 Experience 是否允许某个 Gameplay 行为真正生效。

例如：

是否允许受到伤害。

是否允许死亡。

死亡后是否复活。

是否允许感染。

是否允许购买。

是否允许升级。

是否掉落金币。

是否存在 Friendly Fire。

是否存在倒地。

是否允许技能。

------

## Presentation Visible

玩家当前是否能够在 UI 中看到这个系统。

这是第三件完全不同的事情。

例如：

角色拥有 Health。

不代表 HUD 必须显示 Health Bar。

角色拥有 AbilitySystem。

不代表必须显示技能栏。

角色拥有属性。

不代表必须显示属性面板。

角色拥有 Currency。

不代表任何地图都必须显示金币。

------

# 5. HomeMap 是最简单的例子

HomeMap 不应该因为“不战斗”就使用一套完全不同的玩家角色架构。

玩家仍然可以使用统一：

PlayerState

AbilitySystem

AttributeSet

Pawn

Character

Inventory

Equipment

Interaction

Movement

Cosmetic

等基础系统。

例如角色底层仍然可以拥有：

Health = 100

MaxHealth = 100

MoveSpeed

Level

ASC

但是 HomeMap 的规则可能配置为：

Damage Rule：
禁止常规战斗伤害。

Death Rule：
不进入战斗死亡流程。

Respawn Rule：
无战斗复活机制。

Combat Rule：
关闭主动战斗。

Skill Rule：
不授予战斗技能，或者授予但禁止激活。

HUD Policy：
不显示生命值。

HUD Policy：
不显示技能栏。

HUD Policy：
不显示战斗准星。

Interaction Rule：
启用场景交互。

Movement Rule：
正常启用。

这不是：

```cpp
if (HomeMap)
{
    HideHealth();
}
```

而应该是 HomeMap Experience 选择了一套：

NonCombatRuleSet

和：

HomePresentationPolicy

底层 Health 系统本身不需要知道什么叫 HomeMap。

------

# 6. “存在”和“显示”必须彻底解耦

这是项目的重要设计要求。

例如同样存在：

Health
MaxHealth
Armor
AttackPower
MoveSpeed
CriticalChance

在不同 Experience 中可能表现为：

## HomeMap

底层存在。

玩家看不到。

通常不会发生战斗变化。

------

## 经典 PvP

Health 显示。

攻击力等高级属性不显示。

技能可能不存在可操作入口。

------

## 生化模式

Health 显示。

Zombie Health 可能使用特殊 HUD。

部分隐藏属性用于：

抗击退。

感染抗性。

技能冷却。

移动速度。

但不一定全部展示给玩家。

------

## 挑战模式

Health 显示。

攻击力可能允许强化。

可能显示当前等级。

可能显示金币。

可能显示技能。

------

## 大灾变

Health 显示。

可以存在：

生命强化等级。

攻击强化等级。

局内金币。

角色技能。

特殊 Buff。

Boss Health。

Objective。

------

因此：

Attribute System 决定数据。

Gameplay Rule 决定数据如何参与游戏。

UI Policy 决定哪些数据展示给玩家。

三个系统不要互相混为一谈。

------

# 7. 通用角色属性层

建议从现在就允许角色拥有统一的 Gameplay Attribute 基础。

不要求现在一次实现所有属性。

但是架构不能假定：

玩家永远只有 Health。

建议未来至少可以容纳：

Health

MaxHealth

Armor

MaxArmor

MoveSpeed

AttackPower

DamageMultiplier

DamageResistance

CriticalChance

CriticalDamage

KnockbackPower

KnockbackResistance

SkillPower

CooldownReduction

LifeSteal

Currency

MatchExperience

MatchLevel

SkillPoints

以及未来模式自己的属性。

属性应该允许被：

GameplayEffect

Equipment

Weapon

Character

Experience

Difficulty

Skill

Buff

Debuff

临时修改。

不要把：

“挑战模式攻击力 +20%”

直接写进枪械伤害公式。

------

# 8. 属性来源必须允许叠加

最终角色属性可能来自多个来源。

例如：

Base Character
+
PawnData
+
Equipment
+
Weapon
+
Permanent Progression
+
Match Progression
+
Difficulty Modifier
+
Temporary GameplayEffect
+
Skill
+
Team Buff
+
Map Modifier

得到最终结果。

例如：

FinalDamage =
BaseWeaponDamage
× CharacterDamageModifier
× MatchAttackModifier
× SkillModifier
× DifficultyModifier
× TargetModifier

具体公式以后设计。

现在首先保证系统没有假定：

“攻击力只来自武器。”

------

# 9. 技能系统也是通用能力，不是某个模式的专属功能

未来可能出现：

Human Skill

Zombie Skill

Challenge Skill

Dungeon Skill

Character Passive

Weapon Skill

Temporary Random Skill

Boss Mechanic Ability

Interaction Ability

因此不要创建一套：

ChallengeSkillSystem

再创建：

ZombieSkillSystem

再创建：

DungeonSkillSystem。

优先复用 GAS。

不同 Experience 决定：

授予哪些 AbilitySet。

允许哪些 Ability 激活。

哪些 Ability 有输入。

哪些 Ability 是被动。

哪些 Ability 不对 UI 展示。

------

# 10. “没有技能 UI”不等于“没有 Ability”

这是一个明确要求。

例如普通 PvP 模式：

玩家可能没有主动技能栏。

但底层仍然可能通过 Ability 实现：

死亡。

冲刺。

换弹相关 Gameplay 行为。

装备被动。

伤害触发。

角色被动。

因此 Presentation 不应该决定 Ability 是否存在。

------

# 11. CF 挑战模式总体定义

CF 风格挑战模式属于：

多人合作 PvE。

玩家阵营基本保持为 Human / Player Team。

敌方主要由服务器控制的 AI：

Zombie

Monster

Elite

Boss

Mechanical Enemy

等组成。

不存在经典生化模式那种：

Human 被 Zombie 攻击后加入 Zombie Team

的核心感染循环。

CF 挑战模式历史上存在多种玩法，而不只是固定刷怪。

经典模式包含大量敌人、阶段或波次以及最终 Boss；一些地图属于推进型任务；后来的试炼挑战又增加了不同回合目标、防御设施、角色职业和 PVE 属性成长。

因此本项目中的 Challenge Framework 必须能够表达多种 PvE Objective，而不是只写：

KillAllEnemiesOfWave()。

------

# 12. 挑战模式基础 Match Flow

一种典型流程：

Lobby / Waiting

↓

Load Experience

↓

Player Preparation

↓

Stage Start

↓

Encounter 1

↓

Encounter 2

↓

Encounter 3

↓

Elite Encounter

↓

Boss Encounter

↓

Dungeon Complete

↓

Reward / Result

但是不要假定所有地图都拥有固定数量 Wave。

------

# 13. Encounter 是比 Wave 更高一级的抽象

推荐使用 Encounter 概念。

Encounter 可以是：

Wave Survival

Kill Target Count

Defend Target

Protect NPC

Escort

Reach Destination

Destroy Object

Activate Device

Collect Items

Timed Survival

MiniBoss

Boss

Puzzle

Hold Area

Escape

Continuous Spawn

或者这些目标组合。

例如：

Encounter 1：
消灭 50 个 Zombie。

Encounter 2：
守卫发电机 90 秒。

Encounter 3：
打开门并前往下一个区域。

Encounter 4：
击败 Elite。

Encounter 5：
击败 Boss。

它们都属于同一张 Challenge Map。

因此：

Stage != Wave。

Wave 只是 Encounter 的一种实现。

------

# 14. Encounter Definition 应数据驱动

一个 Encounter 应能够定义类似：

EncounterId

ObjectiveType

SpawnDefinition

SpawnGroup

SpawnCount

SpawnInterval

EnemyPool

ElitePool

BossDefinition

TimeLimit

CompletionCondition

FailureCondition

RewardDefinition

NextEncounter

Checkpoint

DifficultyModifier

不要把一整张副本流程全部硬编码在 GameMode Tick 中。

------

# 15. Spawn Director

PvE 需要独立的 Encounter / Spawn Director。

它负责：

什么时候刷怪。

刷什么怪。

刷多少。

从哪里刷新。

是否动态追加怪物。

是否按照当前玩家人数缩放。

是否受到难度影响。

是否生成 Elite。

是否进入 Boss Phase。

GameMode 不应该自己 Spawn 每一只 Zombie。

GameMode 负责大的 Match Flow。

Encounter Director 负责 PvE Encounter。

------

# 16. Enemy Definition

敌人不能只是：

ZombieCharacter。

至少需要能够表达：

Enemy.Archetype.Normal

Enemy.Archetype.Fast

Enemy.Archetype.Tank

Enemy.Archetype.Ranged

Enemy.Archetype.Exploder

Enemy.Archetype.Support

Enemy.Archetype.Elite

Enemy.Archetype.Boss

每类敌人可以通过：

PawnData

AbilitySet

AttributeSet

BehaviorTree / StateTree

GameplayEffects

GameplayTags

Equipment

配置出不同能力。

Boss 不应该完全使用另一个无法复用的游戏体系。

Boss 仍然是拥有：

Attributes

Abilities

Damage

State

Tags

AI

的 Combat Entity。

只是拥有特殊：

Boss Phase

Boss Mechanic

Boss UI

Boss Objective。

------

# 17. Boss 战

Boss Fight 不应该仅理解为：

生成一个 HP 很大的 Zombie。

Boss 可以具有：

Phase 1

Phase 2

Phase 3

Invulnerable Phase

Weak Point

Breakable Parts

Groggy / Stagger

Summon Adds

Area Attack

Target Selection

Enrage

Timed Mechanic

Interaction Mechanic

Environment Mechanic

Boss Phase 应能够由：

Health Threshold

Time

GameplayEvent

Objective Completion

Breakable Part

触发。

例如：

Boss HP 70%
→ Phase 2

Boss HP 30%
→ Enrage

Destroy Shield Generator
→ Remove Invulnerable

不要在 GameMode 中写具体 Boss 行为。

Boss 自己的 Ability / Component / StateTree 负责行为。

GameMode / Encounter 只关心：

Boss Encounter 是否完成。

------

# 18. CF 风格传统挑战

典型玩法：

玩家进入 PvE 地图。

大量 AI 敌人分波出现。

玩家需要持续存活并消灭敌人。

随着进度增加：

敌人种类增加。

敌人数值增强。

Elite 出现。

最终进入 Boss 战。

Boss 被击败后：

Dungeon Success。

------

# 19. CF 风格推进型挑战

另一种 Challenge 不是一直待在一个竞技场。

流程可以是：

Area A
→ Clear Enemies
→ Door Open

Area B
→ Reach Objective
→ Defend

Area C
→ MiniBoss

Area D
→ Boss

所以需要：

Checkpoint

Area Trigger

Objective Trigger

Encounter Activation

Map Progress

而不是认为：

RoundNumber++ 就能表达整个副本。

------

# 20. CF 风格守卫挑战

游戏目标可以变成保护一个世界 Actor。

例如：

DefenseCore

Generator

Vehicle

NPC

Gate

玩家失败条件可能不是：

AllPlayersDead

而是：

DefenseTarget.Health <= 0

因此 Objective System 必须可以拥有自己的 Health 和 Failure Condition。

------

# 21. CF 风格试炼挑战

未来希望支持类似：

角色职业。

防御机械。

主动技能。

被动属性。

技能点。

PvE 等级。

每个阶段不同任务。

这些内容应该建立在同一个 Challenge Framework 上。

不是制作新的完全独立 GameMode。

例如：

Challenge Core
+
Class System
+
Deployable System
+
PvE Progression
+
Objective Rotation

即可组成试炼类玩法。

------

# 22. CSOL 大灾变总体定义

CSOL 风格大灾变同样属于：

多人合作 PvE。

核心不是玩家之间相互感染。

玩家需要：

杀死大量 AI 敌人。

获得局内资源。

强化自身。

持续完成地图目标。

击败 Elite。

击败 Boss。

最终通关。

大灾变历史上存在多种地图规则，至少应理解：

求生。

追击。

歼灭。

以及不同 Objective 的组合。

因此不要创建一个：

AZombieScenarioGameMode

里面直接写死一套流程。

应该复用 PvE Scenario Framework。

------

# 23. 大灾变求生型规则

核心目标：

在持续出现的大量敌人中存活。

可以按照：

Wave

Time

EnemyCount

Stage

推进。

例如：

Wave 1

Wave 2

Wave 3

...

Final Wave

Boss

玩家需要在敌人持续增强的情况下保持团队存活。

Failure Condition 可以是：

所有玩家进入无法继续战斗状态。

Success Condition 可以是：

完成全部 Stage。

或者完成指定生存目标。

------

# 24. 大灾变追击型规则

追击比传统 Wave 更接近一个线性 PvE Dungeon。

例如：

Spawn

↓

清除障碍

↓

进入区域 A

↓

击杀敌人

↓

开启机关

↓

进入区域 B

↓

防守

↓

进入区域 C

↓

Boss

玩家需要持续向地图深处推进。

地图自身可以包含：

Door

BreakableWall

Generator

Elevator

Vehicle

NPC

Trigger

Checkpoint

BossArena

Objective Actor

这些属于 Map Gameplay。

GameMode 不应该知道：

“第三扇门什么时候打开”。

Encounter / Objective System 应通过 Gameplay Event 驱动地图变化。

------

# 25. 大灾变歼灭型规则

歼灭型目标更强调：

清除指定数量或指定类别的敌人。

例如：

Kill 100 Normal Enemies

Kill 5 Elites

Kill Commander

Destroy Spawn Nest

Kill Boss

完成全部 Objective 后：

Scenario Complete。

------

# 26. 大灾变局内金币

玩家击杀敌人、造成伤害、完成 Objective 等行为可以获得：

Match Currency。

该货币属于：

当前副本 / 当前 Match 的临时经济。

它可能在离开副本后清空。

未来也可以同时存在：

Persistent Currency

和：

Match Currency。

两者不能混为一个 int Money。

------

# 27. 大灾变局内属性强化

玩家可以使用 Match Currency 强化属性。

例如：

Health Level

Attack Level

Armor Level

MoveSpeed Level

Skill Level

具体属性以后决定。

例如：

AttackLevel 1
→ DamageMultiplier + X

AttackLevel 2
→ DamageMultiplier + Y

这里应该通过：

Progression Data

GameplayEffect

Attribute Modifier

实现。

不要直接：

WeaponDamage += 10。

------

# 28. 局内等级与账号永久等级必须区分

最终可能同时存在：

Account Level

Character Level

Weapon Level

PvE Permanent Progression

Match Level

Skill Level

Attribute Upgrade Level

这些生命周期完全不同。

因此从现在开始不要创建一个万能：

PlayerLevel

然后所有系统都读取它。

每一种 Progression 都应该明确自己的：

Scope

Lifetime

Reset Policy。

------

# 29. 生命周期 Scope

建议至少理解以下 Scope。

## Persistent

跨 Match 长期保存。

例如：

账号成长。

角色永久成长。

解锁内容。

------

## Session

本次服务器 Session 有效。

------

## Match

一局副本或比赛有效。

例如：

大灾变局内金币。

局内攻击等级。

随机技能。

------

## Round

只持续一个 Round。

例如：

生化模式士气。

------

## Life

只持续当前一次生命。

死亡后清除。

例如：

某些临时 Buff。

------

## Ability Duration

只持续几秒。

GameplayEffect 到期后结束。

------

# 30. PvE 死亡不是固定规则

不要把 Health <= 0 固定解释成：

Destroy Pawn
→ Respawn 5 秒。

不同 Experience 可以使用不同 Death Policy。

例如：

NoDeath

PermanentDeath

TimedRespawn

LimitedLives

SharedTeamLives

ReviveToken

DownedState

CheckpointRespawn

RespawnAtTeammate

RespawnAtStageStart

SpectateUntilEncounterEnd

因此 Health Component 只负责：

角色进入死亡条件。

真正如何处理死亡：

由 Death / Respawn Rule 决定。

------

# 31. CF 挑战的生命机制

未来可能需要：

玩家拥有有限生命次数。

死亡消耗 Life。

Life > 0：
允许重新加入战斗。

Life == 0：
进入 Spectator。

也可能存在：

Revive Token。

或者：

等待一定时间免费复活。

这些都应该是 Respawn Policy。

不是写进 Character。

------

# 32. CSOL 大灾变复活

大灾变可以允许玩家死亡后重新投入战斗。

具体可以根据：

Difficulty

Map

Item

Checkpoint

ModeRule

改变。

因此：

Death

Respawn Eligibility

Respawn Delay

Respawn Cost

Respawn Location

应该独立配置。

------

# 33. Checkpoint

PvE 副本需要通用 Checkpoint。

Checkpoint 可以决定：

Player Respawn Point。

AI Spawn Boundary。

Encounter Progress。

Join In Progress Spawn。

Objective Resume Point。

Map Progress。

但 Checkpoint 是否保存整个副本状态，需要由具体模式决定。

不要假定每个 Checkpoint 都能完整恢复 Match。

------

# 34. 中途加入

PvE 和 PvP 对 Join In Progress 的处理完全不同。

例如 Challenge / 大灾变：

可能允许中途加入。

新玩家应该：

生成在安全 Checkpoint。

获得合理的初始属性。

获得与当前进度匹配的资源。

不能生成在地图起点然后跑十分钟。

而生化模式：

可能只允许 Spectator 或作为 Zombie 加入。

因此必须存在：

JoinPolicy。

------

# 35. Difficulty 是独立系统

不要创建：

EasyGameMode

NormalGameMode

HardGameMode

NightmareGameMode。

Difficulty 应作为数据修改层。

它可能影响：

Enemy Health

Enemy Damage

Enemy Count

Elite Chance

Boss Mechanic

Spawn Rate

Player Respawn

Revive Cost

Reward

Friendly Fire

Objective Time

但地图流程本身仍然可以复用。

------

# 36. PvE Director 不等于 GameMode

GameMode 负责：

Match Authority。

加载规则。

进入 Match。

结束 Match。

判定整个 Match Success / Failure。

PvE Director 负责：

Encounter。

Spawn。

Enemy Budget。

Stage Progress。

Boss Encounter。

Dynamic Difficulty。

两个职责不要混合。

------

# 37. Objective System 必须独立

项目以后至少会出现：

Kill

Survive

Defend

Escort

Interact

Reach

Escape

Collect

Destroy

Boss

Capture

MultiStage

这些 Objective。

Objective 需要：

Start

Progress

Complete

Fail

Cancel

以及 Replication。

UI 可以订阅 Objective 状态显示：

任务目标。

当前进度。

剩余时间。

Boss。

但是 Objective 本身不应该直接操作具体 HUD Widget。

------

# 38. Progression System 必须独立

Progression 可以消费：

XP。

Currency。

SkillPoint。

KillCount。

ObjectiveReward。

然后产生：

Level Up。

Attribute Upgrade。

Ability Unlock。

Shop Unlock。

Weapon Unlock。

Progression 不应该依赖：

ChallengeGameMode。

它只读取当前 Experience 允许的 Progression Definition。

------

# 39. Shop System 必须独立

Shop 是否可用由规则决定。

例如：

HomeMap：
可能存在永久商店。

Challenge：
可能没有局内商店。

ZombieScenario：
可能存在局内武器购买和属性升级。

Infection：
以后可能存在 Human / Zombie 不同商店。

Shop 读取：

ShopDefinition

Eligibility

CurrencyType

UnlockCondition

而不是：

if ZombieScenario
{
OpenShop();
}

------

# 40. UI 是 Gameplay 状态的观察者，不是规则制定者

HUD 不应该决定：

玩家有没有 Health。

玩家能不能释放技能。

玩家有没有金币。

HUD 只根据：

Presentation Policy

和：

Gameplay State

展示相应信息。

例如所有底层数据都存在：

Health

Currency

Skills

Level

Objective

但 HomeMap 的 HUD Layout 可以只显示：

Interaction Prompt

Navigation

Social UI。

大灾变 HUD 可以显示：

Health

Ammo

Currency

AttackLevel

HealthLevel

Skills

Objective

BossHealth。

生化 HUD 可以显示：

Health

RoundTime

HumanCount

ZombieCount

特殊技能。

这三个模式不需要三套完全没有关系的数据系统。

只是 Presentation 不同。

------

# 41. 推荐将 UI Feature 化

例如：

UI.Feature.Health

UI.Feature.Ammo

UI.Feature.SkillBar

UI.Feature.Currency

UI.Feature.MatchLevel

UI.Feature.AttributeUpgrade

UI.Feature.RoundTimer

UI.Feature.TeamCount

UI.Feature.Objective

UI.Feature.BossHealth

UI.Feature.Respawn

UI.Feature.Shop

Experience 决定加载哪些 UI Feature。

Gameplay 系统不需要知道 Widget 是否存在。

------

# 42. GameplayTag 应描述“状态和能力”，不要描述所有代码分支

建议未来考虑类似：

Mode.Home

Mode.Infection

Mode.PvE

Mode.PvE.Challenge

Mode.PvE.Scenario

Rule.Damage.Enabled

Rule.Combat.Enabled

Rule.Infection.Enabled

Rule.Respawn.Timed

Rule.Respawn.None

Rule.Progression.Match

Rule.Shop.Enabled

State.Alive

State.Dead

State.Downed

State.Respawning

State.Spectating

State.BossFight

State.Encounter.Active

State.Match.Completed

Capability.Health

Capability.Ability

Capability.Inventory

Capability.Currency

具体名称遵循项目现有 GameplayTag 规范。

Tag 用于表达语义。

不要把所有系统都变成：

HasTag(Mode.X) 后执行巨大 switch。

------

# 43. Experience 应承担的职责

Experience 负责选择和装配：

PawnData

Input

GameFeature Plugins

AbilitySets

Rule Components

UI

Player Components

GameState Components

Progression Definition

Encounter Definition

Presentation Policy

Difficulty Definition

而不是亲自实现所有规则。

可以理解为：

Experience = Composition Root。

------

# 44. Rule Component 思路

具体实现名称可以调整，但架构上希望形成可组合规则。

例如：

MatchFlowComponent

RoundRuleComponent

DamageRuleComponent

DeathRuleComponent

RespawnRuleComponent

InfectionRuleComponent

PvEEncounterComponent

ObjectiveComponent

ProgressionComponent

CurrencyComponent

ShopRuleComponent

DifficultyComponent

JoinPolicyComponent

RewardComponent

每个 Experience 只加载需要的规则。

------

# 45. 示例：HomeMap Experience

可以理解为：

HomeExperience

加载：

BasePlayer

Interaction

Inventory

Cosmetic

Social

HomeUI

NonCombatRule

不加载：

InfectionRule

PvEEncounter

RoundRule

CombatHUD

PvEProgression

TimedRespawn

角色仍然可以拥有 ASC 和 Attribute。

------

# 46. 示例：经典生化 Experience

ClassicInfectionExperience

加载：

BaseCombat

Weapon

Health

InfectionRule

RoundRule

HumanTeam

ZombieTeam

ZombieAbility

RoundHUD

InfectionHUD

对应 Respawn Rule。

不加载：

PvEEncounterDirector

DungeonObjective

PvEAttributeShop。

------

# 47. 示例：CF 经典挑战 Experience

ChallengeClassicExperience

加载：

BaseCombat

Health

Weapon

PvEEncounter

WaveEncounter

EnemyAI

BossSystem

PvEHUD

ChallengeRespawn

Reward

Difficulty

不需要：

InfectionRule。

------

# 48. 示例：CSOL 风格大灾变 Experience

ZombieScenarioExperience

加载：

BaseCombat

Health

Weapon

PvEEncounter

Objective

EnemyAI

BossSystem

Checkpoint

TimedRespawn

MatchCurrency

MatchProgression

AttributeUpgrade

PvEShop

SkillSystem

PvEHUD

Difficulty

Reward

不需要：

InfectionRule。

------

# 49. 示例：未来原创 Roguelite 大灾变

完全可以组合：

ZombieScenario Core

- 

RandomSkillSelection

- 

MatchLevel

- 

Currency

- 

RandomShop

- 

EliteAffix

- 

BossModifier

- 

DifficultyScaling

这不应该要求重新复制一套：

Character

GameMode

Health

Weapon

AI

代码。

------

# 50. PlayerState 与 Pawn 生命周期

玩家的：

Player identity

Team

Match Currency

Match Level

Match Statistics

Skill selections

Progression State

应该认真考虑生命周期。

Pawn 可能不断更换。

例如：

Human Pawn
→ Zombie Pawn

或者：

Player Pawn
→ Death
→ Respawned Pawn

或者：

Normal Character
→ Special Character

但同一个 PlayerState 应继续代表同一个玩家。

因此不要将所有 Gameplay 数据都绑死在 Pawn 生命周期上。

------

# 51. GAS 生命周期

应延续 Lyra 值得复用的思想：

AbilitySystem 与玩家长期 Gameplay 状态应能够跨 Pawn 存活。

PawnData / AbilitySet 决定当前 Pawn 获得哪些能力。

当 Pawn：

死亡。

重生。

转换形态。

更换角色。

时，应明确：

哪些 Ability 保留。

哪些 Ability 移除。

哪些 GameplayEffect 保留。

哪些 Attribute Reset。

而不是全部销毁后重新猜测玩家状态。

------

# 52. AI 与玩家应该尽可能共享 Combat Contract

Zombie AI 和 Player 不一定使用完全相同 Character 类。

但它们应该尽量共享：

Damage

Health

GameplayEffect

GameplayTag

Ability

Death

Combat Target

Team Attitude

接口。

这样武器不需要：

if Player
...
else if Zombie
...
else if Boss
...

武器应该攻击：

Combat Target。

目标自己通过属性、Tag、Effect、Resistance 等决定结果。

------

# 53. 一个攻击事件不应该知道所有模式

例如 Weapon 只应该表达：

Source

Target

DamageSpec

DamageType

HitInfo

GameplayTags

然后 Damage Pipeline 决定结果。

它不应该知道：

这是 HomeMap。

这是 Challenge。

这是 Infection。

这是 ZombieScenario。

模式差异应该通过：

Rule

GameplayEffect

Target State

Immunity

Resistance

处理。

------

# 54. 同样的 Health <= 0 可以产生不同结果

这是理解整个架构最重要的例子之一。

Health <= 0 只是事实。

它不等于最终 Gameplay Result。

例如：

HomeMap：
正常情况下 Health 不会进入 0。

Classic PvP：
Death → Respawn。

Infection Human：
可能 Death → Infection。

Hero：
HP 归零 → Infection。

Challenge：
Death → Consume Life → Respawn。

ZombieScenario：
Death → Respawn Timer。

Hardcore PvE：
Death → Spectate。

Downed PvE：
Health 归零 → Downed → Revive / Death。

所以：

Attribute

Damage

Death Condition

Death Consequence

Respawn

必须分层。

------

# 55. 不要过度设计成完全通用引擎

以上要求也不意味着：

为了未来所有可能玩法，先写几十个抽象接口。

正确方式是：

当前实现真实需要的功能。

但选择正确的职责边界。

第一版可以很简单。

例如：

只有一个 HealthSet。

只有一个 PvE Encounter Component。

只有一个 Respawn Policy。

只有一种 Currency。

但是它们必须位于正确的层。

以后增加第二种行为时能够扩展或替换。

而不是推翻整个 GameMode。

------

# 56. 当前阶段 Codex 的工作原则

在继续移植或参考 Lyra Experience 相关实现之前：

首先检查项目目前：

GameMode

GameState

PlayerState

Pawn

ASC

Health

Death

Respawn

Experience

GameFeature

UI

之间的职责。

不要因为 Lyra 当前示例只有：

Elimination

Control

等有限玩法，就假设本项目只需要同样规模的规则。

本项目最终玩法复杂度明显更高。

Lyra 应被当成：

架构参考和基础设施来源。

不是最终游戏设计。

------

# 57. 遇到新需求时的判断顺序

以后实现任何玩法功能前，先判断：

第一：

它是不是所有角色都可能拥有的 Capability？

第二：

它是不是某个 Experience 开启的 Gameplay Rule？

第三：

它是不是某种 Death / Respawn Policy？

第四：

它是不是 Progression？

第五：

它是不是 Objective？

第六：

它是不是 Encounter Content？

第七：

它是不是纯 Presentation / UI？

第八：

它的数据生命周期属于 Persistent、Match、Round 还是 Life？

完成分类以后再决定代码放在哪里。

不要先问：

“这是哪个 GameMode？”

------

# 58. 最终架构目标

本项目最终希望形成：

Shared Gameplay Core

负责：

Player

Pawn

ASC

Attributes

Health

Damage

Equipment

Weapon

Inventory

Interaction

Gameplay Events

基础 Combat Contract。

其上组合：

Game Rules

负责：

Round

Death

Respawn

Infection

Team

Victory

Join

Difficulty。

其上组合：

PvE Framework

负责：

Encounter

Spawn Director

Objective

Checkpoint

Boss

PvE AI

Dungeon Progress。

其上组合：

Progression Framework

负责：

Level

XP

Currency

Attribute Upgrade

Skill Selection

Shop

Rewards。

最后由：

Experience

决定当前玩法加载哪些模块。

UI 通过：

Presentation Policy / UI Feature

决定玩家实际看到哪些信息。

------

# 59. 判断架构是否正确的测试

实现一个系统后，用以下问题检查。

如果给 HomeMap 加 Health Attribute：

是否不需要修改 HealthComponent？

如果把 Health HUD 隐藏：

是否不影响实际 Health 数据？

如果给大灾变加入局内攻击等级：

是否不需要修改所有 Weapon 类？

如果生化模式把 Human 转换为 Zombie：

PlayerState 上的 Match 数据是否仍然存在？

如果 Challenge 改成无限复活：

是否只需改变 Respawn Rule？

如果另一张 Challenge Map 不刷 Wave，而是 Escort：

是否不需要创建新的 GameMode？

如果 Boss 有三个阶段：

是否不需要把 Boss 行为写进 GameMode？

如果新 Experience 不显示技能栏：

是否仍然可以拥有被动 Ability？

如果以上问题大多数答案为“需要修改大量旧代码”，说明职责边界仍然存在问题。

------

# 60. 最终要求

当前不要追求“最快把一个模式跑起来”。

优先建立能够支撑：

Home

PvP

Infection

Challenge PvE

Zombie Scenario PvE

Boss Dungeon

Progression

Skills

Currency

Shop

不同 Death / Respawn

不同 HUD

的基础架构。

不要为了未来尚不存在的功能过度抽象。

但是所有已经确定未来会存在的玩法差异，都必须在当前职责划分中留出正确的位置。

本项目需要的是：

Composable Gameplay Architecture

而不是：

不断增长的 GameMode 条件分支。

这里有一个地方我尤其建议你以后拿来**审 Codex 的代码**：

> `Health <= 0` 只是一个 Gameplay 事实，**不是“死亡并重生”的同义词**。

这句话能解决很多设计问题。你刚才说的生化就是例子：

```
HP <= 0 → Infection
```

挑战模式可能是：

```
HP <= 0 → ConsumeLife → Respawn
```

大灾变可能是：

```
HP <= 0 → RespawnTimer
```

硬核副本可能是：

```
HP <= 0 → Spectator
```

以后甚至：

```
HP <= 0 → Downed → TeammateRevive → Alive
```

所以 **Health、Death、Death Consequence、Respawn 是四层东西**。很多 AI 最容易一口气给你揉成 `HandleDeath()`，以后你就会非常难受。

同样，你那个 HomeMap 例子我认为非常重要。不要让 Codex 理解成“HomeMap 不需要血量，所以 Home Character 没有 HealthComponent”。更合理的是：**数据能力、游戏规则、表现层是三个正交维度**。Health 可以存在，Home 规则不产生常规伤害，Home HUD 又选择不展示它。不是为了代码复用硬留一个 100 HP，而是让角色基础契约保持一致。

这和 CSOL 自己的发展其实很像。官方现在已经把角色能力进一步归类成“大灾变能力值、主动技能、被动技能、额外显示信息”等不同类别，这恰好说明**能力本身和信息是否显示就不应该是同一层**。([反恐精英Online](https://csol.tiancity.com/homepage/article/2026/04/28/67038.html?utm_source=chatgpt.com))

CF 这边也不是只有最老式的 20/30 波刷怪。资料记录的挑战模式包含传统、生存/推进、守卫、试炼等变化，试炼挑战进一步加入不同回合任务、职业、防御设施和 PVE 属性成长；官方海外版本的 ZM3 更新也明确提到 PVE 等级以及可定制角色/防御属性系统。([Crossfire FPS](https://crossfirefps.fandom.com/wiki/Challenge_Mode?utm_source=chatgpt.com))

CSOL 官方对大灾变的定义也明确有局内奖金强化能力，大灾变历史上有求生、追击，后续主流内容又有追击、歼灭；2026 年开发团队甚至专门按“流程长度、怪物配置、Boss 战占比”等区别挑选地图做平衡测试。([反恐精英Online](https://csol.tiancity.com/homepage/v4/course/game_model.html?utm_source=chatgpt.com))

所以我现在反而比较确定：**你不应该让 Codex“照搬 Lyra Experience”，而应该让它“在理解最终玩法矩阵以后，判断 Lyra 哪些东西值得保留、哪些需要在 Lyra 上继续抽象”。** Epic 自己也只是把 Lyra 定位为模块化示例；Experience 可以组合 Game Feature，而 Game Features 的目标之一就是避免无关功能间产生依赖。([Epic Games Developers](https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine?lang=en-US&utm_source=chatgpt.com))

你下一步真要开始让 Codex 动代码时，最好再给它一份更狠的 **《UE5.8 Lyra 派生项目 Gameplay 架构边界与禁止事项》**，里面直接规定诸如“什么允许放 GameMode、什么必须放 GameStateComponent、ASC 为什么放 PlayerState、Encounter Director 怎么拆、哪些 `if Mode == X` 直接视为架构违规”。那份就不是玩法说明，而是专门防止 Codex 把代码写烂的 coding architecture contract。