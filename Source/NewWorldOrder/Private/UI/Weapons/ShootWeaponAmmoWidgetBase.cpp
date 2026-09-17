// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Weapons/ShootWeaponAmmoWidgetBase.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWeaponAmmoWidgetBase)

void UShootWeaponAmmoWidgetBase::SetActiveWeaponData(const FQuickbarSlotData* InSlotData)
{
	const bool bHasWeapon = InSlotData &&
		(InSlotData->ItemInstanceId.IsValid() || InSlotData->ItemDefinition != nullptr);
	const FQuickbarSlotData EmptyData;
	const FQuickbarSlotData& Data = bHasWeapon ? *InSlotData : EmptyData;

	if (VerticalBoxForHiding)
	{
		VerticalBoxForHiding->SetVisibility(
			bHasWeapon ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (AmmoLeftInMagazineWidget)
	{
		AmmoLeftInMagazineWidget->SetText(FText::AsNumber(Data.Ammo));
	}
	if (TotalCountWidget)
	{
		TotalCountWidget->SetText(FText::AsNumber(Data.Reserve));
	}
	if (WeaponNameWidget)
	{
		WeaponNameWidget->SetText(Data.DisplayName);
	}
	if (AmmoIcon)
	{
		// WBP_WeaponAmmoAndName 使用 MI_UI_Icon_Ammo_Counter 表现弹种图标。
		// 保留蓝图配置的材质 Brush，只把 BasicConfig.AmmoIcon 的纹理写入其 TextureMask 参数。
		if (bHasWeapon)
		{
			if (UTexture* AmmoTexture = Cast<UTexture>(Data.AmmoIcon.GetResourceObject()))
			{
				if (UMaterialInstanceDynamic* AmmoIconMaterial = AmmoIcon->GetDynamicMaterial())
				{
					AmmoIconMaterial->SetTextureParameterValue(TEXT("TextureMask"), AmmoTexture);
				}
			}
		}
	}

	BP_OnActiveWeaponDataChanged(bHasWeapon, Data);
}
