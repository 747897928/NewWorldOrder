// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootSkillGrantSource.h"
#include "TimerManager.h"

#include "ShootRandomSkillPickup.generated.h"

class APawn;
class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UShootSkillDefinition;

/**
 * 可复用的随机技能世界来源。它只决定本次提供“随机可用技能”；真正的槽位事务由 PlayerState 的技能组件完成。
 * 未来商人不继承本 Actor，而是实现同一个 IShootSkillGrantSource，以便复用交互 GA 和服务器校验。
 */
UCLASS()
class NEWWORLDORDER_API AShootRandomSkillPickup : public AActor, public IInteractableTarget,
	public IShootSkillGrantSource
{
	GENERATED_BODY()

public:
	AShootRandomSkillPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
		FGameplayEventData& InOutEventData) override;

	virtual bool CanGrantSkillToPawn(const APawn* Pawn) const override;
	virtual bool TryGrantSkillToPawn(APawn* Pawn) override;

	/** 旧调用点兼容；新代码统一使用 IShootSkillGrantSource。 */
	bool CanUsePawn(const APawn* Pawn) const { return CanGrantSkillToPawn(Pawn); }
	void ConsumeByPawn(const APawn* Pawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Available();

	void Respawn();
	void ApplyAvailabilityState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skill Offer|Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionCollision;

	/** 网格、材质和特效由蓝图子类配置，C++ 不引用具体 /Game/ 资产。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skill Offer|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Interaction")
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Interaction", meta=(ClampMin="0.0", ForceUnits="s"))
	float HoldDuration = 0.f;

	/** 服务器提交时的最终距离校验；扫描射线仍负责视线与聚焦，不能再要求 Pawn 必须物理重叠球体。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Interaction", meta=(ClampMin="0.0", ForceUnits="cm"))
	float MaxInteractionDistance = 500.f;

	/** 0 表示成功后销毁；大于 0 时隐藏并在服务器计时重生。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Respawn", meta=(ClampMin="0.0", ForceUnits="s"))
	float RespawnDelay = 5.f;

	UPROPERTY(ReplicatedUsing=OnRep_Available, BlueprintReadOnly, Category="Skill Offer|Respawn")
	bool bAvailable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	/**
	 * 为空时从 Experience 池随机获取；指定后提供固定技能。
	 * 商人商品、三选一和测试地图机器人入口因此共用同一 AcquireSkill 权威事务，不需要调试注入旁路。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Offer")
	TObjectPtr<const UShootSkillDefinition> OfferedSkillDefinition;

	FTimerHandle RespawnTimerHandle;
};
