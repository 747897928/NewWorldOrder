// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Reload_RocketLauncher.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Weapons/ShootRocketLauncherWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Reload_RocketLauncher)

UShootGA_Reload_RocketLauncher::UShootGA_Reload_RocketLauncher()
{
}

void UShootGA_Reload_RocketLauncher::PostInitProperties()
{
	// 不调用 UShootGameplayAbility_ReloadMagazine::PostInitProperties：该类的
	// 历史实现会在 CDO 阶段读取 FShootGameplayTags::Get()，而此时单例可能仍为空。
	UGameplayAbility::PostInitProperties();

	Tag_AbilityWeaponNoFiring = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Weapon.NoFiring")), false);
	Tag_AbilityActivateFail_MagazineFull = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.ActivateFail.MagazineFull")), false);
	Tag_AbilityActivateFail_NoSpareAmmo = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.ActivateFail.NoSpareAmmo")), false);
	Tag_GameplayEvent_ReloadDone = FGameplayTag::RequestGameplayTag(
		FName(TEXT("GameplayEvent.ReloadDone")), false);
	RocketInsertEventTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("GameplayEvent.Reload.RocketInsert")), false);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FGameplayTagContainer AbilityAssetTags = GetAssetTags();
		AbilityAssetTags.AddTag(FGameplayTag::RequestGameplayTag(
			FName(TEXT("Ability.Type.Action.Reload")), false));
		SetAssetTags(AbilityAssetTags);
		ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(
			FName(TEXT("Event.Movement.Reload")), false));
		StartupInputTag = FGameplayTag::RequestGameplayTag(
			FName(TEXT("InputTag.R")), false);
	}
}

void UShootGA_Reload_RocketLauncher::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 先在父类启动角色/武器蒙太奇之前准备弹头。父类的
	// UAbilityTask_PlayMontageAndWait::ReadyForActivation() 可能马上推进第一帧；
	// 如果在 Super 之后才创建，AmmoSocket 动画已经开始，而弹头会错过换弹前半段。
	if (UShootRocketLauncherWeaponInstance* RocketWeapon =
		Cast<UShootRocketLauncherWeaponInstance>(GetWeaponInstance()))
	{
		RocketWeapon->PrepareRocketForReload();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
	{
		return;
	}

	if (RocketInsertEventTag.IsValid())
	{
		RocketInsertEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			RocketInsertEventTag,
			nullptr,
			true,
			true);
		if (RocketInsertEventTask)
		{
			RocketInsertEventTask->EventReceived.AddDynamic(
				this,
				&UShootGA_Reload_RocketLauncher::OnRocketInsertEvent);
			RocketInsertEventTask->ReadyForActivation();
		}
	}
}

void UShootGA_Reload_RocketLauncher::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (RocketInsertEventTask)
	{
		RocketInsertEventTask->EndTask();
		RocketInsertEventTask = nullptr;
	}

	if (UShootRocketLauncherWeaponInstance* RocketWeapon =
		Cast<UShootRocketLauncherWeaponInstance>(GetWeaponInstance()))
	{
		RocketWeapon->CancelRocketReload();
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UShootGA_Reload_RocketLauncher::OnRocketInsertEvent(FGameplayEventData Payload)
{
	if (UShootRocketLauncherWeaponInstance* RocketWeapon =
		Cast<UShootRocketLauncherWeaponInstance>(GetWeaponInstance()))
	{
		// 这个事件只负责表现上的“手 -> 发射器”转挂；实际库存结算仍由
		// 既有 AN_Reload -> GameplayEvent.ReloadDone 链路完成，避免改通用 GA。
		RocketWeapon->CommitRocketReload();
	}
}
