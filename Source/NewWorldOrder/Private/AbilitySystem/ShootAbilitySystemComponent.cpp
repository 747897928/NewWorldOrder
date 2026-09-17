// Copyright ZhaoYiJie


#include "AbilitySystem/ShootAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/ShootGameplayEffectContext.h"
#include "ShootGameplayTags.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "Interface/PlayerInterface.h"

namespace
{
FPredictionKey GetPredictionKeyFromSpec(const FGameplayAbilitySpec& Spec)
{
	if (const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
	{
		return Instance->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	return FPredictionKey();
}
}

FGameplayEffectContextHandle UShootAbilitySystemComponent::MakeEffectContext() const
{
	FGameplayEffectContextHandle Context(new FShootGameplayEffectContext());

	if (ensureMsgf(AbilityActorInfo.IsValid(), TEXT("Unable to make effect context because AbilityActorInfo is not valid.")))
	{
		Context.AddInstigator(AbilityActorInfo->OwnerActor.Get(), AbilityActorInfo->AvatarActor.Get());
	}

	return Context;
}

void UShootAbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::ClientEffectApplied);
	TryActivateAbilitiesOnSpawn();
}

void UShootAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (const UShootGameplayAbility* ShootAbility = Cast<UShootGameplayAbility>(AbilitySpec.Ability))
		{
			// ActorInfo 可能晚于能力授予（PlayerState ASC 重新绑定 Avatar），所以这里重试 OnSpawn 能力。
			ShootAbility->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void UShootAbilitySystemComponent::MulticastActivatePassiveEffect_Implementation(const FGameplayTag& AbilityTag,
	bool bActivate)
{
	ActivatePassiveEffect.Broadcast(AbilityTag, bActivate);
}

// 存档不再持有技能(2026-08-23);技能由 Experience AbilitySet 授予。


void UShootAbilitySystemComponent::UpgradeAttribute(FGameplayTag AttributeTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
}

void UShootAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1.f;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
	}
}

void UShootAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (const TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		if (const UShootGameplayAbility* AuraAbility = Cast<UShootGameplayAbility>(AbilitySpec.Ability))
		{
			if (AuraAbility->StartupInputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraAbility->StartupInputTag);
			}
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(FShootGameplayTags::Get().Abilities_Status_Equipped);
			GiveAbility(AbilitySpec);
		}
	}
	AbilitiesGivenDelegate.Broadcast();
}

void UShootAbilitySystemComponent::AddCharacterPassiveAbilities(
	const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (const TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(FShootGameplayTags::Get().Abilities_Status_Equipped);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void UShootAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	// 输入总线只处理 InputTag，不判断 E、手柄键等具体设备。
	// 已激活能力会在 AbilitySpecInputPressed 中收到一次 ReplicatedEvent；未激活能力按策略决定是否启动。
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpec.InputPressed = true;
			AbilitySpecInputPressed(AbilitySpec);

			if (!AbilitySpec.IsActive())
			{
				const UShootGameplayAbility* ShootAbility = Cast<UShootGameplayAbility>(AbilitySpec.Ability);
				if (ShootAbility && ShootAbility->GetActivationPolicy() == EShootAbilityActivationPolicy::OnInputTriggered)
				{
					TryActivateAbility(AbilitySpec.Handle);
				}
			}
		}
	}
}

void UShootAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	// Held 只服务 WhileInputActive 的激活策略，不能每帧调用 AbilitySpecInputPressed，
	// 否则 Interact 内的 WaitInputPress 会把一次长按误认为多次点击。
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			const UShootGameplayAbility* ShootAbility = Cast<UShootGameplayAbility>(AbilitySpec.Ability);
			if (!AbilitySpec.IsActive() && ShootAbility &&
				ShootAbility->GetActivationPolicy() == EShootAbilityActivationPolicy::WhileInputActive)
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UShootAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	// Release 与 Press 一样只通过 AbilitySpecInputReleased 发送一次 GAS 事件。
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			if (AbilitySpec.Ability)
			{
				AbilitySpec.InputPressed = false;

				if (AbilitySpec.IsActive())
				{
					// AbilitySpecInputReleased 的重写会发送一次 GAS ReplicatedEvent；不要在这里重复发送。
					AbilitySpecInputReleased(AbilitySpec);
				}
			}
		}
	}
}

void UShootAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle,
                                                        FGameplayAbilityActivationInfo ActivationInfo,
                                                        FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(
		FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
	if (ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
	}
}

void UShootAbilitySystemComponent::RemoveAbilitiesWithTag(FGameplayTag AbilityTag)
{
	if (!AbilityTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToClear;

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTag(AbilityTag))
		{
			HandlesToClear.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToClear)
	{
		ClearAbility(Handle);
	}
}

void UShootAbilitySystemComponent::GrantAbilitiesWithKit(const TArray<TSubclassOf<UGameplayAbility>>& Abilities,
                                                         const FGameplayTag& KitTag, bool bActivatePassives)
{
	if (!KitTag.IsValid())
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility> AbilityClass : Abilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);

		// 输入挂载：AbilityInputTagPressed 通过 AbilitySpec 动态标签精确匹配 InputTag。
		// 之前这里只加 KitTag + Equipped，导致手雷等带 StartupInputTag 的能力永远无法被按键触发
		// （手雷 T/右肩键按下无反应）。此函数只保留给仍未迁移的旧蓝图，不得用于新主线。
		if (const UShootGameplayAbility* ShootAbility = Cast<UShootGameplayAbility>(AbilitySpec.Ability))
		{
			if (ShootAbility->StartupInputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(ShootAbility->StartupInputTag);
			}
		}
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(KitTag);
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(FShootGameplayTags::Get().Abilities_Status_Equipped);

		if (bActivatePassives)
		{
			GiveAbilityAndActivateOnce(AbilitySpec);
		}
		else
		{
			GiveAbility(AbilitySpec);
		}
	}
}

void UShootAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
		const FPredictionKey OriginalPredictionKey = GetPredictionKeyFromSpec(Spec);

		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void UShootAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
		const FPredictionKey OriginalPredictionKey = GetPredictionKeyFromSpec(Spec);

		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void UShootAbilitySystemComponent::ClientEffectApplied_Implementation(
	UAbilitySystemComponent* AbilitySystemComponent,
	const FGameplayEffectSpec& EffectSpec,
	FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
