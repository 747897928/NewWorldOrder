// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ShootWeaponInstance.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "Curves/CurveFloat.h"
#include "GameplayTagContainer.h"
#include "Inventory/Fragments/ShootInventoryFragment_RangedWeaponConfig.h"
#include "ShootRangedWeaponInstance.generated.h"

class AShootProjectileBase;
class UGameplayEffect;
class UShootInventoryItemInstance;
class UShootInventoryFragment_ProjectileWeaponConfig;
class UAnimMontage;
struct FProjectileWeaponConfig;

enum class EShootCharacterMontageAction : uint8
{
	Fire,
	Reload,
	Equip,
	Unequip
};

UCLASS()
class UShootRangedWeaponInstance : public UShootWeaponInstance, public ILyraAbilitySourceInterface
{
	GENERATED_BODY()

public:
	UShootRangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnEquipped() override;

	void Tick(float DeltaTime);

	// 当前弹药和容量都由关联 ItemInstance 的 StatTags 提供，Ranged Fragment 不保存重复弹药字段。
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Weapon)
	virtual void ReloadAmmo(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon|Ammo")
	int32 AddReserveAmmo(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon|Ammo")
	int32 AddMagazineAmmo(int32 Amount);

	/** 补给站服务器入口：把当前弹匣和备弹分别补到 ItemInstance 的容量 StatTag。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon|Ammo")
	bool RefillAmmoToCapacity();

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetCurrentReserve() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetPredictedAmmo() const { return GetCurrentAmmo(); }

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetPredictedReserve() const { return GetCurrentReserve(); }

	// 配置访问（Fragment 驱动）
	UFUNCTION(BlueprintPure, Category=Weapon)
	FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetBulletsPerCartridge() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	float GetBulletTraceSweepRadius() const;

	/** 对齐 LyraRangedWeaponInstance：每把武器的数据 Fragment 决定射线与伤害衰减的最大距离。 */
	UFUNCTION(BlueprintPure, Category=Weapon)
	float GetMaxDamageRange() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	float GetSpreadExponent() const;

	/** 兼容现有 Reload 调用命名；唯一数据源是 Inventory.Ammo.MagazineCapacity。 */
	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetMagazineSize() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	int32 GetReserveCapacity() const;

	/** 每把枪的正式 Damage GE；由 B_WeaponInstance_* 蓝图指向对应 GE_Damage_*。 */
	UFUNCTION(BlueprintPure, Category="Weapon|Damage")
	TSubclassOf<UGameplayEffect> GetDamageGameplayEffect() const { return DamageGameplayEffect; }

	UAnimMontage* GetCharacterFireMontage() const;
	UAnimMontage* GetCharacterReloadMontage() const;

	// 扩散系统
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void AddSpread();

	UFUNCTION(BlueprintPure, Category=Weapon)
	float GetCalculatedSpreadAngle() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	float GetCalculatedSpreadAngleMultiplier() const;

	UFUNCTION(BlueprintPure, Category=Weapon)
	bool HasFirstShotAccuracy() const;

	/**
	 * 由共享 ADS GA 写入 0..1 的过渡权重；相机退出和切枪取消也会回写到 0。
	 * 项目 CameraModeStack 只接管本地相机求值，服务器与拥有端仍共用这个武器散布入口。
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Spread")
	void SetAimingAlpha(float InAimingAlpha);

	/**
	 * 共享 ADS GA 使用的武器级配置。正式 B_WeaponInstance_* 蓝图负责覆盖这些可调值；
	 * GA 只读取当前 SourceObject，不按 WeaponId、键盘或手柄写分支。
	 */
	float GetADSCameraFieldOfView() const { return ADSCameraFieldOfView; }
	float GetADSCameraArmLength() const { return ADSCameraArmLength; }
	FVector GetADSCameraSocketOffset() const { return ADSCameraSocketOffset; }
	float GetADSCameraBlendTime() const { return ADSCameraBlendTime; }
	float GetADSMovementSpeedMultiplier() const { return ADSMovementSpeedMultiplier; }

	/**
	 * 当前无专用第一人称动画/机瞄数据，第一人称只允许 Sniper 分类使用全屏 Scope。
	 * 第三人称 ADS 不受此策略影响；未来增加普通枪机瞄时再改为显式武器数据。
	 */
	bool SupportsFirstPersonADS() const;

	/**
	 * 从当前武器 Fragment 取得一次本地相机后坐力（X=Yaw，Y=Pitch）。
	 * 该数据只供拥有者客户端的表现层调用，不能用于复制、散布或命中判定。
	 */
	FVector2D GetLocalCameraRecoil() const;

	// 投射物配置
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile")
	bool IsProjectileWeapon() const;

	UFUNCTION(BlueprintPure, Category="Weapon|Projectile")
	TSubclassOf<AShootProjectileBase> GetProjectileClass() const;

	/**
	 * 可选的“复用已挂接投射物”入口。默认返回 nullptr，榴弹及普通投射物
	 * 继续由通用 Projectile Fire GA 按原路径 SpawnActorDeferred。
	 */
	virtual AShootProjectileBase* TakeProjectileForLaunch(const FTransform& SpawnTransform);

	const FProjectileWeaponConfig& GetProjectileConfig() const;

	/** 供 HUD/Reticle 等读取 ItemInstance 与 Fragment 配置 */
	UShootInventoryItemInstance* GetItemInstance() const;

	// ILyraAbilitySourceInterface
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr) const override;

	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysMat,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr) const override;

protected:
	virtual UAnimMontage* GetCharacterEquipMontage() const override;
	virtual UAnimMontage* GetCharacterUnequipMontage() const override;

	/** GE 伤害主线的数据归属；伤害基础值、距离曲线与材质倍率不从 Item Fragment 回退。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
	TSubclassOf<UGameplayEffect> DamageGameplayEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
	FRuntimeFloatCurve DistanceDamageFalloff;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
	TMap<FGameplayTag, float> MaterialDamageMultiplier;

	// 以下字段对齐 LyraRangedWeaponInstance。三把首批武器在各自 B_WeaponInstance 蓝图中配置，
	// 新武器只需派生蓝图并调整曲线/倍率，不需要为散布再写 C++。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Fire Params", meta=(ClampMin=0.1))
	float SpreadExponent = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToSpreadCurve;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Fire Params", meta=(ForceUnits=s))
	float SpreadRecoveryCooldownDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Fire Params")
	bool bAllowFirstShotAccuracy = false;

	UPROPERTY(EditDefaultsOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Aiming = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aim|Camera", meta=(ClampMin="5.0", ClampMax="170.0", ForceUnits=deg))
	float ADSCameraFieldOfView = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aim|Camera", meta=(ClampMin="0.0", ForceUnits=cm))
	float ADSCameraArmLength = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aim|Camera")
	FVector ADSCameraSocketOffset = FVector(0.0f, 60.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aim|Camera", meta=(ClampMin="0.0", ForceUnits=s))
	float ADSCameraBlendTime = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aim|Movement", meta=(ClampMin="0.0", ClampMax="1.0", ForceUnits=x))
	float ADSMovementSpeedMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_StandingStill = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_StandingStill = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillSpeedThreshold = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillToMovingSpeedRange = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Crouching = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_Crouching = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_JumpingOrFalling = 5.0f;

private:
	const UShootInventoryFragment_RangedWeaponConfig* GetRangedConfig() const;
	const UShootInventoryFragment_ProjectileWeaponConfig* GetProjectileConfigFragment() const;
	UAnimMontage* FindCharacterMontage(EShootCharacterMontageAction Action) const;
	bool UsesInstanceHeatSpreadModel() const;
	bool UsesHeatSpreadModel() const;
	const FRuntimeFloatCurve& GetConfiguredHeatToSpreadCurve() const;
	const FRuntimeFloatCurve& GetConfiguredHeatPerShotCurve() const;
	const FRuntimeFloatCurve& GetConfiguredCooldownCurve() const;
	float GetConfiguredRecoveryDelay() const;
	bool GetConfiguredFirstShotAccuracy() const;
	float ClampHeat(float NewHeat) const;
	void ComputeHeatRange(float& OutMinHeat, float& OutMaxHeat) const;
	void ComputeSpreadRange(float& OutMinSpread, float& OutMaxSpread) const;
	bool UpdateSpread(float DeltaTime);
	bool UpdateMultipliers(float DeltaTime);

	UPROPERTY(Transient)
	float CurrentSpreadAngle = 0.0f;

	UPROPERTY(Transient)
	double LastFireTime = 0.0;

	UPROPERTY(Transient)
	float CurrentHeat = 0.0f;

	UPROPERTY(Transient)
	bool bHasFirstShotAccuracy = false;

	UPROPERTY(Transient)
	float CurrentSpreadAngleMultiplier = 1.0f;

	UPROPERTY(Transient)
	float StandingStillMultiplier = 1.0f;

	UPROPERTY(Transient)
	float JumpFallMultiplier = 1.0f;

	UPROPERTY(Transient)
	float CrouchingMultiplier = 1.0f;

	UPROPERTY(Transient)
	float AimingAlpha = 0.0f;
};
