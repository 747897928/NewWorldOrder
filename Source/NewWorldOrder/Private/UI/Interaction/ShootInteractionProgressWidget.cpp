// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Interaction/ShootInteractionProgressWidget.h"

#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "Player/ShootPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInteractionProgressWidget)

UShootInteractionProgressWidget::UShootInteractionProgressWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UShootInteractionProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 该原生类只提供行为，实际控件由 Interactive_Progress_Bar 蓝图绑定。
	// 配置遗漏时保持安全空状态，不能让 HUD 构造阶段因空 BindWidget 崩溃。
	RefreshInteractionProgress();
}

void UShootInteractionProgressWidget::RefreshInteractionProgress()
{
	const AShootPlayerController* PlayerController = Cast<AShootPlayerController>(GetOwningPlayer());
	const bool bVisible = PlayerController && PlayerController->IsInteractionHoldVisible();
	const ESlateVisibility DesiredVisibility = bVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	// Widget 自身保持可 Tick，只折叠蓝图子控件；否则 Collapsed 后无法再靠蓝图 Tick 唤醒。
	if (InteractionProgressBar)
	{
		InteractionProgressBar->SetVisibility(DesiredVisibility);
		InteractionProgressBar->SetPercent(
			PlayerController ? PlayerController->GetInteractionHoldProgress() : 0.0f);
	}
	if (MessageCommonText)
	{
		MessageCommonText->SetVisibility(DesiredVisibility);
	}
}
