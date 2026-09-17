// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ShootGameplayAbility.generated.h"

class UShootAbilityCost;
class UGameplayEffect;
class AShootCharacter;
class AShootGameModeBase;

/**
 * 与 Lyra 一致的能力激活策略。
 * OnInputTriggered 响应一次 Started，WhileInputActive 在输入保持期间尝试激活，
 * OnSpawn 在能力授予或 ASC 重新绑定 Avatar 后自动激活，适合 Interact 这类常驻系统能力。
 */
UENUM(BlueprintType)
enum class EShootAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive,
	OnSpawn
};

/**
 * 
 */
UCLASS()
class UShootGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	EShootAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	/** 由 OnGiveAbility 与 UShootAbilitySystemComponent::AbilityActorInfoSet 共同调用，覆盖先授予后绑定 Avatar 的顺序。 */
	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
	                               const FGameplayAbilitySpec& Spec) const;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilitySpec& Spec) override;

	// Additional costs that must be paid to activate this ability
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Shoot|Costs")
	TArray<TObjectPtr<UShootAbilityCost>> AdditionalCosts;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Input")
	/**
	 * 固定输入能力可在此配置；Match Skill 必须保持为空，由 UShootSkillLoadoutComponent
	 * 通过 UShootAbilitySet::GiveToAbilitySystem 的 InputTagOverride 按当前槽位授予。
	 */
	FGameplayTag StartupInputTag;

	UFUNCTION(BlueprintCallable, Category = "Shoot|Ability")
	AShootCharacter* GetShootCharacterFromActorInfo() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shoot|Ability Activation")
	EShootAbilityActivationPolicy ActivationPolicy = EShootAbilityActivationPolicy::OnInputTriggered;

	// 通用：对自身应用 GE，支持可选 Duration 覆盖
	void ApplyEffectToOwner(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.f,
	                        float DurationOverride = -1.f) const;
	// 获取 GameMode（便于绑定全局事件）
	AShootGameModeBase* GetShootGameMode() const;

protected:
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                       FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                       const FGameplayAbilityActivationInfo ActivationInfo) const override;

	//float GetManaCost(float InLevel = 1.f) const;
	float GetCooldown(float InLevel = 1.f) const;
};
