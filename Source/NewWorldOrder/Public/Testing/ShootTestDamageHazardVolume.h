// 仅用于 TestMap 的可见持续伤害区域：玩家踩入后自动挂周期 GE，离开后移除。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ShootTestDamageHazardVolume.generated.h"

class APawn;
class UAbilitySystemComponent;
class UBoxComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

/**
 * 玩家可直接踩踏的测试火焰区域。
 *
 * 这是回血包 PIE 验收夹具，不是正式关卡伤害系统：碰撞和 Niagara 都由地图
 * 中的蓝图实例配置；服务器只在 Pawn Overlap 时应用周期伤害 GE，并在 EndOverlap 移除。
 */
UCLASS()
class NEWWORLDORDER_API AShootTestDamageHazardVolume : public AActor
{
	GENERATED_BODY()

public:
	AShootTestDamageHazardVolume();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnHazardBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnHazardEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void ApplyDamageEffectToPawn(APawn* Pawn);
	void RemoveDamageEffectFromPawn(APawn* Pawn);
	void RemoveAllDamageEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hazard")
	TObjectPtr<UBoxComponent> DamageVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hazard|Visual")
	TObjectPtr<UNiagaraComponent> HazardVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hazard|Damage")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hazard|Damage", meta=(ClampMin="0.1"))
	float DamageGameplayEffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hazard|Rules")
	bool bOnlyPlayerControlled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hazard|Visual")
	TObjectPtr<UNiagaraSystem> HazardVFXSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hazard|Visual", meta=(ClampMin="0.1"))
	FVector HazardVFXScale = FVector(2.0f);

	// 记录每个 Pawn 当前在区域内的 Active GE；EndOverlap 只移除自己的伤害效果。
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle> ActiveDamageEffects;
};
