// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_SetStats.generated.h"

class UShootInventoryItemInstance;

/**
 * 物品实例初始 StatTag 配置。
 *
 * 该类按 Lyra InventoryFragment_SetStats 的职责迁入，但写入项目自己的
 * UShootInventoryItemInstance。武器弹匣和备弹必须由 ItemDefinition 数据配置，
 * 不能由拾取 Actor、QuickBar 或具体 GA 硬编码。
 */
UCLASS()
class NEWWORLDORDER_API UShootInventoryFragment_SetStats : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UShootInventoryItemInstance* Instance) const override;

	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetItemStatByTag(FGameplayTag Tag) const;

	/** 创建 ItemInstance 时写入的初始数值；存档或掉落快照会在随后整体覆盖这些默认值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	TMap<FGameplayTag, int32> InitialItemStats;
};
