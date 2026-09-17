# Experience 体验架构审计

# 结论

- 不建议把 Lyra 的 GameFeature Experience 全量复制到项目。
- 当前 `UShootExperienceDefinition` 已证明能够承载地图到 HUD 与 UIExtension 的最小纵切，但还不足以直接承载正式生化模式。
- 推荐保留项目 Experience，并补齐 PawnData、可撤销 AbilitySet 句柄、明确的 Ready/Failed/Deactivating 状态和副本级属性初始化事务。
- GameFeature 只在未来确实需要 DLC、运行时插件装卸或独立内容包时引入。现在复制完整 Lyra 会同时扩大调试面、资产迁移量和多人生命周期风险，却不能自动解决现有蓝图硬编码。

# 2026-09-04 当前代码与资产事实

## HUD 生命周期

- `BP_ShootHUD.EventGraph` 的 BeginPlay 无条件 Push 已由用户删除；HUD Actor 不再创建根布局。
- Home、Dungeon、ExpeditionSandbox、SplitScreenTest Experience 都显式把 `W_DefaultHUD` 推入 `UI.Layer.Game`，
  因而每个 LocalPlayer 只有一套由 Experience 回收的 HUD 根布局。
- `W_DefaultHUD` 继承自 `ULyraActivatableWidget`，使用父类上的 `InputConfig=Game` 与
  `GetDesiredInputConfig()`；衣柜/副本菜单关闭后依靠 CommonUI 栈恢复游戏输入，不新增 PlayerController `SetInputMode`。
- `W_DefaultHUD` 已不再直接嵌入 `W_Healthbar` 与 `W_Shieldbar`，原位置改为 `HUD.Slot.Health` 和
  `HUD.Slot.Shield` 两个 ExtensionPoint。
- Dungeon、ExpeditionSandbox、SplitScreenTest 注入两条战斗状态条；Home 只加载根布局而不注入 Health、Shield、
  QuickBar、Respawn 或 SkillBar，因此安全区无战斗 HUD，同时保留正确的 Game 输入配置基线。

## 角色蓝图直接授予

- `BP_ShootCharacter` 的 Startup Abilities 仍含 `GA_Hero_Jump`。
- Startup Passive Abilities 仍含 `GA_ListenForEvent`。
- Core Combat Abilities 仍含 QuickbarNext、QuickbarPrevious、DropWeapon。
- Core Interaction Abilities 仍含 `ShootGA_Interact`。
- 默认属性 GameplayEffect 仍由角色蓝图配置 Primary、Secondary、Vital 三组效果。
- 这些授予不属于 Experience 的可撤销句柄，切换地图或 Experience 时不能可靠卸载，并可能与后续 AbilitySet 重复授予。

## 属性、等级与存档边界

- 等级、经验和属性点仍在 PlayerState 运行，但当前 SaveVersion=3 已不把它们写入存档，方向符合“每局从零开始”。
- 仍缺少一套服务器权威的比赛开始、回合重置、玩家晚加入和比赛退出的统一初始化事务。
- 当前 AbilitySet 卸载动态 AttributeSet 时只清理记录，尚未完整调用 ASC 的 `RemoveSpawnedAttribute`，正式模式切换前必须修正。

# 推荐职责边界

## Experience

- 选择 GameMode、PawnData、Human/Infected AbilitySet、HUD Layout、HUD Extension、输入配置和模式规则资产。
- 管理加载状态与可撤销句柄。
- 不直接保存账号成长，也不在 Experience 中写具体胜负循环。

## PawnData

- 承载角色基础能力和角色类别所需配置。
- Jump、Interact、QuickBar 输入能力从 `BP_ShootCharacter` 数组迁入 PawnData AbilitySet。
- 人类与感染者差异使用独立 PawnData 或角色 AbilitySet 表达，禁止在角色基类中写 `IsZombie` 分支。

## GameMode 与 GameState

- GameMode 负责服务器权威回合状态转换、初始感染者选择、胜负判定、重生或感染流程。
- GameState 复制回合阶段、剩余时间、人类数量、感染者数量和胜负结果。
- 阵营判断统一使用 `ILyraTeamAgentInterface`，Human 与 Infected 使用不同 TeamId 或可配置阵营资产。

## 属性与等级

- 每次进入副本按模式默认值初始化；退出副本不保存。
- 回合制副本在每回合开始由 GameMode 统一重置玩家与敌人属性并重授模式套件。
- 装备与购买物属于战斗期 RuntimeOnly 数据；账号仓库、外观与 QuickBar 配置才进入 SaveGame。

# 分阶段实施

## 阶段 1：HUD 单一入口

- 已完成：删除 `BP_ShootHUD` 无条件推 HUD 的旧路径。
- 已完成：Home 与三个当前可玩战斗 Experience 显式选择 HUD 根布局和片段。
- 已完成：Health 与 Shield 从 `W_DefaultHUD` 直接子控件迁为两个可独立选择的 HUD ExtensionPoint。
- 待 PIE：HomeMap 无战斗状态条且菜单关闭后恢复 Game 输入；战斗地图只生成一套 HUD；分屏每个 LocalPlayer
  只操作自己的 UI。

## 阶段 2：PawnData 与基础能力收敛

- 建立最小 `UShootPawnData`，只迁移 Jump、Interact、QuickBarNext、QuickBarPrevious、DropWeapon 和必要被动。
- Experience/PawnData 记录每次授予的句柄，并在卸载时撤销。
- 资产验收后再清空 `BP_ShootCharacter` 对应数组，禁止先删旧路径。

## 阶段 3：模式属性与等级生命周期

- 建立副本开始和回合重置事务。
- 修正 AttributeSet 卸载，确保 ASC 不保留上一 Experience 的 SpawnedAttribute。
- 验收重新进入副本和角色重生后不会叠加属性或技能。

## 阶段 4：加载状态与晚加入

- 补齐 Loading、Ready、Failed、Deactivating 状态。
- 处理 Listen Server 晚加入、断线重连、LocalPlayer 动态加入和 Travel 期间的句柄回收。

## 阶段 5：GameFeature 决策门

- 只有出现可独立安装的玩法包、DLC 或运行时插件开关需求时，再引入 Lyra GameFeature Action。
- 在此之前不得为了“对齐 Lyra”复制无调用方的类。

# 生化模式的最小落点

- 新建 Biochemical Experience，引用生化 GameMode、Human PawnData、Infected PawnData、模式 HUD 和输入配置。
- GameMode 采用 Waiting、Preparing、Reveal、Active、Ending、Intermission、MatchEnding 状态。
- 感染是服务器权威的角色状态与阵营切换，使用 AbilitySet/GameplayEffect 切换能力和属性，不直接修改蓝图数组。
- 第一版只做规则纵切和白盒地图；动画、感染者模型与高级技能可使用占位资产，不阻塞架构验证。

# 决策状态

- 本文保留 2026-09-01 架构结论，并在 2026-09-04 更新当前实现事实。
- HUD 单一入口、AbilitySet/AttributeSet 可撤销生命周期和四槽 Match Skill 基础已按分阶段方案实施。
- PawnData 与完整 Experience 状态机仍按真实需求逐步引入，不为了形式对齐 Lyra 建立第二套授予链。
- 键位修复、HomeMap 衣柜交互、GameplayEffects 演示 Actor 迁移和生化白盒不依赖该决策，可独立完成。
