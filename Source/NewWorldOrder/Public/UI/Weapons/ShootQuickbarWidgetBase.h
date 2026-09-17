// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Character/QuickbarMessageTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/ShootTaggedWidget.h"
#include "ShootQuickbarWidgetBase.generated.h"

class UShootQuickBarComponent;
class UShootQuickbarSlotWidgetBase;
class UShootWeaponAmmoWidgetBase;

/**
 * Quickbar Widget 基类：监听 FQuickbarSlotsChanged/FQuickbarActiveIndexChanged 消息
 * 并缓存当前槽位数据，供蓝图绘制 UI（详见 Docs/Tasks/InventorySystem/Implementation_实现指南.md Phase1-B）。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootQuickbarWidgetBase : public UShootTaggedWidget
{
	GENERATED_BODY()

public:
	UShootQuickbarWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 主动请求一次完整刷新（例如 Tab 打开时调用） */
	UFUNCTION(BlueprintCallable, Category="Quickbar")
	void RequestQuickbarRefresh();

	/** C++ 默认更新现有三个槽位壳；蓝图可追加动画，不需要重复消息订阅。 */
	UFUNCTION(BlueprintNativeEvent, Category="Quickbar")
	void HandleQuickbarSlotsUpdated();

	/** C++ 默认更新选中态和当前武器信息；蓝图可追加动画。 */
	UFUNCTION(BlueprintNativeEvent, Category="Quickbar")
	void HandleQuickbarActiveIndexUpdated(int32 NewIndex);

	/** 当前 Quickbar 槽位快照（以 FQuickbarSlotData 为准） */
	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	TArray<FQuickbarSlotData> QuickbarSlots;

	/** 当前激活槽位索引（-1 表示未装备） */
	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	int32 ActiveSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UShootQuickbarSlotWidgetBase> WeaponSlot1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UShootQuickbarSlotWidgetBase> WeaponSlot2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UShootQuickbarSlotWidgetBase> WeaponSlot3;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UShootWeaponAmmoWidgetBase> WeaponAmmoAndName;

private:
	void RefreshNativeWidgets();
	void RegisterQuickbarListeners();
	void UnregisterQuickbarListeners();
	void HandleSlotsChanged(FGameplayTag Channel, const FQuickbarSlotsChangedMessage& Message);
	void HandleActiveIndexChanged(FGameplayTag Channel, const FQuickbarActiveIndexChangedMessage& Message);
	void HandleWeaponAmmoChanged(FGameplayTag Channel, const FWeaponAmmoChangedMessage& Message);

	APawn* GetOwningPlayerPawnChecked() const;
	UShootQuickBarComponent* GetQuickBarComponent() const;

	FGameplayMessageListenerHandle SlotsChangedHandle;
	FGameplayMessageListenerHandle ActiveIndexHandle;
	FGameplayMessageListenerHandle AmmoChangedHandle;
};
