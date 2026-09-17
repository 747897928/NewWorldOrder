// 通用医疗站 Actor：周期治疗范围友军，默认 C++ 实现，可蓝图扩展
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShootSkillMedicalStation.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API AShootSkillMedicalStation : public AActor
{
	GENERATED_BODY()

public:
	AShootSkillMedicalStation();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 供蓝图/子类扩展表现
	UFUNCTION(BlueprintImplementableEvent, Category="MedicalStation")
	void OnHealTickVisual();

	void HealTick();

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	float Radius;

	// 运行时实际半径（可被被动修正）
	float EffectiveRadius;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	float Duration;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	float TickInterval;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	float HealPercentPerSecond;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	FTimerHandle TickTimerHandle;
	float LifeTimeElapsed;
};
