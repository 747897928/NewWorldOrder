// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Weapon_AutoReload.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/ShootGA_Reload_ShotgunPerShell.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility_ReloadMagazine.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Weapon_AutoReload)

UShootGA_Weapon_AutoReload::UShootGA_Weapon_AutoReload()
{
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UShootGA_Weapon_AutoReload::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FGameplayTagContainer AbilityAssetTags = GetAssetTags();
		AbilityAssetTags.AddTag(FShootGameplayTags::Get().Ability_Type_Passive_AutoReload);
		SetAssetTags(AbilityAssetTags);
	}
}

void UShootGA_Weapon_AutoReload::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !GetWeaponInstance())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle,
			this,
			&ThisClass::CheckAutoReload,
			PollInterval,
			true,
			PollInterval);
	}
}

void UShootGA_Weapon_AutoReload::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Weapon_AutoReload::CheckAutoReload()
{
	UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Weapon || !ASC || Weapon->GetPredictedAmmo() != 0 || Weapon->GetPredictedReserve() <= 0)
	{
		return;
	}

	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	if (Weapon->GetTimeSinceLastInteractedWith() < TimeSinceActivityToReload
		|| ASC->HasMatchingGameplayTag(GameplayTags.Event_Movement_Reload))
	{
		return;
	}

	TryActivateReloadAbility(*ASC, *Weapon);
}

bool UShootGA_Weapon_AutoReload::TryActivateReloadAbility(
	UAbilitySystemComponent& AbilitySystemComponent,
	const UShootRangedWeaponInstance& Weapon) const
{
	FGameplayAbilitySpecHandle ReloadAbilityHandle;

	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent.GetActivatableAbilities())
	{
		const bool bIsSupportedReloadAbility = AbilitySpec.Ability
			&& (AbilitySpec.Ability->IsA<UShootGameplayAbility_ReloadMagazine>()
				|| AbilitySpec.Ability->IsA<UShootGA_Reload_ShotgunPerShell>());
		if (AbilitySpec.SourceObject.Get() == &Weapon
			&& bIsSupportedReloadAbility)
		{
			// 直接检查同一 SourceObject 的 Reload Spec，避免 AutoReload 每 0.25 秒重复请求
			// 已经运行中的逐发装填；这里不依赖两种 Reload GA 是否声明了完全相同的运行时标签。
			if (AbilitySpec.IsActive())
			{
				return true;
			}
			ReloadAbilityHandle = AbilitySpec.Handle;
			break;
		}
	}

	// 项目的弹匣式 Reload 主线是受保护代码，AutoReload 不给它增加 GameplayEvent Trigger，
	// 也不调用 AbilityInputTagPressed 伪造输入。原生 Ability CDO 的 AssetTag 早于项目 Tag 初始化，
	// 不能作为可靠检索条件；同一 SourceObject + 两种项目正式 Reload 类型锁定当前武器能力。
	// Shotgun_A 仍由逐发装填 GA 自己处理 InsertShell、循环和火力打断，本能力只负责空仓激活它。
	return ReloadAbilityHandle.IsValid()
		&& AbilitySystemComponent.TryActivateAbility(ReloadAbilityHandle);
}

UShootRangedWeaponInstance* UShootGA_Weapon_AutoReload::GetWeaponInstance() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UShootRangedWeaponInstance>(Spec->SourceObject.Get());
	}
	return nullptr;
}
