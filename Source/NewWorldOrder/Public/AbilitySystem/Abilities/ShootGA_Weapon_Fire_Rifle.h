// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ShootGameplayAbility_Weapon_Fire.h"
#include "ShootGA_Weapon_Fire_Rifle.generated.h"

class UAnimMontage;
class UAbilityTask_WaitInputRelease;

/**
 * UShootGameplayAbility_Weapon_Fire_Rifle
 * 
 * 步枪射击能力的完整实现（参考 Lyra 的 GA_Weapon_Fire_Rifle 蓝图）
 * 
 * 流程：
 */
UCLASS()
class UShootGA_Weapon_Fire_Rifle : public UShootGameplayAbility_Weapon_Fire
{
	GENERATED_BODY()

public:
	UShootGA_Weapon_Fire_Rifle(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	/**
	 * 当武器目标数据准备好时（重写父类的蓝图事件为 C++ 实现）
	 * 
	 * 在这里做：
	 * 1. 应用伤害 GE 到所有命中目标
	 * 2. 触发 GameplayCue.Weapon.Fire（枪口火光、音效、弹壳等）
	 * 3. 为每个命中目标触发 GameplayCue.Impact（血花、弹孔等）
	 */
	virtual void OnRangedWeaponTargetDataReady_Implementation(const FGameplayAbilityTargetDataHandle& TargetData);

	/**
	 * 播放枪口特效
	 * 
	 * GameplayCue 会自动处理：
	 * - 枪口火光（粒子系统）
	 * - 开火音效（在所有客户端播放）
	 * - 弹壳抛出（只在本地播放，通过 IsLocallyControlled 判断）
	 * - 相机抖动（只在本地）
	 */
	void PlayFireEffects(const FGameplayAbilityTargetDataHandle& TargetData);

	/**
	 * 处理所有命中：播放特效 + 应用伤害
	 * 
	 * 一次循环解决所有事情：
	 * - 播放命中特效（所有客户端）
	 * - 生成命中 Actor（服务器，可选）
	 * - 应用伤害 GE（服务器）
	 */
	void ProcessHits(const FGameplayAbilityTargetDataHandle& TargetData);

	/** 播放一次当前角色骨架对应的开火 Montage；武器 Montage 继续由 AnimNotify 同步。 */
	void PlayFireMontage();

	/** 只在本地拥有者执行一次预测射击；远端服务器由重复 TargetData 驱动权威结算。 */
	void ExecuteLocallyControlledShot();

	/** 全自动 Rifle 的下一发定时回调；每次都复查 AbilitySpec.InputPressed，松开后绝不补发。 */
	void HandleAutomaticFireTick();

	UFUNCTION()
	void HandleAutomaticFireReleased(float TimeHeld);

	bool IsFireInputPressed() const;

public:
	// ========== 配置属性 ==========

	/** 角色开火动画蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	UAnimMontage* CharacterFireMontage;

	UPROPERTY(EditAnywhere, Category="Weapon|Fire")
	float AutoRate = 1.0f;

	UPROPERTY(EditAnywhere, Category="Weapon|Fire")
	float FireDelayTimeSecs = 0.12f;

	/** 命中时生成的 Actor 类（可选） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire")
	TSubclassOf<AActor> FieldActorToSpawnOnImpact;

	/** 
	 * 枪口特效的 GameplayCue Tag（例如 GameplayCue.Weapon.Rifle.Fire）
	 * 
	 * 这个 Cue 应该处理：
	 * - 枪口火光
	 * - 开火音效
	 * - 弹壳抛出
	 * - 镜头抖动
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Effects")
	FGameplayTag FireGameplayCueTag;

	/**
	 * 命中特效的 GameplayCue Tag（例如 GameplayCue.Weapon.Rifle.Impact）
	 * 
	 * 这个 Cue 应该处理：
	 * - 血花特效（命中角色）
	 * - 弹孔特效（命中墙壁）
	 * - 命中音效
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Effects")
	FGameplayTag ImpactGameplayCueTag;

private:
	/** PlayMontageAndWait 任务的句柄（用于取消） */
	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> InputReleaseTask;

	FTimerHandle AutomaticFireTimerHandle;
};
