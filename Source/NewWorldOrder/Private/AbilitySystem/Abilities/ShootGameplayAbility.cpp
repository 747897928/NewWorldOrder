// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Abilities/ShootAbilityCost.h"
#include "Character/ShootCharacter.h"
#include "GameplayEffect.h"
#include "GameModes/ShootGameModeBase.h"

AShootCharacter* UShootGameplayAbility::GetShootCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AShootCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

void UShootGameplayAbility::ApplyEffectToOwner(TSubclassOf<UGameplayEffect> EffectClass, float Level, float DurationOverride) const
{
	if (!EffectClass || !CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddSourceObject(this);
	if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, Level, Ctx); Spec.IsValid())
	{
		if (DurationOverride >= 0.f)
		{
			Spec.Data->SetDuration(DurationOverride, true);
		}
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

AShootGameModeBase* UShootGameplayAbility::GetShootGameMode() const
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid())
	{
		return nullptr;
	}
	if (UWorld* World = CurrentActorInfo->AvatarActor->GetWorld())
	{
		return World->GetAuthGameMode<AShootGameModeBase>();
	}
	return nullptr;
}

bool UShootGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}

	// Verify we can afford any additional costs
	for (const TObjectPtr<UShootAbilityCost>& AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, /*inout*/ OptionalRelevantTags))
			{
				return false;
			}
		}
	}

	return true;
}

void UShootGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	check(ActorInfo);

	// Used to determine if the ability actually hit a target (as some costs are only spent on successful attempts)
	auto DetermineIfAbilityHitTarget = [&]()
	{
		if (ActorInfo->IsNetAuthority())
		{
			if (UShootAbilitySystemComponent* ASC = Cast<UShootAbilitySystemComponent>(
				ActorInfo->AbilitySystemComponent.Get()))
			{
				FGameplayAbilityTargetDataHandle TargetData;
				ASC->GetAbilityTargetData(Handle, ActivationInfo, TargetData);
				for (int32 TargetDataIdx = 0; TargetDataIdx < TargetData.Data.Num(); ++TargetDataIdx)
				{
					if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetData, TargetDataIdx))
					{
						return true;
					}
				}
			}
		}

		return false;
	};

	// Pay any additional costs
	bool bAbilityHitTarget = false;
	bool bHasDeterminedIfAbilityHitTarget = false;
	for (const TObjectPtr<UShootAbilityCost>& AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (AdditionalCost->ShouldOnlyApplyCostOnHit())
			{
				if (!bHasDeterminedIfAbilityHitTarget)
				{
					bAbilityHitTarget = DetermineIfAbilityHitTarget();
					bHasDeterminedIfAbilityHitTarget = true;
				}

				if (!bAbilityHitTarget)
				{
					continue;
				}
			}

			AdditionalCost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
		}
	}
}

// 项目不使用Mana系统，此函数已废弃,但保留其代码让其他开发者学习GAS的一些用法
/*float UShootGameplayAbility::GetManaCost(float InLevel) const
{
	float ManaCost = 0.f;
	if (const UGameplayEffect* CostEffect = GetCostGameplayEffect())
	{
		for (FGameplayModifierInfo Mod : CostEffect->Modifiers)
		{
			if (Mod.Attribute == UShootAttributeSet::GetManaAttribute())
			{
				Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(InLevel, ManaCost);
				break;
			}
		}
	}
	return ManaCost;
}*/

float UShootGameplayAbility::GetCooldown(float InLevel) const
{
	float Cooldown = 0.f;
	if (const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect())
	{
		CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(InLevel, Cooldown);
	}
	return Cooldown;
}
void UShootGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UShootGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec) const
{
	// Lyra 调用链：GiveAbility -> OnGiveAbility；若当时 Avatar 尚未绑定，
	// 后续 InitAbilityActorInfo -> AbilityActorInfoSet 会再次进入本函数，且 Spec.IsActive() 保证幂等。
	if (!ActorInfo || Spec.IsActive() || ActivationPolicy != EShootAbilityActivationPolicy::OnSpawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!ASC || !AvatarActor || AvatarActor->GetTearOff() || AvatarActor->GetLifeSpan() > 0.f)
	{
		return;
	}

	const bool bLocalPolicy = NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted
		|| NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly;
	const bool bServerPolicy = NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		|| NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	if ((ActorInfo->IsLocallyControlled() && bLocalPolicy) || (ActorInfo->IsNetAuthority() && bServerPolicy))
	{
		// 与 Lyra 相同：本地预测能力由本地玩家发起，ServerOnly/ServerInitiated 才由服务器发起。
		ASC->TryActivateAbility(Spec.Handle);
	}
}
