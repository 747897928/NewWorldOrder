// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Inventory/ResourceInventoryComponent.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ResourceInventoryBlueprintLibrary.generated.h"

class UResourceInventoryComponent;
class UShootInventoryItemDefinition;
class AController;
class APawn;
class AShootPlayerState;

/**
 * 资源仓库蓝图函数库
 *
 * 设计目的：
 * - 统一从 Pawn/Controller/PlayerState 获取 ResourceInventoryComponent
 * - 供拾取、UI、制作逻辑在蓝图中调用 Add/Consume/Query
 *
 * 重要区分：
 * - ResourceInventoryComponent = 数量型资源仓库（材料/货币/徽章/设计图）
 * - UShootInventoryManagerComponent = 有身份的物品/武器仓库
 *   QuickBar/Equipment 只和后者交互，Hub 配置与副本内换枪走同一条线
 */
UCLASS()
class UResourceInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 从任意上下文（Pawn/Controller/PlayerState/组件）获取资源仓库组件 */
	UFUNCTION(BlueprintCallable, Category="Resource", meta=(DefaultToSelf="WorldContextObject"))
	static UResourceInventoryComponent* GetResourceInventory(const UObject* WorldContextObject);

	/** 服务器：增加资源（材料/货币），成功返回 true */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource")
	static bool AddResource(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta);

	/** 服务器：消耗资源，若数量不足返回 false */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource")
	static bool ConsumeResource(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta);

	/** 客户端/服务器：查询资源数量 */
	UFUNCTION(BlueprintPure, Category="Resource")
	static int32 GetResourceCount(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef);

	/** 客户端：获取所有资源条目，供 UI/VM 一次性绘制 */
	UFUNCTION(BlueprintCallable, Category="Resource")
	static void GetAllResources(const UObject* WorldContextObject, TArray<FResourceEntry>& OutEntries);

	/** 客户端：检查资源是否满足消耗条件（制作按钮可用状态） */
	UFUNCTION(BlueprintPure, Category="Resource")
	static bool HasEnoughResource(const UObject* WorldContextObject, const FResourceCost& ResourceCost);
};
