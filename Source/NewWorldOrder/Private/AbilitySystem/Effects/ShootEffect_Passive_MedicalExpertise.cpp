// 医疗专精：长期提升治疗输出/受益（默认输出 *1.2，受益 *1.0，可在资产/子类覆盖）
#include "AbilitySystem/Effects/ShootEffect_Passive_MedicalExpertise.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_Passive_MedicalExpertise::UShootEffect_Passive_MedicalExpertise()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	{
		// 使用 SetByCaller 由 GA 注入治疗输出倍率，避免硬编码
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetHealingDoneMultiplierAttribute();
		Mod.ModifierOp = EGameplayModOp::Multiplicitive;
		FSetByCallerFloat SetByCaller;
		// CDO 构造时原生 Tag 可能尚未初始化，DataName 与调用方双写入口共同避免 Data None。
		SetByCaller.DataName = FName("SetByCaller.HealingDoneMultiplier");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}

	{
		// 使用 SetByCaller 由 GA 注入治疗受益倍率，便于按等级调整
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetHealingReceivedMultiplierAttribute();
		Mod.ModifierOp = EGameplayModOp::Multiplicitive;
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.HealingReceivedMultiplier");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}
}
