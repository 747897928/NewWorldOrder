# 内容目录整理任务入口

## 任务定位

本任务包定义 NewWorldOrder 的 Unreal Content 目录整理标准，借鉴 Lyra 按功能域和游戏特性组织内容的方式，结合本项目的角色、武器、库存、PVE、Mutable 和 HomeMap 需求形成项目自己的目录规范。

初始交付物是整理方案，不默认执行资产移动、重命名、Redirector 修复或删除；用户明确授权后按批次执行，
并在本任务包的状态和检查清单中记录实际完成结果。

## 接手顺序

1. `STATUS.md`
2. `Requirements_需求.md`
3. `DesignSpec_设计规格.md`
4. `Implementation_实现指南.md`
5. `Checklist_检查清单.md`

## 本轮结论

- `/Game/Assets` 应逐步废弃，但资产必须按用途和归属迁移，不能整体平移到新的泛化目录。
- `/Game/Blueprints` 不作为正式生产目录；Blueprint 应放到其所属的角色、AI、武器、Gameplay、UI 或地图功能域。
- `ArchVizInteriorVol3` 本轮按用户确认将全部资产（含三张地图）迁入 HomeMap 生产目录并保留；`NiagaraExamples` 仍先按第三方示例包隔离，实际使用内容再提取到生产目录。
- `CC_Shaders` 属于 Character Creator 插件边界，本任务不处理。
- AmmoBox、Dressing_Table_Set、FirstKitAid 的模型属于环境中的可交互 Gameplay Props；未来的物品定义和交互 Actor 仍与模型分开。
- 角色动画以骨架和角色归属为第一层，MF/MM 共用动画放入 `CC/Shared`，不要无条件全部塞进 `MF`。

## 尚未进入本轮的内容

- ArchViz 本轮已通过 Unreal Editor 资产操作完成迁移；后续未授权的批量资产移动仍不执行。
- 不执行 `/Game/Assets` 删除。
- 不执行 `/Game/Blueprints` 批量迁移。
- 不删除 Niagara 示例中的未使用资产。
- 不移动或重命名 `/Game/CC_Shaders`。
- 不在本任务中重新设计 C++、GameplayTag、SaveGame 或运行时资产加载逻辑。
