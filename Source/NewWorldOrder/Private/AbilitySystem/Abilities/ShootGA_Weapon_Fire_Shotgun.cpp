// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Shotgun.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h"
#include "Weapons/ShootRangedWeaponInstance.h"

UShootGA_Weapon_Fire_Shotgun::UShootGA_Weapon_Fire_Shotgun(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Shotgun.Fire"), false);
	// 与 Lyra 一致，三把主武器共用 Rifle.Impact；该 Cue 只补表面音效，不重复 Fire Cue 的粒子和贴花。
	ImpactGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rifle.Impact"));
	SetAssetTags(
		FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.WeaponFire"), false)));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Movement.WeaponFire"), false));
	StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.LMB"), false);

	// 非空弹匣换弹时，开火能力激活后取消 Reload；空仓换弹只在第一发提交前由 NoFiring
	// 阻止开火，第一发进入弹匣后即可用开火取消逐发换弹并立即射击。
	// GameplayAbility CDO 可能早于 FShootGameplayTags 单例初始化；逐发换弹的取消/阻挡关系
	// 必须直接请求稳定 Tag，否则 CDO 会留下空容器，开火输入就无法打断 Reload。
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(
		FName("Ability.Type.Action.Reload"), false));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(
		FName("Ability.Weapon.NoFiring"), false));

	// 弹药成本：统一使用 AmmoTagStack（Inventory_Ammo_Magazine / 每次 1 发），支持 Overload 无限弹
	if (!AdditionalCosts.ContainsByPredicate([](const UShootAbilityCost* Cost){ return Cost && Cost->IsA<UShootAbilityCost_AmmoTagStack>(); }))
	{
		UShootAbilityCost_AmmoTagStack* AmmoCost = ObjectInitializer.CreateDefaultSubobject<UShootAbilityCost_AmmoTagStack>(this, TEXT("ShotgunAmmoCost"));
		AdditionalCosts.Add(AmmoCost);
	}
}

void UShootGA_Weapon_Fire_Shotgun::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsLocallyControlled())
	{
		StartRangedWeaponTargeting();
	}

	const UShootRangedWeaponInstance* WeaponInstance = GetWeaponInstance();
	UAnimMontage* MontageToPlay = WeaponInstance ? WeaponInstance->GetCharacterFireMontage() : nullptr;

	if (MontageToPlay)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, MontageToPlay, AutoRate, NAME_None, false, 1.0f, 0.0f, false);
		MontageTask->OnCompleted.AddDynamic(this, &UShootGA_Weapon_Fire_Shotgun::K2_EndAbility);
		MontageTask->OnInterrupted.AddDynamic(this, &UShootGA_Weapon_Fire_Shotgun::K2_EndAbility);
		MontageTask->OnCancelled.AddDynamic(this, &UShootGA_Weapon_Fire_Shotgun::K2_EndAbility);
		MontageTask->ReadyForActivation();
	}

	FTimerHandle TH;
	GetWorld()->GetTimerManager().SetTimer(TH, [this]() { if (IsActive()) K2_EndAbility(); }, FireDelayTimeSecs, false);
}

void UShootGA_Weapon_Fire_Shotgun::OnRangedWeaponTargetDataReady_Implementation(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	// 触发枪口火光
	if (FireGameplayCueTag.IsValid())
	{
		const FHitResult& FirstHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, 0);
		FGameplayCueParameters CueParameters = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(FirstHit);
		if (UShootRangedWeaponInstance* W = GetWeaponInstance())
		{
			CueParameters.SourceObject = W;
			if (!FirstHit.bBlockingHit)
			{
				CueParameters.Location = W->GetMuzzleLocation();
				CueParameters.Normal = GetAimDirection();
			}
		}
		K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, CueParameters);
	}

	// 命中/伤害
	const int32 Count = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(TargetData);
	const bool bAuth = K2_HasAuthority();
	for (int32 i = 0; i < Count; ++i)
	{
		const FHitResult& Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, i);
		if (!Hit.bBlockingHit) continue;

		if (ImpactGameplayCueTag.IsValid())
		{
			FGameplayCueParameters IP = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(Hit);
			K2_ExecuteGameplayCueWithParams(ImpactGameplayCueTag, IP);
		}
		if (bAuth) { ApplyWeaponDamageToTarget(Hit); }
	}

	BroadcastReticleHitNotify(TargetData);
}
