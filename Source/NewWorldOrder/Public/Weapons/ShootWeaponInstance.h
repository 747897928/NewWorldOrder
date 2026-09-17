// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Equipment/ShootEquipmentInstance.h"
#include "Animation/ShootAnimLayerSelectionSet.h"
#include "ShootWeaponInstance.generated.h"

UENUM(BlueprintType)
enum class EShootWeaponAnimAction : uint8
{
	Show,
	Hide,
	Destroy
};

class UAnimInstance;
class UAnimMontage;

/**
 * UShootWeaponInstance
 *
 * 武器实例基类（UObject）- 继承自 EquipmentInstance，添加武器特有功能
 *
 * 设计要点：
 *   - UObject 而非 Actor：逻辑与表现分离，性能更好
 *   - 通过 Instigator 访问 ItemInstance：获取弹药（StatTags）
 *   - 支持动画层：装备/卸载时切换角色动画
 *
 * 与 Lyra 对应关系：
 *   - Lyra: ULyraWeaponInstance
 *   - 我们: UShootWeaponInstance
 *
 * 子类：
 *   - UShootRangedWeaponInstance：远程武器（步枪、手枪、狙击枪等）
 *   - UShootMeleeWeaponInstance：近战武器（未来）
 *
 * 使用场景：
 *   1. 装备武器：
 *      - EquipmentManager->EquipItem(EquipmentDef)
 *      - 创建 UShootRangedWeaponInstance
 *      - WeaponInstance->OnEquipped()
 *      - 应用动画层
 *   2. 开火：
 *      - Ability 通过 SourceObject 访问 WeaponInstance
 *      - WeaponInstance->UpdateFiringTime()
 *      - 通过 Instigator 访问弹药（ItemInstance->StatTags）
 *   3. 卸载武器：
 *      - WeaponInstance->OnUnequipped()
 *      - 移除动画层
 *
 * 与旧 AShootWeaponActor 的区别：
 *   旧（Actor）                 新（UObject）
 *   -------------------------------------------
 *   继承自 AActor              继承自 UShootEquipmentInstance
 *   自带 WeaponMesh            通过 SpawnedActors
 *   成员变量存储弹药           通过 Instigator->StatTags
 *   简单预测机制               Lyra PreReplication 机制
 *   独立管理 Ability           通过 EquipmentManager 统一管理
 */
UCLASS(MinimalAPI)
class UShootWeaponInstance : public UShootEquipmentInstance
{
	GENERATED_BODY()

public:
	UShootWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ========================================================================
	// UShootEquipmentInstance Interface
	// ========================================================================

	/**
	 * 装备时回调
	 *
	 * 执行流程：
	 *   1. 调用父类 OnEquipped
	 *   2. 记录装备时间
	 *   3. 应用武器动画层（如果有）
	 */
	virtual void OnEquipped() override;

	/**
	 * 卸载时回调
	 *
	 * 执行流程：
	 *   1. 移除武器动画层
	 *   2. 调用父类 OnUnequipped
	 */
	virtual void OnUnequipped() override;

	// ========================================================================
	// 开火时间跟踪
	// ========================================================================

	/**
	 * 更新开火时间
	 *
	 * 调用时机：开火 Ability 中调用
	 *
	 * 用途：
	 *   - 记录最后开火时间
	 *   - 用于 Tick 更新扩散、热量等（子类实现）
	 */
	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	/**
	 * 获取上次交互时间（装备或开火）
	 *
	 * 用途：
	 *   - 判断武器是否长时间未使用
	 *   - 自动收起武器等逻辑
	 *
	 * 返回值：
	 *   - 装备时间和开火时间中的较晚者
	 */
	UFUNCTION(BlueprintPure, Category=Animation)
	double GetTimeSinceLastInteractedWith() const;

	/** UObject 版本的 HasAuthority（走 Outer Actor） */
	bool HasAuthority() const;

	/** AnimNotify 驱动的显隐控制 */
	virtual void HandleVisualAnimCue(EShootWeaponAnimAction Action);

	/**
	 * 按当前 Pawn 的外观标签重新选择并链接装备或空手动画层。
	 * 角色性别切换时会再次调用，不能只依赖第一次 OnEquipped。
	 */
	void RefreshAnimLayer(bool bEquipped) const;

	/** Blueprint 可读取 Lyra 式选择结果，便于资产调试。 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Animation)
	TSubclassOf<UAnimInstance> PickBestAnimLayer(bool bEquipped,
		const FGameplayTagContainer& CosmeticTags) const;

protected:
	/**
	 * 客户端若先收到 EquipmentList、后收到 Instigator，就在这里补播第一次缺失的 Equip Montage。
	 * 只补 Montage，不重复链接动画层、显隐武器或调用蓝图装备事件。
	 */
	virtual void OnInstigatorReplicated() override;

	/**
	 * Lyra 的 B_WeaponInstance_Base 在切换 Linked Anim Layer 后播放角色 Equip/Unequip Montage。
	 * 基类默认返回空，具体武器实例必须从 ItemDefinition 的按骨架配置中选择，不能按性别或玩家序号猜资源。
	 */
	virtual UAnimMontage* GetCharacterEquipMontage() const;
	virtual UAnimMontage* GetCharacterUnequipMontage() const;
	virtual float GetCharacterEquipMontageStartPosition() const;

	/** 在角色主 Mesh 上播放切换 Montage；起播位置来自当前骨架的数据项，不由运行时猜测角色类型。 */
	void PlayCharacterTransitionMontage(UAnimMontage* MontageToPlay, float StartPosition = 0.0f) const;

	/**
	 * 装备时间戳
	 *
	 * 记录武器装备时的游戏时间
	 */
	UPROPERTY(Transient)
	double TimeLastEquipped = 0.0;

	/**
	 * 开火时间戳
	 *
	 * 记录最后一次开火的游戏时间
	 */
	UPROPERTY(Transient)
	double TimeLastFired = 0.0;

	/** FastArray 已执行 OnEquipped，但当时 Instigator 尚未复制到客户端。 */
	UPROPERTY(Transient)
	bool bEquipMontageAwaitingInstigator = false;

	// ========================================================================
	// 动画层支持（可选）
	// ========================================================================

	/**
	 * 装备时选择当前武器的角色 Linked Anim Layer。
	 * 这里按 WeaponInstance 蓝图逐把配置，不要恢复 EShootWeaponAnimationStyle 枚举或按 WeaponId 写分支：
	 * Lyra 同样使用 EquippedAnimSet/UnequippedAnimSet；新增特殊持枪姿势时应增加独立 Layer 资产并改蓝图配置。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FShootAnimLayerSelectionSet EquippedAnimSet;

	/** 卸下时选择对应性别的 Unarmed 层，不能只 Unlink 后留下空接口姿势。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FShootAnimLayerSelectionSet UnequippedAnimSet;

	/**
	 * Equip Montage 的起播位置（秒）。
	 * 重定向到正式 CC 骨架后若开头存在不合适的过渡姿势，由各武器实例蓝图配置裁剪；
	 * 播放入口不按角色性别或枪型硬编码。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Animation, meta=(ClampMin="0.0", ForceUnits=s))
	float EquipMontageStartPosition = 0.0f;

private:
};
