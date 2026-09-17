// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "ShootAmmoSupplyStation.generated.h"

class UGameplayAbility;
class ULocalPlayer;
class USphereComponent;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 * 地图常驻弹药补给站。
 *
 * 目标只提供交互选项和距离校验；3 秒按住/松开由常驻 UShootGA_Interact 处理，
 * 真正补弹由动态授予的 UShootGA_Interaction_RefillAmmo 在服务器执行。
 */
UCLASS()
class NEWWORLDORDER_API AShootAmmoSupplyStation : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootAmmoSupplyStation();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;

	/** 服务器执行能力在结算前再次校验玩家和距离，不能只相信客户端完成读条。 */
	bool CanSupplyPawn(const APawn* Pawn) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetLocalPromptVisible(const APawn* Pawn, bool bVisible);
	UWidgetComponent* FindOrCreateLocalPrompt(const APawn* Pawn);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Supply", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Supply", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Supply|UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> InteractionPromptComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Supply|Interaction",
		meta=(AllowPrivateAccess="true", ClampMin="0.1", ForceUnits="s"))
	float HoldDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Supply|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Supply|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Supply|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Supply|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	/**
	 * Screen Space WidgetComponent 必须绑定到具体 LocalPlayer；共享一个组件会让分屏玩家互相看见提示。
	 * InteractionPromptComponent 只作为蓝图可调的样式/位置模板，运行时为每个本地玩家创建独立实例。
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<ULocalPlayer>, TObjectPtr<UWidgetComponent>> LocalPromptComponents;
};
