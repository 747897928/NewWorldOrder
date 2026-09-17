// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Weapons/ShootQuickbarSlotWidgetBase.h"

#include "CommonNumericTextBlock.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootQuickbarSlotWidgetBase)

void UShootQuickbarSlotWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// QuickBar 父 Widget 可能在子槽 Construct 之前先下发初始空数据。
	// 此时蓝图动画尚未建立 Slate 实例，第一次 ToEmpty 不会改变设计时默认可见的 ItemGlow。
	// Construct 完成后必须重放当前业务终态；这里仍只传状态，具体视觉继续由 WBP_WeaponSlot 维护。
	ApplyNativeVisuals();
	BP_OnSlotDataChanged(SlotData, bOccupied);
	BP_OnSlotOccupancyChanged(bOccupied);
	BP_OnSlotSelectionChanged(bSelected && bOccupied);
}

void UShootQuickbarSlotWidgetBase::SetSlotData(
	const FQuickbarSlotData& InSlotData, int32 InSlotIndex, bool bInSelected)
{
	const bool bWasInitialized = bHasReceivedSlotData;
	const bool bWasSelected = bSelected;
	const bool bWasOccupied = bOccupied;
	const bool bWasEffectivelySelected = bWasSelected && bWasOccupied;

	SlotData = InSlotData;
	SlotIndex = InSlotIndex;
	bSelected = bInSelected;
	bOccupied = SlotData.ItemInstanceId.IsValid() || SlotData.ItemDefinition != nullptr;
	const bool bIsEffectivelySelected = bSelected && bOccupied;

	ApplyNativeVisuals();
	BP_OnSlotDataChanged(SlotData, bOccupied);

	// Lyra 的 UpdateSlot 每次都会把 IsEmpty 同步进 ToEmpty；项目事件桥此前漏掉了首帧 false -> false。
	// 首次数据也必须通知蓝图，使 ItemGlow / ItemGlow_Boost 可靠收敛到空槽动画终态。
	if (!bWasInitialized || bWasOccupied != bOccupied)
	{
		BP_OnSlotOccupancyChanged(bOccupied);
	}
	bHasReceivedSlotData = true;

	// Lyra 的选中态由 InactiveToActive / ActiveToInactive 动画表达。
	// C++ 只在“槽内有物品且被选中”的有效状态实际变化时通知蓝图，不直接操纵 ItemGlow_Boost。
	if (bWasEffectivelySelected != bIsEffectivelySelected)
	{
		BP_OnSlotSelectionChanged(bIsEffectivelySelected);
	}
}

void UShootQuickbarSlotWidgetBase::ClearSlot(int32 InSlotIndex, bool bInSelected)
{
	SetSlotData(FQuickbarSlotData(), InSlotIndex, bInSelected);
}

void UShootQuickbarSlotWidgetBase::ApplyNativeVisuals()
{
	if (WeaponCard)
	{
		// WBP_WeaponSlot 直接沿用 Lyra 的 MI_UI_WeaponCard 材质壳。
		// 不能 SetBrush 覆盖成裸 Texture，否则会丢掉卡片底色、边框、遮罩和发光层。
		if (bOccupied)
		{
			if (UTexture* IconTexture = Cast<UTexture>(SlotData.Icon.GetResourceObject()))
			{
				if (UMaterialInstanceDynamic* WeaponCardMaterial = WeaponCard->GetDynamicMaterial())
				{
					WeaponCardMaterial->SetTextureParameterValue(TEXT("IconTexture"), IconTexture);
				}
			}
		}
		WeaponCard->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (WeaponAmmoCount)
	{
		WeaponAmmoCount->SetCurrentValue(SlotData.Ammo);
		WeaponAmmoCount->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (EmptyText)
	{
		EmptyText->SetVisibility(bOccupied ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
	}
	if (AmmoIcon)
	{
		// 与 WeaponCard 一样，AmmoIcon 必须保留 WBP_WeaponSlot 配置的 MI_UI_Icon_Ammo_Card 材质壳。
		// BasicConfig.AmmoIcon 只提供弹种遮罩纹理；直接 SetBrush 会覆盖材质并丢失着色、透明和发光效果。
		if (bOccupied)
		{
			if (UTexture* AmmoTexture = Cast<UTexture>(SlotData.AmmoIcon.GetResourceObject()))
			{
				if (UMaterialInstanceDynamic* AmmoIconMaterial = AmmoIcon->GetDynamicMaterial())
				{
					AmmoIconMaterial->SetTextureParameterValue(TEXT("TextureMask"), AmmoTexture);
				}
			}
		}
		AmmoIcon->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}
