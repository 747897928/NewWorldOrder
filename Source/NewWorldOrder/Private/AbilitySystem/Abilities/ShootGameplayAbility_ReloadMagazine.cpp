// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/ShootGameplayAbility_ReloadMagazine.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "BlueprintGameplayTagLibrary.h"
#include "ShootGameplayTags.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "Animation/AnimMontage.h"

UShootGameplayAbility_ReloadMagazine::UShootGameplayAbility_ReloadMagazine()
{
	CharacterReloadMontage = nullptr;
	// 设置实例化策略
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 对齐 Lyra：LocalPredicted 能力由 GAS 在拥有端和服务器分别执行，Ability UObject 本身不复制。
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	bServerRespectsRemoteAbilityCancellation = true;
	bRetriggerInstancedAbility = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}

void UShootGameplayAbility_ReloadMagazine::PostInitProperties()
{
	Super::PostInitProperties();

	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();

	// 这些字段不是反射属性。InstancedPerActor 的运行时 Ability 必须逐实例初始化，
	// 否则 WaitGameplayEvent 会监听空 Tag。
	Tag_AbilityWeaponNoFiring = GameplayTags.Ability_Weapon_NoFiring;
	Tag_AbilityActivateFail_MagazineFull = GameplayTags.Ability_ActivateFail_MagazineFull;
	Tag_AbilityActivateFail_NoSpareAmmo = GameplayTags.Ability_ActivateFail_NoSpareAmmo;
	Tag_GameplayEvent_ReloadDone = GameplayTags.GameplayEvent_ReloadDone;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FGameplayTagContainer AbilityAssetTags = GetAssetTags();
		AbilityAssetTags.AddTag(GameplayTags.Ability_Type_Action_Reload);
		SetAssetTags(AbilityAssetTags);
		ActivationOwnedTags.AddTag(GameplayTags.Event_Movement_Reload);
		if (!StartupInputTag.IsValid())
		{
			StartupInputTag = GameplayTags.InputTag_R;
		}
	}
}

bool UShootGameplayAbility_ReloadMagazine::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 先检查父类条件
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 获取武器实例
	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon)
	{
		return false;
	}

	// 客户端读取预测弹药，服务器读取权威值
	const bool bUsePrediction = ActorInfo && !ActorInfo->IsNetAuthority() && ActorInfo->IsLocallyControlled();
	const int32 CurrentAmmoValue = bUsePrediction ? Weapon->GetPredictedAmmo() : Weapon->GetCurrentAmmo();
	const int32 CurrentReserveValue = bUsePrediction ? Weapon->GetPredictedReserve() : Weapon->GetCurrentReserve();

	// 检查是否需要换弹
	const bool bNeedsReload = CurrentAmmoValue < Weapon->GetMagazineSize();
	const bool bHasReserveAmmo = CurrentReserveValue > 0;

	// 只有在弹夹未满且有备弹时才能换弹
	if (!bNeedsReload || !bHasReserveAmmo)
	{
		if (OptionalRelevantTags)
		{
			if (!bNeedsReload && Tag_AbilityActivateFail_MagazineFull.IsValid())
			{
				OptionalRelevantTags->AddTag(Tag_AbilityActivateFail_MagazineFull);
			}
			if (!bHasReserveAmmo && Tag_AbilityActivateFail_NoSpareAmmo.IsValid())
			{
				OptionalRelevantTags->AddTag(Tag_AbilityActivateFail_NoSpareAmmo);
			}
		}
		return false;
	}

	return true;
}

void UShootGameplayAbility_ReloadMagazine::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 获取武器实例
	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reload Ability: No valid weapon instance."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bUsePrediction = ActorInfo && !ActorInfo->IsNetAuthority() && ActorInfo->IsLocallyControlled();
	const int32 AmmoForDecision = bUsePrediction ? Weapon->GetPredictedAmmo() : Weapon->GetCurrentAmmo();
	DidBlockFiring = AmmoForDecision == 0;
	if (DidBlockFiring && Tag_AbilityWeaponNoFiring.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(
			GetOwningActorFromActorInfo(),
			UBlueprintGameplayTagLibrary::MakeLiteralGameplayTagContainer(
				FGameplayTagContainer(Tag_AbilityWeaponNoFiring)),
			false
		);
	}

	UAnimMontage* MontageToPlay = Weapon->GetCharacterReloadMontage();
	if (!MontageToPlay)
	{
		MontageToPlay = CharacterReloadMontage;
	}

	if (MontageToPlay)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			MontageToPlay,
			PlayRate,
			NAME_None,
			false, // AN_Reload 在装匣帧结束服务器 Ability 后，蒙太奇仍需播放完收尾段。
			1.0f, // AnimRootMotionTranslationScale
			0.0f, // StartTimeSeconds
			false // bIgnoreIfAlreadyPlaying
		);

		// 绑定蒙太奇结束回调
		MontageTask->OnCompleted.AddDynamic(this, &UShootGameplayAbility_ReloadMagazine::K2_EndAbilityLocally);
		MontageTask->OnInterrupted.AddDynamic(this, &UShootGameplayAbility_ReloadMagazine::K2_EndAbility);
		MontageTask->OnCancelled.AddDynamic(this, &UShootGameplayAbility_ReloadMagazine::K2_EndAbility);

		MontageTask->ReadyForActivation();
	}
	else
	{
		// 正式链路必须由蒙太奇上的 AN_Reload 驱动；缺资产时明确失败，不能静默瞬间装满掩盖配置错误。
		UE_LOG(LogTemp, Error, TEXT("Reload Ability: weapon %s has no character reload montage."), *GetNameSafe(Weapon));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 对齐 Lyra GA_Weapon_ReloadMagazine：正式角色蒙太奇在装匣帧由
	// /Game/Characters/Heroes/Abilities/AN_Reload 发送 GameplayEvent.ReloadDone，服务器收到后才结算。
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		Tag_GameplayEvent_ReloadDone,
		nullptr, // OptionalExternalTarget（监听自己）
		true, // OnlyTriggerOnce = true 只触发一次
		true // OnlyMatchExact = true（精确匹配 Tag）
	);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &UShootGameplayAbility_ReloadMagazine::OnReloadEventReceived);
		WaitEventTask->ReadyForActivation();
	}
}

void UShootGameplayAbility_ReloadMagazine::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	//Restore ability to fire weapon if we blocked it during reloading
	if (DidBlockFiring && Tag_AbilityWeaponNoFiring.IsValid())
	{
		UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(
			GetOwningActorFromActorInfo(),
			UBlueprintGameplayTagLibrary::MakeLiteralGameplayTagContainer(
				FGameplayTagContainer(Tag_AbilityWeaponNoFiring)),
			false
		);
	}

	// 清理任务（如果还在运行）
	if (WaitEventTask)
	{
		WaitEventTask->EndTask();
		WaitEventTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UShootGameplayAbility_ReloadMagazine::OnReloadEventReceived(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		// 对齐 Lyra：库存弹药只由服务器在 AN_Reload 事件时点修改。
		ReloadAmmoIntoMagazine();
		K2_EndAbility();
	}
	// 客户端不伪造库存值；蒙太奇 OnCompleted 只结束本地预测实例，复制结果负责纠正 UI。
}

void UShootGameplayAbility_ReloadMagazine::ReloadAmmoIntoMagazine()
{
	UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon)
	{
		return;
	}
	const int32 MagazineSize = Weapon->GetMagazineSize();
	const int32 CurrentAmmo = Weapon->GetCurrentAmmo();
	const int32 AmmoNeeded = MagazineSize - CurrentAmmo;
	Weapon->ReloadAmmo(AmmoNeeded);
}

UShootRangedWeaponInstance* UShootGameplayAbility_ReloadMagazine::GetWeaponInstance() const
{
	if (FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UShootRangedWeaponInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}
