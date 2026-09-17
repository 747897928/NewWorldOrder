对，你说的那个英文就是 **Experience**，更完整一点是 **Lyra Experience / `ULyraExperienceDefinition`**。

你之前说“玩家初始技能、属性、UMG、UI 按钮图标由 GameMode 决定”，**意图是对的，但职责名称说偏了**。更准确的架构应该是：

**Experience 决定这一局“装配什么”；GameMode 负责服务器权威规则；具体技能、属性、UI 由各自的数据和 Feature 实现。**

Epic 在 UE 5.8 的 Lyra 文档里甚至直接把 Experience 描述成“更高级版本的 GameMode”。Experience 可以选择默认 PawnData、Gameplay 功能和其他 Action；同一个 ShooterCore 也可以被多个 Experience 复用。([Epic Games Developers](https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine?utm_source=chatgpt.com))

### 你可以这样理解各层

| 层                          | 应该回答的问题                                     |
| --------------------------- | -------------------------------------------------- |
| **Experience**              | “这一局是什么游戏？需要装哪些模块？”               |
| **GameMode**                | “服务器如何裁决这一局？”                           |
| **GameState**               | “这一局现在进行到哪里？所有客户端需要知道什么？”   |
| **PawnData**                | “这种玩家 Pawn 默认是什么角色配置？”               |
| **AbilitySet**              | “初始有哪些 GA、GE、AttributeSet？”                |
| **GameFeature / ActionSet** | “当前 Experience 额外装入什么玩法功能？”           |
| **PlayerState / ASC**       | “这个玩家当前拥有哪些技能、属性、效果和玩家状态？” |
| **UI Feature / HUD Layout** | “本地玩家这一局应该看到什么？”                     |

Lyra 官方的 `ULyraAbilitySet` 本身就能包含 **Gameplay Ability、Gameplay Effect 和 Attribute Set**；PawnData 可以引用这些 AbilitySet，而默认 PawnData 又由 Experience 决定。另外模式特有技能还可以通过 `UGameFeatureAction_AddAbilities` 动态授予。([Epic Games Developers](https://dev.epicgames.com/documentation/unreal-engine/abilities-in-lyra-in-unreal-engine?utm_source=chatgpt.com))

所以你的思路完全可以变成：

```text
Experience_PvE_ZombieScenario
    ↓
Default PawnData
    ↓
基础移动
基础交互
基础战斗
基础 Health
基础死亡能力

+
PvE GameFeature
    ↓
PvE 属性
PvE 技能
副本金币
属性升级
Encounter
Boss

+
ZombieScenario UI Feature
    ↓
血量
弹药
金币
攻击等级
生命等级
技能按钮
任务目标
Boss 血条
```

换成 HomeMap：

```text
Experience_Home
    ↓
同一个 Shared Player Framework

+
Home Gameplay Features
    ↓
移动
交互
换装
背包
社交

+
Home UI
    ↓
交互提示
菜单
导航

不加载：
Combat HUD
PvE Progression HUD
Skill Bar
Boss HUD
Encounter
ZombieScenario Progression
```

这里还有一个我想修正我们前面说法的小地方：

**“底层兼容”不意味着所有模式都必须把所有 AttributeSet 实例化出来。**

例如最好可以拆成：

```text
CoreHealthSet
CoreCombatSet

PvEProgressionSet
    AttackLevel
    HealthLevel
    PvECurrency
    PvEExp

InfectionSet
    InfectionResistance
    KnockbackResistance
    Evolution
    Rage
```

HomeMap 可以仍然使用 `CoreHealthSet`，只是没有正常战斗伤害，也不显示 Health HUD。

但它**完全没必要加载 `PvEProgressionSet` 或 `InfectionSet`**。

这样才是真正的模块化：

> 公共契约统一，但按 Experience 装载真正需要的数据和功能。

而不是另一个极端——为了“代码统一”，每个人身上挂 100 个永远不用的 Attribute。

------

## 还有一个非常重要的问题：UI 不应该让 GameMode 直接管

尤其你是多人游戏。

UE 官方明确规定：**GameMode 只存在服务器端，不复制给远程客户端**；需要客户端知道的比赛状态应该通过 GameState 等复制对象同步。([Epic Games Developers](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine?utm_source=chatgpt.com))

所以千万别让 Codex以后写这种东西：

```cpp
HUD->GetGameMode();

if (GameMode->bShowSkillBar)
{
    ShowSkillBar();
}
```

远程客户端这里架构上就是错的。

更好的关系是：

```text
Experience
    ↓
加载 UI Feature / HUD Layout

Gameplay State
    ↓
GameState / PlayerState / ASC
    ↓ replication
Client
    ↓
UMG / CommonUI
```

例如：

**Experience 决定技能栏这个 Widget 是否存在。**

然后：

**PlayerState/ASC 决定玩家到底有什么技能。**

最后：

**Widget 观察 ASC，把技能图标、冷却、等级显示出来。**

这样三层完全解耦。

比如：

```text
大灾变
Experience:
    Load SkillBar

PlayerState / ASC:
    GA_Heal
    GA_Grenade
    GA_Dash

SkillBar:
    自动观察这些可展示技能
```

换成 HomeMap：

```text
Experience:
    不加载 SkillBar

PlayerState / ASC:
    ASC 仍然存在
    甚至仍然有一些被动 GA / Interaction GA

UI:
    完全没有技能栏
```

这正是你想要的效果。

------

## Listen Server 对这个架构基本没有影响

你说的应该是 **Listen Server（监听服务器）**。

你完全可以只做 Listen Server，不租 Dedicated Server。

官方定义就是：Listen Server 的房主机器同时运行 **Server + 本地 Client**，房主本身也参加游戏；其他玩家以 Client 身份连接房主。房主仍然是权威服务器。([Epic Games Developers](https://dev.epicgames.com/documentation/unreal-engine/setting-up-dedicated-servers-in-unreal-engine?utm_source=chatgpt.com))

也就是说：

```text
房主电脑

Server Authority
├── GameMode
├── GameState Server
├── AI
├── Damage Authority
├── Spawn
├── Infection
├── Encounter
├── Reward
└── Boss

同时还有

Host Local Client
├── PlayerController
├── LocalPlayer
├── HUD
└── UMG
```

其他玩家：

```text
Remote Client
├── replicated GameState
├── replicated PlayerState
├── replicated Pawn
├── HUD
└── UMG

没有 GameMode Instance
```

因此你仍然应该按照**标准 Server Authority**写。

不要因为你只做 Listen Server，就写成：

```text
房主能执行 = Server
```

或者：

```cpp
IsLocallyControlled()
```

来代替：

```cpp
HasAuthority()
```

这两个不是一回事。

Listen Server 下房主恰好同时拥有“本地玩家”和“服务器权威”，特别容易把 Bug 藏起来。

比如你代码错误地：

```cpp
if (IsLocallyControlled())
{
    GiveMoney();
}
```

房主测试时可能感觉完全正常。

远程客户端一进来就炸。

正确应该是：

```text
Server:
决定玩家获得 100 金币
        ↓
PlayerState
        ↓ replication
Client:
看到金币变成 100
        ↓
UI 刷新
```

Epic 对 UE 网络模型的定义也是服务器保存唯一权威状态；Listen Server 只是这个服务器恰好还有一个本地玩家。([Epic Games Developers](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine?utm_source=chatgpt.com))

------

我建议你把下面这段**直接追加给 Codex**，这句话能把你前面跟它讲的内容纠正回来：

## Experience、GameMode、技能、属性和 UI 的职责补充

之前提到“由 GameMode 决定玩家初始获得哪些技能、属性以及显示哪些 UMG/UI”，这里需要更准确地理解。

本项目采用 Lyra Experience 架构。

应该由 Experience 作为当前游戏玩法的 Composition Root，决定当前游戏加载和组合哪些 Gameplay Feature、PawnData、AbilitySet、Rule、UI Feature 和其他玩法模块。

不要把这些配置直接塞入 GameMode。

例如：

Experience 可以决定当前玩家使用哪一个默认 PawnData。

PawnData 可以进一步定义该角色初始需要的 AbilitySet。

AbilitySet 可以授予 Gameplay Ability、Gameplay Effect 和 AttributeSet。

模式特有能力还可以通过 GameFeature Action 动态添加。

因此：

Experience 决定这一局需要什么能力。

PawnData / AbilitySet / GameFeature 负责实际授予这些能力。

ASC / AttributeSet / GameplayEffect 负责技能和属性运行。

GameMode 不负责保存完整的玩家技能配置和 UI 配置。

------

UI 同样遵循这个原则。

Experience 可以决定当前游戏加载哪些 UI Feature 或 HUD Layout。

例如大灾变 Experience 可以加载：

Health UI

Ammo UI

Skill Bar

PvE Currency UI

Attribute Upgrade UI

Objective UI

Boss Health UI

而 HomeMap Experience 可以只加载：

Interaction UI

Navigation UI

Menu UI

即使 HomeMap 没有显示 Health、Combat Attribute 和 Skill Bar，也不意味着底层公共 Gameplay Framework 必须删除 ASC、Health 或其他通用能力。

Gameplay Capability 是否存在、Gameplay Rule 是否启用、Presentation 是否显示，是三个不同维度。

同时也不要为了统一架构让所有 Experience 强制加载所有 AttributeSet。

公共属性可以属于 Core AttributeSet。

模式专用属性应通过 Experience / Gameplay Feature 按需加载。

例如：

CoreHealthSet

CoreCombatSet

PvEProgressionSet

InfectionSet

HomeMap 可以使用 CoreHealthSet，但不加载 PvEProgressionSet 和 InfectionSet。

------

GameMode 保持服务器权威职责。

例如：

玩家是否合法加入。

开始 Match。

服务器裁决 Round / Match。

生成 Player。

判断胜负。

触发 Respawn。

启动服务器端 Gameplay Flow。

GameMode 不应该成为：

Skill Database

Attribute Database

UI Configuration Database

Shop Database

Encounter Database

的集中容器。

------

当前项目使用 Listen Server，不计划部署 Dedicated Server。

但是所有 Gameplay 代码仍然必须按照标准 Server Authoritative Multiplayer 架构实现。

Listen Server 只是服务器和一个本地客户端运行在同一个进程/机器上。

它不改变：

GameMode 仅存在于服务器。

GameState 在服务器和客户端之间复制。

PlayerState 用于玩家级复制状态。

服务器负责权威的 Damage、Death、Spawn、Reward、Currency、Progression、Infection、AI、Encounter 和 Match Result。

客户端负责输入和 Presentation。

禁止因为 Host 同时是 Client 和 Server，而使用 LocalPlayer、IsLocallyControlled 或本地 UI 状态代替服务器 Authority 判断。

实现和测试时必须确保 Remote Client 与 Host Client 都正确运行。

UI 不允许通过直接读取 GameMode Runtime State 来决定显示。

UI 应通过当前加载的 Experience / UI Feature 获得布局，通过 GameState、PlayerState、ASC 和其他 replicated Gameplay State 获得运行时数据。

最终职责可以概括为：

Experience = 当前玩法装配什么

GameMode = 服务器如何裁决

GameState = 当前整局发生了什么

PlayerState / ASC = 当前玩家处于什么状态

Pawn = 当前玩家正在控制什么实体

GameFeature = 当前 Experience 注入哪些独立玩法能力

UI = 当前本地玩家应该看到什么

不要将这些职责合并成一个巨大 GameMode。

最后一个建议：**虽然你现在明确不准备使用 Dedicated Server，但代码最好不要主动把 Dedicated Server 兼容性做死。**

这不会让你多花服务器钱，也不代表要部署 Dedicated Server。只是保持：

```
Server Authority → Replication → Client Presentation
```

这套正确结构。

这样 Listen Server 一样正常，而且以后万一游戏赚到钱、Steam 玩家量起来了，想增加社区服或者 Dedicated Server 时，不至于把游戏底层推倒重写。UE 官方也明确说 Listen Server 很适合休闲多人和合作类游戏，你这种 CF 挑战/大灾变尤其合理。([Epic Games Developers](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/setting-up-dedicated-servers-in-unreal-engine?utm_source=chatgpt.com))