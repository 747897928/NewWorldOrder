# GE 伤害系统回归 Lyra - 任务包入口

## 任务目标

- 先调查，后改代码。
- 目标：Rifle、Pistol、Shotgun 的每发命中伤害由各自配置的 GameplayEffect 为权威入口，不再由开火 GA 内部硬编码伤害字段和倍率。
- 保留 PhysMat_Player 与 PhysMat_Player_WeakSpot 的弱点/爆头语义，并让该语义重新进入 GAS 伤害结算。
- 为后续 RPG 属性影响攻击力预留扩展点，本阶段不实现 RPG 数值。
- 本任务包已进入实施与回归阶段；当前权威进度以 Status_状态.md 为准。

## 最终文档

- Final/README_最终实施指南.md：最终干净的实施文档，从这篇开始。

## 过程笔记（非最终）
## 目录

- Overview_总览.md：目标、边界、当前架构与目标架构。
- Status_状态.md：调查进度、已确认事实、未决问题、下一步。
- Investigation_当前项目伤害链.md：NewWorldOrder 当前实现与资产事实。
- Investigation_Lyra伤害链.md：Lyra 11000 本机源码与资产事实。
- Evidence_字段引用与资产事实.md：可复验的字段引用表、资产属性表与调查方法。
- Implementation_迁移方案.md：可直接照着写代码的实施指南。
- Lyra关键代码摘录.md：Lyra 关键 C++ 代码与蓝图位置，实施时对照使用。
- Lyra全量适配对照.md：伤害、距离衰减、弱点、准星、命中提示、WeaponState、后坐力的全量映射。
- SetByCallerInitialization_SetByCaller初始化.md：C++ GameplayEffect CDO 初始化顺序、双通道写入与套件幂等规则。

## 阅读顺序

1. 先读 Status_状态.md。
2. 再读 Evidence_字段引用与资产事实.md 建立事实基线。
3. 再读 Investigation_Lyra伤害链.md 与 Investigation_当前项目伤害链.md。
4. 实施前先读 Lyra全量适配对照.md 和 Lyra关键代码摘录.md，再读 Implementation_迁移方案.md。
5. 决策前回到 Overview_总览.md 核对边界。

## 本任务包与现有任务包的关系

- 上游任务：Docs/Tasks/LyraShooterCoreAdaptation/Status_状态.md 后续系统任务第 1 条明确列出武器伤害 GE 审计。
- 上游任务：Docs/Tasks/WeaponSystem/ 负责武器表现、装备、QuickBar 主线。
- 上游任务：Docs/Tasks/CombatSystem/ 负责技能、控制、治疗与现有伤害 GE 资产扫描。
- 本任务包只接管武器开火伤害链，不重复审计动画、装备、QuickBar、技能 Actor。

## 工作红线

- 不读取 .uasset 文件字节；资产事实只通过 UE MCP 查询。
- Lyra 与第三方 Plugins 源码只读。
- 代码与资产修改必须先由调查证据证明必要，并按阶段编译、PIE 验收和选择性提交。
- 中文文档以 UTF-8 保存，写入后检查无乱码。
- 结论必须区分：代码事实、MCP 资产事实、待 Lyra MCP 验证、推测。
