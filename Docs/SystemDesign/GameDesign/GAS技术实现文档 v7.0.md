# 新秩序 - GAS 技术实现文档 v7.x

最后更新：2026-01-30

## 目标
- 记录当前工程的 GAS 实现事实
- 作为代码与任务包的索引，不保留过期逻辑

## 核心架构

### ASC 与复制
- 玩家 ASC 位于 PlayerState，ReplicationMode=Mixed
- AI ASC 位于 Character，ReplicationMode=Minimal
- PlayerState 与 Character 在 OnRep 中重绑 AbilityActorInfo

代码路径
- Source/NewWorldOrder/Public/Player/ShootPlayerState.h
- Source/NewWorldOrder/Private/Player/ShootPlayerState.cpp
- Source/NewWorldOrder/Public/AbilitySystem/ShootAbilitySystemComponent.h

### GameplayTag 约定
- 统一使用宏注册或 RequestGameplayTag 兜底
- 构造函数内不直接访问 FShootGameplayTags::Get

参考路径
- Docs/Engineering/Notes/GameplayTagInitialization.md
- Source/NewWorldOrder/Public/ShootGameplayTags.h

### 装备与武器逻辑
- WeaponInstance 为逻辑层 UObject
- WeaponActor 仅作表现壳
- Ability 的 SourceObject 为 WeaponInstance

代码路径
- Source/NewWorldOrder/Public/Weapons/ShootWeaponInstance.h
- Source/NewWorldOrder/Public/Weapons/ShootRangedWeaponInstance.h
- Source/NewWorldOrder/Public/Character/CombatComponent.h

### 库存与资源
- InventoryManager 管理有身份物品
- ResourceInventory 管理数量型资源
- QuickBar 只引用 InventoryManager 的 ItemInstanceId

代码路径
- Source/NewWorldOrder/Public/Inventory/ShootInventoryManagerComponent.h
- Source/NewWorldOrder/Public/Inventory/ResourceInventoryComponent.h
- Source/NewWorldOrder/Public/Character/CombatComponent.h

### 弹药与成本
- 弹药通过 ItemInstance.StatTags 管理
- AbilityCost 使用 UShootAbilityCost_AmmoTagStack
- Status_Overload 跳过扣弹

代码路径
- Source/NewWorldOrder/Public/AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h
- Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.cpp

### GameplayCue 管理
- 使用 ULyraGameplayCueManager 进行延迟加载

代码路径
- Source/NewWorldOrder/Public/AbilitySystem/LyraGameplayCueManager.h
- Source/NewWorldOrder/Private/AbilitySystem/LyraGameplayCueManager.cpp

### 存档
- SaveGameSubsystem 保存 Persistent 物品与 QuickBar
- RuntimeOnly 在切换副本时清理
- ResourceInventory 尚未进入存档

代码路径
- Source/NewWorldOrder/Public/System/ShootSaveGame.h
- Source/NewWorldOrder/Private/System/SaveGameSubsystem.cpp

## 当前缺口
- ResourceInventory 资源未持久化
- InventoryList::AddEntry(Instance) 未实现
- WeaponInstance 预测接口尚未提供独立预测值

## 冲突处理规则
- 正史以 游戏设计完整文档 v7.0 Final.md 为准
- 若任务包与正史冲突，先查 Source/NewWorldOrder 实际实现并记录差异
