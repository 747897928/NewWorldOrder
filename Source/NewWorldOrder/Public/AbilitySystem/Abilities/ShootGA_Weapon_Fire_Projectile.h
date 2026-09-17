// Copyright ZhaoYiJie

// AbilitySystem/Abilities/ShootGA_Weapon_Fire_Projectile.h
#pragma once
#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.h"
#include "ShootGA_Weapon_Fire_Projectile.generated.h"

class UShootRangedWeaponInstance;
class AShootProjectileBase;

/**
 * 通用投射物开火 GA（榴弹 & 火箭筒复用）
 */
UCLASS()
class UShootGA_Weapon_Fire_Projectile : public UShootGameplayAbility_Weapon_Fire
{
	GENERATED_BODY()
public:
	UShootGA_Weapon_Fire_Projectile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * 父类完成 Lyra 风格的本地预测、服务器重 Trace、Commit 与 TargetData 清理；
	 * 特殊武器只把第一条权威命中转换为枪口到准星目标点的实体投射方向。
	 */
	virtual void OnRangedWeaponTargetDataReady_Implementation(
		const FGameplayAbilityTargetDataHandle& TargetData) override;
	virtual bool ShouldUseClientTargetDataOnServer() const override { return true; }
	bool ResolveFireDirectionFromTargetData(
		const FGameplayAbilityTargetDataHandle& TargetData, FVector& OutFireDirection) const;
	void ExecuteCommittedFire(const FVector& FireDirection);

	/** 服务器从权威枪口沿已校验方向生成投射物。 */
	virtual void SpawnProjectile(const FVector& FireDirection);
	bool HasValidProjectileConfig() const;

	FVector GetMuzzleLocation() const;

public:
	UPROPERTY(EditDefaultsOnly, Category="Effects") FGameplayTag FireGameplayCueTag;
	UPROPERTY(EditAnywhere, Category="Weapon|Fire") float EndAbilityDelaySecs = 0.1f;

private:
	UPROPERTY() class UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;
};
