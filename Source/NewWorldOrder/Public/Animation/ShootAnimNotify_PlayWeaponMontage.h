// Copyright ZhaoYiJie

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ShootAnimNotify_PlayWeaponMontage.generated.h"

class UAnimMontage;
class UParticleSystem;
class USoundBase;

/**
 * 角色武器蒙太奇开始时，让当前装备 Actor 的武器 AnimInstance 跟播配套蒙太奇。
 *
 * 调用链：正式 CC 角色 Fire/Reload Montage Notify -> Pawn 上的 EquipmentManager ->
 * 当前 UShootWeaponInstance -> Spawned Weapon Actor -> Weapon SkeletalMesh AnimInstance。
 * 这里只同步表现，不修改弹药、伤害或装备状态；服务器权威仍由对应 GAS Ability 负责。
 */
UCLASS(meta=(DisplayName="Shoot Play Weapon Montage"))
class NEWWORLDORDER_API UShootAnimNotify_PlayWeaponMontage : public UAnimNotify
{
	GENERATED_BODY()

public:
	/**
	 * 编辑角色 Montage 中的武器同步 Notify。
	 *
	 * UE Python 可以创建 Notify，却不能直接修改 FAnimNotifyEvent 内嵌 Notify 对象的
	 * MontageToPlay。这个入口只在资产制作阶段写入该引用，不参与运行时武器逻辑。
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon Montage")
	static bool ConfigureCharacterMontageNotify(
		UAnimMontage* CharacterMontage,
		UAnimMontage* WeaponMontage,
		float TriggerTime = 0.0001f,
		float PlayRate = 1.0f);

	/** 为商城武器的武器 Montage 写入声音 Notify；重复执行会更新同一时刻的已有 Notify。 */
	UFUNCTION(BlueprintCallable, Category="Weapon Montage")
	static bool AddMontageSoundNotify(
		UAnimMontage* WeaponMontage,
		USoundBase* Sound,
		float TriggerTime,
		FName AttachName = NAME_None);

	/** 为商城武器的武器 Montage 写入 Cascade 粒子 Notify；重复执行会更新同一时刻的已有 Notify。 */
	UFUNCTION(BlueprintCallable, Category="Weapon Montage")
	static bool AddMontageParticleNotify(
		UAnimMontage* WeaponMontage,
		UParticleSystem* ParticleSystem,
		float TriggerTime,
		FName SocketName);

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** 与角色蒙太奇配套的武器 SkeletalMesh 蒙太奇，由正式 CC 角色蒙太奇资产逐项配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Montage")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** 武器蒙太奇播放速率；需与来源角色蒙太奇上的 Lyra 同步 Notify 配置一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Montage", meta=(ClampMin="0.01"))
	float PlayRate = 1.0f;
};
