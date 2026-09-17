// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "System/ShootSaveGame.h"
#include "ShootAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, const FGameplayTagContainer& /*AssetTags*/);
DECLARE_MULTICAST_DELEGATE(FAbilitiesGiven);
DECLARE_MULTICAST_DELEGATE_TwoParams(FActivatePassiveEffect, const FGameplayTag& /*AbilityTag*/, bool /*bActivate*/);
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UShootAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityActorInfoSet();
	/** PlayerState ASC 换绑新 Pawn 后重试 OnSpawn 能力；否则角色重生或切换后常驻能力可能没有激活。 */
	void TryActivateAbilitiesOnSpawn();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastActivatePassiveEffect(const FGameplayTag& AbilityTag, bool bActivate);



	void UpgradeAttribute(FGameplayTag AttributeTag);

	UFUNCTION(Server, Reliable)
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	FEffectAssetTags EffectAssetTags;
	FAbilitiesGiven AbilitiesGivenDelegate;
	FActivatePassiveEffect ActivatePassiveEffect;

	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);
	void AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities);

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagHeld(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/** 移除带指定 Tag 的所有能力（用于性别切换时清理另一套技能） */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	void RemoveAbilitiesWithTag(FGameplayTag AbilityTag);

	/**
	 * 旧蓝图兼容入口。当前 Experience/Pawn/装备主线禁止调用；正式授予必须由 AbilitySet 独立持有撤销句柄。
	 * 确认所有历史蓝图均无调用后删除。
	 */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	void GrantAbilitiesWithKit(const TArray<TSubclassOf<UGameplayAbility>>& Abilities, const FGameplayTag& KitTag, bool bActivatePassives=false);

	/** Gets the ability target data associated with the given ability handle and activation info */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);

	virtual FGameplayEffectContextHandle MakeEffectContext() const override;

protected:
	UFUNCTION(Client, Reliable)
	void ClientEffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec,
	                         FActiveGameplayEffectHandle ActiveEffectHandle);

	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
};
