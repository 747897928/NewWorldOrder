// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "ShootSkillBarWidgetBase.generated.h"

class UImage;
class UInputAction;
class UTextBlock;
class UWidget;
class ULyraActionWidget;
class UShootSkillLoadoutComponent;

/**
 * 四槽技能栏的单一数据桥。
 *
 * W_SkillBar 负责绑定此类，四个 W_SkillSlot 继续是用户维护的纯蓝图视觉壳。槽壳直接复用
 * W_ActionTouchButton、MI_UI_Base_WeaponCard_CoolDown 与 Lyra QuickBar 材质链；这里仅给材质参数写技能纹理、
 * 给输入控件写 Experience 配置的 IA，并按本地 PlayerState 的四槽组件刷新冷却。不得 SetBrush 覆盖材质壳。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSkillBarWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BindToOwningPlayer();
	void HandleSlotsChanged(UShootSkillLoadoutComponent* ChangedComponent, int32 ChangedSlotIndex);
	void CacheVisualSlots();
	void RefreshAllSlots();
	void RefreshSlot(int32 SlotIndex);
	void RefreshCooldowns();

	/** 名称与用户 W_SkillBar 蓝图中的四个稳定子控件一致；布局和样式仍完全由蓝图维护。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UUserWidget> SkillSlot1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UUserWidget> SkillSlot2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UUserWidget> SkillSlot3;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UUserWidget> SkillSlot4;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Match Skills")
	TObjectPtr<UShootSkillLoadoutComponent> BoundLoadout;

private:
	struct FRuntimeSkillSlotVisual
	{
		TWeakObjectPtr<UUserWidget> SlotShell;
		TWeakObjectPtr<UUserWidget> ActionTouchButton;
		TWeakObjectPtr<UImage> WeaponCard;
		TWeakObjectPtr<UImage> ItemGlow;
		TWeakObjectPtr<UImage> ItemGlowBoost;
		TWeakObjectPtr<ULyraActionWidget> InputActionWidget;
		TWeakObjectPtr<UTextBlock> EmptyText;
		TWeakObjectPtr<UTextBlock> LevelText;
		TWeakObjectPtr<UWidget> LevelSizer;
		TWeakObjectPtr<UTextBlock> SkillNameText;
		TWeakObjectPtr<UWidget> SkillNameSizer;
		TWeakObjectPtr<UTextBlock> RemainingCooldownText;
		TWeakObjectPtr<UWidget> CooldownSizer;
		TWeakObjectPtr<UImage> CooldownFrame;
	};

	FRuntimeSkillSlotVisual BuildVisualSlot(UUserWidget* SlotShell) const;
	void SetSlotInputAction(FRuntimeSkillSlotVisual& Visual, UInputAction* InputAction) const;
	void SetCooldownPercent(FRuntimeSkillSlotVisual& Visual, float ReadyPercent) const;
	void SetCooldownText(FRuntimeSkillSlotVisual& Visual, bool bHasCooldown, float RemainingSeconds) const;
	void SetRadialCooldown(FRuntimeSkillSlotVisual& Visual, bool bHasCooldown,
		float RemainingSeconds, float DurationSeconds) const;

	TArray<FRuntimeSkillSlotVisual> VisualSlots;
};
