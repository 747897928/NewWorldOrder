// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Skills/ShootSkillBarWidgetBase.h"

#include "AbilitySystem/Skills/ShootSkillDefinition.h"
#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/ShootPlayerState.h"
#include "UI/Foundation/LyraActionWidget.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootSkillBarWidgetBase)

namespace ShootSkillBarWidget
{
	const FName IconTextureParameter(TEXT("IconTexture"));
	const FName CooldownParameter(TEXT("Animate_Cooldown"));
	const FName TimerStartParameter(TEXT("TimerStart"));
	const FName DurationParameter(TEXT("Duration"));
	const FName AssociatedActionProperty(TEXT("AssociatedAction"));
}

void UShootSkillBarWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	CacheVisualSlots();
	BindToOwningPlayer();
}

void UShootSkillBarWidgetBase::NativeDestruct()
{
	if (BoundLoadout)
	{
		BoundLoadout->OnSkillSlotsChanged().RemoveAll(this);
		BoundLoadout = nullptr;
	}
	Super::NativeDestruct();
}

void UShootSkillBarWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!BoundLoadout)
	{
		// HUD 可能早于 PlayerState/Experience 完成构造；只在尚未绑定时重试，避免分屏拿错 LocalPlayer。
		BindToOwningPlayer();
	}
	RefreshCooldowns();
}

void UShootSkillBarWidgetBase::BindToOwningPlayer()
{
	if (BoundLoadout)
	{
		BoundLoadout->OnSkillSlotsChanged().RemoveAll(this);
		BoundLoadout = nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	AShootPlayerState* ShootPlayerState = OwningPlayer
		? OwningPlayer->GetPlayerState<AShootPlayerState>()
		: nullptr;
	BoundLoadout = ShootPlayerState ? ShootPlayerState->GetSkillLoadoutComponent() : nullptr;
	if (BoundLoadout)
	{
		BoundLoadout->OnSkillSlotsChanged().AddUObject(this, &ThisClass::HandleSlotsChanged);
	}
	RefreshAllSlots();
}

void UShootSkillBarWidgetBase::HandleSlotsChanged(UShootSkillLoadoutComponent* ChangedComponent,
	int32 ChangedSlotIndex)
{
	if (ChangedComponent != BoundLoadout)
	{
		return;
	}

	if (ChangedSlotIndex == INDEX_NONE)
	{
		RefreshAllSlots();
	}
	else
	{
		RefreshSlot(ChangedSlotIndex);
	}
}

void UShootSkillBarWidgetBase::CacheVisualSlots()
{
	VisualSlots.Reset(UShootSkillLoadoutComponent::SkillSlotCount);
	VisualSlots.Add(BuildVisualSlot(SkillSlot1));
	VisualSlots.Add(BuildVisualSlot(SkillSlot2));
	VisualSlots.Add(BuildVisualSlot(SkillSlot3));
	VisualSlots.Add(BuildVisualSlot(SkillSlot4));
}

UShootSkillBarWidgetBase::FRuntimeSkillSlotVisual UShootSkillBarWidgetBase::BuildVisualSlot(
	UUserWidget* SlotShell) const
{
	FRuntimeSkillSlotVisual Visual;
	Visual.SlotShell = SlotShell;
	if (!SlotShell)
	{
		return Visual;
	}

	// W_SkillSlot 是纯蓝图视觉壳；这些名称属于该壳的稳定、文档化契约。
	UUserWidget* ActionTouchButton = Cast<UUserWidget>(SlotShell->GetWidgetFromName(TEXT("ActionTouchButton")));
	Visual.ActionTouchButton = ActionTouchButton;
	Visual.InputActionWidget = Cast<ULyraActionWidget>(SlotShell->GetWidgetFromName(TEXT("InputActionWidget")));
	// W_SkillSlot 的等级契约由用户蓝图提供：LevelSizer 管显隐，SkillLevel 显示 1-3。
	Visual.LevelSizer = SlotShell->GetWidgetFromName(TEXT("LevelSizer"));
	Visual.LevelText = Cast<UTextBlock>(SlotShell->GetWidgetFromName(TEXT("SkillLevel")));
	// 这四个名称由用户在 W_SkillSlot 中新增；C++ 只管理业务文本和显隐，不改蓝图布局。
	Visual.SkillNameSizer = SlotShell->GetWidgetFromName(TEXT("SkillNameSizer"));
	Visual.SkillNameText = Cast<UTextBlock>(SlotShell->GetWidgetFromName(TEXT("SkillName")));
	Visual.CooldownSizer = SlotShell->GetWidgetFromName(TEXT("CooldownSizer"));
	Visual.CooldownFrame = Cast<UImage>(SlotShell->GetWidgetFromName(TEXT("CooldownFrame")));
	Visual.RemainingCooldownText = Cast<UTextBlock>(SlotShell->GetWidgetFromName(TEXT("RemainingCooldownTime")));
	if (ActionTouchButton)
	{
		Visual.WeaponCard = Cast<UImage>(ActionTouchButton->GetWidgetFromName(TEXT("WeaponCard")));
		Visual.ItemGlow = Cast<UImage>(ActionTouchButton->GetWidgetFromName(TEXT("ItemGlow")));
		Visual.ItemGlowBoost = Cast<UImage>(ActionTouchButton->GetWidgetFromName(TEXT("ItemGlow_Boost")));
		Visual.EmptyText = Cast<UTextBlock>(ActionTouchButton->GetWidgetFromName(TEXT("EmptyText")));
	}
	return Visual;
}

void UShootSkillBarWidgetBase::RefreshAllSlots()
{
	for (int32 SlotIndex = 0; SlotIndex < UShootSkillLoadoutComponent::SkillSlotCount; ++SlotIndex)
	{
		RefreshSlot(SlotIndex);
	}
}

void UShootSkillBarWidgetBase::RefreshSlot(int32 SlotIndex)
{
	if (!VisualSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	FRuntimeSkillSlotVisual& Visual = VisualSlots[SlotIndex];
	const FShootSkillSlot SlotData = BoundLoadout ? BoundLoadout->GetSlot(SlotIndex) : FShootSkillSlot();
	const UShootSkillDefinition* Definition = SlotData.SkillDefinition;
	const bool bOccupied = Definition && SlotData.Level > 0;
	const FSlateBrush* DisplayIcon = bOccupied ? &Definition->GetIconForMode(SlotData.CurrentModeTag) : nullptr;
	const FText DisplayName = bOccupied
		? Definition->GetDisplayNameForMode(SlotData.CurrentModeTag)
		: FText::GetEmpty();

	// 空槽不能继续向 W_ActionTouchButton 注入 IA，否则玩家点击一个显示 EMPTY 的槽仍会发送技能输入。
	// 输入提示也只在获得技能后显示；具体按键仍来自 Experience 配置和玩家改键结果。
	SetSlotInputAction(Visual,
		bOccupied && BoundLoadout ? BoundLoadout->GetSlotInputAction(SlotIndex) : nullptr);
	if (UImage* WeaponCard = Visual.WeaponCard.Get())
	{
		// 与 UShootQuickbarSlotWidgetBase::ApplyNativeVisuals 相同：保留 MI_UI_WeaponCard 材质壳，
		// 只写 IconTexture 参数。直接 SetBrush 会丢失用户蓝图里的底色、边框、遮罩和发光层。
		if (bOccupied)
		{
			if (UTexture* IconTexture = DisplayIcon
				? Cast<UTexture>(DisplayIcon->GetResourceObject())
				: nullptr)
			{
				if (UMaterialInstanceDynamic* CardMaterial = WeaponCard->GetDynamicMaterial())
				{
					CardMaterial->SetTextureParameterValue(ShootSkillBarWidget::IconTextureParameter, IconTexture);
				}
			}
		}
		WeaponCard->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UImage* ItemGlow = Visual.ItemGlow.Get())
	{
		// W_ActionTouchButton 的设计时发光层默认可见。首次收到空槽数据也必须显式关掉，
		// 否则会出现四块蓝色实心卡片，与 WBP_WeaponSlot 的 EMPTY 终态不一致。
		ItemGlow->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UImage* ItemGlowBoost = Visual.ItemGlowBoost.Get())
	{
		// Boost 层预留给将来的强化/选中状态。普通已占用槽和空槽都不应常亮。
		ItemGlowBoost->SetVisibility(ESlateVisibility::Hidden);
	}
	if (UTextBlock* EmptyText = Visual.EmptyText.Get())
	{
		// EmptyText 位于用户迁移到 W_ActionTouchButton 的内部 WidgetTree；它不是 W_SkillSlot 的直接子控件。
		// C++ 只维护业务显隐，不修改该蓝图的文字、字体、布局或 EventGraph。
		EmptyText->SetVisibility(bOccupied ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
	}
	if (ULyraActionWidget* InputWidget = Visual.InputActionWidget.Get())
	{
		InputWidget->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UTextBlock* LevelText = Visual.LevelText.Get())
	{
		LevelText->SetText(bOccupied ? FText::AsNumber(SlotData.Level) : FText::GetEmpty());
		LevelText->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UWidget* LevelSizer = Visual.LevelSizer.Get())
	{
		// 空槽不显示默认的“1”；升级复制到客户端后与槽数据在同一次刷新中更新。
		LevelSizer->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UTextBlock* SkillNameText = Visual.SkillNameText.Get())
	{
		SkillNameText->SetText(DisplayName);
		SkillNameText->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UWidget* SkillNameSizer = Visual.SkillNameSizer.Get())
	{
		SkillNameSizer->SetVisibility(bOccupied ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (UUserWidget* SlotShell = Visual.SlotShell.Get())
	{
		SlotShell->SetToolTipText(bOccupied
			? FText::Format(NSLOCTEXT("ShootSkillBar", "SkillTooltip", "{0}\n{1}"),
				DisplayName, Definition->Description)
			: FText::GetEmpty());
	}
	if (!bOccupied)
	{
		SetCooldownPercent(Visual, 1.f);
		SetCooldownText(Visual, false, 0.f);
		SetRadialCooldown(Visual, false, 0.f, 0.f);
	}
}

void UShootSkillBarWidgetBase::SetSlotInputAction(FRuntimeSkillSlotVisual& Visual, UInputAction* InputAction) const
{
	if (ULyraActionWidget* InputWidget = Visual.InputActionWidget.Get())
	{
		InputWidget->SetAssociatedInputAction(InputAction);
	}

	// AssociatedAction 定义在蓝图 W_ActionTouchButton 上，它的 OnClicked 图负责把同一 IA 注入 Enhanced Input。
	// 这里保留蓝图可读调用链，只设置蓝图公开的配置变量，不复制或删除用户图表。
	if (UUserWidget* ActionButton = Visual.ActionTouchButton.Get())
	{
		if (FObjectPropertyBase* ActionProperty = FindFProperty<FObjectPropertyBase>(
			ActionButton->GetClass(), ShootSkillBarWidget::AssociatedActionProperty))
		{
			ActionProperty->SetObjectPropertyValue_InContainer(ActionButton, InputAction);
		}
	}
}

void UShootSkillBarWidgetBase::RefreshCooldowns()
{
	if (!BoundLoadout)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < VisualSlots.Num(); ++SlotIndex)
	{
		float Remaining = 0.f;
		float Duration = 0.f;
		const bool bHasCooldown = BoundLoadout->GetCooldownRemaining(SlotIndex, Remaining, Duration) &&
			Remaining > 0.f && Duration > 0.f;
		// W_ActionTouchButton 的材质约定 0=刚进入冷却，1=技能重新可用。
		SetCooldownPercent(VisualSlots[SlotIndex], bHasCooldown
			? FMath::Clamp(1.f - Remaining / Duration, 0.f, 1.f)
			: 1.f);
		SetCooldownText(VisualSlots[SlotIndex], bHasCooldown, Remaining);
		SetRadialCooldown(VisualSlots[SlotIndex], bHasCooldown, Remaining, Duration);
	}
}

void UShootSkillBarWidgetBase::SetRadialCooldown(FRuntimeSkillSlotVisual& Visual, const bool bHasCooldown,
	const float RemainingSeconds, const float DurationSeconds) const
{
	UImage* CooldownFrame = Visual.CooldownFrame.Get();
	if (!CooldownFrame)
	{
		return;
	}

	CooldownFrame->SetVisibility(bHasCooldown
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
	if (!bHasCooldown || DurationSeconds <= 0.f)
	{
		return;
	}

	if (UMaterialInstanceDynamic* Material = CooldownFrame->GetDynamicMaterial())
	{
		// M_UI_RadialProgress_Ability 使用游戏时间推进。TimerStart 与数字倒计时都由同一份
		// GAS Remaining/Duration 推导，因此不会出现材质转完而文本仍未归零的第二计时器。
		const UWorld* World = GetWorld();
		const float Now = World ? World->GetTimeSeconds() : 0.f;
		Material->SetScalarParameterValue(ShootSkillBarWidget::DurationParameter, DurationSeconds);
		Material->SetScalarParameterValue(ShootSkillBarWidget::TimerStartParameter,
			Now - FMath::Max(0.f, DurationSeconds - RemainingSeconds));
	}
}

void UShootSkillBarWidgetBase::SetCooldownPercent(FRuntimeSkillSlotVisual& Visual, float ReadyPercent) const
{
	for (UImage* CooldownImage : {Visual.WeaponCard.Get(), Visual.ItemGlow.Get()})
	{
		if (CooldownImage)
		{
			if (UMaterialInstanceDynamic* CooldownMaterial = CooldownImage->GetDynamicMaterial())
			{
				CooldownMaterial->SetScalarParameterValue(
					ShootSkillBarWidget::CooldownParameter, ReadyPercent);
			}
		}
	}
}

void UShootSkillBarWidgetBase::SetCooldownText(FRuntimeSkillSlotVisual& Visual, const bool bHasCooldown,
	const float RemainingSeconds) const
{
	if (UTextBlock* CooldownText = Visual.RemainingCooldownText.Get())
	{
		// 向上取整可避免最后不足一秒时过早显示 0；冷却结束或槽位清空时立即清空文本。
		CooldownText->SetText(bHasCooldown
			? FText::AsNumber(FMath::Max(1, FMath::CeilToInt(RemainingSeconds)))
			: FText::GetEmpty());
		CooldownText->SetVisibility(bHasCooldown
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Hidden);
	}
	if (UWidget* CooldownSizer = Visual.CooldownSizer.Get())
	{
		CooldownSizer->SetVisibility(bHasCooldown
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Hidden);
	}
}
