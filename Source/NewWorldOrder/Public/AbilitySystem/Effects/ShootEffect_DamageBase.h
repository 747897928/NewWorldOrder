// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ShootEffect_DamageBase.generated.h"

/**
 * 每把枪独立 Damage GE 的项目基类。
 *
 * GE 只保存策划可调的基础伤害并选择统一 Execution；命中、距离和物理材质仍来自
 * Fire GA 写入的 EffectContext 与 SourceObject WeaponInstance。这里不读取存档属性，
 * 也不复制 Lyra 当前没有初始化值的 CombatSet.BaseDamage 捕获链。
 */
UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API UShootEffect_DamageBase : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_DamageBase();

	float GetBaseDamage() const { return BaseDamage; }
	float GetMarkedWeakSpotMultiplier() const { return MarkedWeakSpotMultiplier; }

protected:
	/** 由每把枪的 GE 蓝图分别配置；Execution 以该值作为伤害公式起点。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage", meta=(ClampMin="0.0"))
	float BaseDamage = 10.0f;

	/**
	 * 命中 Gameplay_Zone.WeakSpot 且目标带 Status.Marked 时的可选倍率。
	 * 默认 1 表示不启用；特殊武器应在自己的 Damage GE 中配置，不在 GA 里手算。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage", meta=(ClampMin="1.0"))
	float MarkedWeakSpotMultiplier = 1.0f;
};
