// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "ShootRocketLauncherWeaponInstance.generated.h"

class AShootProjectileBase;
class USkeletalMeshComponent;

/**
 * 火箭筒专用 WeaponInstance。
 *
 * 火箭弹头是同一个可复制的投射物 Actor：装填提交点前后，它被挂在火箭筒
 * 的弹头插槽上；开火时只把这个 Actor 脱离并交给通用 Projectile Fire GA
 * 初始化，不在开火路径重新生成第二个弹头网格。
 *
 * 该类不改变 ItemInstance 的弹药 StatTag 语义。弹匣/备弹仍由父类和现有
 * Reload GA 结算；本类只维护可见弹头 Actor 的生命周期。
 */
UCLASS()
class NEWWORLDORDER_API UShootRocketLauncherWeaponInstance : public UShootRangedWeaponInstance
{
	GENERATED_BODY()

public:
	UShootRocketLauncherWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;
	virtual void ReloadAmmo(int32 Amount) override;

	/**
	 * 火箭筒专用换弹 Ability 在播放角色/武器蒙太奇前调用；服务器在这里
	 * 创建并挂接弹头到角色手部，让它从换弹动画的手部位置一路跟随到
	 * 发射器。弹头在专用 RocketInsert 通知到达前不会参与弹药结算。
	 */
	void PrepareRocketForReload();

	/**
	 * 火箭筒专用插入通知调用：把同一个弹头 Actor 从角色手部转挂到
	 * 武器 AmmoSocket。实际弹药数仍由现有 AN_Reload/ReloadDone 事件结算。
	 */
	void CommitRocketReload();

	/** 换弹被打断时清理手里的临时弹头，避免取消后弹头悬在角色手上。 */
	void CancelRocketReload();

	/**
	 * 供 Projectile Fire GA 调用。成功时返回当前挂在火箭筒上的同一个投射物
	 * Actor，并将其变为待发射状态；返回 nullptr 时由通用 GA 保留原有生成路径。
	 */
	virtual AShootProjectileBase* TakeProjectileForLaunch(const FTransform& SpawnTransform) override;

	/** 火箭筒网格上用于容纳弹头的 Socket；当前资产已配置在 Ammo 骨骼上。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rocket Launcher|Ammo")
	FName RocketAmmoSocketName = TEXT("AmmoSocket");

	/** 当前武器 SkeletalMesh 内置的动画导向弹头骨骼；运行时由外部投射物替代其可见网格。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rocket Launcher|Ammo")
	FName RocketAmmoBoneName = TEXT("Ammo");

	/** 弹头相对于 RocketAmmoSocket 的微调，负责最终位置和旋转。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rocket Launcher|Ammo")
	FTransform RocketAmmoRelativeTransform = FTransform::Identity;

	/**
	 * 换弹时承载弹头的角色 Socket/骨骼名称。当前 CC 骨架没有专用左手
	 * Socket，因此默认使用 hand_l 骨骼；如果后续资产增加 RocketReloadHand
	 * Socket，只需在该武器实例蓝图中改名，不改通用武器代码。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rocket Launcher|Reload")
	FName RocketReloadHandSocketName = TEXT("hand_l");

	/** 弹头相对于角色换弹手部 Socket 的位置和旋转校正。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rocket Launcher|Reload")
	FTransform RocketReloadHandRelativeTransform = FTransform::Identity;

protected:
	virtual void OnInstigatorReplicated() override;

	/** 由换弹提交点调用；当前实现也会在 ReloadAmmo 成功后自动调用。 */
	void EnsureHeldRocketAttached();

	/** 把已复制到客户端的弹头重新挂回武器 Socket。 */
	void AttachHeldRocketToWeapon();
	/** 把弹头挂到角色换弹手部 Socket/骨骼。 */
	void AttachHeldRocketToReloadHand();
	/** 根据复制状态选择手部或武器作为当前挂点。 */
	void AttachHeldRocketToCurrentTarget();
	void HideIntegratedAmmoBone(USkeletalMeshComponent* WeaponMesh) const;

	USkeletalMeshComponent* FindWeaponMeshComponent() const;
	USkeletalMeshComponent* FindCharacterMeshComponent() const;
	FName ResolveRocketAmmoSocket(const USkeletalMeshComponent* WeaponMesh) const;
	void DestroyHeldRocket();

	UFUNCTION()
	void OnRep_HeldRocketActor();

	UFUNCTION()
	void OnRep_RocketReloadInHand();

	/** 当前可见弹头；发射后清空，Actor 本身继续作为飞行投射物存在。 */
	UPROPERTY(ReplicatedUsing=OnRep_HeldRocketActor, Transient)
	TObjectPtr<AShootProjectileBase> HeldRocketActor;

	/** 服务器权威的挂点状态，避免客户端 OnRep 把手里的弹头错误地挂回武器。 */
	UPROPERTY(ReplicatedUsing=OnRep_RocketReloadInHand, Transient)
	bool bRocketReloadInHand = false;
};
