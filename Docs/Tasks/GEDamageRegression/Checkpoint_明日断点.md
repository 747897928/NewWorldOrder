# 明日接续断点

## 当前存档

- 原调查分支：feature/ge-damage-regression-docs。
- 2026-08-19 已把两个纯文档提交选择性接入并推送到 main：e47483e、23e06fd。
- 后续以 main@23e06fd 或从该提交新建的 GE 专用分支为权威起点，不再直接从旧 feature 分支续作。
- 任务包目录：Docs/Tasks/GEDamageRegression
- 旧 feature 分支还包含 Luna 的动画提交 cd2f0e9；其补丁已等价接入 main 为 5e69e5b，禁止再次合并或 cherry-pick。
- 其他 AI 的工作区改动保持原样，不要提交，不要回滚。

## 明天从这里开始

1. 先读 Docs/Tasks/GEDamageRegression/Status_状态.md。
2. 再读 Evidence_字段引用与资产事实.md 的 Lyra 11000 MCP 验证结果。
3. BaseDamage 来源已完成可见链路复核：Character 与 PlayerState 虽创建 CombatSet，BaseDamage 默认值却为 0；ShooterHero 只授予 `GE_IsPlayer`，三把枪 AbilitySet 没有 Attribute/GE，火力 GA 只应用 `GE_Damage_*`，源码未找到 CombatSet.BaseDamage 写入。不要把 Lyra Attribute Capture 当成可直接迁移的数值来源。
4. 距离曲线 Key 已复读并写入 Investigation_Lyra伤害链.md：Pistol 20 m 后 50%，Rifle 28 m 后 50%，Shotgun 6.4 m 后 70%、20 m 后 50%；Pistol/Rifle 保持原 Constant 阶跃。
5. 仅确认是否存在运行时外部 BaseDamage 注入；若没有，不再为“找 Lyra BaseDamage 写入点”扩大搜索。
6. 基于事实给出项目侧权威数值来源的最小方案，提交用户确认。
7. 用户批准前不修改任何代码和资产。

## 已经确认且不要重复调查

- 当前项目没有每枪独立 Damage GE 资产。
- 当前三把枪开火 GA 默认使用 UShootEffect_DamageSetByCaller。
- 当前 BaseDamage 全部为 10，距离衰减曲线为空，MaterialDamageMultiplier 为空，爆头字段未读取。
- 当前 WeakSpot + Status.Marked 的 10% 加成硬编码在 Rifle、Shotgun、Sniper 的 ApplyDamageToTarget 中。
- 当前 AbilitySet 的 GrantedGameplayEffects 为空。
- 当前 Sniper 的 DamageGameplayEffectClass 为 null。
- Lyra 的 C++ 伤害链已读完，结论在 Investigation_Lyra伤害链.md。
- Lyra GA_Weapon_Fire 有 GE_Damage 变量，子蓝图分别指向 GE_Damage_Pistol / GE_Damage_RifleAuto / GE_Damage_Shotgun。
- 三个 GE_Damage 均无 Modifier，只有 ULyraDamageExecution。
- WeaponInstance MaterialDamageMultiplier：Pistol 2.0 / Rifle 1.5 / Shotgun 1.75，Tag 为 Gameplay.Zone.WeakSpot。
- PhysMat_Player 无 Tag，PhysMat_Player_WeakSpot 为 Gameplay.Zone.WeakSpot。

## 提交纪律

- 只提交本任务包目录下的文档。
- 使用 git commit --only -- <file> 防止把其他 AI 已暂存的动画资产带进提交。
- 提交到当前 GE 专用工作分支并推送远端；创建分支时必须从最新 main 开始，禁止复用旧 feature/ge-damage-regression-docs。
