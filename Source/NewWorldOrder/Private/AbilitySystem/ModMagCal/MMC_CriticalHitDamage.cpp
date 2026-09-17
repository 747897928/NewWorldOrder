// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_CriticalHitDamage.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_CriticalHitDamage::UMMC_CriticalHitDamage()
{
	// 暴击伤害为固定值，不需要捕获属性
}

float UMMC_CriticalHitDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 文档v7.2：暴击倍率固定为1.5倍（150%）
	// 技能可能通过GE的Multiplier进一步修改（例如精准专家Lv3提升到180%）
	// 返回值150表示150%伤害
	//
	// 设计说明：
	// - 爆头和暴击是独立乘区，可以叠加
	// - 爆头暴击 = 爆头倍率(2.45) × 暴击倍率(1.5) = 3.675倍
	return 150.f;
}
