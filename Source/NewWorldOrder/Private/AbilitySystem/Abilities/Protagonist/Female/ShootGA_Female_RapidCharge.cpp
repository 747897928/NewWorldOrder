// 女主C：疾速充能，默认移速/换弹 Buff（击杀刷新逻辑待接入击杀事件）
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_RapidCharge.h"

#include "AbilitySystem/Effects/ShootEffect_MoveSpeed_SetByCaller.h"
#include "AbilitySystem/Effects/ShootEffect_ReloadSpeed_SetByCaller.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/ShootCharacter.h"
#include "GameModes/ShootGameModeBase.h"
#include "ShootGameplayTags.h"

UShootGA_Female_RapidCharge::UShootGA_Female_RapidCharge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.SkillC.RapidCharge"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Utility);
	SetAssetTags(AssetTags);

	MoveSpeedBuffClass = UShootEffect_MoveSpeed_SetByCaller::StaticClass();
	ReloadBuffClass = UShootEffect_ReloadSpeed_SetByCaller::StaticClass();
	BuffDuration = 4.f;
	MoveSpeedBonusLv1 = 0.5f;
	MoveSpeedBonusLv2 = 0.6f;
	ReloadSpeedBonusLv3 = 0.5f;
	KillExtendSecondsLv1 = 4.f;
	KillExtendSecondsLv3 = 2.f;
}

void UShootGA_Female_RapidCharge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyBuffsAndDuration(BuffDuration);

	// 监听击杀刷新持续时间（仅服务器有 GameMode）
	if (!bKillDelegateBound)
	{
		if (AShootGameModeBase* GM = GetShootGameMode())
		{
			GM->OnCharacterKilled.AddDynamic(this, &ThisClass::HandleKill);
			bKillDelegateBound = true;
		}
	}
}

void UShootGA_Female_RapidCharge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AShootGameModeBase* ShootGameModeBase = GetShootGameMode())
	{
		ShootGameModeBase->OnCharacterKilled.RemoveDynamic(this, &ThisClass::HandleKill);
	}
	bKillDelegateBound = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (MoveSpeedBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(MoveSpeedBuffHandle);
			MoveSpeedBuffHandle.Invalidate();
		}
		if (ReloadBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ReloadBuffHandle);
			ReloadBuffHandle.Invalidate();
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Female_RapidCharge::HandleKill(AActor* Killer, AActor* Victim)
{
	AShootCharacter* OwnerCharacter = GetShootCharacterFromActorInfo();
	if (!OwnerCharacter || !UShootAbilitySystemLibrary::IsKillAttributedToActor(Killer, OwnerCharacter))
	{
		return;
	}

	// 击杀刷新持续时间（无上限）
	float ExtendSeconds = KillExtendSecondsLv1;
	const float AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	if (AbilityLevel >= 3.f)
	{
		ExtendSeconds = KillExtendSecondsLv3;
	}

	float Remaining = 0.f;
	if (UWorld* World = GetWorld())
	{
		Remaining = World->GetTimerManager().GetTimerRemaining(DurationTimerHandle);
	}
	ApplyBuffsAndDuration(FMath::Max(0.f, Remaining) + ExtendSeconds);

	// TODO: RapidCharge - 精英击杀奖励与 ADS 移动惩罚调整，按设计补齐。
}

void UShootGA_Female_RapidCharge::HandleDurationExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShootGA_Female_RapidCharge::RefreshBuffsAndDuration()
{
	ApplyBuffsAndDuration(BuffDuration);
}

void UShootGA_Female_RapidCharge::ApplyBuffsAndDuration(float NewDuration)
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	const float Level = GetAbilityLevel(CurrentSpecHandle, ActorInfo);

	// 刷新 Buff：移除旧句柄再应用，避免叠层无限累积
	if (MoveSpeedBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(MoveSpeedBuffHandle);
		MoveSpeedBuffHandle.Invalidate();
	}
	if (ReloadBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ReloadBuffHandle);
		ReloadBuffHandle.Invalidate();
	}

	FGameplayEffectContextHandle BaseCtx = ASC->MakeEffectContext();
	BaseCtx.AddSourceObject(this);

	if (MoveSpeedBuffClass)
	{
		if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(MoveSpeedBuffClass, Level, BaseCtx); Spec.IsValid())
		{
			Spec.Data->SetDuration(NewDuration, true);
			const float MoveBonus = (Level >= 2.f) ? MoveSpeedBonusLv2 : MoveSpeedBonusLv1;
			const FGameplayTag MoveSpeedTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.MoveSpeedMultiplier"), false);
			if (MoveSpeedTag.IsValid())
			{
				FShootGameplayTags::SetSetByCallerMagnitude(*Spec.Data.Get(), MoveSpeedTag, 1.0f + MoveBonus);
			}
			MoveSpeedBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	if (ReloadBuffClass && Level >= 3.f)
	{
		if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ReloadBuffClass, Level, BaseCtx); Spec.IsValid())
		{
			Spec.Data->SetDuration(NewDuration, true);
			const FGameplayTag ReloadTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.ReloadSpeedMultiplier"), false);
			if (ReloadTag.IsValid())
			{
				FShootGameplayTags::SetSetByCallerMagnitude(
					*Spec.Data.Get(), ReloadTag, 1.0f + ReloadSpeedBonusLv3);
			}
			ReloadBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
		World->GetTimerManager().SetTimer(DurationTimerHandle, this, &ThisClass::HandleDurationExpired, NewDuration, false);
	}
}
