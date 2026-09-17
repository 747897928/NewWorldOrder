# GAS 与 Lyra 整合现状盘点

日期：2026-01-30
状态：可用
来源：游戏设计完整文档 v7.0 Final.md（v7.2）、Source/NewWorldOrder 代码、Lyra 示例代码

## 正史基线
- 技术栈：GAS + Lyra 风格库存/装备管线
- 正史文档以 游戏设计完整文档 v7.0 Final.md 为准
- 若任务包与正史冲突，需以 Source/NewWorldOrder 实际实现为准并记录差异

## 当前实现（Source/NewWorldOrder）
- ASC 位于 PlayerState，ReplicationMode=Mixed
- PlayerState 持有 InventoryManager 与 ResourceInventory
- InventoryManager 使用 FastArray + SubObject 复制，ItemInstance 存 StatTags 与生命周期，InstanceMap 缓存 Guid→Instance
- QuickBar 在 UCombatComponent（Pawn 侧），槽位保存 ItemInstanceId 与 Lifetime，缓存 EquipmentInstance 与 WeaponInstance
- EquipmentManager 在 Pawn 上，负责 Equip/Unequip 并生成 SpawnedActors
- WeaponInstance 为逻辑层，WeaponActor 仅作表现壳（SpawnedActors 内部使用）
- 弹药成本通过 UShootAbilityCost_AmmoTagStack -> ItemTagStack 走 StatTags 扣减
- SaveGameSubsystem 仅保存 Persistent 物品与 QuickBar/外观快照，RuntimeOnly 清理
- GameplayMessageSubsystem 广播库存/QuickBar/资源变化消息

## Lyra 对齐点
- FastArray + SubObject 复制模型与 Lyra InventoryManager 一致
- EquipmentList 结构与 Lyra EquipmentManager 相同，依赖 AbilitySet 授予
- QuickBar 逻辑继承 Lyra 模式，但实现位置改为 CombatComponent
- 使用 GameplayMessageSubsystem 分发 UI 事件
- Actor 作为 Outer 的复制规避 UE-127172 与 Lyra 一致

## 关键差异
- Lyra QuickBar 在 ControllerComponent；本项目在 Pawn 的 CombatComponent
- InventoryManager 在 PlayerState，而非 Character
- QuickBar 槽位存储 Guid/Lifetime，并允许 RuntimeOnly 临时武器
- WeaponActor 仍保留为表现壳（Lyra 更偏向纯 UObject + SpawnedActors）

## 已知缺口与风险（基于 Source）
- ResourceInventory 未进入 SaveGame，资源跨关卡不持久化
- InventoryList::AddEntry(Instance) 为 unimplemented，但 AddItemInstance 仍暴露入口
- CanAddItemDefinition 未做堆栈/唯一性/容量检查
- ConsumeItemsByDefinition 不支持部分消耗且 O(n^2)
- QuickBar 槽位类型匹配与安全区判定仍为 TODO（当前用关卡名包含判断）

## 建议优先级
- 补 ResourceInventory 存档与恢复链路
- 明确 AddItemInstance 的使用场景并实现 AddEntry(Instance) 或移除入口
- 增加堆栈/唯一性/容量检查与部分消耗
- 用 GameState 标记或地图 Tag 替换安全区字符串判断
- 补齐 QuickBar 槽位类型校验

## 证据路径
- `Source/NewWorldOrder/Public/Player/ShootPlayerState.h`
- `Source/NewWorldOrder/Private/Player/ShootPlayerState.cpp`
- `Source/NewWorldOrder/Public/Inventory/ShootInventoryManagerComponent.h`
- `Source/NewWorldOrder/Private/Inventory/ShootInventoryManagerComponent.cpp`
- `Source/NewWorldOrder/Public/Inventory/ResourceInventoryComponent.h`
- `Source/NewWorldOrder/Private/Inventory/ResourceInventoryComponent.cpp`
- `Source/NewWorldOrder/Public/Character/CombatComponent.h`
- `Source/NewWorldOrder/Private/Character/CombatComponent.cpp`
- `Source/NewWorldOrder/Public/Weapons/ShootRangedWeaponInstance.h`
- `Source/NewWorldOrder/Private/Weapons/ShootRangedWeaponInstance.cpp`
- `Source/NewWorldOrder/Public/AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h`
- `Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.cpp`
- `Source/NewWorldOrder/Public/System/ShootSaveGame.h`
- `Source/NewWorldOrder/Private/System/SaveGameSubsystem.cpp`
- `Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Inventory/LyraInventoryManagerComponent.*`
- `Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Equipment/LyraQuickBarComponent.*`
- `Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Equipment/LyraEquipmentManagerComponent.*`
- `Docs/ExampleProjectCode/LyraStarterGame/Plugins/GameplayMessageRouter/Source/GameplayMessageRuntime/Public/GameFramework/GameplayMessageSubsystem.h`
