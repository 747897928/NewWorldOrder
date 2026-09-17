// 手雷冷却效果：使用 GameplayEffectComponent 授予阻断 Tag，兼容 UE 5.8 的 GameplayEffect API。
#include "AbilitySystem/Effects/ShootEffect_GrenadeCooldown.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UShootEffect_GrenadeCooldown::UShootEffect_GrenadeCooldown(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));

	const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Cooldown.Weapon.Grenade"), false);
	if (!CooldownTag.IsValid())
	{
		return;
	}

	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CooldownTag);
	UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
	if (TargetTags)
	{
		GEComponents.Add(TargetTags);
		TargetTags->SetAndApplyTargetTagChanges(TagChanges);
	}
}
