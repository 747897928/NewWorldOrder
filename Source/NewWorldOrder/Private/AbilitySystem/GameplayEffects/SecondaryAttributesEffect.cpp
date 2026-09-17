// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/SecondaryAttributesEffect.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/ModMagCal/MMC_MaxHealth.h"
#include "AbilitySystem/ModMagCal/MMC_Armor.h"
#include "AbilitySystem/ModMagCal/MMC_ArmorPenetration.h"
#include "AbilitySystem/ModMagCal/MMC_CriticalHitChance.h"
#include "AbilitySystem/ModMagCal/MMC_CriticalHitDamage.h"
#include "AbilitySystem/ModMagCal/MMC_ShieldCapacity.h"
#include "AbilitySystem/ModMagCal/MMC_DamageReduction.h"

USecondaryAttributesEffect::USecondaryAttributesEffect()
{
	// 设置持续策略为无限，因为次级属性应该持续存在
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 创建并配置 MaxHealth 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_MaxHealth::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetMaxHealthAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 Armor 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_Armor::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetArmorAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 ArmorPenetration 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_ArmorPenetration::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetArmorPenetrationAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 CriticalHitChance 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_CriticalHitChance::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetCriticalHitChanceAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 CriticalHitDamage 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_CriticalHitDamage::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetCriticalHitDamageAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 ShieldCapacity 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_ShieldCapacity::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetShieldCapacityAttribute();

		Modifiers.Add(ModifierInfo);
	}

	// 创建并配置 DamageReduction 修改器
	{
		FGameplayModifierInfo ModifierInfo;
		FCustomCalculationBasedFloat CustomCalc;
		CustomCalc.CalculationClassMagnitude = UMMC_DamageReduction::StaticClass();
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
		ModifierInfo.ModifierOp = EGameplayModOp::Override;
		ModifierInfo.Attribute = UShootAttributeSet::GetDamageReductionAttribute();

		Modifiers.Add(ModifierInfo);
	}
}
