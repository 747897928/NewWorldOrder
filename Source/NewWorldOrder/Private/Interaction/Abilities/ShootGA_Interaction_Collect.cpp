// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_Collect.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayCueManager.h"
#include "Pickups/ShootInventoryGrantActor.h"
#include "Pickups/ShootWeaponPickupActor.h"
#include "Pickups/ShootResourcePickup.h"
#include "GameFramework/Pawn.h"
#include "ShootGameplayTags.h"

UShootGA_Interaction_Collect::UShootGA_Interaction_Collect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	// Collect 能力挂在玩家 ASC 上，客户端按 E 触发时需要让 GAS 自动携带 EventData 请求服务器激活。
	// 服务器仍在 Handle*Pickup 分支里做权威库存修改，客户端只负责预测反馈。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	PickupCueTag = FShootGameplayTags::Get().GameplayCue_Interaction_Pickup;
}

void UShootGA_Interaction_Collect::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo,
                                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// PressToInteract 由 UShootGA_Interact 通过 GameplayEvent 触发，玩家 Pawn 写在 EventData.Instigator。
	// 这里优先读事件来源，避免交互目标临时 ActorInfo 配错时把领取物当成 Avatar，导致按 E 看得到提示但不授予。
	APawn* InstigatorPawn = TriggerEventData ? const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get())) : nullptr;
	if (!InstigatorPawn)
	{
		InstigatorPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	}
	const AActor* TargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	AActor* PickupActor = TargetActor ? const_cast<AActor*>(TargetActor) : nullptr;

	if (!InstigatorPawn || !PickupActor)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CachedPickupActor = PickupActor;

	// 资源/武器拾取物可以客户端提前隐藏来避免重复交互。
	// 调试库存发放器由 AShootInventoryGrantActor::bDestroyOnGrant 决定是否消失，不能在通用 Collect 能力里强行隐藏。
	if (!PickupActor->IsA<AShootInventoryGrantActor>())
	{
		PickupActor->SetActorEnableCollision(false);
		PickupActor->SetActorHiddenInGame(true);
	}

	const bool bIsServer = ActorInfo && ActorInfo->IsNetAuthority();
	if (bIsServer)
	{
		if (AShootResourcePickup* ResourcePickup = Cast<AShootResourcePickup>(PickupActor))
		{
			HandleResourcePickup(ResourcePickup, InstigatorPawn);
			ApplyFactionEffects(ResourcePickup, GetAbilitySystemComponentFromActorInfo());
		}
		else if (AShootWeaponPickupActor* WeaponPickup = Cast<AShootWeaponPickupActor>(PickupActor))
		{
			HandleWeaponPickup(WeaponPickup, InstigatorPawn);
		}
		else if (AShootInventoryGrantActor* GrantActor = Cast<AShootInventoryGrantActor>(PickupActor))
		{
			HandleInventoryGrantActor(GrantActor, InstigatorPawn);
		}
	}

	if (PickupCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->ExecuteGameplayCue(PickupCueTag);
		}
	}

	if (PickupMontage && InstigatorPawn)
	{
		PlayPickupMontage(InstigatorPawn);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShootGA_Interaction_Collect::HandleResourcePickup(AShootResourcePickup* PickupActor, APawn* InstigatorPawn)
{
	if (!PickupActor || !InstigatorPawn)
	{
		return;
	}

	const bool bCollected = PickupActor->ProcessPickupFromAbility(InstigatorPawn);
	if (bCollected && PickupActorLifeSpan > 0.f)
	{
		PickupActor->SetLifeSpan(PickupActorLifeSpan);
	}
}

void UShootGA_Interaction_Collect::HandleWeaponPickup(AShootWeaponPickupActor* PickupActor, APawn* InstigatorPawn)
{
	if (!PickupActor || !InstigatorPawn)
	{
		return;
	}

	const bool bCollected = PickupActor->HandlePickup(InstigatorPawn);
	if (bCollected && PickupActorLifeSpan > 0.f)
	{
		PickupActor->SetLifeSpan(PickupActorLifeSpan);
	}
}

void UShootGA_Interaction_Collect::HandleInventoryGrantActor(AShootInventoryGrantActor* GrantActor, APawn* InstigatorPawn)
{
	if (!GrantActor || !InstigatorPawn)
	{
		return;
	}

	// 调试发放 Actor 通常需要留在 HomeMap 里反复领取；是否销毁由 AShootInventoryGrantActor::bDestroyOnGrant 控制。
	GrantActor->ProcessGrantFromAbility(InstigatorPawn);
}

void UShootGA_Interaction_Collect::ApplyFactionEffects(AShootResourcePickup* PickupActor,
                                                      UAbilitySystemComponent* InstigatorASC)
{
	if (!PickupActor || !InstigatorASC)
	{
		return;
	}

	const TArray<FShootFactionEffectEntry>& EffectEntries = PickupActor->GetFactionEffects();
	if (EffectEntries.Num() == 0)
	{
		return;
	}

	for (const FShootFactionEffectEntry& Entry : EffectEntries)
	{
		if (!Entry.EffectClass)
		{
			continue;
		}

		const bool bMatchesFaction = !Entry.FactionTag.IsValid() || InstigatorASC->HasMatchingGameplayTag(
			Entry.FactionTag);
		if (bMatchesFaction)
		{
			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
			InstigatorASC->ApplyGameplayEffectToSelf(Entry.EffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f,
			                                         ContextHandle);
			break;
		}
	}
}

void UShootGA_Interaction_Collect::PlayPickupMontage(APawn* InstigatorPawn)
{
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, PickupMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPickupMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPickupMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPickupMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnPickupMontageFinished);
		MontageTask->ReadyForActivation();
	}
	else
	{
		OnPickupMontageFinished();
	}
}

void UShootGA_Interaction_Collect::OnPickupMontageFinished()
{
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
