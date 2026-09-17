// 通用护盾墙 Actor：为拥有者提供正面减伤/护盾加成，可蓝图扩展反伤/破碎爆炸
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// 需要完整类型：FActiveGameplayEffectHandle 以值存储，前置声明不够
#include "GameplayEffectTypes.h"
#include "ShootSkillShieldWall.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API AShootSkillShieldWall : public AActor
{
	GENERATED_BODY()

public:
	AShootSkillShieldWall();

	// 初始化：设定拥有者 ASC，用于应用护盾 GE
	void InitShield(AActor* InOwnerActor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 蓝图表现钩子
	UFUNCTION(BlueprintImplementableEvent, Category="ShieldWall")
	void OnShieldActivated();
	UFUNCTION(BlueprintImplementableEvent, Category="ShieldWall")
	void OnShieldBroken();

	void ApplyShield();
	void RemoveShield();

	UPROPERTY(EditDefaultsOnly, Category="ShieldWall")
	float Duration;

	UPROPERTY(EditDefaultsOnly, Category="ShieldWall")
	float DamageReduction; // 额外减伤（加到 DamageReductionBonus）

	UPROPERTY(EditDefaultsOnly, Category="ShieldWall")
	float ShieldBonus; // 额外护盾容量（加法）

	UPROPERTY(EditDefaultsOnly, Category="ShieldWall")
	TSubclassOf<UGameplayEffect> ShieldEffectClass; // 可覆盖，若为空使用默认 GE

	UPROPERTY(EditDefaultsOnly, Category="ShieldWall")
	TSubclassOf<UGameplayEffect> BreakExplosionEffectClass; // 可选，破碎时应用

	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerActor;

	UPROPERTY()
	TWeakObjectPtr<class UAbilitySystemComponent> OwnerASC;

	FActiveGameplayEffectHandle ActiveShieldHandle;
};
