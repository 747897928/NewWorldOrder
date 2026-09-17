/**
 * 男主 Q：战术突击（TacticalAssault）
 * - 设计：短距冲刺+i-frame，命中施加冲撞伤害/硬直+易伤，结束后短时间移速/射速提升，可与战术超载叠加。
 * - 当前实现：LaunchCharacter 位移 + 移速/射速 Buff；服务器 Sweep 只为将来配置伤害 GE 预留。
 * - 当前冷却：15 秒，与原数值文档一致；释放时发送服务器权威 GameplayCue。
 * - 缺失：阵营与遮挡过滤、伤害/控场配置、i-frame 状态处理、等级成长和动画钩子。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Male_TacticalAssault.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Male_TacticalAssault : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Male_TacticalAssault();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/* 当前 C++ 路径：LaunchCharacter 位移 + 射速/移速 Buff；无敌帧尚未实现，满级撞击可扩展
	 * 可覆盖资产：
	 *   MoveSpeedBuffClass：默认使用 UShootEffect_MoveSpeed
	 *   FireRateBuffClass：默认使用 UShootEffect_FireRate
	 */
	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Buff")
	TSubclassOf<UGameplayEffect> MoveSpeedBuffClass;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Buff")
	TSubclassOf<UGameplayEffect> FireRateBuffClass;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Dash")
	float DashDistance;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Dash")
	float DashCapsuleRadius;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Dash")
	float DashCapsuleHalfHeight;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Effects")
	TSubclassOf<UGameplayEffect> DashDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Effects")
	TSubclassOf<UGameplayEffect> DashVulnerableEffect;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Dash")
	float DashInvulnerableTime;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Buff")
	float BuffDuration;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Impact")
	float ImpactDamage;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Impact")
	float ImpactStunDuration;

	UPROPERTY(EditDefaultsOnly, Category="TacticalAssault|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag ActivationGameplayCueTag;
};
