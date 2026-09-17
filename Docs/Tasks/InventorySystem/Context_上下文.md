# 任务背景
- GameDesign v7.0/v7.2 明确要求：武器之外需要完整的背包、材料、徽章、设计图、金币管理，以及 Hub 制作/商店循环。
- 现有实现仅有 `UCombatComponent` 三槽武器栏和 `AShootWeaponActor` 的弹药字段，没有统一的“物品/材料”容器，也缺少背包 UI 和持久化。
- 新需求还引入消耗品、材料掉落倍率、制作动画、NPC 对话等跨系统逻辑，必须先有权威背包模型才能驱动。

# 当前实现概览
1. `UCombatComponent`（Source/NewWorldOrder/Public|Private/Character）  
   - 负责复制 3 个 `AShootWeaponActor` 槽位和当前激活索引。  
   - 通过 GameplayMessageSubsystem 把槽位、弹药数据推给 UI。  
   - 不关心材料/徽章/设计图；背包=武器插槽。
2. `AShootWeaponActor`  
   - 只跟踪 Magazine/Reserve，备弹暂时由“未来库存系统”管理。  
   - 具备客户端预测（`PredictedAmmo`），但与实际背包资源无直接关联。
3. 物品数据  
  - 现已改为 ItemDefinition + WeaponConfig Fragments（基础/射击/投射物），不再使用 `UWeaponDefinition`。  
  - 引入 `InventoryItemDefinition/Instance` + `GameplayTagStack` 架构，与 Lyra 对齐。  

# 任务目标
- 评估是否引入 Lyra 物品/库存基建（InventoryManagerComponent + ItemInstance + TagStack）。  
- 梳理 GameDesign 中背包/材料/制作的硬性需求，形成实现团队可直接执行的任务包。  
- 给出最小可行方案（MVP）与后续扩展计划，并列出受影响模块/文件。  
- 产出交付物：Requirements、DesignSpec、Implementation 指南、Checklist、更新版 STATUS.md。

# 范围与依赖
- 范围包括：武器/材料/徽章/设计图/金币的容器模型、UI 数据流、与 GAS Cost 的接口、制作/消耗流程。  
- 暂不触及：掉落表实现、NPC 剧情文本、资产制作，但需在方案中预留接口。  
- 需要参考：`Docs/ExampleProjectCode/LyraStarterGame`（Inventory/Equipment/System 目录），`Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md` §7.3-7.4。
