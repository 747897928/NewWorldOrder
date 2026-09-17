// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceInventoryComponent.generated.h"

class UShootInventoryItemDefinition;

/**
 * 账号级资源条目（材料/货币/徽章/图纸等）
 */
USTRUCT(BlueprintType)
struct FResourceEntry
{
	GENERATED_BODY()

	/** 资源对应的物品定义（DataAsset），用于区分资源类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UShootInventoryItemDefinition> ItemDef = nullptr;

	/** 当前数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 0;
};

/**
 * 资源变化消息，用于 UI/制作界面刷新
 */
USTRUCT(BlueprintType)
struct FResourceChangedMessage
{
	GENERATED_BODY()

	/** 拥有资源的组件所属 Actor（通常是 PlayerState） */
	UPROPERTY(BlueprintReadOnly, Category="Resource")
	TObjectPtr<AActor> Owner = nullptr;

	/** 哪种资源发生了变化 */
	UPROPERTY(BlueprintReadOnly, Category="Resource")
	TSubclassOf<UShootInventoryItemDefinition> ItemDef = nullptr;

	/** 新数量 */
	UPROPERTY(BlueprintReadOnly, Category="Resource")
	int32 NewCount = 0;

	/** 变化量（NewCount - OldCount） */
	UPROPERTY(BlueprintReadOnly, Category="Resource")
	int32 Delta = 0;
};

/**
 * 制作/消耗所需的资源条目
 */
USTRUCT(BlueprintType)
struct FResourceCostEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
	TSubclassOf<UShootInventoryItemDefinition> ItemDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ClampMin="0"))
	int32 RequiredCount = 0;
};

/**
 * 组合型资源成本（多个 CostEntry 组成）
 */
USTRUCT(BlueprintType)
struct FResourceCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
	TArray<FResourceCostEntry> Entries;
};

/**
 * 账号级资源仓库组件（材料/货币/徽章/设计图）
 *
 * 设计目标：
 * - 挂在 PlayerState，表示账号/存档层的资源，不与战斗背包混用
 * - 提供 Add/Consume/Query 接口
 * - 资源变化通过 GameplayMessage 通知 UI
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UResourceInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 增加资源（仅服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource")
	bool AddResource(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta);

	/** 消耗资源（仅服务器，可选允许不足返回失败） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource")
	bool ConsumeResource(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta);

	/** 查询资源数量（客户端/服务器均可） */
	UFUNCTION(BlueprintPure, Category="Resource")
	int32 GetResourceCount(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const;

	/** Blueprint：批量获取当前所有资源（UI 一次性刷新用） */
	UFUNCTION(BlueprintPure, Category="Resource")
	void GetAllResources(TArray<FResourceEntry>& OutEntries) const;

	/** 校验是否拥有足够资源（制作/预览 UI 调用，客户端可用） */
	UFUNCTION(BlueprintPure, Category="Resource")
	bool HasEnoughResource(const FResourceCost& ResourceCost) const;

	/** 构建资源存档数据（仅服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource|SaveGame")
	void BuildResourceSaveData(TArray<FResourceEntry>& OutEntries) const;

	/** 应用资源存档数据（仅服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Resource|SaveGame")
	void ApplyResourceSaveData(const TArray<FResourceEntry>& InEntries);

private:
	/** 查找或创建资源条目索引，返回 INDEX_NONE 表示非法参数 */
	int32 FindOrAddEntry(TSubclassOf<UShootInventoryItemDefinition> ItemDef);

	/** 获取资源允许的最大数量（<=0 表示不限制） */
	int32 GetMaxAllowedCount(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const;

	/** 广播资源变化消息 */
	void BroadcastChange(const TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 OldCount, int32 NewCount);

	/** 复制回调，供客户端刷新 UI */
	UFUNCTION()
	void OnRep_Entries();

private:
	/** 资源列表（简单数组，数量有限，复制即可） */
	UPROPERTY(ReplicatedUsing=OnRep_Entries)
	TArray<FResourceEntry> Entries;

	/** 客户端缓存上一次数量，用于 OnRep 计算 Delta（不复制） */
	UPROPERTY(Transient)
	TMap<TSubclassOf<UShootInventoryItemDefinition>, int32> LastKnownCounts;
};
