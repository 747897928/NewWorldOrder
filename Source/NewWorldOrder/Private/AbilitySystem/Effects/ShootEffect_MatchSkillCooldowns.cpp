// Match Skill 冷却数值的第一版基线；终极技能的充能系统落地前先使用可复验的临时冷却。
#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

void UShootEffect_MatchSkillCooldownBase::ConfigureCooldown(const FObjectInitializer& ObjectInitializer,
	const TCHAR* TagName, const float DurationSeconds)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));

	const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
	if (!CooldownTag.IsValid())
	{
		return;
	}

	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CooldownTag);
	UTargetTagsGameplayEffectComponent* TargetTags =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
	if (TargetTags)
	{
		GEComponents.Add(TargetTags);
		TargetTags->SetAndApplyTargetTagChanges(TagChanges);
	}
}

UShootEffect_TacticalAssaultCooldown::UShootEffect_TacticalAssaultCooldown(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureCooldown(ObjectInitializer, TEXT("Cooldown.Skill.TacticalAssault"), 15.f);
}

UShootEffect_TacticalOverloadCooldown::UShootEffect_TacticalOverloadCooldown(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 战术超载和医疗站原设计依赖充能；充能系统尚未实现时使用 30 秒冷却，避免无代价连续释放。
	ConfigureCooldown(ObjectInitializer, TEXT("Cooldown.Skill.TacticalOverload"), 30.f);
}

UShootEffect_MedicalStationCooldown::UShootEffect_MedicalStationCooldown(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureCooldown(ObjectInitializer, TEXT("Cooldown.Skill.MedicalStation"), 30.f);
}

UShootEffect_RobotCompanionCooldown::UShootEffect_RobotCompanionCooldown(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureCooldown(ObjectInitializer, TEXT("Cooldown.Skill.RobotCompanion"), 20.f);
}
