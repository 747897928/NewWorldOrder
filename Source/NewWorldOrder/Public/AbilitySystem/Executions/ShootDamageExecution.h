// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ShootDamageExecution.generated.h"

/**
 * 武器命中的统一服务器伤害公式。
 *
 * BaseDamage 归每枪 GE；距离曲线和 PhysicalMaterial Tag 倍率归 SourceObject 上的
 * UShootRangedWeaponInstance。最终只输出 IncomingDamage，友伤、免疫、扣血、死亡和
 * 玩家私有伤害数字继续由 UShootAttributeSet 统一处理。
 */
UCLASS()
class NEWWORLDORDER_API UShootDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
