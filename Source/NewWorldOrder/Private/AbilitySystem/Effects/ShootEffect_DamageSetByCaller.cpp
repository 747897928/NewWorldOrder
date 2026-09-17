#include "AbilitySystem/Effects/ShootEffect_DamageSetByCaller.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "GameplayTagContainer.h"

UShootEffect_DamageSetByCaller::UShootEffect_DamageSetByCaller()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UShootAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	// 原生 GE 的 CDO 可能早于 FShootGameplayTags 完成注册；构造阶段请求 GameplayTag 会得到空 Tag。
	// GAS 原生支持 FName SetByCaller，使用同名通道可彻底避开模块初始化顺序。
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("SetByCaller.Damage");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
