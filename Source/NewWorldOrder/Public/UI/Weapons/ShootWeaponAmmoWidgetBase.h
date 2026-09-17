// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Character/QuickbarMessageTypes.h"
#include "UI/ShootTaggedWidget.h"

#include "ShootWeaponAmmoWidgetBase.generated.h"

class UCommonTextBlock;
class UImage;
class UVerticalBox;

/** 当前激活武器的名称、弹匣和备弹显示桥。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootWeaponAmmoWidgetBase : public UShootTaggedWidget
{
	GENERATED_BODY()

public:
	void SetActiveWeaponData(const FQuickbarSlotData* InSlotData);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Quickbar", meta=(DisplayName="On Active Weapon Data Changed"))
	void BP_OnActiveWeaponDataChanged(bool bHasWeapon, const FQuickbarSlotData& InSlotData);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBoxForHiding;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> AmmoLeftInMagazineWidget;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TotalCountWidget;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> WeaponNameWidget;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> AmmoIcon;
};
