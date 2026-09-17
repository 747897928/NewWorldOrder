// Copyright ZhaoYiJie
// AbilitySystem/Abilities/ShootGA_Weapon_Fire_Sniper.cpp
#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Sniper.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h"
#include "Weapons/ShootRangedWeaponInstance.h"

UShootGA_Weapon_Fire_Sniper::UShootGA_Weapon_Fire_Sniper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    FireGameplayCueTag   = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Sniper.Fire"), false);
	// 命中特效沿用项目已有的通用 Rifle Impact Cue；Sniper 只新增开火 Cue，避免注册没有资产承载的伪专用标签。
	ImpactGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rifle.Impact"), false);
    SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.WeaponFire"), false)));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Movement.WeaponFire"), false));
    StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.LMB"), false);

	// 弹药成本：统一使用 AmmoTagStack（Inventory_Ammo_Magazine / 每次 1 发），支持 Overload 无限弹
	if (!AdditionalCosts.ContainsByPredicate([](const UShootAbilityCost* Cost){ return Cost && Cost->IsA<UShootAbilityCost_AmmoTagStack>(); }))
	{
		UShootAbilityCost_AmmoTagStack* AmmoCost = ObjectInitializer.CreateDefaultSubobject<UShootAbilityCost_AmmoTagStack>(this, TEXT("SniperAmmoCost"));
		AdditionalCosts.Add(AmmoCost);
	}
}

void UShootGA_Weapon_Fire_Sniper::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 本地客户端先做预测；服务器对远端 Pawn 也必须独立重 Trace，不能等待客户端命中结果。
	if (IsLocallyControlled() || K2_HasAuthority())
	{
		StartRangedWeaponTargeting();
	}

	if (UShootRangedWeaponInstance* Weapon = GetWeaponInstance(); Weapon && Weapon->GetCharacterFireMontage())
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Weapon->GetCharacterFireMontage(), 1.0f, NAME_None, false, 1.0f, 0.0f, false);
        MontageTask->OnCompleted.AddDynamic(this, &UShootGA_Weapon_Fire_Sniper::K2_EndAbility);
        MontageTask->OnInterrupted.AddDynamic(this, &UShootGA_Weapon_Fire_Sniper::K2_EndAbility);
        MontageTask->OnCancelled.AddDynamic(this, &UShootGA_Weapon_Fire_Sniper::K2_EndAbility);
        MontageTask->ReadyForActivation();
    }

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this](){ if (IsActive()) K2_EndAbility(); }, FireDelayTimeSecs, false);
}

void UShootGA_Weapon_Fire_Sniper::OnRangedWeaponTargetDataReady_Implementation(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (FireGameplayCueTag.IsValid())
	{
		const FHitResult& FirstHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, 0);
		FGameplayCueParameters P = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(FirstHit);
		if (UShootRangedWeaponInstance* W = GetWeaponInstance())
		{
			P.SourceObject = W;
			if (!FirstHit.bBlockingHit)
			{
				P.Location = W->GetMuzzleLocation();
				P.Normal = GetAimDirection();
			}
		}
		K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, P);
	}

    const int32 Count = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(TargetData);
    const bool bAuth = K2_HasAuthority();
    for (int32 i=0;i<Count;++i)
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
