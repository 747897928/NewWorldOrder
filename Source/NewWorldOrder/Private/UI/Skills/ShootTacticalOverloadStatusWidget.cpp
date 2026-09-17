// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Skills/ShootTacticalOverloadStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "GameplayEffect.h"
#include "GameFramework/PlayerController.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootTacticalOverloadStatusWidget)

void UShootTacticalOverloadStatusWidget::RefreshInteractionProgress()
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const AShootPlayerState* ShootPlayerState = PlayerController
		? PlayerController->GetPlayerState<AShootPlayerState>()
		: nullptr;
	UAbilitySystemComponent* ASC = ShootPlayerState
		? ShootPlayerState->GetAbilitySystemComponent()
		: nullptr;
	const FGameplayTag OverloadTag = FShootGameplayTags::Get().Status_Overload;

	float RemainingSeconds = 0.f;
	float DurationSeconds = 0.f;
	if (ASC && OverloadTag.IsValid() && ASC->HasMatchingGameplayTag(OverloadTag))
	{
		FGameplayTagContainer StatusTags;
		StatusTags.AddTag(OverloadTag);
		const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StatusTags));
		for (const TPair<float, float>& Time : Times)
		{
			if (Time.Key > RemainingSeconds)
			{
				RemainingSeconds = Time.Key;
				DurationSeconds = Time.Value;
			}
		}
	}

	const bool bVisible = RemainingSeconds > 0.f && DurationSeconds > 0.f;
	const ESlateVisibility DesiredVisibility = bVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;
	if (InteractionProgressBar)
	{
		// 与换弹/交互读条一样只折叠子控件，保留蓝图 Event Tick 用于下一次状态激活时自恢复。
		InteractionProgressBar->SetVisibility(DesiredVisibility);
		InteractionProgressBar->SetPercent(bVisible
			? FMath::Clamp(RemainingSeconds / DurationSeconds, 0.f, 1.f)
			: 0.f);
	}
	if (MessageCommonText)
	{
		MessageCommonText->SetVisibility(DesiredVisibility);
		if (bVisible)
		{
			const FText Label = ActiveLabel.IsEmpty()
				? NSLOCTEXT("ShootSkills", "TacticalOverload", "Tactical Overload")
				: ActiveLabel;
			MessageCommonText->SetText(FText::Format(
				NSLOCTEXT("ShootSkills", "TimedStatus", "{0}  {1}s"),
				Label, FText::AsNumber(FMath::Max(1, FMath::CeilToInt(RemainingSeconds)))));
		}
	}
}
