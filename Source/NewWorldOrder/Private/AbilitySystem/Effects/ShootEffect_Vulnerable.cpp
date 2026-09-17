// 通用易伤效果：授予 Vulnerable Tag，并减少减伤 30%
#include "AbilitySystem/Effects/ShootEffect_Vulnerable.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "ShootGameplayTags.h"

UShootEffect_Vulnerable::UShootEffect_Vulnerable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

	// 通过 GameplayEffectComponent 配置授予 Tag（替代已废弃的 InheritableOwnedTagsContainer）
	{
		const FGameplayTag VulnerableTag = FGameplayTag::RequestGameplayTag(FName("Status.Vulnerable"), false);
		if (VulnerableTag.IsValid())
		{
			FInheritedTagContainer TagChanges;
			TagChanges.AddTag(VulnerableTag);

			UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
			if (TargetTags)
			{
				GEComponents.Add(TargetTags);
				TargetTags->SetAndApplyTargetTagChanges(TagChanges);
			}
		}
	}

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetDamageReductionBonusAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-0.3f)); // 额外易伤 30%
	Modifiers.Add(Mod);
}
