// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/ResourceInventoryComponent.h"
#include "ShootHUDResourceToastComponent.generated.h"

class UShootResourceToastWidgetBase;
class UGameplayMessageSubsystem;
class APlayerController;

/**
 * 资源拾取 HUD 组件：监听 Inventory.Resource / UI.Toast 消息，并通知 Toast Widget 播放。
 * 架构说明：
 * - ResourceInventoryComponent = 数量型账号仓库，只负责材料/货币/徽章/图纸的 Add/Consume/Query
 * - InventoryManagerComponent = 有身份背包（武器/装备/消耗品），QuickBar/Equipment 仅引用该组件
 * - 此组件仅挂在本地 PlayerController，用于驱动 UI，不与服务器权威逻辑耦合
 */
UCLASS(ClassGroup="UI", meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootHUDResourceToastComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootHUDResourceToastComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

protected:
	void HandleResourceToast(FGameplayTag Channel, const FResourceChangedMessage& Message);
	void CreateToastWidget();
	void DestroyToastWidget();

	bool IsLocalPlayerController() const;

protected:
	/** HUD 使用的 Toast Widget 类（蓝图可继承 UShootResourceToastWidgetBase） */
	UPROPERTY(EditDefaultsOnly, Category="ResourceToast")
	TSubclassOf<UShootResourceToastWidgetBase> ResourceToastWidgetClass;

	/** Toast Widget AddToViewport 的层级 */
	UPROPERTY(EditDefaultsOnly, Category="ResourceToast")
	int32 ResourceToastZOrder = 12;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	UPROPERTY(Transient)
	TWeakObjectPtr<UGameplayMessageSubsystem> CachedMessageSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UShootResourceToastWidgetBase> ResourceToastWidget;

	FGameplayMessageListenerHandle ResourceToastHandle;
};
