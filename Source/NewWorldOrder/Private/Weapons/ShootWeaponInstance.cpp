// Copyright ZhaoYiJie

#include "Weapons/ShootWeaponInstance.h"
#include "Character/ShootCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWeaponInstance)

UShootWeaponInstance::UShootWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// 记录装备时间
	UWorld* World = GetWorld();
	TimeLastEquipped = World ? World->GetTimeSeconds() : 0.0;

	// 与 Lyra 相同，由装备实例选择并链接 Item AnimLayer；项目层直接作用于正式 CC Mesh。
	// Weapon、Hair、Shoe 分属不同接口，武器切换不会覆盖 Mutable 的头发和鞋子表现。
	RefreshAnimLayer(true);

	HandleVisualAnimCue(EShootWeaponAnimAction::Show);

	// 对齐 Lyra B_WeaponInstance_Base：先链接武器层，再在角色主 Mesh 播放对应骨架的 Equip Montage。
	// EquipmentList 的 FastArray 与 Instance 的 Instigator 分别复制，客户端可能先进入这里；
	// 具体 Montage 配置定义在 ItemDefinition 的 RangedWeaponConfig 中，必须等 Instigator 到达后才能解析。
	UAnimMontage* EquipMontage = GetCharacterEquipMontage();
	bEquipMontageAwaitingInstigator = EquipMontage == nullptr && GetInstigator() == nullptr;
	PlayCharacterTransitionMontage(EquipMontage, GetCharacterEquipMontageStartPosition());
}

void UShootWeaponInstance::OnUnequipped()
{
	bEquipMontageAwaitingInstigator = false;

	// 与 Lyra B_WeaponInstance_Base 一致：服务器卸装以及客户端 FastArray
	// PreReplicatedRemove 都在该实例所属 Pawn 上应用对应性别的 Unarmed 层。
	RefreshAnimLayer(false);
	PlayCharacterTransitionMontage(GetCharacterUnequipMontage());

	HandleVisualAnimCue(EShootWeaponAnimAction::Hide);

	Super::OnUnequipped();
}

void UShootWeaponInstance::OnInstigatorReplicated()
{
	Super::OnInstigatorReplicated();

	if (!bEquipMontageAwaitingInstigator || GetInstigator() == nullptr)
	{
		return;
	}

	// 若 Instigator 先于 FastArray 到达，OnEquipped 会直接播放且不会设置待补标记；
	// 只有 FastArray 先到的客户端才会进入这里，因此不会把正常 Equip Montage 播放两次。
	bEquipMontageAwaitingInstigator = false;
	PlayCharacterTransitionMontage(GetCharacterEquipMontage(), GetCharacterEquipMontageStartPosition());
}

void UShootWeaponInstance::UpdateFiringTime()
{
	UWorld* World = GetWorld();
	TimeLastFired = World ? World->GetTimeSeconds() : 0.0;
}

double UShootWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	UWorld* World = GetWorld();
	const double CurrentTime = World ? World->GetTimeSeconds() : 0.0;
	const double LastInteractTime = FMath::Max(TimeLastEquipped, TimeLastFired);
	return CurrentTime - LastInteractTime;
}

bool UShootWeaponInstance::HasAuthority() const
{
	if (const AActor* OwnerActor = GetTypedOuter<AActor>())
	{
	 return OwnerActor->HasAuthority();
	}
	return false;
}

void UShootWeaponInstance::HandleVisualAnimCue(EShootWeaponAnimAction Action)
{
	const TArray<AActor*> SpawnedActorList = GetSpawnedActors();
	for (AActor* Actor : SpawnedActorList)
	{
		if (!Actor)
		{
			continue;
		}

		switch (Action)
		{
		case EShootWeaponAnimAction::Show:
			Actor->SetActorHiddenInGame(false);
			Actor->SetActorEnableCollision(true);
			break;
		case EShootWeaponAnimAction::Hide:
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
			break;
		case EShootWeaponAnimAction::Destroy:
			Actor->Destroy();
			break;
		default:
			break;
		}
	}
}

void UShootWeaponInstance::RefreshAnimLayer(bool bEquipped) const
{
	if (AShootCharacter* Character = Cast<AShootCharacter>(GetPawn()))
	{
		Character->ApplyWeaponPresentation(bEquipped ? EquippedAnimSet : UnequippedAnimSet, bEquipped);
	}
}

TSubclassOf<UAnimInstance> UShootWeaponInstance::PickBestAnimLayer(bool bEquipped,
	const FGameplayTagContainer& CosmeticTags) const
{
	const FShootAnimLayerSelectionSet& SetToQuery = bEquipped ? EquippedAnimSet : UnequippedAnimSet;
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

UAnimMontage* UShootWeaponInstance::GetCharacterEquipMontage() const
{
	return nullptr;
}

UAnimMontage* UShootWeaponInstance::GetCharacterUnequipMontage() const
{
	return nullptr;
}

float UShootWeaponInstance::GetCharacterEquipMontageStartPosition() const
{
	return EquipMontageStartPosition;
}

void UShootWeaponInstance::PlayCharacterTransitionMontage(
	UAnimMontage* MontageToPlay,
	const float StartPosition) const
{
	if (!MontageToPlay)
	{
		return;
	}

	// Ability Montage 会由 ASC 处理；Equip/Unequip 则与 Lyra 蓝图一致，直接在角色主 Mesh 播放。
	// Mutable 主 Mesh 可能使用瞬态 Skeleton，但 ItemDefinition 已通过身体 Mesh 的正式 Skeleton 选出兼容 Montage。
	if (const ACharacter* Character = Cast<ACharacter>(GetPawn()))
	{
		if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
			{
				// 起播位置由当前武器实例蓝图提供；男女共用同一重定向动作的安全过渡点。
				// 这样只调整数据，不会把角色性别或枪型条件写进播放入口。
				const float ClampedStartPosition = FMath::Clamp(StartPosition, 0.0f, MontageToPlay->GetPlayLength());
				AnimInstance->Montage_Play(
					MontageToPlay,
					1.0f,
					EMontagePlayReturnType::MontageLength,
					ClampedStartPosition,
					true);
			}
		}
	}
}
