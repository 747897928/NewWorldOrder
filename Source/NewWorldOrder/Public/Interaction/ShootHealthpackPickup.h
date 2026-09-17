// 治疗包拾取物：走项目交互链（PressToInteract），交互时动态授予治疗包执行能力。
// 使用后在服务器应用即时治疗，默认隐藏并按 RespawnDelay 重生；RespawnDelay 为 0 时销毁，绝不进入 QuickBar / 库存 / SaveGame。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "TimerManager.h"
#include "ShootHealthpackPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGameplayAbility;

UCLASS()
class NEWWORLDORDER_API AShootHealthpackPickup : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootHealthpackPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 交互扫描（客户端）与能力结算（服务器）共用的可用性校验：玩家受控、在范围内、未满血。 */
	bool CanUsePawn(const APawn* Pawn) const;

	/** 服务器专用：治疗成功后处理视觉反馈，并按 RespawnDelay 消耗或暂时隐藏拾取物。 */
	void ConsumeByPawn(const APawn* Pawn);

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Available();

	void Respawn();
	void ApplyAvailabilityState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Healthpack|Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionCollision;

	/** 视觉网格由 BP_HealthpackPickup 引用 SM_healthpackFull；C++ 不含 /Game/ 资产路径。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Healthpack", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Healthpack|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Healthpack|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionSubText;

	/** 点按即用（HoldDuration=0）；保留为非零可改成长按交互。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Healthpack|Interaction", meta=(AllowPrivateAccess="true", ClampMin="0.0", ForceUnits="s"))
	float HoldDuration = 0.f;

	/** 成功使用后的重生秒数；默认 30 秒，设置为 0 表示一次性消耗并销毁。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Healthpack|Respawn", meta=(AllowPrivateAccess="true", ClampMin="0.0", ForceUnits="s"))
	float RespawnDelay = 30.f;

	/** 由服务器复制的可用状态；隐藏和碰撞由 OnRep_Available 在各端同步应用。 */
	UPROPERTY(ReplicatedUsing=OnRep_Available, BlueprintReadOnly, Category="Healthpack|Respawn", meta=(AllowPrivateAccess="true"))
	bool bAvailable = true;

	/** 由交互链动态授予的执行能力，蓝图可覆盖为子类以调整治疗量等。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Healthpack|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	FTimerHandle RespawnTimerHandle;
};
