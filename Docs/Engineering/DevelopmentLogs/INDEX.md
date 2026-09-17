# 开发日志索引

## 2025年11月

### 2025-11-01

SaveGame跨平台崩溃排查
- 文件：2025-11/2025-11-01_SaveGame跨平台崩溃排查.md
- 问题：macOS PIE模式下SaveGame序列化崩溃
- 解决：UTexture2D裸指针改为TSoftObjectPtr
- 相关笔记：Notes/UE5/SaveGame_序列化规范.md, Notes/UE5/SoftObjectPtr_软引用最佳实践.md

### 2025-11-02

武器实例分层改造
- 文件：2025-11/2025-11-02_武器实例分层改造.md
- 任务：拆分Hitscan和Projectile武器实例
- 相关笔记：Notes/Weapons/Hitscan_命中实现.md, Notes/Weapons/Projectile_投射物设计.md

爆炸伤害弱点倍率设计
- 文件：2025-11/2025-11-02_爆炸伤害弱点倍率.md
- 任务：为爆炸武器添加弱点倍率开关
- 相关笔记：Notes/Weapons/Projectile_投射物设计.md

GAS武器命中流程整合
- 文件：2025-11/2025-11-02_GAS武器命中流程.md
- 任务：对齐Lyra命中流程，整合TargetData
- 相关笔记：Notes/GAS/Weapon_命中流程.md, Notes/GAS/TargetData_预测流程.md

Sonar约束实践
- 文件：2025-11/2025-11-02_Sonar约束实践.md
- 任务：重构武器代码以符合Sonar质量约束
- 相关笔记：Notes/Patterns/Sonar_代码质量约束.md

### 2025-11-06

文档体系重构
- 文件：2025-11/2025-11-06_文档体系重构.md
- 任务：应用3NF原则重构知识管理架构
- 相关ADR：ADR/ADR-001_知识管理架构.md

### 2025-11-14

库存系统集成方案
- 文件：2025-11/2025-11-14_库存系统集成方案.md
- 任务：评估并确定引入 Lyra 库存/TagStack 管线的实施方案
- 相关资料：Docs/Tasks/InventorySystem/*, Docs/ExampleProjectCode/LyraStarterGame

库存系统会议记录
- 文件：2025-11/2025-11-14_库存系统会议记录.md
- 内容：Codex Session 会议纪要，涵盖用户/GPT5 对话、需求、资料、未决问题
- 用途：为后续 AI 恢复上下文提供单一入口

### 2025-11-16

Inventory 技术债梳理
- 文件：2025-11/2025-11-16_InventoryTechDebt_技术债梳理.md
- 内容：QuickBar/ItemLifetime/武器实例并行架构的技术债、行动计划、验证用例
- 用途：指导 Phase1/2 实施顺序，确保 RuntimeOnly/ Persistent 行为与武器 GA 迁移路径一致

## 按主题分类

### UE5引擎
- 2025-11-01: SaveGame跨平台崩溃排查

### GAS系统
- 2025-11-02: GAS武器命中流程整合

### 武器系统
- 2025-11-02: 武器实例分层改造
- 2025-11-02: 爆炸伤害弱点倍率设计

### 代码质量
- 2025-11-02: Sonar约束实践

### 文档管理
- 2025-11-06: 文档体系重构

### 物品与库存
- 2025-11-14: 库存系统集成方案
- 2025-11-14: 库存系统会议记录
- 2025-11-16: Inventory 技术债梳理
