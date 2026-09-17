// Copyright ZhaoYiJie

#include "Weapons/ShootRocketLauncherWeaponInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRocketLauncherWeaponInstance)

UShootRocketLauncherWeaponInstance::UShootRocketLauncherWeaponInstance(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootRocketLauncherWeaponInstance::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UShootRocketLauncherWeaponInstance, HeldRocketActor);
	DOREPLIFETIME(UShootRocketLauncherWeaponInstance, bRocketReloadInHand);
}

void UShootRocketLauncherWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// Rocket_Launcher_A 的 SkeletalMesh 自带一个跟随 Ammo 骨骼的动画导向网格。
	// 弹匣为空时没有外部投射物可替代它，先隐藏该导向网格；有弹时等外部
	// HeldRocketActor 成功挂接后再隐藏，保留资产配置失败时的可见回退。
	if (GetCurrentAmmo() <= 0)
	{
		HideIntegratedAmmoBone(FindWeaponMeshComponent());
	}

	// 只有服务器创建权威弹头；Actor 自身复制到远端后由 OnRep/Attachment 维持表现。
	if (HasAuthority() && GetCurrentAmmo() > 0)
	{
		EnsureHeldRocketAttached();
	}

	AttachHeldRocketToCurrentTarget();
}

void UShootRocketLauncherWeaponInstance::OnUnequipped()
{
	bRocketReloadInHand = false;
	DestroyHeldRocket();
	Super::OnUnequipped();
}

void UShootRocketLauncherWeaponInstance::ReloadAmmo(const int32 Amount)
{
	const int32 AmmoBeforeReload = GetCurrentAmmo();
	Super::ReloadAmmo(Amount);

	// 通用 Reload GA 仍只负责在 AN_Reload 事件时调用 ReloadAmmo；Rocket 专用
	// 表现挂接放在本类，避免改动受保护的通用换弹能力。
	if (HasAuthority() && GetCurrentAmmo() > AmmoBeforeReload)
	{
		// 如果 RocketInsert Notify 漏配，AN_Reload 仍然是安全兜底：库存已经
		// 结算，此时把临时手部弹头收回武器；正常路径在这里已经是 false。
		if (bRocketReloadInHand && HeldRocketActor)
		{
			bRocketReloadInHand = false;
			AttachHeldRocketToWeapon();
		}
		else
		{
			EnsureHeldRocketAttached();
		}
	}
}

void UShootRocketLauncherWeaponInstance::PrepareRocketForReload()
{
	if (!HasAuthority()
		|| GetCurrentAmmo() >= GetMagazineSize()
		|| GetCurrentReserve() <= 0)
	{
		return;
	}

	bRocketReloadInHand = true;
	EnsureHeldRocketAttached();

	if (HeldRocketActor)
	{
		AttachHeldRocketToReloadHand();
	}
	else
	{
		// 资产或运行时网格不完整时不要留下“正在手持弹头”的复制状态。
		bRocketReloadInHand = false;
	}
}

void UShootRocketLauncherWeaponInstance::CommitRocketReload()
{
	if (!HeldRocketActor)
	{
		return;
	}

	bRocketReloadInHand = false;
	AttachHeldRocketToWeapon();
}

void UShootRocketLauncherWeaponInstance::CancelRocketReload()
{
	if (!bRocketReloadInHand)
	{
		return;
	}

	bRocketReloadInHand = false;
	if (GetCurrentAmmo() > 0 && HeldRocketActor)
	{
		AttachHeldRocketToWeapon();
	}
	else
	{
		DestroyHeldRocket();
	}
}

AShootProjectileBase* UShootRocketLauncherWeaponInstance::TakeProjectileForLaunch(
	const FTransform& SpawnTransform)
{
	if (!HasAuthority() || !HeldRocketActor)
	{
		return nullptr;
	}

	AShootProjectileBase* ProjectileToLaunch = HeldRocketActor;
	HeldRocketActor = nullptr;

	// InitializeProjectile 会在脱离后重新开启碰撞、移动和飞行表现；这里仅保留
	// 当前弹头 Actor，避免生成新的可见网格。
	ProjectileToLaunch->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bRocketReloadInHand = false;
	ProjectileToLaunch->SetActorTransform(
		SpawnTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	return ProjectileToLaunch;
}

void UShootRocketLauncherWeaponInstance::OnInstigatorReplicated()
{
	Super::OnInstigatorReplicated();

	// EquipmentInstance 与 SpawnedActor 的复制顺序没有固定保证；如果弹头引用
	// 先到，补一次挂接即可。真正的挂接状态也会由 Actor Attachment 复制维持。
	AttachHeldRocketToCurrentTarget();
}

void UShootRocketLauncherWeaponInstance::EnsureHeldRocketAttached()
{
	// 正常装备/换弹结算前，只要当前弹匣或备弹仍有弹，就需要维持一枚可见弹头。
	// 火箭筒是一发一装：整发换弹会在当前弹匣为 0、备弹仍大于 0 时先把新弹头
	// 生成到角色手里，因此这里不能只看当前弹匣。
	if (!HasAuthority() || HeldRocketActor
		|| (GetCurrentAmmo() <= 0 && GetCurrentReserve() <= 0))
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* OwnerPawn = GetPawn();
	USkeletalMeshComponent* WeaponMesh = FindWeaponMeshComponent();
	TSubclassOf<AShootProjectileBase> ProjectileClass = GetProjectileClass();
	if (!World || !OwnerPawn || !WeaponMesh || !ProjectileClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Rocket ammo attach skipped: missing World/Pawn/weapon mesh/projectile class on %s."),
			*GetNameSafe(this));
		return;
	}

	const FName SocketName = ResolveRocketAmmoSocket(WeaponMesh);
	if (SocketName.IsNone())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Rocket ammo attach skipped: socket %s is missing on the weapon mesh for %s."),
			*RocketAmmoSocketName.ToString(),
			*GetNameSafe(this));
		return;
	}

	const FTransform SpawnTransform = WeaponMesh->GetSocketTransform(SocketName, RTS_World);
	AShootProjectileBase* NewRocket = World->SpawnActorDeferred<AShootProjectileBase>(
		ProjectileClass,
		SpawnTransform,
		OwnerPawn,
		OwnerPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewRocket)
	{
		return;
	}

	NewRocket->PrepareAsHeldProjectile(GetProjectileConfig());
	NewRocket->FinishSpawning(SpawnTransform, true);
	HeldRocketActor = NewRocket;
	AttachHeldRocketToCurrentTarget();
}

void UShootRocketLauncherWeaponInstance::HideIntegratedAmmoBone(
	USkeletalMeshComponent* WeaponMesh) const
{
	if (!WeaponMesh || RocketAmmoBoneName.IsNone()
		|| WeaponMesh->GetBoneIndex(RocketAmmoBoneName) == INDEX_NONE)
	{
		return;
	}

	WeaponMesh->HideBoneByName(RocketAmmoBoneName, PBO_None);
}

void UShootRocketLauncherWeaponInstance::AttachHeldRocketToWeapon()
{
	if (!HeldRocketActor)
	{
		return;
	}

	USkeletalMeshComponent* WeaponMesh = FindWeaponMeshComponent();
	if (!WeaponMesh)
	{
		return;
	}
	const FName SocketName = ResolveRocketAmmoSocket(WeaponMesh);
	if (SocketName.IsNone())
	{
		return;
	}
	HideIntegratedAmmoBone(WeaponMesh);

	HeldRocketActor->AttachToComponent(
		WeaponMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName);
	HeldRocketActor->SetActorRelativeTransform(RocketAmmoRelativeTransform);
}

void UShootRocketLauncherWeaponInstance::AttachHeldRocketToReloadHand()
{
	if (!HeldRocketActor)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = FindCharacterMeshComponent();
	if (!CharacterMesh || RocketReloadHandSocketName.IsNone())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Rocket reload hand attach skipped: character mesh or hand socket is missing on %s."),
			*GetNameSafe(this));
		return;
	}

	if (!CharacterMesh->DoesSocketExist(RocketReloadHandSocketName)
		&& CharacterMesh->GetBoneIndex(RocketReloadHandSocketName) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Rocket reload hand attach skipped: socket/bone %s is missing on character mesh for %s."),
			*RocketReloadHandSocketName.ToString(),
			*GetNameSafe(this));
		return;
	}

	HeldRocketActor->AttachToComponent(
		CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		RocketReloadHandSocketName);
	HeldRocketActor->SetActorRelativeTransform(RocketReloadHandRelativeTransform);
}

void UShootRocketLauncherWeaponInstance::AttachHeldRocketToCurrentTarget()
{
	if (bRocketReloadInHand)
	{
		AttachHeldRocketToReloadHand();
	}
	else
	{
		AttachHeldRocketToWeapon();
	}
}

USkeletalMeshComponent* UShootRocketLauncherWeaponInstance::FindWeaponMeshComponent() const
{
	for (AActor* SpawnedActor : GetSpawnedActors())
	{
		if (!SpawnedActor)
		{
			continue;
		}

		TInlineComponentArray<USkeletalMeshComponent*> MeshComponents;
		SpawnedActor->GetComponents(MeshComponents);
		for (USkeletalMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent && MeshComponent->GetSkeletalMeshAsset())
			{
				return MeshComponent;
			}
		}
	}

	return nullptr;
}

USkeletalMeshComponent* UShootRocketLauncherWeaponInstance::FindCharacterMeshComponent() const
{
	APawn* OwnerPawn = GetPawn();
	if (!OwnerPawn)
	{
		return nullptr;
	}

	if (const ACharacter* Character = Cast<ACharacter>(OwnerPawn))
	{
		return Character->GetMesh();
	}

	TInlineComponentArray<USkeletalMeshComponent*> MeshComponents;
	OwnerPawn->GetComponents(MeshComponents);
	for (USkeletalMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && MeshComponent->GetSkeletalMeshAsset())
		{
			return MeshComponent;
		}
	}

	return nullptr;
}

FName UShootRocketLauncherWeaponInstance::ResolveRocketAmmoSocket(
	const USkeletalMeshComponent* WeaponMesh) const
{
	if (!WeaponMesh)
	{
		return NAME_None;
	}

	if (!RocketAmmoSocketName.IsNone() && WeaponMesh->DoesSocketExist(RocketAmmoSocketName))
	{
		return RocketAmmoSocketName;
	}

	// RocketAmmo 必须是独立插槽；不能回退到 MuzzleFlash，否则资产接线错误
	// 会被伪装成“能发射但弹头位置不对”。
	return NAME_None;
}

void UShootRocketLauncherWeaponInstance::DestroyHeldRocket()
{
	if (HeldRocketActor)
	{
		HeldRocketActor->Destroy();
		HeldRocketActor = nullptr;
	}
}

void UShootRocketLauncherWeaponInstance::OnRep_HeldRocketActor()
{
	AttachHeldRocketToCurrentTarget();
}

void UShootRocketLauncherWeaponInstance::OnRep_RocketReloadInHand()
{
	AttachHeldRocketToCurrentTarget();
}
