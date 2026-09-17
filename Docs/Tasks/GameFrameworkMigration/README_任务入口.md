# 游戏框架迁移任务入口

本任务包用于把 NewWorldOrder 当前混合的 Aura、Lyra 和项目自研链路，逐步收敛为适合多玩法模式的 Lyra 风格框架。

迁移不是复制整个 Lyra，也不是一次性删除 Aura。目标是采用 Lyra 的数据分层和生命周期，同时保留已经验证可用的项目业务系统。

# 新会话阅读顺序

1. `STATUS.md`
2. `Requirements_需求.md`
3. `Audit_现状审计.md`
4. `TargetArchitecture_目标架构.md`
5. `AbilityGranting_能力授予迁移.md`
6. `ExperienceAndPawnData_玩法与角色数据.md`
7. `MigrationPlan_迁移计划.md`

# 一句话目标

用 Experience 表达整局玩法，用 PawnData 表达每个玩家操控的角色，用 AbilitySet 表达可撤销的能力来源，用 GameplayTagRelationshipMapping 表达能力标签关系；存档只保存长期进度、解锁和玩家配置，不再把运行时 AbilitySpec 当作权威数据源。

# 已确认边界

- ASC 继续位于 PlayerState，玩家切换 Pawn 时重绑 Avatar。
- Listen Server 是单人、本地分屏和在线合作的统一网络架构。
- 所有玩家私有 UI 必须按 LocalPlayer 创建和管理。
- `UShootAbilitySet`、库存、装备、QuickBar、SaveGame、CommonUI、GameplayMessage、阵营接口、衣柜和角色切换后端均有可保留实现。
- 旧蓝图 `GA_Interact` 作为 Lyra 人工迁移参考资产保留，不是运行时主线。
- `GA_Skill1` 至 `GA_Skill4` 作为后续技能扩展资产保留，不得因当前未授予而删除。
- GameFeatures 不是第一阶段前置条件；只有基础 Experience/PawnData 流程稳定后，才评估是否按模式拆插件。

# 不在本任务包内

- 直接修改蓝图、C++、地图、DataAsset 或存档格式。
- 完成角色技能内容、美术表现或全部游戏模式。
- 把 Lyra 示例项目的所有类、插件和资产原样搬入本项目。

# 权威来源

- 游戏方向：`Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md`
- 技术约束：`AGENTS.md`、`Docs/Engineering/Tech_Constraints.md`
- 当前实现：`Source/NewWorldOrder` 和 `/Game` 下的实际资产
- 本次迁移的任务状态：本目录 `STATUS.md`

若旧文档与本任务包冲突，先核对当前代码和最新需求。不得仅因旧文档存在就保留兼容分支。
