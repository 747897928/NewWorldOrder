// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Character/QuickbarMessageTypes.h"
#include "UI/ShootTaggedWidget.h"

#include "ShootQuickbarSlotWidgetBase.generated.h"

class UBorder;
class UCommonNumericTextBlock;
class UCommonTextBlock;
class UImage;

/**
 * WBP_WeaponSlot 的稳定数据桥。
 * Lyra W_QuickBarSlot 监听 QuickBar 消息后更新每个槽；项目把消息过滤集中在父 QuickBar，
 * 子槽只负责显示自己的 FQuickbarSlotData，避免三个槽各自重复注册监听器和 Tick。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootQuickbarSlotWidgetBase : public UShootTaggedWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Quickbar")
	void SetSlotData(const FQuickbarSlotData& InSlotData, int32 InSlotIndex, bool bInSelected);

	UFUNCTION(BlueprintCallable, Category="Quickbar")
	void ClearSlot(int32 InSlotIndex, bool bInSelected);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Quickbar", meta=(DisplayName="On Slot Data Changed"))
	void BP_OnSlotDataChanged(const FQuickbarSlotData& InSlotData, bool bInOccupied);

	UFUNCTION(BlueprintImplementableEvent, Category="Quickbar", meta=(DisplayName="On Slot Selection Changed"))
	void BP_OnSlotSelectionChanged(bool bInSelected);

	/**
	 * 只在槽位由空变为有物品、或由有物品变为空时触发。
	 * WBP_WeaponSlot 用它播放 Lyra 的 ToEmpty 正向/反向动画，不能复用 On Slot Data Changed，
	 * 否则弹药数等普通数据刷新也会反复重播空槽转场。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Quickbar", meta=(DisplayName="On Slot Occupancy Changed"))
	void BP_OnSlotOccupancyChanged(bool bInOccupied);

	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	FQuickbarSlotData SlotData;

	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category="Quickbar")
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UBorder> SelectionBorder;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> WeaponCard;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonNumericTextBlock> WeaponAmmoCount;

	/**
	 * WBP_WeaponSlot 现有的空槽文字。它属于 UMG 表现壳，C++ 只根据槽位业务状态控制显隐；
	 * 如果不绑定，占用槽也会永久覆盖一层 EMPTY，造成后端已拾枪但 HUD 看起来仍为空。
	 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> EmptyText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> AmmoIcon;

private:
	void ApplyNativeVisuals();

	/**
	 * 新建槽位 Widget 的业务默认值和 UMG 动画初态不是同一件事：bOccupied 默认 false，
	 * 但 Lyra 复制来的 ItemGlow 初态仍可能可见。首次收到数据时必须显式同步一次占用态，
	 * 否则 false -> false 不会触发 ToEmpty，空槽会残留蓝色发光。
	 */
	bool bHasReceivedSlotData = false;
};
