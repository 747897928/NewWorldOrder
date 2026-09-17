/**
 * 男主 C：钢铁壁垒（SteelBulwark）
 * - 设计：角色前方生成盾墙，阻挡/减伤正面攻击，可为队伍提供护盾或 DR，可手动取消或持续结束。
 * - 当前实现：生成盾墙 Actor，提供护盾/减伤占位。
 * - 缺失：朝向/覆盖角度阻挡、反射/破碎爆炸、手动取消与 FX Cue。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Male_SteelBulwark.generated.h"

class AShootSkillShieldWall;

UCLASS()
class NEWWORLDORDER_API UShootGA_Male_SteelBulwark : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Male_SteelBulwark();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/* 可覆盖资产：
	 *   ShieldWallClass：默认 AShootSkillShieldWall（加减伤/护盾）
	 */
	UPROPERTY(EditDefaultsOnly, Category="SteelBulwark")
	TSubclassOf<AShootSkillShieldWall> ShieldWallClass;

	UPROPERTY(EditDefaultsOnly, Category="SteelBulwark")
	float ShieldDuration;

	UPROPERTY(EditDefaultsOnly, Category="SteelBulwark")
	float ShieldBonus;

	UPROPERTY(EditDefaultsOnly, Category="SteelBulwark")
	float DamageReduction;
};
