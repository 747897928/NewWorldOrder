#include "UI/Weapons/ShootSniperScopeWidgetBase.h"

#include "Animation/WidgetAnimation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootSniperScopeWidgetBase)

void UShootSniperScopeWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Hidden);

	if (ScopeZoomIn)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &ThisClass::HandleScopeAnimationFinished);
		BindToAnimationFinished(ScopeZoomIn, FinishedEvent);
	}
}

void UShootSniperScopeWidgetBase::NativeDestruct()
{
	if (ScopeZoomIn)
	{
		UnbindAllFromAnimationFinished(ScopeZoomIn);
	}

	Super::NativeDestruct();
}

void UShootSniperScopeWidgetBase::OnReticleADSVisualChanged_Implementation(bool bIsADS)
{
	bHideWhenAnimationFinishes = !bIsADS;

	if (!ScopeZoomIn)
	{
		SetVisibility(bIsADS ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PlayAnimation(
		ScopeZoomIn,
		bIsADS ? 0.0f : ScopeZoomIn->GetEndTime(),
		1,
		bIsADS ? EUMGSequencePlayMode::Forward : EUMGSequencePlayMode::Reverse);
}

void UShootSniperScopeWidgetBase::HandleScopeAnimationFinished()
{
	if (bHideWhenAnimationFinishes)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
}
