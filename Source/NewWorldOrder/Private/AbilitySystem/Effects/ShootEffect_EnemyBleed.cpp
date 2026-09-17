#include "AbilitySystem/Effects/ShootEffect_EnemyBleed.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_EnemyBleed::UShootEffect_EnemyBleed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
	Period = 1.0f;
	bExecutePeriodicEffectOnApplication = false;

	// 同一个 Bleeder 连续命中只刷新 3 秒窗口；不同来源仍可分别施加，便于多人/多怪测试。
	// UE 5.8 的 SetStackingType 未从 GameplayAbilities 模块导出，跨项目模块调用会链接失败；
	// 构造原生 GE 只能在限定范围内写入旧字段，后续若 Epic 导出 Setter 再移除该兼容段。
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UShootAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	// 与即时伤害 GE 相同，使用 FName 通道避开原生 GameplayTag 的 CDO 初始化顺序问题。
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("SetByCaller.Damage");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
