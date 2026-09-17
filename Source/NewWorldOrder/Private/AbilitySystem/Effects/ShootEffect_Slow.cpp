// 通用减速效果：授予 Slowed Tag，并降低移动速度倍率（默认 0.6x）
#include "AbilitySystem/Effects/ShootEffect_Slow.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UShootEffect_Slow::UShootEffect_Slow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

	const FGameplayTag SlowedTag = FGameplayTag::RequestGameplayTag(FName("Status.Slowed"), false);
	if (SlowedTag.IsValid())
	{
		FInheritedTagContainer TagChanges;
		TagChanges.AddTag(SlowedTag);
		UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
		if (TargetTags)
		{
			GEComponents.Add(TargetTags);
			TargetTags->SetAndApplyTargetTagChanges(TagChanges);
		}
	}

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetMoveSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.6f));
	Modifiers.Add(Mod);
}
