// 男主被动：装甲强化长期 Buff（ShieldCapacityBonus/DamageReductionBonus 通过 SetByCaller 配置）
#include "AbilitySystem/Effects/ShootEffect_Passive_ArmorEnhancement.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_Passive_ArmorEnhancement::UShootEffect_Passive_ArmorEnhancement()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.0f)); // 默认永久

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetShieldCapacityBonusAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		// GameplayEffect CDO 可能早于 AssetManager 的原生 Tag 初始化，DataTag 会被永久固化为 None。
		SetByCaller.DataName = FName("SetByCaller.ShieldCapacityBonus");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetDamageReductionBonusAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.DamageReductionBonus");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}
}
