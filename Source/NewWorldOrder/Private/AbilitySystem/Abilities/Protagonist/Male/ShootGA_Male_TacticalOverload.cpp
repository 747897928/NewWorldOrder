// 男主大招：战术超载，默认 Buff：射速/移速/换弹，大招充能逻辑待接入
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalOverload.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"
#include "AbilitySystem/Effects/ShootEffect_TacticalOverloadState.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"

UShootGA_Male_TacticalOverload::UShootGA_Male_TacticalOverload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.Ultimate.TacticalOverload"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Ultimate);
	SetAssetTags(AssetTags);
	CooldownGameplayEffectClass = UShootEffect_TacticalOverloadCooldown::StaticClass();
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(
		FName("Cooldown.Skill.TacticalOverload"), false));
	ActivationGameplayCueTag = FGameplayTag::RequestGameplayTag(
		FName("GameplayCue.Skill.TacticalOverload.Activate"), false);
	ActiveGameplayCueTag = Tags.GameplayCue_Skill_TacticalOverload_Active;
	OverloadBuffEffect = UShootEffect_TacticalOverloadState::StaticClass();

}

void UShootGA_Male_TacticalOverload::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const int32 AbilityLevel = FMath::Max(1, GetAbilityLevel(Handle, ActorInfo));
	const float EffectDuration = BaseDuration + DurationPerLevel * static_cast<float>(AbilityLevel - 1);
	if (!OverloadBuffEffect || EffectDuration <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		OverloadBuffEffect, AbilityLevel, Context);
	if (!SpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SpecHandle.Data->SetDuration(EffectDuration, true);
	ActiveOverloadEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!ActiveOverloadEffectHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ActorInfo->IsNetAuthority() && ActivationGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
		CueParameters.Instigator = GetAvatarActorFromActorInfo();
		CueParameters.EffectCauser = GetAvatarActorFromActorInfo();
		K2_ExecuteGameplayCueWithParams(ActivationGameplayCueTag, CueParameters);
	}
	if (ActorInfo->IsNetAuthority() && ActiveGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Instigator = GetAvatarActorFromActorInfo();
		CueParameters.EffectCauser = GetAvatarActorFromActorInfo();
		ASC->AddGameplayCue(ActiveGameplayCueTag, CueParameters);
		bActiveGameplayCueAdded = true;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(OverloadEndTimerHandle, FTimerDelegate::CreateUObject(
			this, &ThisClass::HandleOverloadFinished, Handle, ActorInfo, ActivationInfo, false), EffectDuration, false);
	}
}

void UShootGA_Male_TacticalOverload::HandleOverloadFinished(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const bool bCancelled)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, bCancelled);
}

void UShootGA_Male_TacticalOverload::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OverloadEndTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (bActiveGameplayCueAdded && ActiveGameplayCueTag.IsValid())
		{
			ASC->RemoveGameplayCue(ActiveGameplayCueTag);
		}
		bActiveGameplayCueAdded = false;
		if (ActiveOverloadEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveOverloadEffectHandle);
			ActiveOverloadEffectHandle.Invalidate();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
