// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "ShootMapTravelPortal.generated.h"

class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UWorld;

/**
 * 玩家交互式地图入口。
 *
 * Standalone 使用 OpenLevel；Listen Server 只允许权威端 ServerTravel，使远端客户端和本地分屏
 * 一起进入目标 World。返回 Hub 时可先清理所有 PlayerController 的 RuntimeOnly QuickBar 会话。
 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API AShootMapTravelPortal : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootMapTravelPortal();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;

	/** 由 UShootGA_Interaction_Travel 在服务器调用。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Portal")
	bool HandleTravel(APawn* InstigatorPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftObjectPtr<UWorld> DestinationMap;

	/** 测试副本返回 HomeMap 时开启；进入 TestMap_SplitScreen 的 Portal 保持 false。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	bool bClearAllRuntimeSessionsBeforeTravel = false;

	/**
	 * 在线副本返回 HomeMap 时开启。Listen Server 由 ShootSessionCoordinatorSubsystem 销毁房间，
	 * 让房主与远端客户端各自走统一返回链；Standalone 不受影响，仍直接 OpenLevel。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	bool bEndOnlineSessionBeforeTravel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal|Interaction")
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::PressToInteract;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal|Interaction")
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal|Interaction")
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

private:
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	void ClearAllRuntimeSessions();
};
