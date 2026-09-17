// 通用爆炸 Actor（默认延时引爆 + SphereOverlap + 应用 GE），可被技能继承覆盖
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShootSkillExplosionActor.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API AShootSkillExplosionActor : public AActor
{
	GENERATED_BODY()

public:
	AShootSkillExplosionActor();

	// GA 侧可在 SpawnDeferred 后配置参数，FinishSpawning 前调用
	void ConfigureExplosion(float InDamage, float InRadius, float InFuseTime, float InVulnerableDuration, float InStunDuration,
		float InKnockbackStrength = 0.f,
		TSubclassOf<UGameplayEffect> InDamageEffect = nullptr,
		TSubclassOf<UGameplayEffect> InControlEffect = nullptr,
		TSubclassOf<UGameplayEffect> InVulnerableEffect = nullptr);

protected:
	virtual void BeginPlay() override;

	// 可覆盖的执行逻辑，蓝图也可重写
	virtual void Explode();

	// 触发引爆（定时或外部调用）
	void TriggerExplosion();

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float FuseTime;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float Radius;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float Damage;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float StunDuration;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float VulnerableDuration;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	float KnockbackStrength;

	// 可选覆盖的伤害 GE（默认使用内置即时 SetByCaller 伤害）
	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 可选覆盖的控制/易伤 GE
	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	TSubclassOf<UGameplayEffect> ControlEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Explosion")
	TSubclassOf<UGameplayEffect> VulnerableEffectClass;

	FTimerHandle FuseTimerHandle;
};
