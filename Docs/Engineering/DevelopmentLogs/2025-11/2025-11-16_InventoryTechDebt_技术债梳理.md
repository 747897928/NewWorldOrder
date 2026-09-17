# 2025-11-16 Inventory Technical Debt 复盘

## 目的
- 明确当前库存/武器链路的技术债清单，避免实现阶段临时起意。
- 确认 QuickBar 与 ItemLifetime 的操作顺序，指导 Phase1/2 编码。
- 梳理 `ARangedWeaponInstance`（AActor）与 `UShootRangedWeaponInstance`（UObject）并存造成的交叉依赖，提出迁移路线。

## 当前痛点
1. QuickBar（`UCombatComponent`）仍保存 `ARangedWeaponInstance` 指针，导致：
   - Persistent 与 RuntimeOnly 武器没有明确界限，副本拾取可能直接覆盖账号槽。
   - SaveGame 逻辑无法仅序列化 ItemInstance，因为槽位里没有 Item GUID。
2. InventoryManager 缺少 `AddPersistentItem/AddRuntimeItem` 双入口，拾取 Actor 无法根据副本/Hub 语境分流。
3. `ARangedWeaponInstance` 与 `AHitscanWeaponInstance` 继续承担“数据 + 表现”双重职责：
   - 开火/换弹 GA 直接从 Actor 读取备弹，无法共享 TagStack。
   - `UShootRangedWeaponInstance` 半成品孤立存在，部分 GA 代码引用它，导致 SourceObject 类型不一致。

## 行动计划（按优先级）
1. **QuickBar 槽位引用 ItemInstance**  
   - `UCombatComponent` 保存 `FShootQuickBarSlot{FGuid ItemInstanceId, ESlotType Type}`，激活时通过 InventoryManager 查询 Instance。  
   - 仅当 Instance 有 WeaponFragment 时才生成/附着 `ARangedWeaponInstance`，槽位不再直接存 Actor。  
   - RuntimeOnly 武器：占用当前槽位但不写入 SaveGame，副本结束调用 InventoryManager 清除所有 RuntimeOnly 并刷新 QuickBar。  
2. **ItemLifetime API**  
   - `UShootInventoryItemInstance` 增加 `EShootItemLifetime ItemLifetime` + `FGuid InstanceId`（如未存在）。  
   - InventoryManager 对外暴露：`AddPersistentItem`、`AddRuntimeItem`、`RemoveItemById`、`RemoveAllRuntimeItems`。  
   - SaveGame 仅序列化 Persistent 实例（DefinitionId + InstanceId + StatTags + QuickBarSlotIndex）。  
3. **武器实例分层**  
   - 阶段一（兼容期）：QuickBar 仍生成 `ARangedWeaponInstance`/`AHitscanWeaponInstance`，但其 Ammo/状态全部来源于 ItemInstance StatTags。Actor 只负责 Mesh/FX。  
   - 阶段二：将 GA（开火/换弹） SourceObject 切换为 `UShootInventoryItemInstance`，并通过 Fragment 接口访问 Actor（表现层）。  
   - 阶段三：当 UObject 版本稳定后，开始淘汰 `ARangedWeaponInstance` 数据字段，仅保留表现组件或独立 `AShootWeaponDisplayActor`。  

## 影响分析
- QuickBar 改造需要同步更新：HUD 快捷栏 UI、输入映射、`ShootPlayerController` 中的槽位切换逻辑。  
- ItemLifetime 新字段要求 UI/拾取/制作在调用 InventoryManager 时明确语义，所有 Blueprint 节点需增加 Lifetime 选择。  
- 武器 GA 迁移涉及文件：`ShootGameplayAbility_Weapon_Fire.*`、`ShootGA_Weapon_Fire_*`、`ShootGA_Reload_*`、`ShootAbilityCost_Ammo.*`、`ShootAbilityCost_ItemTagStack.*`。  
- 需要新增验证用例：
  1. 副本内拾取 RuntimeOnly 武器并离开副本后槽位恢复。  
  2. Hub 制作出 Persistent 武器并手动将其装备到槽位，重登后保持。  
  3. Ammo TagStack 与 `UShootAbilityCost_Ammo` 协同工作，客户端预测扣弹仍走 PredictedAmmo。  

## 下一步
1. 在 `UShootInventoryManagerComponent` 中实现 Runtime/Persistent Item API（含清理函数）。
2. QuickBar 改造 PoC：单槽绑定 ItemInstance，验证装备流程与消息广播。
3. 起草 GA 改造 checklist，列出所有事件/Tag 触点，提交到 `Docs/Tasks/InventorySystem/STATUS.md`。 
