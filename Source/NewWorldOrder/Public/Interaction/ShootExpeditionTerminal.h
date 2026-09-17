// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"

#include "ShootExpeditionTerminal.generated.h"

class UCommonActivatableWidget;
class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UShootLocalInteractionPrompt;

/** HomeMap 的副本入口。交互只为触发该 LocalPlayer 的 CommonUI 页面，不在服务器直接 Travel。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API AShootExpeditionTerminal : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootExpeditionTerminal();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;

	/** 由本地交互 GA 调用；显式使用 Pawn 的 LocalPlayer，兼容本地分屏。 */
	bool OpenExpeditionScreenForPawn(APawn* InstigatorPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expedition Terminal")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expedition Terminal")
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	/** 样式由 BP 配置，运行时按 LocalPlayer 生成近距离提示实例。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expedition Terminal|UI")
	TObjectPtr<UShootLocalInteractionPrompt> InteractionPrompt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition Terminal|UI")
	TSoftClassPtr<UCommonActivatableWidget> ExpeditionScreenClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition Terminal|Interaction")
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition Terminal|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition Terminal|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;
};
