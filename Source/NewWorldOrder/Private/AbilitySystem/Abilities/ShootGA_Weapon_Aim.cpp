#include "AbilitySystem/Abilities/ShootGA_Weapon_Aim.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "Camera/ShootCameraModeStackComponent.h"
#include "Character/ShootCharacter.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Weapon_Aim)

UShootGA_Weapon_Aim::UShootGA_Weapon_Aim()
{
	ActivationPolicy = EShootAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	bServerRespectsRemoteAbilityCancellation = true;
}

void UShootGA_Weapon_Aim::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
		FGameplayTagContainer AbilityAssetTags = GetAssetTags();
		AbilityAssetTags.AddTag(GameplayTags.Ability_Type_Action_ADS);
		SetAssetTags(AbilityAssetTags);
		StartupInputTag = GameplayTags.InputTag_RMB;
	}
}

bool UShootGA_Weapon_Aim::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo)
	{
		return false;
	}

	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	const AShootCharacter* Character = Cast<AShootCharacter>(ActorInfo->AvatarActor.Get());
	if (!Weapon || !Character)
	{
		return false;
	}

	// 项目不制作专用第一人称动画/机瞄数据。第一人称普通枪不激活伪 ADS，
	// Sniper 仍使用现有全屏 Scope；第三人称继续保留全部武器的肩射 ADS。
	const UShootCameraModeStackComponent* CameraModes = Character->GetCameraModeStack();
	if (CameraModes
		&& CameraModes->GetPerspective() == EShootCameraPerspective::FirstPerson
		&& !Weapon->SupportsFirstPersonADS())
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UShootGA_Weapon_Aim::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	AShootCharacter* Character = ActorInfo ? Cast<AShootCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Weapon || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedWeapon = Weapon;
	CachedCharacter = Character;
	bControlsLocalCamera = ActorInfo->IsLocallyControlled();
	Character->SetAimingMovementSpeedMultiplier(Weapon->GetADSMovementSpeedMultiplier());
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 本项目在 AssetManager 启动阶段初始化 FShootGameplayTags，原生 Ability CDO 的
		// PostInitProperties 早于该时机，不能依赖 ActivationOwnedTags 在 CDO 上固化有效 Tag。
		// 本地预测端立即写入，服务器使用 CountToOwner 让同一 ADS Tag 同步到模拟代理动画。
		const EGameplayTagReplicationState ReplicationState = ActorInfo->IsNetAuthority()
			? EGameplayTagReplicationState::CountToOwner
			: EGameplayTagReplicationState::None;
		ASC->AddLooseGameplayTag(FShootGameplayTags::Get().Event_Movement_ADS, 1, ReplicationState);
		bAppliedADSTag = true;
	}

	BroadcastADSMessage(true);
	BeginAimTransition(1.0f);

	UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::HandleInputReleased);
	WaitReleaseTask->ReadyForActivation();
}

void UShootGA_Weapon_Aim::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (CachedCharacter.IsValid())
	{
		CachedCharacter->SetAimingMovementSpeedMultiplier(1.0f);
	}
	if (bAppliedADSTag)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const EGameplayTagReplicationState ReplicationState = ActorInfo && ActorInfo->IsNetAuthority()
				? EGameplayTagReplicationState::CountToOwner
				: EGameplayTagReplicationState::None;
			ASC->RemoveLooseGameplayTag(FShootGameplayTags::Get().Event_Movement_ADS, 1, ReplicationState);
		}
		bAppliedADSTag = false;
	}

	BroadcastADSMessage(false);
	// UGameplayAbility::EndAbility 会结束 AbilityTask。反向过渡必须在 Super 之后启动，
	// 否则 Timer 会随 Ability 结束失效。CameraModeStack 把权重退回 0 后会自动回到
	// 当前 Experience 选择的第一或第三人称基础端点。
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	BeginAimTransition(0.0f);
}

void UShootGA_Weapon_Aim::HandleInputReleased(float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UShootRangedWeaponInstance* UShootGA_Weapon_Aim::GetWeaponInstance() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UShootRangedWeaponInstance>(Spec->SourceObject.Get());
	}
	return nullptr;
}

void UShootGA_Weapon_Aim::BeginAimTransition(float TargetAlpha)
{
	TargetAimAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
	UShootRangedWeaponInstance* Weapon = CachedWeapon.Get();
	UWorld* World = GetWorld();
	if (!Weapon || !World || Weapon->GetADSCameraBlendTime() <= KINDA_SMALL_NUMBER)
	{
		ApplyTransitionAlpha(TargetAimAlpha);
		if (TargetAimAlpha <= KINDA_SMALL_NUMBER)
		{
			bControlsLocalCamera = false;
			CachedWeapon.Reset();
			CachedCharacter.Reset();
		}
		return;
	}

	World->GetTimerManager().SetTimer(
		AimTransitionTimerHandle,
		this,
		&ThisClass::TickAimTransition,
		1.0f / 60.0f,
		true,
		0.0f);
}

void UShootGA_Weapon_Aim::TickAimTransition()
{
	UShootRangedWeaponInstance* Weapon = CachedWeapon.Get();
	UWorld* World = GetWorld();
	if (!Weapon || !World)
	{
		if (TargetAimAlpha <= KINDA_SMALL_NUMBER)
		{
			ApplyTransitionAlpha(0.0f);
		}
		if (World)
		{
			World->GetTimerManager().ClearTimer(AimTransitionTimerHandle);
		}
		return;
	}

	const float BlendTime = FMath::Max(Weapon->GetADSCameraBlendTime(), KINDA_SMALL_NUMBER);
	const float NewAlpha = FMath::FInterpConstantTo(
		CurrentAimAlpha,
		TargetAimAlpha,
		World->GetDeltaSeconds(),
		1.0f / BlendTime);
	ApplyTransitionAlpha(NewAlpha);

	if (FMath::IsNearlyEqual(CurrentAimAlpha, TargetAimAlpha, KINDA_SMALL_NUMBER))
	{
		World->GetTimerManager().ClearTimer(AimTransitionTimerHandle);
		if (TargetAimAlpha <= KINDA_SMALL_NUMBER)
		{
			bControlsLocalCamera = false;
			CachedWeapon.Reset();
			CachedCharacter.Reset();
		}
	}
}

void UShootGA_Weapon_Aim::ApplyTransitionAlpha(float NewAlpha)
{
	CurrentAimAlpha = FMath::Clamp(NewAlpha, 0.0f, 1.0f);
	UShootRangedWeaponInstance* Weapon = CachedWeapon.Get();
	AShootCharacter* Character = CachedCharacter.Get();
	if (Weapon)
	{
		Weapon->SetAimingAlpha(CurrentAimAlpha);
	}

	// EndAbility 之后 CurrentActorInfo 不再是可靠的激活上下文，但退出 ADS 的反向过渡仍需继续。
	// 使用 ActivateAbility 时缓存的本地相机所有权，确保松开、快速点击和取消能力都能把
	// 模式栈权重恢复到 0，不依赖已结束的 CurrentActorInfo。
	if (!Character || !bControlsLocalCamera)
	{
		return;
	}

	if (UShootCameraModeStackComponent* CameraModes = Character->GetCameraModeStack())
	{
		CameraModes->SetAimModeWeight(Weapon, CurrentAimAlpha);
	}
}

void UShootGA_Weapon_Aim::BroadcastADSMessage(bool bIsADS) const
{
	if (!CurrentActorInfo || !CurrentActorInfo->IsLocallyControlled())
	{
		return;
	}

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	FShootReticleADSMessage Message;
	Message.SourceActor = AvatarActor;
	Message.bIsAds = bIsADS;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		FShootGameplayTags::Get().Msg_UI_Reticle_ADS,
		Message);
}
