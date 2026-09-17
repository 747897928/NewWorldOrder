// 通用眩晕效果：授予 Stunned/NoFiring 标签，时长可由外部 GE Spec 覆盖
#include "AbilitySystem/Effects/ShootEffect_Stun.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UShootEffect_Stun::UShootEffect_Stun(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.0f));

	// 通过 GameplayEffectComponent 配置授予 Tag
	const FGameplayTag StunTag = FGameplayTag::RequestGameplayTag(FName("Status.Stunned"), false);
	const FGameplayTag NoFiringTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.NoFiring"), false);
	if (StunTag.IsValid() || NoFiringTag.IsValid())
	{
		FInheritedTagContainer TagChanges;
		if (StunTag.IsValid())
		{
			TagChanges.AddTag(StunTag);
		}
		if (NoFiringTag.IsValid())
		{
			TagChanges.AddTag(NoFiringTag);
		}

		UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
		if (TargetTags)
		{
			GEComponents.Add(TargetTags);
			TargetTags->SetAndApplyTargetTagChanges(TagChanges);
		}
	}
}
