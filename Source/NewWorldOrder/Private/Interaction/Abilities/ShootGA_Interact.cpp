// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interact.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/InteractionStatics.h"
#include "Interaction/LyraInteractionDurationMessage.h"
#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SingleLineTrace.h"
#include "NativeGameplayTags.h"
#include "ShootGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Interaction_Activate_Core, "Ability.Interaction.Activate");

UShootGA_Interact::UShootGA_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 该能力在玩家身上常驻（按键交互 GA），客户端本地预测
	// 这里只负责通用交互扫描与触发，不再为角色切换单独维护长按状态机。
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;
	// DA_AbilitySet_PlayerCore 显式把同一 InputTag 写进 Spec 的动态来源标签，
	// 这样 IA_Interact -> InputTag.Ability.Interact -> ASC -> OnSpawn 常驻扫描能力才能闭合。
	// 这里不能直接依赖 FShootGameplayTags::Get()，因为构造阶段存在原生标签初始化时序问题。
	StartupInputTag = FGameplayTag::RequestGameplayTag(FName(TEXT("InputTag.Ability.Interact")), false);
}

void UShootGA_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo* ActorInfo,
                                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                                 const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const APlayerController* PlayerController = ActorInfo ? Cast<APlayerController>(ActorInfo->PlayerController.Get())
		: nullptr;
	if (!PlayerController)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	LookForInteractables();
	StartInteractPressScan();
}

void UShootGA_Interact::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
                                            bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActiveScanTask)
	{
		ActiveScanTask->EndTask();
		ActiveScanTask = nullptr;
	}

	if (ActiveInputTask)
	{
		ActiveInputTask->EndTask();
		ActiveInputTask = nullptr;
	}

	CancelInteractionHold(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Interact::LookForInteractables()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	APawn* Pawn = Info ? Cast<APawn>(Info->AvatarActor.Get()) : nullptr;
	AController* Controller = Info ? Info->PlayerController.Get() : nullptr;
	if (!Pawn || !Controller)
	{
		return;
	}

	FInteractionQuery Query;
	Query.RequestingAvatar = Pawn;
	Query.RequestingController = Controller;

	FGameplayAbilityTargetingLocationInfo StartLocationInfo;
	StartLocationInfo.LocationType = EGameplayAbilityTargetingLocationType::ActorTransform;
	StartLocationInfo.SourceActor = Pawn;

	ActiveScanTask =
		UAbilityTask_WaitForInteractableTargets_SingleLineTrace::WaitForInteractableTargets_SingleLineTrace(
			this,
			Query,
			InteractionTraceProfile,
			StartLocationInfo,
			InteractionScanRange,
			InteractionScanRate,
			bDebugScanLines);

	if (ActiveScanTask)
	{
		ActiveScanTask->InteractableObjectsChanged.AddDynamic(this, &ThisClass::HandleInteractableObjectsChanged);
		ActiveScanTask->ReadyForActivation();
	}
}

void UShootGA_Interact::StartInteractPressScan()
{
	ActiveInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	if (ActiveInputTask)
	{
		ActiveInputTask->OnPress.AddDynamic(this, &ThisClass::HandleInteractInputPressed);
		ActiveInputTask->ReadyForActivation();
	}
}

void UShootGA_Interact::HandleInteractableObjectsChanged(
	const TArray<FInteractionOption>& InteractableOptions)
{
	// 长按期间锁定最初目标。玩家移开准星、走出范围或目标选项发生变化时立即取消，
	// 防止按住补给站后转向另一个交互物仍在原地完成。
	if (bInteractionHoldActive &&
		(InteractableOptions.Num() == 0 || InteractableOptions[0] != ActiveHoldOption))
	{
		CancelInteractionHold(true);
	}

	RefreshInteractionWidgets(InteractableOptions);

}

void UShootGA_Interact::RefreshInteractionWidgets(const TArray<FInteractionOption>& NewOptions)
{
	UpdateInteractions(NewOptions);
}

bool UShootGA_Interact::HasValidFocusedOption() const
{
	return CurrentOptions.Num() > 0 && CurrentOptions[0].TargetAbilitySystem != nullptr;
}

void UShootGA_Interact::HandleInteractInputPressed(float TimeWaited)
{
	ActiveInputTask = nullptr;

	if (!HasValidFocusedOption())
	{
		StartInteractPressScan();
		return;
	}

	TriggerCurrentInteraction();
	if (!bInteractionHoldActive)
	{
		StartInteractPressScan();
	}
}

void UShootGA_Interact::TriggerCurrentInteraction()
{
	if (!HasValidFocusedOption())
	{
		return;
	}

	const FInteractionOption& FocusedOption = CurrentOptions[0];
	if (FocusedOption.HoldDuration > KINDA_SMALL_NUMBER)
	{
		StartInteractionHold(FocusedOption);
		return;
	}

	TriggerInteractionOption(FocusedOption);
}

void UShootGA_Interact::StartInteractionHold(const FInteractionOption& TargetOption)
{
	CancelInteractionHold(false);

	bInteractionHoldActive = true;
	ActiveHoldOption = TargetOption;
	BroadcastInteractionDuration(TargetOption.HoldDuration);

	// 同一个常驻交互 AbilitySpec 拥有 InputTag.Ability.Interact，因此 WaitInputRelease 能正确接住
	// 键盘、手柄或触摸映射的同一次松开，不需要在 C++ 中判断具体按键。
	ActiveReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	if (ActiveReleaseTask)
	{
		ActiveReleaseTask->OnRelease.AddDynamic(this, &ThisClass::HandleInteractInputReleased);
		ActiveReleaseTask->ReadyForActivation();
	}

	ActiveHoldDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, TargetOption.HoldDuration);
	if (ActiveHoldDelayTask)
	{
		ActiveHoldDelayTask->OnFinish.AddDynamic(this, &ThisClass::HandleInteractionHoldCompleted);
		ActiveHoldDelayTask->ReadyForActivation();
	}
}

void UShootGA_Interact::CancelInteractionHold(bool bRestartPressScan)
{
	if (ActiveReleaseTask)
	{
		ActiveReleaseTask->EndTask();
		ActiveReleaseTask = nullptr;
	}
	if (ActiveHoldDelayTask)
	{
		ActiveHoldDelayTask->EndTask();
		ActiveHoldDelayTask = nullptr;
	}

	const bool bWasActive = bInteractionHoldActive;
	bInteractionHoldActive = false;
	ActiveHoldOption = FInteractionOption();
	if (bWasActive)
	{
		BroadcastInteractionDuration(0.0f);
	}

	if (bRestartPressScan && IsActive())
	{
		StartInteractPressScan();
	}
}

void UShootGA_Interact::HandleInteractInputReleased(float TimeHeld)
{
	(void)TimeHeld;
	ActiveReleaseTask = nullptr;
	CancelInteractionHold(true);
}

void UShootGA_Interact::HandleInteractionHoldCompleted()
{
	ActiveHoldDelayTask = nullptr;
	const FInteractionOption CompletedOption = ActiveHoldOption;

	if (ActiveReleaseTask)
	{
		ActiveReleaseTask->EndTask();
		ActiveReleaseTask = nullptr;
	}
	bInteractionHoldActive = false;
	ActiveHoldOption = FInteractionOption();
	BroadcastInteractionDuration(0.0f);

	TriggerInteractionOption(CompletedOption);
	StartInteractPressScan();
}

void UShootGA_Interact::BroadcastInteractionDuration(float DurationSeconds) const
{
	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	FLyraInteractionDurationMessage Message;
	Message.Instigator = GetAvatarActorFromActorInfo();
	Message.Duration = FMath::Max(0.0f, DurationSeconds);
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_INTERACTION_DURATION_MESSAGE, Message);
}

void UShootGA_Interact::TriggerInteractionOption(const FInteractionOption& TargetOption)
{
	if (!TargetOption.TargetAbilitySystem)
	{
		return;
	}

	AActor* InstigatorActor = GetAvatarActorFromActorInfo();
	AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(
		TargetOption.InteractableTarget);

	FGameplayEventData Payload;
	Payload.EventTag = TAG_Ability_Interaction_Activate_Core;
	Payload.Instigator = InstigatorActor;
	Payload.Target = InteractableTargetActor;
	// 救援掩护期间提供救援速度倍率，交互能力可读取 EventMagnitude 作为耗时倍率
	if (UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FShootGameplayTags& Tags = FShootGameplayTags::Get();
		if (Tags.Status_RescueSpeedBoost.IsValid() && InstigatorASC->HasMatchingGameplayTag(Tags.Status_RescueSpeedBoost))
		{
			// RescueSpeedMultiplier = 2.0f → EventMagnitude = 0.5（耗时减半）
			Payload.EventMagnitude = 0.5f;
		}
	}

	if (TargetOption.InteractableTarget)
	{
		TargetOption.InteractableTarget->CustomizeInteractionEventData(TAG_Ability_Interaction_Activate_Core,
			Payload);
	}

	AActor* TargetActor = const_cast<AActor*>(ToRawPtr(Payload.Target));

	const UAbilitySystemComponent* SourceAbilitySystem = GetAbilitySystemComponentFromActorInfo();
	FGameplayAbilityActorInfo* AbilityActorInfo =
		(TargetOption.TargetAbilitySystem == SourceAbilitySystem)
			? const_cast<FGameplayAbilityActorInfo*>(GetCurrentActorInfo())
			: nullptr;
	FGameplayAbilityActorInfo InteractionActorInfo;
	if (!AbilityActorInfo)
	{
		// 目标自身拥有 ASC 时才把 ActorInfo 临时切到目标；玩家自己持有的 Collect 能力必须继续使用玩家 Pawn。
		InteractionActorInfo.InitFromActor(InteractableTargetActor, TargetActor, TargetOption.TargetAbilitySystem);
		AbilityActorInfo = &InteractionActorInfo;
	}

	TargetOption.TargetAbilitySystem->TriggerAbilityFromGameplayEvent(TargetOption.TargetInteractionAbilityHandle,
	                                                                  AbilityActorInfo,
	                                                                  TAG_Ability_Interaction_Activate_Core,
	                                                                  &Payload,
	                                                                  *TargetOption.TargetAbilitySystem);
}
