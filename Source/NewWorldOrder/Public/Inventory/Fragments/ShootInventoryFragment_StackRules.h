// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_StackRules.generated.h"

/**
 * UShootInventoryFragment_StackRules
 *
 * 【堆叠/唯一性规则片段】为物品定义提供堆叠上限与唯一性约束
 *
 * 用途：
 * - ResourceInventory：用于材料/徽章/设计图等数量上限控制
 * - InventoryManager：用于消耗品/可堆叠物品的堆叠上限或唯一性
 *
 * 规则说明：
 * - MaxStackCount <= 0 表示不限制
 * - bUnique=true 时等价于 MaxStackCount=1
 */
UCLASS(BlueprintType)
class UShootInventoryFragment_StackRules : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** 最大堆叠/存储数量（<=0 表示不限制） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	int32 MaxStackCount = 0;

	/** 是否唯一（只允许拥有 1 个） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	bool bUnique = false;
};
