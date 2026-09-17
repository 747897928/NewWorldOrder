// 智能辅助：长期小幅增益（默认换弹速度 *1.05），无人机/AI 行为留到后续 Phase 实现
#include "AbilitySystem/Effects/ShootEffect_Passive_SmartAssist.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_Passive_SmartAssist::UShootEffect_Passive_SmartAssist()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetReloadSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.05f));
	Modifiers.Add(Mod);
}

