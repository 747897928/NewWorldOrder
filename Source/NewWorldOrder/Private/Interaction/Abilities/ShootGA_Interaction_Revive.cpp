// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_Revive.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/LyraInteractionDurationMessage.h"
#include "ShootGameplayTags.h"

UShootGA_Interaction_Revive::UShootGA_Interaction_Revive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	BaseRescueDuration = 3.0f;
}

void UShootGA_Interaction_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CachedInstigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	CachedTarget = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;

	// EventMagnitude 作为耗时倍率（0.5 表示耗时减半）
	float Duration = BaseRescueDuration;
	if (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
	{
		Duration = BaseRescueDuration * TriggerEventData->EventMagnitude;
	}
	Duration = FMath::Max(0.1f, Duration);

	BroadcastRescueDurationMessage(Duration);

	RescueDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	if (RescueDelayTask)
	{
		RescueDelayTask->OnFinish.AddDynamic(this, &ThisClass::HandleRescueDelayFinished);
		RescueDelayTask->ReadyForActivation();
	}
	else
	{
		HandleRescueDelayFinished();
	}
}

void UShootGA_Interaction_Revive::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (RescueDelayTask)
	{
		RescueDelayTask->EndTask();
		RescueDelayTask = nullptr;
	}

	// 交互结束时发送 Duration=0 通知 UI 关闭进度条
	BroadcastRescueDurationMessage(0.f);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Interaction_Revive::HandleRescueDelayFinished()
{
	const AActor* InstigatorActor = CachedInstigator.Get();
	const AActor* TargetActor = CachedTarget.Get();

	if (InstigatorActor)
	{
		FGameplayEventData Payload;
		Payload.EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.Rescue.Completed"), false);
		Payload.Instigator = const_cast<AActor*>(InstigatorActor);
		Payload.Target = const_cast<AActor*>(TargetActor);

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(const_cast<AActor*>(InstigatorActor), Payload.EventTag, Payload);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShootGA_Interaction_Revive::BroadcastRescueDurationMessage(float DurationSeconds) const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	FLyraInteractionDurationMessage Message;
	Message.Instigator = GetAvatarActorFromActorInfo();
	Message.Duration = DurationSeconds;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_INTERACTION_DURATION_MESSAGE, Message);
}
