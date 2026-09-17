/**
 * 男主被动：装甲强化（ArmorEnhancement）
 * - 设计：提供护盾/减伤，护盾被打破触发爆炸/击退并施加减速/易伤。
 * - 当前实现：施加护盾容量与减伤占位 GE。
 * - 缺失：护盾破碎检测与爆炸/控制效果。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "ShootGA_Passive_ArmorEnhancement.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Passive_ArmorEnhancement : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Passive_ArmorEnhancement();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 可覆盖装甲强化 Buff（推荐使用 ShootEffect_Passive_ArmorEnhancement，内部通过 SetByCaller 注入护盾/减伤）
	UPROPERTY(EditDefaultsOnly, Category="ArmorEnhancement")
	TSubclassOf<UGameplayEffect> ArmorEnhancementBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="ArmorEnhancement")
	float ShieldBonusValue;

	UPROPERTY(EditDefaultsOnly, Category="ArmorEnhancement")
	float DamageReductionValue;

	UPROPERTY(EditDefaultsOnly, Category="ArmorEnhancement")
	float BuffDuration;

	/** 被动自身施加的 GE 句柄；性别套件或 Experience 卸载时精确撤销。 */
	FActiveGameplayEffectHandle ArmorBuffHandle;
};
