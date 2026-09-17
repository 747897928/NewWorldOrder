// 女主E：救援掩护，默认给予隐身/移速 Buff，复活逻辑留待事件对接
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_RescueCloak.h"

#include "AbilitySystem/Effects/ShootEffect_MoveSpeed_SetByCaller.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Effects/ShootEffect_HealInstant.h"
#include "AbilitySystem/Effects/ShootEffect_ImmuneDeath.h"
#include "ShootGameplayTags.h"

UShootGA_Female_RescueCloak::UShootGA_Female_RescueCloak()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.SkillE.RescueCloak"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Secondary);
	SetAssetTags(AssetTags);

	MoveSpeedBuffClass = UShootEffect_MoveSpeed_SetByCaller::StaticClass();
	CloakStatusTag = Tags.Status_Cloaked;
	CloakDuration = 6.f;
	MoveSpeedBonus = 0.3f;
	ReviveHealPercent = 0.5f;
	ReviveImmuneDeathDuration = 3.f;
	ReviveHealEffectClass = UShootEffect_HealInstant::StaticClass();
	ReviveProtectionEffectClass = UShootEffect_ImmuneDeath::StaticClass();
	RescueSpeedMultiplier = 2.0f;
	WeaponFireTag = FGameplayTag::RequestGameplayTag(FName("Event.Movement.WeaponFire"), false);
	RescueCompletedTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.Rescue.Completed"), false);
	RescueSpeedStatusTag = Tags.Status_RescueSpeedBoost;
}

void UShootGA_Female_RescueCloak::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		// 使用 SetByCaller 注入移速倍率（1.0 + MoveSpeedBonus）
		if (MoveSpeedBuffClass)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			Ctx.AddSourceObject(this);
			if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(MoveSpeedBuffClass, 1.f, Ctx); Spec.IsValid())
			{
				Spec.Data->SetDuration(CloakDuration, true);
				const FGameplayTag MoveSpeedTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.MoveSpeedMultiplier"), false);
				if (MoveSpeedTag.IsValid())
				{
					FShootGameplayTags::SetSetByCallerMagnitude(
						*Spec.Data.Get(), MoveSpeedTag, 1.0f + MoveSpeedBonus);
				}
				MoveSpeedBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}

		const FShootGameplayTags& Tags = FShootGameplayTags::Get();
		const FGameplayTag UseCloakTag = CloakStatusTag.IsValid() ? CloakStatusTag : Tags.Status_Cloaked;
		ASC->AddLooseGameplayTag(UseCloakTag);
		if (RescueSpeedStatusTag.IsValid())
		{
			ASC->AddLooseGameplayTag(RescueSpeedStatusTag);
		}

		// 监听开火标签，攻击立即破隐
		if (WeaponFireTag.IsValid())
		{
			WeaponFireTagHandle = ASC->RegisterGameplayTagEvent(WeaponFireTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &ThisClass::HandleWeaponFireTagChanged);
		}

		// 监听救援完成事件，为被救起目标附加治疗与保护
		BindRescueEvent(ASC);

		// TODO(主角-RescueCloak-CloakBehavior): 感知/显示层面依赖 Status_Cloaked，在 AI 感知组件和 UI 里补完。
		// TODO(主角-RescueCloak-Revive): 救援/复活速度加成、队友范围隐身按设计补齐。

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CloakTimerHandle);
			World->GetTimerManager().SetTimer(CloakTimerHandle, this, &ThisClass::HandleCloakExpired, CloakDuration, false);
		}
	}
}

void UShootGA_Female_RescueCloak::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CloakTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		const FShootGameplayTags& Tags = FShootGameplayTags::Get();
		const FGameplayTag UseCloakTag = CloakStatusTag.IsValid() ? CloakStatusTag : Tags.Status_Cloaked;
		ASC->RemoveLooseGameplayTag(UseCloakTag);
		if (RescueSpeedStatusTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(RescueSpeedStatusTag);
		}

		if (MoveSpeedBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(MoveSpeedBuffHandle);
			MoveSpeedBuffHandle.Invalidate();
		}

		if (WeaponFireTag.IsValid() && WeaponFireTagHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(WeaponFireTag, EGameplayTagEventType::NewOrRemoved).Remove(WeaponFireTagHandle);
			WeaponFireTagHandle.Reset();
		}

		UnbindRescueEvent(ASC);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Female_RescueCloak::HandleCloakExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShootGA_Female_RescueCloak::HandleWeaponFireTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 开火即破隐
	if (NewCount > 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShootGA_Female_RescueCloak::BindRescueEvent(UAbilitySystemComponent* ASC)
{
	if (!ASC || !RescueCompletedTag.IsValid() || RescueCompletedHandle.IsValid())
	{
		return;
	}

	RescueCompletedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(RescueCompletedTag)
		.AddUObject(this, &ThisClass::HandleRescueCompleted);
}

void UShootGA_Female_RescueCloak::UnbindRescueEvent(UAbilitySystemComponent* ASC)
{
	if (!ASC || !RescueCompletedTag.IsValid() || !RescueCompletedHandle.IsValid())
	{
		return;
	}

	ASC->GenericGameplayEventCallbacks.FindOrAdd(RescueCompletedTag).Remove(RescueCompletedHandle);
	RescueCompletedHandle.Reset();
}

void UShootGA_Female_RescueCloak::HandleRescueCompleted(const FGameplayEventData* Payload)
{
	if (!Payload || !Payload->Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
		const_cast<AActor*>(Payload->Target.Get()));
	if (!TargetASC)
	{
		return;
	}

	// 救起后回复目标生命值百分比
	if (ReviveHealEffectClass && ReviveHealPercent > 0.f)
	{
		const FGameplayTag HealTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.Heal"), false);
		FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ReviveHealEffectClass, 1.f, Ctx);
		if (SpecHandle.IsValid())
		{
			const float TargetMaxHealth = TargetASC->GetNumericAttribute(UShootAttributeSet::GetMaxHealthAttribute());
			const float HealAmount = FMath::Max(0.f, TargetMaxHealth * ReviveHealPercent);
			if (HealTag.IsValid())
			{
				FShootGameplayTags::SetSetByCallerMagnitude(*SpecHandle.Data.Get(), HealTag, HealAmount);
			}
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 救起后短暂保护，默认使用免疫致死
	if (ReviveProtectionEffectClass && ReviveImmuneDeathDuration > 0.f)
	{
		FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ReviveProtectionEffectClass, 1.f, Ctx);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetDuration(ReviveImmuneDeathDuration, true);
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
