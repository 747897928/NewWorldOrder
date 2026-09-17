// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Interaction/Abilities/ShootGameplayAbility_Interact.h"
#include "Interaction/InteractionOption.h"
#include "ShootGA_Interact.generated.h"

class UAbilityTask_WaitForInteractableTargets_SingleLineTrace;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitInputPress;
class UAbilityTask_WaitInputRelease;

/**
 * C++ 版交互主能力（等价 Lyra GA_Interact 蓝图）
 * - 持续扫描 IInteractableTarget
 * - 维护当前聚焦交互选项
 * - 监听 InputTag.Ability.Interact
 * - 玩家按键时触发对应交互能力（例如 CollectPickup）
 * - 不再承载角色切换专属长按；角色切换入口如需独立确认，必须放在入口层自己实现
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interact : public UShootGameplayAbility_Interact
{
	GENERATED_BODY()

public:
	UShootGA_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	/** 使用单线扫描持续寻找可交互对象 */
	void LookForInteractables();

	/** 循环监听交互按键 */
	void StartInteractPressScan();

	/** 触发当前聚焦交互 */
	void TriggerCurrentInteraction();

	/** 当前选项要求长按时，锁定本次目标并启动完成/松开两条竞速任务。 */
	void StartInteractionHold(const FInteractionOption& TargetOption);

	/** 取消当前长按并恢复下一次 Press 监听。 */
	void CancelInteractionHold(bool bRestartPressScan);

	/** 向所属 LocalPlayer 广播 Lyra 兼容的交互时长消息；0 表示隐藏进度。 */
	void BroadcastInteractionDuration(float DurationSeconds) const;

	/** 触发指定交互选项。 */
	void TriggerInteractionOption(const FInteractionOption& TargetOption);

	/** 将当前选项同步给 UI（调用基类 UpdateInteractions） */
	void RefreshInteractionWidgets(const TArray<FInteractionOption>& NewOptions);

	/** 交互任务回调 */
	UFUNCTION()
	void HandleInteractableObjectsChanged(const TArray<FInteractionOption>& InteractableOptions);

	/** 输入任务回调 */
	UFUNCTION()
	void HandleInteractInputPressed(float TimeWaited);

	UFUNCTION()
	void HandleInteractInputReleased(float TimeHeld);

	UFUNCTION()
	void HandleInteractionHoldCompleted();

	bool HasValidFocusedOption() const;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitForInteractableTargets_SingleLineTrace> ActiveScanTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> ActiveInputTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> ActiveReleaseTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ActiveHoldDelayTask;

	UPROPERTY()
	FInteractionOption ActiveHoldOption;

	bool bInteractionHoldActive = false;

	/** 射线检测使用的碰撞配置 */
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	FCollisionProfileName InteractionTraceProfile = FCollisionProfileName(TEXT("Interactable_BlockDynamic"));

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	bool bDebugScanLines = false;
};
