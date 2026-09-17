# 十三回合 PVE 生存副本任务包

## 状态

- 建立日期：2026-08-29
- 当前状态：架构决策已固化，P0 武器音频与测试夹具收尾后开始实现
- 产品定位：可发售的合作生存 Experience，不是 TestMap 脚本或临时 GameMode
- 网络形态：Standalone、本地双人分屏、最多四人 Listen Server 共用同一套规则

## 完成目标

- 一局固定 13 个回合，完成后胜利，全队满足最终死亡条件时失败。
- 回合间有权威倒计时、玩家/局内数据重置、刷怪、清场和下回合转换。
- 普通怪、精英和后续特殊感染者从数据目录选取，不在 GameMode 或 Spawner 写类型分支。
- HUD 显示当前回合、回合状态/倒计时、本回合剩余敌人和必要的队伍状态。
- 将来由 HomeMap 的 `W_LyraExperienceTileButton` 选择并进入该 Experience；入口 UI 不反向拥有副本规则。

## 权威分层

```text
HomeMap Experience Tile
  -> 服务器选择 UShootExperienceDefinition
  -> 生产 PVE GameMode：回合状态机、胜负、生成导演、重置时序
  -> 生产 PVE GameState：复制回合、倒计时终点、剩余敌人、胜负状态
  -> 刷怪导演：按回合数据挑选敌人类型与生成点
  -> Zombie Controller + Blackboard + BehaviorTree：单只敌人的感知和决策
  -> Zombie Character + GAS/GE：攻击校验、伤害和状态
  -> GameplayMessage / ViewModel / UIExtension：每个 LocalPlayer 的 HUD 投影
```

- GameMode 只存在服务器，是回合流程和胜负的唯一写入者。
- GameState 只复制 UI 和新加入客户端所需的局状态；客户端用服务器时间终点推导倒计时，不逐秒 RPC。
- 生成导演不控制单只敌人的移动/攻击；Behavior Tree 不判定回合胜负。
- `AShootEnemyTestSpawner` 和 AI 暂停开关是 TestMap 夹具，不进入正式 Experience。正式刷怪导演不沿用 `Test` 命名。

## 数据设计

建立 PVE 回合配置 DataAsset，至少包含：

- 回合编号和准备时间。
- 同时存活上限、本回合总数、生成节奏。
- 按权重引用的敌人 archetype 蓝图类，以及精英上限/保底规则。
- 可选的回合修正数据，例如生命、伤害或移速系数；修正应通过模式 GE/AbilitySet 表达，不直接改 AttributeSet 当前值。

- 13 个回合是 DataAsset 的 13 条数据，不在 C++ 构造函数逐条 `Add` 硬编码。
- 当前 Walker/Runner/Bruiser/Bleeder/Elite 是可用的首批 archetype；新增特感时增加蓝图子类和 GA/GE，不修改通用刷怪分支。

## 回合状态机

```text
WaitingForExperience
  -> PreparingRound
  -> RoundInProgress
  -> ClearingRound
  -> Intermission
  -> PreparingRound ...
  -> MatchWon / MatchLost
```

- `PreparingRound`：调用 GameMode 已有的 `InitializeRoundForAll`，清理上回合 RuntimeOnly 残留，按产品规则恢复玩家。
- `RoundInProgress`：导演按本回合配额生成；“未生成数 + 存活数”就是 HUD 的剩余敌人，不用击杀分数反推。
- `ClearingRound`：本回合配额已全部生成且存活数为 0 后进入，防止死亡复制与下回合计时竞态。
- `Intermission`：复制统一终点时间；第 13 回合完成后直接 `MatchWon`。
- 新玩家途中加入必须从 GameState 获取当前快照，不重启本回合。

## HUD 与 `WBP_ScoreWidget`

- 保留现有 `/Game/UI/Hud/WBP_ScoreWidget` 及用户创建的备份，不直接覆盖它的双方分数语义；生化/PVP 后续仍可以使用。
- PVE 新建语义明确的 Widget 子类或独立 Widget，将原“敌方 Score”区域表达为“本回合剩余敌人”。
- 数据来自 GameState 复制属性 -> GameplayMessage -> LocalPlayer ViewModel/Widget；不允许 Widget 自己遍历世界数敌人。
- 通过 Experience 的 UIExtension 动作注册到 `W_DefaultHUD` 上方中央稳定插槽，每个 LocalPlayer 拥有自己的 Widget 实例。

## HomeMap 入口边界

- `W_LyraExperienceTileButton` 的 `Audio Lobby Select` 依赖尚未完整迁入时，不阻塞 PVE Experience 、GameMode、GameState 和关卡的独立开发。
- 待 Tile 可用后，HomeMap 交互物只提交“选择哪个 Experience/会话”的请求，不在 UMG 里创建 GameMode 或手写回合规则。
- 生化模式将是另一个 Experience；Unity CrossFire 代码只作产品思路参考，不复制其运行时架构。

## 实施阶段

1. 建立生产 PVE GameMode/GameState 基类、复制状态结构与 13 回合 DataAsset。
2. 建立生产刷怪导演与地图生成点，复用正式 Zombie BT/GAS，不复用 TestSpawner。
3. 建立 PVE HUD Widget/ViewModel/GameplayMessage，用 UIExtension 注入 Experience。
4. 配置正式 PVE Experience 与地图，完成 Standalone、SplitScreen、Listen Server 回合状态同步。
5. Tile 迁入依赖完成后，接 HomeMap 选择、会话创建/加入和返回 Hub 流程。
6. 上述全部完成后才执行最终玩家上手回归，不把技术检查写成人工听感/手感已通过。

## 验收标准

- 三种网络形态都由同一 Experience 加载，回合号、剩余数和倒计时一致。
- 第 1 至 13 回合可连续完成，不依赖控制台或开发者直接调函数。
- 回合内的 Zombie 全部由 Behavior Tree 决策，攻击全部通过 GAS/GE 结算。
- 敌人死亡、同时死亡、玩家途中加入/离开、主机与远端客户端都不会使剩余数变成负数或提前过关。
- 分屏 HUD 不串数据或输入焦点；新加入客户端能立即看到当前回合快照。
- 测试 AI 暂停按钮不出现在正式 PVE Experience 的可发售入口。
