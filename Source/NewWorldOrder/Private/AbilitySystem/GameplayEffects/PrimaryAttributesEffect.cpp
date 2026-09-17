// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/PrimaryAttributesEffect.h"
#include "AbilitySystem/ShootAttributeSet.h"

UPrimaryAttributesEffect::UPrimaryAttributesEffect()
{
	// 设置持续策略为瞬时，主属性更改立即生效
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 创建并配置 Strength 修改器
	// 使用 Additive 操作，允许叠加多个效果
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.ModifierMagnitude = FScalableFloat(0.0f); // 默认为0，可在Blueprint中配置
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.Attribute = UShootAttributeSet::GetStrengthAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 Vitality 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.ModifierMagnitude = FScalableFloat(0.0f);
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.Attribute = UShootAttributeSet::GetVitalityAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 Agility 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.ModifierMagnitude = FScalableFloat(0.0f);
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.Attribute = UShootAttributeSet::GetAgilityAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 Perception 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.ModifierMagnitude = FScalableFloat(0.0f);
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.Attribute = UShootAttributeSet::GetPerceptionAttribute();

		Modifiers.Add(ModifierInfo);
	}
}
