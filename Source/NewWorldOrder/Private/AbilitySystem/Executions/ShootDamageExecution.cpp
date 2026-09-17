// Copyright ZhaoYiJie

#include "AbilitySystem/Executions/ShootDamageExecution.h"

#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"
#include "AbilitySystem/ShootGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "GameplayEffect.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "ShootGameplayTags.h"
#include "Weapons/ShootRangedWeaponInstance.h"

void UShootDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const UShootEffect_DamageBase* DamageEffect = Cast<UShootEffect_DamageBase>(Spec.Def);
	const UShootRangedWeaponInstance* Weapon = Cast<UShootRangedWeaponInstance>(Spec.GetContext().GetSourceObject());
	const FHitResult* HitResult = Spec.GetContext().GetHitResult();
	const FShootGameplayEffectContext* ShootContext = FShootGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	if (!DamageEffect || !HitResult)
	{
		// 正式伤害 GE 必须带 HitResult；缺少命中上下文时失败关闭。
		return;
	}

	const bool bIsRadialDamage = ShootContext && ShootContext->HasRadialDamage();
	if (!Weapon && !bIsRadialDamage)
	{
		// 手持手雷可以没有 WeaponInstance，但普通枪械必须由 B_WeaponInstance 提供来源。
		return;
	}

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float FinalDamage = DamageEffect->GetBaseDamage();
	const FVector ImpactLocation = HitResult->ImpactPoint.IsNearlyZero()
		? HitResult->TraceEnd
		: HitResult->ImpactPoint;
	// 径向上下文中 TraceStart 由 AShootProjectileBase::ApplyRadialDamage 写成爆心，ImpactPoint 写成目标位置；
	// 因而这里的 Distance 是“爆心 -> 目标”，不是“枪口 -> 爆心”的投射物飞行距离。
	const float Distance = FVector::Dist(HitResult->TraceStart, ImpactLocation);

	if (bIsRadialDamage)
	{
		const float InnerRadius = FMath::Max(0.0f, ShootContext->GetDamageInnerRadius());
		const float OuterRadius = FMath::Max(InnerRadius, ShootContext->GetDamageOuterRadius());
		if (OuterRadius > 0.0f)
		{
			const float DistanceAlpha = OuterRadius > InnerRadius
				? FMath::Clamp((Distance - InnerRadius) / (OuterRadius - InnerRadius), 0.0f, 1.0f)
				: (Distance <= InnerRadius ? 0.0f : 1.0f);
			const float Falloff = FMath::Clamp(ShootContext->GetDamageFalloff(), 0.0f, 1.0f);
			FinalDamage *= FMath::Lerp(1.0f, 1.0f - Falloff, DistanceAlpha);
		}
	}
	else if (Weapon)
	{
		// 只有 hitscan 才消费 WeaponInstance 的距离曲线。Rocket/Grenade 已在上面的径向分支完成
		// 爆炸半径衰减，绝不能再把飞行距离曲线乘一次形成双重衰减。
		FinalDamage *= Weapon->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
	}

	const bool bApplyMaterialMultipliers = !bIsRadialDamage
		|| (ShootContext && ShootContext->ShouldApplyMaterialMultipliers());
	if (Weapon && bApplyMaterialMultipliers && HitResult->PhysMaterial.IsValid())
	{
		FinalDamage *= Weapon->GetPhysicalMaterialAttenuation(
			HitResult->PhysMaterial.Get(), SourceTags, TargetTags);
	}

	const bool bApplyMarkedWeakSpotMultiplier = !bIsRadialDamage
		|| (ShootContext && ShootContext->ShouldApplyMaterialMultipliers());
	if (bApplyMarkedWeakSpotMultiplier && DamageEffect->GetMarkedWeakSpotMultiplier() > 1.0f &&
		ExecutionParams.GetTargetAbilitySystemComponent() &&
		ExecutionParams.GetTargetAbilitySystemComponent()->HasMatchingGameplayTag(FShootGameplayTags::Get().Status_Marked))
	{
		if (const UPhysicalMaterialWithTags* PhysMat = Cast<UPhysicalMaterialWithTags>(HitResult->PhysMaterial.Get());
			PhysMat && PhysMat->Tags.HasTag(FShootGameplayTags::Get().Gameplay_Zone_WeakSpot))
		{
			FinalDamage *= DamageEffect->GetMarkedWeakSpotMultiplier();
		}
	}

	if (FinalDamage > KINDA_SMALL_NUMBER)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UShootAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
	}
}
