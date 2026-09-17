// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Inventory/ResourceInventoryComponent.h"
#include "UI/LyraActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ShootResourceListWidgetBase.generated.h"

/**
 * 材料/货币列表 Widget 基类
 *
 * - 监听 ResourceInventoryComponent 的 GameplayMessage（Inventory.Resource.Message.Changed）
 * - 通过 BlueprintLibrary 查询账号层资源，缓存到 ResourceEntries 数组
 * - 蓝图子类只需绑定 `HandleResourceEntriesUpdated` 来刷新列表
 *
 * 重要：ResourceInventory = 数量型账号仓库，武器 QuickBar 仍然只引用 InventoryManager
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootResourceListWidgetBase : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UShootResourceListWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	/** 主动刷新当前所有资源条目（可供蓝图/外部调用） */
	UFUNCTION(BlueprintCallable, Category="ResourceList")
	void RefreshResourceEntries();

	/** 蓝图实现：收到刷新后，用 ResourceEntries 更新 UI */
	UFUNCTION(BlueprintImplementableEvent, Category="ResourceList")
	void HandleResourceEntriesUpdated();

	/** 当前账号资源列表（材料/货币/徽章/图纸等） */
	UPROPERTY(BlueprintReadOnly, Category="ResourceList")
	TArray<FResourceEntry> ResourceEntries;

private:
	void RegisterMessageListener();
	void UnregisterMessageListener();
	void HandleResourceChanged(FGameplayTag Channel, const FResourceChangedMessage& Message);

	/** 缓存 GameplayMessage 句柄 */
	FGameplayMessageListenerHandle ResourceListHandle;
};
