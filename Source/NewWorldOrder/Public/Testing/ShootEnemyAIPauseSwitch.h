// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "ShootEnemyAIPauseSwitch.generated.h"

class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UUserWidget;

/**
 * TestMap 专用的服务器权威 AI 暂停开关。
 *
 * 它只调用 AShootEnemyTestSpawner 已有的 Behavior Tree 暂停/恢复入口，不参与寻敌、
 * 移动或伤害决策。正式 Experience 不放置该 Actor，避免把验收开关暴露给发售玩家。
 */
UCLASS()
class NEWWORLDORDER_API AShootEnemyAIPauseSwitch : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootEnemyAIPauseSwitch();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CanToggleForPawn(const APawn* Pawn) const;

	/** 只由交互 GA 在服务器调用；遍历地图中的 Spawner 并统一切换它们的 BT。 */
	void ToggleEnemyBehavior();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Test AI")
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Test AI")
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test AI|Interaction")
	FText PauseInteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test AI|Interaction")
	FText ResumeInteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test AI|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test AI|Interaction")
	TSubclassOf<UUserWidget> InteractionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test AI|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Test AI")
	bool bEnemyBehaviorEnabled = true;
};
