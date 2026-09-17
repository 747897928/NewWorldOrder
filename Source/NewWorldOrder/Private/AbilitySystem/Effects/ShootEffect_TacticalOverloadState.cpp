// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Effects/ShootEffect_TacticalOverloadState.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEffect_TacticalOverloadState)

UShootEffect_TacticalOverloadState::UShootEffect_TacticalOverloadState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));

	// GameplayEffect CDO 可能早于 FShootGameplayTags::InitializeNativeGameplayTags 构造。
	// 这里必须按名称向 GameplayTagsManager 请求已经在 DefaultGameplayTags.ini 声明的 Tag；
	// 直接读取 FShootGameplayTags::Get().Status_Overload 会得到无效值，表现为 Cue 正常但无限弹药和状态 HUD 同时失效。
	const FGameplayTag OverloadTag = FGameplayTag::RequestGameplayTag(FName("Status.Overload"), false);
	if (OverloadTag.IsValid())
	{
		UTargetTagsGameplayEffectComponent* TargetTags =
			ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
		if (TargetTags)
		{
			FInheritedTagContainer TagChanges;
			TagChanges.AddTag(OverloadTag);
			TargetTags->SetAndApplyTargetTagChanges(TagChanges);
			GEComponents.Add(TargetTags);
		}
	}
}
