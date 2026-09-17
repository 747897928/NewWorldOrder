---
task_id: BiochemicalMode
status: in_progress
assigned_to: Codex
progress: 35%
---

# 当前状态

- 2026-09-01 已完成 Nano 代码审计、CSOL/CrossFire 公开规则研究和首版白盒结构设计。
- 首版地图资产名确定为 `/Game/Maps/TestMap_Biochemical`。
- 当前先创建白盒环境，不在用户确认 Experience 高风险迁移前创建正式 GameMode/PawnData 资产。
- `/Game/Maps/TestMap_Biochemical` 白盒已创建。初始版本为 44 个 Actor；用户补齐 Lighting 后为 48 个 Actor。
- 地图范围约为 X `-7500..7500`、Y `-6000..6000`，最高守点约 Z `1125`；含 Warehouse、Vent、Observation 三个守点及各自正面/侧后坡道。
- 4 个 `HumanStart` 与 4 个 `InfectedStart` 已通过 Actor Tag 区分。当前尚未接入生化 GameMode，因此这些 Tag 只完成地图数据准备，不代表出生规则已经实现。
- 编辑器 Lit 视图已确认白盒几何存在。2026-09-01 用户从现有地图迁入 `DirectionalLight`、`ExponentialHeightFog`、`SkyAtmosphere`、`SkyLight`、`VolumetricCloud` 与 `SM_SkySphere`；保存后资产复读为 48 个 Actor，天空、大气、阴影与云层已正常显示。
- `SM_SkySphere` 是使用球形网格与天空材质提供背景的传统天空穹顶。当前它与 `SkyAtmosphere + VolumetricCloud` 同时存在但截图无明显遮挡，白盒阶段保留；正式环境阶段若出现重复太阳、云层遮挡或天空色不一致，再通过可见性对比决定是否移除。
- 冷启动后对该地图执行 Standalone PIE，白盒、天空、云层与方向光运行时可见；本轮日志未出现 Blueprint Runtime Error、Accessed None、Fatal 或 Ensure。地图当前未设置 GameMode Override，使用项目失效的全局 `NewWorldOrderGameMode` 配置后回退为 `GameModeBase`，因此本次只验证环境，不把角色出生或生化规则记为通过。

# 当前执行顺序

- 2026-09-01 用户决定暂时搁置生化地图，不继续补 GameMode、规则或美术；优先处理技能系统与 Experience 架构决策。
- 恢复本任务时，再基于 `Research_规则与地图研究.md` 细化三处守点的感染者反制路线、出生安全距离和正式 GameMode。
- Experience 继续停留在设计讨论阶段；只有用户确认 `ExperienceAudit_体验架构审计.md` 后，才实施 HUD 单一入口和 PawnData 第一阶段。

# 已知边界

- 白盒阶段不承诺最终美术、动画与感染者模型。
- 生化规则不能直接照搬 Nano.cs；必须补齐时间胜负、回合状态、网络权威和句柄回收。
- 地图设计先保证守点有反制、路线有回环，再做材质与主题包装。
