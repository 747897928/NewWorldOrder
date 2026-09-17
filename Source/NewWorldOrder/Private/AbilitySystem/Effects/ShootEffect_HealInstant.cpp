// 通用即时治疗：Health += SetByCaller.Heal
#include "AbilitySystem/Effects/ShootEffect_HealInstant.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_HealInstant::UShootEffect_HealInstant()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;

	// CDO 构造早于 AssetManager 原生 Tag 初始化时，RequestGameplayTag 会把 DataTag 固化为 None。
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("SetByCaller.Heal");
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Mod);
}
