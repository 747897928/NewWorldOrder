// Copyright ZhaoYiJie

#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"

#include "AbilitySystem/Executions/ShootDamageExecution.h"

UShootEffect_DamageBase::UShootEffect_DamageBase()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UShootDamageExecution::StaticClass();
	Executions.Add(ExecutionDefinition);
}
