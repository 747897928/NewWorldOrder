// Copyright ZhaoYiJie

#include "Weapons/ShootRangedWeaponInstance.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Inventory/ShootInventoryItemDefinition_Weapon.h"
#include "Inventory/Fragments/ShootInventoryFragment_RangedWeaponConfig.h"
#include "Inventory/Fragments/ShootInventoryFragment_ProjectileWeaponConfig.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "ShootGameplayTags.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRangedWeaponInstance)

namespace
{
	static const FProjectileWeaponConfig GRangedWeaponDefaultProjectileConfig;
}

UShootRangedWeaponInstance::UShootRangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootRangedWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	if (UsesHeatSpreadModel())
	{
		float MinHeat = 0.0f;
		float MaxHeat = 0.0f;
		ComputeHeatRange(MinHeat, MaxHeat);
		// Lyra 的武器实例通常只在真正授予/移除装备时进入 OnEquipped；本项目 RuntimeOnly
		// QuickBar 切槽会对同一实例反复 Unequip/Equip。若照搬 Lyra 的热量中点，每次切枪
		// 都会凭空制造一次高散布，并让准星表现成“从最大值恢复”。装备动画已经提供切枪
		// 时间窗口，因此项目层从曲线最小热量开始，且准星与服务器实际 Trace 使用同一数值。
		CurrentHeat = MinHeat;
		CurrentSpreadAngle = GetConfiguredHeatToSpreadCurve().GetRichCurveConst()->Eval(CurrentHeat);
		if (const UWorld* World = GetWorld())
		{
			// 对齐 Lyra：装备后的热量会按冷却曲线自然回落，等待后可获得首发精准。
			LastFireTime = World->GetTimeSeconds() - GetConfiguredRecoveryDelay();
		}
	}

	// 与 Lyra 一致：每次装备从无加成状态开始，由 Tick 平滑逼近站立、蹲伏、腾空和瞄准倍率。
	CurrentSpreadAngleMultiplier = 1.0f;
	StandingStillMultiplier = 1.0f;
	JumpFallMultiplier = 1.0f;
	CrouchingMultiplier = 1.0f;
	bHasFirstShotAccuracy = false;
}

void UShootRangedWeaponInstance::Tick(float DeltaTime)
{
	if (!GetPawn())
	{
		return;
	}

	const bool bSpreadAtMinimum = UpdateSpread(DeltaTime);
	const bool bMultipliersAtMinimum = UpdateMultipliers(DeltaTime);
	bHasFirstShotAccuracy = GetConfiguredFirstShotAccuracy() && bSpreadAtMinimum && bMultipliersAtMinimum;
}

void UShootRangedWeaponInstance::ReloadAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	const int32 MagazineCapacity = GetMagazineSize();
	const int32 CurrentMag = GetCurrentAmmo();
	const int32 RoomInMag = FMath::Max(0, MagazineCapacity - CurrentMag);
	int32 ReserveCount = 0;
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		// 服务器：从后备弹药 Tag 读取当前可用弹量，避免 Reserve 为 0 时空加子弹
		ReserveCount = ItemInstance->GetStatTagStackCount(FShootGameplayTags::Get().Inventory_Ammo_Reserve);
	}

	const int32 AmmoToReload = FMath::Min3(RoomInMag, Amount, ReserveCount);

	if (AmmoToReload <= 0)
	{
		return;
	}

	if (UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		ItemInstance->AddStatTagStack(FShootGameplayTags::Get().Inventory_Ammo_Magazine, AmmoToReload);
		ItemInstance->RemoveStatTagStack(FShootGameplayTags::Get().Inventory_Ammo_Reserve, AmmoToReload);
	}
}

int32 UShootRangedWeaponInstance::AddReserveAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 ReserveCapacity = GetReserveCapacity();
	const int32 RoomInReserve = FMath::Max(0, ReserveCapacity - GetCurrentReserve());
	const int32 AmmoToAdd = FMath::Min(RoomInReserve, Amount);
	if (AmmoToAdd <= 0)
	{
		return 0;
	}

	if (UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		ItemInstance->AddStatTagStack(FShootGameplayTags::Get().Inventory_Ammo_Reserve, AmmoToAdd);
		return AmmoToAdd;
	}
	return 0;
}

int32 UShootRangedWeaponInstance::AddMagazineAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 MagazineCapacity = GetMagazineSize();
	const int32 CurrentMag = GetCurrentAmmo();
	const int32 RoomInMag = FMath::Max(0, MagazineCapacity - CurrentMag);
	const int32 AmmoToReload = FMath::Min(RoomInMag, Amount);

	if (AmmoToReload <= 0)
	{
		return 0;
	}

	if (UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		ItemInstance->AddStatTagStack(FShootGameplayTags::Get().Inventory_Ammo_Magazine, AmmoToReload);
		return AmmoToReload;
	}

	return 0;
}

bool UShootRangedWeaponInstance::RefillAmmoToCapacity()
{
	if (!GetPawn() || !GetPawn()->HasAuthority())
	{
		return false;
	}

	const int32 MagazineAdded = AddMagazineAmmo(FMath::Max(0, GetMagazineSize() - GetCurrentAmmo()));
	const int32 ReserveAdded = AddReserveAmmo(FMath::Max(0, GetReserveCapacity() - GetCurrentReserve()));
	return MagazineAdded > 0 || ReserveAdded > 0;
}

int32 UShootRangedWeaponInstance::GetCurrentAmmo() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		return ItemInstance->GetStatTagStackCount(FShootGameplayTags::Get().Inventory_Ammo_Magazine);
	}
	return 0;
}

int32 UShootRangedWeaponInstance::GetCurrentReserve() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		return ItemInstance->GetStatTagStackCount(FShootGameplayTags::Get().Inventory_Ammo_Reserve);
	}
	return 0;
}

FVector UShootRangedWeaponInstance::GetMuzzleLocation() const
{
	const TArray<AActor*> SpawnedActorList = GetSpawnedActors();
	if (SpawnedActorList.Num() > 0 && SpawnedActorList[0])
	{
		if (USkeletalMeshComponent* WeaponMesh = SpawnedActorList[0]->FindComponentByClass<USkeletalMeshComponent>())
		{
			const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig();
			const FName MuzzleSocket = (RangedCfg && RangedCfg->MuzzleSocketName != NAME_None)
				                           ? RangedCfg->MuzzleSocketName
				                           : FName(TEXT("MuzzleFlash"));
			if (WeaponMesh->DoesSocketExist(MuzzleSocket))
			{
				return WeaponMesh->GetSocketLocation(MuzzleSocket);
			}
			return WeaponMesh->GetComponentLocation();
		}
	}
	return FVector::ZeroVector;
}

int32 UShootRangedWeaponInstance::GetBulletsPerCartridge() const
{
	if (const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig())
	{
		return RangedCfg->BulletsPerCartridge;
	}
	return 1;
}

float UShootRangedWeaponInstance::GetBulletTraceSweepRadius() const
{
	if (const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig())
	{
		return RangedCfg->BulletTraceSweepRadius;
	}
	return 0.0f;
}

float UShootRangedWeaponInstance::GetMaxDamageRange() const
{
	if (const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig())
	{
		return FMath::Max(0.0f, RangedCfg->MaxRange);
	}
	return 10000.0f;
}

float UShootRangedWeaponInstance::GetSpreadExponent() const
{
	return SpreadExponent;
}

int32 UShootRangedWeaponInstance::GetMagazineSize() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		return FMath::Max(0, ItemInstance->GetStatTagStackCount(
			FShootGameplayTags::Get().Inventory_Ammo_MagazineCapacity));
	}
	return 0;
}

int32 UShootRangedWeaponInstance::GetReserveCapacity() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		return FMath::Max(0, ItemInstance->GetStatTagStackCount(
			FShootGameplayTags::Get().Inventory_Ammo_ReserveCapacity));
	}
	return 0;
}

UAnimMontage* UShootRangedWeaponInstance::GetCharacterFireMontage() const
{
	return FindCharacterMontage(EShootCharacterMontageAction::Fire);
}

UAnimMontage* UShootRangedWeaponInstance::GetCharacterReloadMontage() const
{
	return FindCharacterMontage(EShootCharacterMontageAction::Reload);
}

UAnimMontage* UShootRangedWeaponInstance::GetCharacterEquipMontage() const
{
	return FindCharacterMontage(EShootCharacterMontageAction::Equip);
}

UAnimMontage* UShootRangedWeaponInstance::GetCharacterUnequipMontage() const
{
	return FindCharacterMontage(EShootCharacterMontageAction::Unequip);
}

UAnimMontage* UShootRangedWeaponInstance::FindCharacterMontage(const EShootCharacterMontageAction Action) const
{
	const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig();
	const ACharacter* Character = Cast<ACharacter>(GetPawn());
	if (!RangedCfg || !Character)
	{
		return nullptr;
	}

	TArray<const USkeleton*, TInlineAllocator<4>> CharacterSkeletons;
	TInlineComponentArray<USkeletalMeshComponent*> CharacterMeshes(Character);
	for (const USkeletalMeshComponent* CharacterMesh : CharacterMeshes)
	{
		const USkeleton* Skeleton = CharacterMesh && CharacterMesh->GetSkeletalMeshAsset()
			? CharacterMesh->GetSkeletalMeshAsset()->GetSkeleton()
			: nullptr;
		if (Skeleton)
		{
			CharacterSkeletons.AddUnique(Skeleton);
		}
	}

	for (const FShootWeaponCharacterMontageSet& MontageSet : RangedCfg->CharacterMontages)
	{
		if (MontageSet.TargetSkeleton && CharacterSkeletons.Contains(MontageSet.TargetSkeleton))
		{
			switch (Action)
			{
			case EShootCharacterMontageAction::Fire:
				return MontageSet.FireMontage;
			case EShootCharacterMontageAction::Reload:
				return MontageSet.ReloadMontage;
			case EShootCharacterMontageAction::Equip:
				return MontageSet.EquipMontage;
			case EShootCharacterMontageAction::Unequip:
				return MontageSet.UnequipMontage;
			default:
				return nullptr;
			}
		}
	}

	// Mutable 可能给主 CharacterMesh0 生成瞬态 Skeleton，而 Body Mesh 仍持有正式 CC Skeleton。
	// 因此这里检查 Pawn 上全部 SkeletalMeshComponent，但仍只接受 ItemDefinition 中显式配置的精确 Skeleton；
	// 配置缺项时宁可不播，也不通过玩家序号、性别或资产路径硬编码猜测资源。
	return nullptr;
}

void UShootRangedWeaponInstance::AddSpread()
{
	if (!UsesHeatSpreadModel())
	{
		return;
	}

	const float HeatPerShot = GetConfiguredHeatPerShotCurve().GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);
	CurrentSpreadAngle = GetConfiguredHeatToSpreadCurve().GetRichCurveConst()->Eval(CurrentHeat);
	if (const UWorld* World = GetWorld())
	{
		LastFireTime = World->GetTimeSeconds();
	}
	UpdateFiringTime();
}

float UShootRangedWeaponInstance::GetCalculatedSpreadAngle() const
{
	return CurrentSpreadAngle;
}

float UShootRangedWeaponInstance::GetCalculatedSpreadAngleMultiplier() const
{
	if (!UsesHeatSpreadModel())
	{
		return 1.0f;
	}

	return bHasFirstShotAccuracy ? 0.0f : CurrentSpreadAngleMultiplier;
}

bool UShootRangedWeaponInstance::HasFirstShotAccuracy() const
{
	return bHasFirstShotAccuracy;
}

void UShootRangedWeaponInstance::SetAimingAlpha(float InAimingAlpha)
{
	AimingAlpha = FMath::Clamp(InAimingAlpha, 0.0f, 1.0f);
}

bool UShootRangedWeaponInstance::SupportsFirstPersonADS() const
{
	const UShootInventoryItemInstance* ItemInstance = GetItemInstance();
	const TSubclassOf<UShootInventoryItemDefinition> ItemDefinitionClass =
		ItemInstance ? ItemInstance->GetItemDef() : nullptr;
	const UShootInventoryItemDefinition_Weapon* WeaponDefinition = ItemDefinitionClass
		? Cast<UShootInventoryItemDefinition_Weapon>(ItemDefinitionClass->GetDefaultObject())
		: nullptr;
	return WeaponDefinition
		&& WeaponDefinition->WeaponCategory == EShootWeaponItemCategory::Sniper;
}

FVector2D UShootRangedWeaponInstance::GetLocalCameraRecoil() const
{
	if (const UShootInventoryFragment_RangedWeaponConfig* RangedCfg = GetRangedConfig())
	{
		return FVector2D(
			FMath::FRandRange(RangedCfg->LocalCameraYawRecoilMin, RangedCfg->LocalCameraYawRecoilMax),
			FMath::FRandRange(RangedCfg->LocalCameraPitchRecoilMin, RangedCfg->LocalCameraPitchRecoilMax));
	}

	return FVector2D::ZeroVector;
}

bool UShootRangedWeaponInstance::UsesInstanceHeatSpreadModel() const
{
	return HeatToSpreadCurve.GetRichCurveConst()->HasAnyData()
		&& HeatToHeatPerShotCurve.GetRichCurveConst()->HasAnyData()
		&& HeatToCoolDownPerSecondCurve.GetRichCurveConst()->HasAnyData();
}

bool UShootRangedWeaponInstance::UsesHeatSpreadModel() const
{
	return UsesInstanceHeatSpreadModel();
}

const FRuntimeFloatCurve& UShootRangedWeaponInstance::GetConfiguredHeatToSpreadCurve() const
{
	if (UsesInstanceHeatSpreadModel())
	{
		return HeatToSpreadCurve;
	}

	static const FRuntimeFloatCurve EmptyCurve;
	return EmptyCurve;
}

const FRuntimeFloatCurve& UShootRangedWeaponInstance::GetConfiguredHeatPerShotCurve() const
{
	if (UsesInstanceHeatSpreadModel())
	{
		return HeatToHeatPerShotCurve;
	}

	static const FRuntimeFloatCurve EmptyCurve;
	return EmptyCurve;
}

const FRuntimeFloatCurve& UShootRangedWeaponInstance::GetConfiguredCooldownCurve() const
{
	if (UsesInstanceHeatSpreadModel())
	{
		return HeatToCoolDownPerSecondCurve;
	}

	static const FRuntimeFloatCurve EmptyCurve;
	return EmptyCurve;
}

float UShootRangedWeaponInstance::GetConfiguredRecoveryDelay() const
{
	if (UsesInstanceHeatSpreadModel())
	{
		return SpreadRecoveryCooldownDelay;
	}

	return 0.0f;
}

bool UShootRangedWeaponInstance::GetConfiguredFirstShotAccuracy() const
{
	if (UsesInstanceHeatSpreadModel())
	{
		return bAllowFirstShotAccuracy;
	}

	return false;
}

void UShootRangedWeaponInstance::ComputeHeatRange(float& OutMinHeat, float& OutMaxHeat) const
{
	OutMinHeat = 0.0f;
	OutMaxHeat = 0.0f;
	if (UsesHeatSpreadModel())
	{
		float MinHeat = 0.0f;
		float MaxHeat = 0.0f;
		GetConfiguredHeatToSpreadCurve().GetRichCurveConst()->GetTimeRange(OutMinHeat, OutMaxHeat);
		GetConfiguredHeatPerShotCurve().GetRichCurveConst()->GetTimeRange(MinHeat, MaxHeat);
		OutMinHeat = FMath::Min(OutMinHeat, MinHeat);
		OutMaxHeat = FMath::Max(OutMaxHeat, MaxHeat);
		GetConfiguredCooldownCurve().GetRichCurveConst()->GetTimeRange(MinHeat, MaxHeat);
		OutMinHeat = FMath::Min(OutMinHeat, MinHeat);
		OutMaxHeat = FMath::Max(OutMaxHeat, MaxHeat);
	}
}

float UShootRangedWeaponInstance::ClampHeat(float NewHeat) const
{
	float MinHeat = 0.0f;
	float MaxHeat = 0.0f;
	ComputeHeatRange(MinHeat, MaxHeat);
	return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
}

void UShootRangedWeaponInstance::ComputeSpreadRange(float& OutMinSpread, float& OutMaxSpread) const
{
	OutMinSpread = 0.0f;
	OutMaxSpread = 0.0f;
	if (UsesHeatSpreadModel())
	{
		GetConfiguredHeatToSpreadCurve().GetRichCurveConst()->GetValueRange(OutMinSpread, OutMaxSpread);
	}
}

bool UShootRangedWeaponInstance::UpdateSpread(float DeltaTime)
{
	if (UsesHeatSpreadModel())
	{
		if (const UWorld* World = GetWorld();
			World && World->TimeSince(LastFireTime) > GetConfiguredRecoveryDelay())
		{
			const float CooldownRate = GetConfiguredCooldownCurve().GetRichCurveConst()->Eval(CurrentHeat);
			CurrentHeat = ClampHeat(CurrentHeat - CooldownRate * DeltaTime);
			CurrentSpreadAngle = GetConfiguredHeatToSpreadCurve().GetRichCurveConst()->Eval(CurrentHeat);
		}

		float MinSpread = 0.0f;
		float MaxSpread = 0.0f;
		ComputeSpreadRange(MinSpread, MaxSpread);
		return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
	}

	return true;
}

bool UShootRangedWeaponInstance::UpdateMultipliers(float DeltaTime)
{
	// 姿态倍率属于 WeaponInstance 的热量散布模型。RangedWeaponConfig Fragment 只保留
	// 弹药、射程、动画、弹道和本地后坐力等结构化配置，不再承载散布/倍率曲线。
	if (!UsesHeatSpreadModel())
	{
		CurrentSpreadAngleMultiplier = 1.0f;
		return true;
	}

	constexpr float MultiplierNearlyEqualThreshold = 0.05f;
	APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		return false;
	}

	UCharacterMovementComponent* CharacterMovement =
		Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent());

	// 与 Lyra 相同：速度在阈值到阈值+范围之间，把静止奖励平滑羽化回 1。
	const float PawnSpeed = Pawn->GetVelocity().Size();
	const float MovementTargetValue = FMath::GetMappedRangeValueClamped(
		FVector2D(StandingStillSpeedThreshold, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange),
		FVector2D(SpreadAngleMultiplier_StandingStill, 1.0f),
		PawnSpeed);
	StandingStillMultiplier = FMath::FInterpTo(
		StandingStillMultiplier, MovementTargetValue, DeltaTime, TransitionRate_StandingStill);
	const bool bStandingStillAtMinimum = FMath::IsNearlyEqual(
		StandingStillMultiplier,
		SpreadAngleMultiplier_StandingStill,
		FMath::Max(KINDA_SMALL_NUMBER, SpreadAngleMultiplier_StandingStill * 0.1f));

	const bool bIsCrouching = CharacterMovement && CharacterMovement->IsCrouching();
	const float CrouchingTargetValue = bIsCrouching ? SpreadAngleMultiplier_Crouching : 1.0f;
	CrouchingMultiplier = FMath::FInterpTo(
		CrouchingMultiplier, CrouchingTargetValue, DeltaTime, TransitionRate_Crouching);
	const bool bCrouchingAtTarget = FMath::IsNearlyEqual(
		CrouchingMultiplier, CrouchingTargetValue, MultiplierNearlyEqualThreshold);

	const bool bIsJumpingOrFalling = CharacterMovement && CharacterMovement->IsFalling();
	const float JumpFallTargetValue = bIsJumpingOrFalling ? SpreadAngleMultiplier_JumpingOrFalling : 1.0f;
	JumpFallMultiplier = FMath::FInterpTo(
		JumpFallMultiplier, JumpFallTargetValue, DeltaTime, TransitionRate_JumpingOrFalling);
	const bool bJumpFallAtMinimum = FMath::IsNearlyEqual(
		JumpFallMultiplier, 1.0f, MultiplierNearlyEqualThreshold);

	// AimingAlpha 由共享 ADS GA 写入；CameraModeStack 只组合本地视图，不改变武器散布入口。
	const float AimingMultiplier = FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, 1.0f),
		FVector2D(1.0f, SpreadAngleMultiplier_Aiming),
		AimingAlpha);
	const bool bAimingAtMinimum = FMath::IsNearlyEqual(
		AimingMultiplier, SpreadAngleMultiplier_Aiming, KINDA_SMALL_NUMBER);

	CurrentSpreadAngleMultiplier =
		AimingMultiplier * StandingStillMultiplier * CrouchingMultiplier * JumpFallMultiplier;
	return bStandingStillAtMinimum && bCrouchingAtTarget && bJumpFallAtMinimum && bAimingAtMinimum;
}

bool UShootRangedWeaponInstance::IsProjectileWeapon() const
{
	return GetProjectileConfigFragment() != nullptr;
}

TSubclassOf<AShootProjectileBase> UShootRangedWeaponInstance::GetProjectileClass() const
{
	if (const UShootInventoryFragment_ProjectileWeaponConfig* ProjCfg = GetProjectileConfigFragment())
	{
		return ProjCfg->ProjectileClass;
	}
	return nullptr;
}

AShootProjectileBase* UShootRangedWeaponInstance::TakeProjectileForLaunch(
	const FTransform& SpawnTransform)
{
	return nullptr;
}

const FProjectileWeaponConfig& UShootRangedWeaponInstance::GetProjectileConfig() const
{
	if (const UShootInventoryFragment_ProjectileWeaponConfig* ProjCfg = GetProjectileConfigFragment())
	{
		return ProjCfg->ProjectileConfig;
	}
	return GRangedWeaponDefaultProjectileConfig;
}

float UShootRangedWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags) const
{
	const FRichCurve* InstanceCurve = DistanceDamageFalloff.GetRichCurveConst();
	if (InstanceCurve && InstanceCurve->GetNumKeys() > 0)
	{
		return InstanceCurve->Eval(Distance, 1.0f);
	}

	// 空曲线表示在 MaxRange 射线端点内保持 100% 伤害，不代表无限射程；
	// UShootGameplayAbility_Weapon_Fire 仍用 RangedWeaponConfig.MaxRange 截断 Trace。
	// 当前 Sniper 使用该语义，后续若需要远距衰减只配置 WeaponInstance 曲线，不另加 GA 公式。
	return 1.0f;
}

float UShootRangedWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysMat,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags) const
{
	if (!PhysMat)
	{
		return 1.0f;
	}

	const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<UPhysicalMaterialWithTags>(PhysMat);
	if (!PhysMatWithTags)
	{
		return 1.0f;
	}

	if (MaterialDamageMultiplier.Num() > 0)
	{
		float Multiplier = 1.0f;
		for (const TPair<FGameplayTag, float>& Pair : MaterialDamageMultiplier)
		{
			if (PhysMatWithTags->Tags.HasTag(Pair.Key))
			{
				Multiplier *= Pair.Value;
			}
		}
		return Multiplier;
	}

	return 1.0f;
}

UShootInventoryItemInstance* UShootRangedWeaponInstance::GetItemInstance() const
{
	return Cast<UShootInventoryItemInstance>(GetInstigator());
}

const UShootInventoryFragment_RangedWeaponConfig* UShootRangedWeaponInstance::GetRangedConfig() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		if (const TSubclassOf<UShootInventoryItemDefinition> ItemDefClass = ItemInstance->GetItemDef())
		{
			if (const UShootInventoryItemDefinition* ItemDef = ItemDefClass->GetDefaultObject<UShootInventoryItemDefinition>())
			{
				return Cast<UShootInventoryFragment_RangedWeaponConfig>(
					ItemDef->FindFragmentByClass(UShootInventoryFragment_RangedWeaponConfig::StaticClass()));
			}
		}
	}
	return nullptr;
}

const UShootInventoryFragment_ProjectileWeaponConfig* UShootRangedWeaponInstance::GetProjectileConfigFragment() const
{
	if (const UShootInventoryItemInstance* ItemInstance = GetItemInstance())
	{
		if (const TSubclassOf<UShootInventoryItemDefinition> ItemDefClass = ItemInstance->GetItemDef())
		{
			if (const UShootInventoryItemDefinition* ItemDef = ItemDefClass->GetDefaultObject<UShootInventoryItemDefinition>())
			{
				return Cast<UShootInventoryFragment_ProjectileWeaponConfig>(
					ItemDef->FindFragmentByClass(UShootInventoryFragment_ProjectileWeaponConfig::StaticClass()));
			}
		}
	}
	return nullptr;
}
