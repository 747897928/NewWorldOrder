// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootQuickbarSlotTypes.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_QuickbarSlotRules.generated.h"

/**
 * UShootInventoryFragment_QuickbarSlotRules
 *
 * 【快捷栏槽位规则】限制物品可放入的槽位类型
 *
 * 规则说明：
 * - AllowedSlotTypes 为空或包含 Any 时，表示不限制
 */
UCLASS(BlueprintType)
class UShootInventoryFragment_QuickbarSlotRules : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** 允许放入的槽位类型（为空表示不限制） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	TArray<EShootQuickbarSlotType> AllowedSlotTypes;
};
