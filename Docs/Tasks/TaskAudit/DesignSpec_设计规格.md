# 设计规格

## 盘点范围
- 任务包：Docs/Tasks/CharacterSwitching、InventorySystem、CombatSystem、Protagonist
- 设计正史：Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md
- 代码事实：Source/NewWorldOrder

## 数据对齐策略
- 以 Source/NewWorldOrder 记录当前实现事实
- 以 游戏设计完整文档 v7.0 Final.md 记录目标需求
- 任务包作为落地计划与流程，若不一致则标记为待更新
- 当代码未完成但文档描述明确时，先记录为缺口，不反推为已实现

## 输出结构
- 任务状态表：任务包名称、当前状态、证据文件、缺口
- 文档清理表：已归档文档、归档原因、归档路径
- 缺口清单：与正史冲突或未实现的功能点

## 现有任务清单（本轮复核）
1. CharacterSwitching
   - 状态：进行中（2026-01-30 已更新 STATUS）
   - 代码证据：
     - `Source/NewWorldOrder/Private/Player/ShootPlayerState.cpp` 内含 `SwitchToCharacter`、快照保存/加载、`IsInHubOrSafeArea`
   - 主要缺口：
     - Hub/安全区判定仍为关卡名临时逻辑
     - 快照未覆盖主动效果/冷却/高级战斗状态
     - 宿舍区角色切换站 UI 与确认流程未落地

2. InventorySystem
   - 状态：进行中（2026-01-30 已更新 STATUS）
   - 代码证据：InventoryManager/ResourceInventory/QuickBar 链路已落地
   - 主要缺口：
     - ResourceInventory 未入存档
     - InventoryList::AddEntry(Instance) 未实现
     - CanAddItemDefinition/部分消耗等校验缺失

3. CombatSystem
   - 状态：进行中（已建立任务包）
   - 代码证据：`Docs/Tasks/CombatSystem/DamageAndControl_AssetsScan.md`
   - 主要缺口：
     - 缺少通用 Stun/Knockback/Slow GE
     - 缺少可执行的补齐清单与验收

4. Protagonist
   - 状态：TODO 已拆分为可执行子任务
   - 代码证据：`Docs/Tasks/Protagonist/Protagonist_TODO_主角技能收敛.md`
   - 主要缺口：
     - 多数技能 TODO 仍未实现（被动自动激活、击杀归属、控场/表现等）

## 缺口清单（基于代码与正史比对）
- ResourceInventory 缺少 SaveGame 持久化
- InventoryList::AddEntry(Instance) 未实现但入口仍暴露
- CharacterSwitching 安全区判定仍使用 MapName 临时逻辑
- CharacterSwitching 快照未覆盖主动效果/冷却/高级战斗状态
- CharacterSwitching 宿舍区切换站 UI 与确认流程未落地
- Protagonist 多数 TODO（技能逻辑/表现/数值对接）未落地
- CombatSystem 缺少可执行验收与控制效果资产

## 文档清理原则
- 旧内容不与新内容并存
- 过期文档移入 Docs/Archives
- 仅保留可执行、可验证的内容
