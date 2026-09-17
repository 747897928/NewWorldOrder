/**
 * 男主 E：震撼手雷（ShockGrenade）
 * - 设计：投掷手雷 → 落地爆炸，对范围敌人造成伤害+眩晕/击退并施加易伤；支持抛物线落点与配置化引信/半径。
 * - 当前实现：直接生成爆炸 Actor，具备伤害/控制/易伤占位。
 * - 缺失：投掷轨迹/落点控制、命中规则（每目标控制）、与 FX/动画的 Cue 绑定。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Male_ShockGrenade.generated.h"

class AShootSkillExplosionActor;

UCLASS()
class NEWWORLDORDER_API UShootGA_Male_ShockGrenade : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Male_ShockGrenade();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/* 可覆盖资产：
	 *   ExplosionClass：默认 AShootSkillExplosionActor
	 */
	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	TSubclassOf<AShootSkillExplosionActor> ExplosionClass;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float Damage;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float Radius;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float FuseTime;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float VulnerableDuration;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float StunDuration;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	float KnockbackStrength;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	TSubclassOf<UGameplayEffect> VulnerableEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="ShockGrenade")
	TSubclassOf<UGameplayEffect> ControlEffectClass;
};
