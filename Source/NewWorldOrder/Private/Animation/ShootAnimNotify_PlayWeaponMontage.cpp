// Copyright ZhaoYiJie

#include "Animation/ShootAnimNotify_PlayWeaponMontage.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify_PlayParticleEffect.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "Components/SkeletalMeshComponent.h"
#include "Equipment/ShootEquipmentManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Weapons/ShootWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_PlayWeaponMontage)

bool UShootAnimNotify_PlayWeaponMontage::ConfigureCharacterMontageNotify(
	UAnimMontage* CharacterMontage,
	UAnimMontage* WeaponMontage,
	const float TriggerTime,
	const float PlayRate)
{
	if (!CharacterMontage || !WeaponMontage || PlayRate <= 0.0f)
	{
		return false;
	}

	CharacterMontage->Modify();

	FAnimNotifyEvent* ExistingNotify = nullptr;
	for (int32 NotifyIndex = CharacterMontage->Notifies.Num() - 1; NotifyIndex >= 0; --NotifyIndex)
	{
		FAnimNotifyEvent& NotifyEvent = CharacterMontage->Notifies[NotifyIndex];
		const UAnimNotify* NotifyObject = NotifyEvent.Notify;
		const FString NotifyClassName = NotifyObject ? NotifyObject->GetClass()->GetName() : FString();
		const bool bIsWeaponMontageNotify = NotifyObject &&
			(NotifyObject->IsA<UShootAnimNotify_PlayWeaponMontage>() ||
			 NotifyClassName.Contains(TEXT("PlayWeaponMontage")));

		if (!bIsWeaponMontageNotify)
		{
			continue;
		}

		if (!ExistingNotify)
		{
			ExistingNotify = &NotifyEvent;
			continue;
		}

		// 同一角色 Montage 只保留一个武器跟播 Notify，避免旧 Lyra Notify 与项目 Notify
		// 同时触发，导致武器 Montage 被重复播放或同步到错误的武器资产。
		CharacterMontage->Notifies.RemoveAt(NotifyIndex);
	}

	if (!ExistingNotify)
	{
		FAnimNotifyEvent& NewNotify = CharacterMontage->Notifies.AddDefaulted_GetRef();
		NewNotify.Notify = NewObject<UShootAnimNotify_PlayWeaponMontage>(CharacterMontage);
		NewNotify.TrackIndex = 0;
		ExistingNotify = &NewNotify;
	}

	UShootAnimNotify_PlayWeaponMontage* WeaponNotify = Cast<UShootAnimNotify_PlayWeaponMontage>(ExistingNotify->Notify);
	if (!WeaponNotify)
	{
		WeaponNotify = NewObject<UShootAnimNotify_PlayWeaponMontage>(CharacterMontage);
		ExistingNotify->Notify = WeaponNotify;
	}

	WeaponNotify->MontageToPlay = WeaponMontage;
	WeaponNotify->PlayRate = PlayRate;
	ExistingNotify->Link(CharacterMontage, FMath::Clamp(TriggerTime, 0.0f, CharacterMontage->GetPlayLength()));
	ExistingNotify->SetTime(FMath::Clamp(TriggerTime, 0.0f, CharacterMontage->GetPlayLength()));
	ExistingNotify->TriggerTimeOffset = 0.0f;
	ExistingNotify->NotifyName = NAME_None;
	CharacterMontage->SortNotifies();
	CharacterMontage->RefreshCacheData();
	CharacterMontage->MarkPackageDirty();
	return true;
}

bool UShootAnimNotify_PlayWeaponMontage::AddMontageSoundNotify(
	UAnimMontage* WeaponMontage,
	USoundBase* Sound,
	const float TriggerTime,
	const FName AttachName)
{
	if (!WeaponMontage || !Sound)
	{
		return false;
	}

	WeaponMontage->Modify();
	const float ClampedTime = FMath::Clamp(TriggerTime, 0.0f, WeaponMontage->GetPlayLength());
	UAnimNotify_PlaySound* SoundNotify = nullptr;
	for (FAnimNotifyEvent& NotifyEvent : WeaponMontage->Notifies)
	{
		if (NotifyEvent.Notify && NotifyEvent.Notify->IsA<UAnimNotify_PlaySound>() &&
			FMath::IsNearlyEqual(NotifyEvent.GetTriggerTime(), ClampedTime, 0.0005f))
		{
			UAnimNotify_PlaySound* ExistingSoundNotify = Cast<UAnimNotify_PlaySound>(NotifyEvent.Notify);
			if (!ExistingSoundNotify ||
				ExistingSoundNotify->Sound != Sound ||
				ExistingSoundNotify->AttachName != AttachName)
			{
				continue;
			}

			SoundNotify = ExistingSoundNotify;
			NotifyEvent.SetTime(ClampedTime);
			break;
		}
	}

	if (!SoundNotify)
	{
		FAnimNotifyEvent& NotifyEvent = WeaponMontage->Notifies.AddDefaulted_GetRef();
		SoundNotify = NewObject<UAnimNotify_PlaySound>(WeaponMontage);
		NotifyEvent.Notify = SoundNotify;
		NotifyEvent.TrackIndex = 1;
		NotifyEvent.Link(WeaponMontage, ClampedTime);
	}

	SoundNotify->Sound = Sound;
	SoundNotify->AttachName = AttachName;
	SoundNotify->bFollow = AttachName != NAME_None;
	SoundNotify->VolumeMultiplier = 1.0f;
	SoundNotify->PitchMultiplier = 1.0f;
	WeaponMontage->SortNotifies();
	WeaponMontage->RefreshCacheData();
	WeaponMontage->MarkPackageDirty();
	return true;
}

bool UShootAnimNotify_PlayWeaponMontage::AddMontageParticleNotify(
	UAnimMontage* WeaponMontage,
	UParticleSystem* ParticleSystem,
	const float TriggerTime,
	const FName SocketName)
{
	if (!WeaponMontage || !ParticleSystem || SocketName.IsNone())
	{
		return false;
	}

	WeaponMontage->Modify();
	const float ClampedTime = FMath::Clamp(TriggerTime, 0.0f, WeaponMontage->GetPlayLength());
	UAnimNotify_PlayParticleEffect* ParticleNotify = nullptr;
	for (FAnimNotifyEvent& NotifyEvent : WeaponMontage->Notifies)
	{
		if (NotifyEvent.Notify && NotifyEvent.Notify->IsA<UAnimNotify_PlayParticleEffect>() &&
			FMath::IsNearlyEqual(NotifyEvent.GetTriggerTime(), ClampedTime, 0.0005f))
		{
			UAnimNotify_PlayParticleEffect* ExistingParticleNotify =
				Cast<UAnimNotify_PlayParticleEffect>(NotifyEvent.Notify);
			if (!ExistingParticleNotify ||
				ExistingParticleNotify->PSTemplate != ParticleSystem ||
				ExistingParticleNotify->SocketName != SocketName)
			{
				continue;
			}

			ParticleNotify = ExistingParticleNotify;
			NotifyEvent.SetTime(ClampedTime);
			break;
		}
	}

	if (!ParticleNotify)
	{
		FAnimNotifyEvent& NotifyEvent = WeaponMontage->Notifies.AddDefaulted_GetRef();
		ParticleNotify = NewObject<UAnimNotify_PlayParticleEffect>(WeaponMontage);
		NotifyEvent.Notify = ParticleNotify;
		NotifyEvent.TrackIndex = 0;
		NotifyEvent.Link(WeaponMontage, ClampedTime);
	}

	ParticleNotify->PSTemplate = ParticleSystem;
	ParticleNotify->SocketName = SocketName;
	ParticleNotify->Attached = true;
	ParticleNotify->LocationOffset = FVector::ZeroVector;
	ParticleNotify->RotationOffset = FRotator::ZeroRotator;
	ParticleNotify->Scale = FVector::OneVector;
	WeaponMontage->SortNotifies();
	WeaponMontage->RefreshCacheData();
	WeaponMontage->MarkPackageDirty();
	return true;
}

void UShootAnimNotify_PlayWeaponMontage::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* CharacterOwner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAnimInstance* CharacterAnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	const UAnimMontage* CharacterMontage = Cast<UAnimMontage>(Animation);
	if (!CharacterOwner || !CharacterAnimInstance || !CharacterMontage || !MontageToPlay)
	{
		return;
	}

	UShootEquipmentManagerComponent* EquipmentManager =
		CharacterOwner->FindComponentByClass<UShootEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		return;
	}

	// EquipmentManager 的已装备实例是 Pawn 表现层的权威入口；不要从 Controller QuickBar
	// 反查武器，否则 Listen Server 的模拟代理和本地分屏第二名玩家会读到错误的本地状态。
	const TArray<UShootEquipmentInstance*> WeaponInstances =
		EquipmentManager->GetEquipmentInstancesOfType(UShootWeaponInstance::StaticClass());
	for (UShootEquipmentInstance* EquipmentInstance : WeaponInstances)
	{
		if (!EquipmentInstance)
		{
			continue;
		}

		for (AActor* SpawnedActor : EquipmentInstance->GetSpawnedActors())
		{
			if (!SpawnedActor)
			{
				continue;
			}

			TInlineComponentArray<USkeletalMeshComponent*> WeaponMeshes;
			SpawnedActor->GetComponents(WeaponMeshes);
			for (USkeletalMeshComponent* WeaponMesh : WeaponMeshes)
			{
				UAnimInstance* WeaponAnimInstance = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
				if (!WeaponAnimInstance)
				{
					continue;
				}

				// MontageSync_Follow 要求 leader 与 follower 都已经处于播放状态，因此先播放武器
				// 蒙太奇，再让它跟随触发本 Notify 的正式 CC 角色蒙太奇。
				if (WeaponAnimInstance->Montage_Play(MontageToPlay, PlayRate) > 0.0f)
				{
					WeaponAnimInstance->MontageSync_Follow(
						MontageToPlay,
						CharacterAnimInstance,
						CharacterMontage);
					return;
				}
			}
		}
	}
}

FString UShootAnimNotify_PlayWeaponMontage::GetNotifyName_Implementation() const
{
	return MontageToPlay
		? FString::Printf(TEXT("Play Weapon: %s"), *MontageToPlay->GetName())
		: TEXT("Play Weapon Montage");
}
