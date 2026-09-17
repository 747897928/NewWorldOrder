// Ranged weapon structure fragment: 射程/弹药/动画/弹道与本地后坐力；伤害 GE、散布和射速不在此重复保存。
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_RangedWeaponConfig.generated.h"

class UAnimMontage;
class USkeleton;

/**
 * 同一把武器针对不同角色骨架的动作蒙太奇。
 *
 * ItemDefinition 的 RangedWeaponConfig 按当前 Pawn Mesh 的 Skeleton 选择匹配项，
 * 因而 Fire/Reload GA 不需要硬编码性别，也不会从 SaveGame 推断战斗角色。
 */
USTRUCT(BlueprintType)
struct FShootWeaponCharacterMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<USkeleton> TargetSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	/** 对齐 Lyra B_WeaponInstance_Base：链接武器层后播放的角色装备 Montage。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> EquipMontage;

	/** 对齐 Lyra B_WeaponInstance_Base：链接空手层后播放的角色卸下 Montage。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<UAnimMontage> UnequipMontage;
};

UCLASS(BlueprintType)
class UShootInventoryFragment_RangedWeaponConfig : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** 射线最大距离；伤害数值与衰减曲线归 B_WeaponInstance/GE。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Range")
	float MaxRange = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	TArray<FShootWeaponCharacterMontageSet> CharacterMontages;

	// 散布与热量模型只从 B_WeaponInstance 读取，避免 Fragment 与武器实例出现两套状态。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Spread")
	float BulletTraceSweepRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Spread")
	int32 BulletsPerCartridge = 1;

	/**
	 * 仅本地相机表现：每次成功消耗弹药后，从该范围随机取俯仰输入。
	 * 不复制、不参与服务器 Trace 或伤害结算；数值单位是 PlayerController 输入量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
	float LocalCameraPitchRecoilMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
	float LocalCameraPitchRecoilMax = 0.0f;

	/** 水平后坐力范围。设置为负到正可得到左右随机偏移；同样只作用于开火者本地相机。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
	float LocalCameraYawRecoilMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
	float LocalCameraYawRecoilMax = 0.0f;

	// 插槽配置
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Socket")
	FName MuzzleSocketName = TEXT("MuzzleFlash");
};
