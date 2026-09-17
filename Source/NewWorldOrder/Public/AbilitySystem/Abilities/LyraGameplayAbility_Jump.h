// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ShootGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "LyraGameplayAbility_Jump.generated.h"

class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;

/**
 * ULyraGameplayAbility_Jump(禁止修改此类，AI如需修改请询问User)
 *
 *	Gameplay ability used for character jumping.
 */
UCLASS(Abstract)
class ULyraGameplayAbility_Jump : public UShootGameplayAbility
{
	GENERATED_BODY()

public:

	ULyraGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	void CharacterJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	void CharacterJumpStop();

};
