// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/ShootInteractionTypes.h"
#include "ShootReviveInteractableComponent.generated.h"

class UGameplayAbility;

/**
 * 救援交互组件：挂在倒地角色上，提供按键交互选项
 * - 默认使用 UShootGA_Interaction_Revive
 * - 仅玩家可触发（可配置）
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootReviveInteractableComponent : public UActorComponent, public IInteractableTarget
{
	GENERATED_BODY()

public:
	UShootReviveInteractableComponent();

	// IInteractableTarget
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	                                            FGameplayEventData& InOutEventData) override;

	// 是否允许救援交互（用于倒地/复活状态切换）
	UFUNCTION(BlueprintCallable, Category="Interaction|Revive")
	void SetReviveEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Interaction|Revive")
	bool IsReviveEnabled() const { return bReviveEnabled; }

protected:
	// 交互能力（默认 Revive）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	EShootInteractionTriggerMode TriggerMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	EShootInteractionUserFilter UserFilter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	FText InteractionSubText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Revive")
	bool bReviveEnabled;

private:
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;
};
