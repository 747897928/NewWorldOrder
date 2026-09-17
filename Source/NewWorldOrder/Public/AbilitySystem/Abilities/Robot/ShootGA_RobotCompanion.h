// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_RobotCompanion.generated.h"

class AShootRobotCompanionCharacter;
class UGameplayEffect;

/** 玩家四槽中的召唤/指令能力；蓝图子类负责选择机器人外观类和可调参数。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootGA_RobotCompanion : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_RobotCompanion();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot")
	TSubclassOf<AShootRobotCompanionCharacter> RobotCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot")
	TSubclassOf<UGameplayEffect> DestroyedCooldownEffectClass;

	/** X=前后、Y=左右、Z=上下，以 Owner Pawn 局部坐标换算。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Spawn")
	FVector SpawnOffset = FVector(-120.f, 160.f, 10.f);

	/** 首次召唤在目标点上方生成并自然落地；0 表示关闭下落表现。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Spawn", meta=(ClampMin="0.0", Units="cm"))
	float SummonDropHeight = 500.f;

	/** 有限存续结束后自爆并进入重召冷却；模式切换期间不占用另一套输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Lifetime", meta=(ClampMin="1.0", Units="s"))
	float CompanionLifetime = 30.f;

	/** 每级增加少量在场时间；默认 30/35/40 秒，避免升级只变成不可感知的后台数值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Lifetime", meta=(ClampMin="0.0", Units="s"))
	float CompanionLifetimePerLevel = 5.f;

	/** 默认关闭：Owner 死亡是所有权清理，不等价于机器人被敌人击毁。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Cooldown")
	bool bOwnerDeathStartsCooldown = false;
};
