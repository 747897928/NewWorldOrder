// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Equipment/ShootEquipmentDefinition.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_EquippableItem.generated.h"

/**
 * UShootInventoryFragment_EquippableItem
 *
 * 【可装备物品片段】连接库存系统和装备系统的桥梁
 *
 * 设计目的：
 *   - 标记物品可以被装备（武器、护甲、工具等）
 *   - 提供装备时使用的 EquipmentDefinition
 *   - 实现库存系统 → 装备系统的数据流转
 *
 * 系统架构：
 *   ```
 *   库存系统 (Inventory)                    装备系统 (Equipment)
 *   ┌─────────────────────┐                ┌──────────────────────┐
 *   │ ItemDefinition      │                │ EquipmentDefinition  │
 *   │   DisplayName       │                │   InstanceType       │
 *   │   Fragments[]       │                │   AbilitySetsToGrant │
 *   │     ↓               │                │   ActorsToSpawn      │
 *   │   EquippableItem ───┼────────────────┼→ (引用)             │
 *   │     (Fragment)      │                │                      │
 *   └─────────────────────┘                └──────────────────────┘
 *            ↓                                        ↓
 *   ┌─────────────────────┐                ┌──────────────────────┐
 *   │ ItemInstance        │                │ EquipmentInstance    │
 *   │   ItemDef           │                │   Instigator ←───────┼─┐
 *   │   StatTags          │                │   SpawnedActors      │ │
 *   │   (弹药存储)        │←───────────────┼──────────────────────┘ │
 *   └─────────────────────┘                        (连接)           │
 *                                                                   │
 *   快捷栏 (QuickBar)                                               │
 *   ┌─────────────────────┐                                        │
 *   │ Slots[ItemInstance] │                                        │
 *   │ ActiveSlotIndex     │                                        │
 *   │                     │                                        │
 *   │ EquipItemInSlot():  │                                        │
 *   │   1. GetSlot() → ItemInstance                                │
 *   │   2. FindFragment<EquippableItem>()                          │
 *   │   3. EquipmentManager->EquipItem(Fragment->EquipmentDef,     │
 *   │      ItemInstance)                                           │
 *   │      （EquipItem 现会自动 SetInstigator） ──────────────────┘
 *   └─────────────────────┘
 *   ```
 *
 * 数据流转：
 *   ```
 *   [玩家切换武器]
 *   1. QuickBar->SetActiveSlotIndex(2)
 *      ↓
 *   2. QuickBar->UnequipItemInSlot() (卸载旧武器)
 *      - EquipmentManager->UnequipItem(OldWeapon)
 *      - OldWeapon->OnUnequipped()
 *      - 移除 AbilitySets
 *      - 销毁 SpawnedActors
 *      ↓
 *   3. QuickBar->EquipItemInSlot() (装备新武器)
 *      - ItemInstance = Slots[2]
 *      - EquippableFragment = ItemInstance->ItemDef->FindFragment<EquippableItem>()
 *      - EquipmentDef = EquippableFragment->EquipmentDefinition
 *      ↓
 *   4. EquipmentManager->EquipItem(EquipmentDef, ItemInstance)
 *      - 创建 EquipmentInstance
 *      - 自动 SetInstigator，以便武器访问弹药
 *      - 生成 SpawnedActors (武器 Mesh)
 *      - 授予 AbilitySets (开火、装填等)
 *      ↓
 *   5. [完成] 玩家现在装备了新武器
 *   ```
 *
 * 关键连接：Instigator
 *   - EquipmentInstance.Instigator = ItemInstance
 *   - 武器通过 Instigator 访问弹药：
 *     ```cpp
 *     UShootInventoryItemInstance* ItemInstance = Cast<UShootInventoryItemInstance>(GetInstigator());
 *     int32 Ammo = ItemInstance->GetStatTagStackCount(Tag_Weapon_Ammo_Magazine);
 *     ```
 *
 * Fragment 的作用：
 *   - 标记：ItemDefinition 包含 EquippableItem Fragment → 此物品可装备
 *   - 桥接：Fragment 引用 EquipmentDefinition → 装备时使用此 Definition
 *   - 解耦：库存系统不需要知道装备系统的细节，只需要 Fragment 作为接口
 *
 * 设计优势：
 *   - 数据驱动：不同武器只需创建不同的 DataAsset
 *   - 可复用：多个 ItemDefinition 可以共享同一个 EquipmentDefinition
 *   - 灵活：一个物品可以既是材料又是武器（添加多个 Fragment）
 *
 * 示例用法：
 *   ```
 *   创建 ItemDefinition: DA_Item_Rifle
 *   - DisplayName: "突击步枪"
 *   - Fragments:
 *     [0] EquippableItem:
 *       EquipmentDefinition: DA_Equipment_Rifle
 *     [1] SetStats:
 *       InitialItemStats:
 *         Tag.Weapon.Ammo.Magazine: 30
 *         Tag.Weapon.Ammo.Reserve: 90
 *
 *   创建 EquipmentDefinition: DA_Equipment_Rifle
 *   - InstanceType: UShootRangedWeaponInstance
 *   - AbilitySetsToGrant: [AS_Rifle]
 *   - ActorsToSpawn: [BP_WeaponMesh_Rifle]
 *   ```
 */
UCLASS()
class UShootInventoryFragment_EquippableItem : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * 装备定义
	 *
	 * 指向装备系统的 EquipmentDefinition
	 * 装备此物品时使用此 Definition
	 *
	 * 用途：
	 *   - QuickBar 切换装备时：
	 *     ```cpp
	 *     const UShootInventoryFragment_EquippableItem* EquipFragment =
	 *         ItemInstance->FindFragmentByClass<UShootInventoryFragment_EquippableItem>();
	 *     if (EquipFragment && EquipFragment->EquipmentDefinition)
	 *     {
	 *         EquippedItem = EquipmentManager->EquipItem(EquipFragment->EquipmentDefinition, ItemInstance);
	 *     }
	 *     ```
	 *
	 * 数据资产引用：
	 *   - 在编辑器中，此字段引用另一个 DataAsset（EquipmentDefinition）
	 *   - 这种引用是轻量的（只是指针），不会重复数据
	 *
	 * 示例：
	 *   - ItemDef: DA_Item_Rifle
	 *     - Fragment[EquippableItem]:
	 *       - EquipmentDefinition: DA_Equipment_Rifle
	 *   - ItemDef: DA_Item_Shotgun
	 *     - Fragment[EquippableItem]:
	 *       - EquipmentDefinition: DA_Equipment_Shotgun
	 *
	 * 复用场景：
	 *   - 多个 ItemDefinition 可以共享同一个 EquipmentDefinition
	 *   - 例如：DA_Item_Rifle_Blue 和 DA_Item_Rifle_Red（不同皮肤）
	 *     可以都引用 DA_Equipment_Rifle（相同功能）
	 */
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition;
};
