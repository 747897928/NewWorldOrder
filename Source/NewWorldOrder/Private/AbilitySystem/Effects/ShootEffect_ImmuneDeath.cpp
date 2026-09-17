// 临时免疫致死：授予 Status.ImmuneDeath 标签，持续时间可由 Spec 覆盖
#include "AbilitySystem/Effects/ShootEffect_ImmuneDeath.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UShootEffect_ImmuneDeath::UShootEffect_ImmuneDeath(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.0f));

	const FGameplayTag ImmuneTag = FGameplayTag::RequestGameplayTag(FName("Status.ImmuneDeath"), false);
	if (ImmuneTag.IsValid())
	{
		FInheritedTagContainer TagChanges;
		TagChanges.AddTag(ImmuneTag);
		UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
		if (TargetTags)
		{
			GEComponents.Add(TargetTags);
			TargetTags->SetAndApplyTargetTagChanges(TagChanges);
		}
	}
}
