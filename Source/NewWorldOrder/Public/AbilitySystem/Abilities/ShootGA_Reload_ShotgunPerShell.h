// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ShootGameplayAbility.h"
#include "ShootGA_Reload_ShotgunPerShell.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UShootRangedWeaponInstance;

/**
 * 霰弹枪逐发装填（Per-Shell Reload）
 *
 * 流程：
 *  - 只播放 WeaponInstance 按角色骨架解析出的 Reload Montage；缺少显式映射时直接失败，避免静默走旧动画；
 *  - Start/Loop 段接收 GameplayEvent.Reload.InsertShell，服务器每次只把一发备弹转入弹匣；
 *  - 按一次 Reload 后持续逐发装填；弹匣满、备弹为 0 或显式结束事件时跳到 End；
 *  - 空仓开始装填时只在第一发提交前临时阻止射击；第一发进入弹匣后，允许 Shotgun Fire GA
 *    取消本能力并立即开火。非空弹匣从开始就允许打断。
 *
 * ItemInstance 是弹药 StatTags 的唯一数据源；AbilitySpec.SourceObject 必须是
 * UShootRangedWeaponInstance。该能力不读取武器 Actor，也不创建第二套弹药状态。
 */
UCLASS()
class UShootGA_Reload_ShotgunPerShell : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Reload_ShotgunPerShell();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	FName SectionStartName = TEXT("ShotgunStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	FName SectionLoopName = TEXT("ShotgunLoop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	FName SectionEndName = TEXT("ShotgunEnd");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation|Events")
	FGameplayTag InsertShellEventTag;

	/** 可选的外部结束事件；正常流程也会在 Montage 自然完成时结束。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation|Events")
	FGameplayTag ReloadDoneEventTag;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UShootRangedWeaponInstance* GetWeaponInstance() const;

	UFUNCTION()
	void OnInsertShellEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnReloadDoneEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	void ContinueOrEndReload();
	void RequestEndReload();
	void SetFiringBlocked(bool bShouldBlock);
	bool HasRequiredSections(const UAnimMontage* Montage) const;
	void JumpToSection(FName Section);
	void SetNextSection(FName From, FName To);

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UShootRangedWeaponInstance> CachedWeapon;

	UPROPERTY(Transient)
	int32 ShellsInserted = 0;

	UPROPERTY(Transient)
	int32 InitialAmmo = 0;

	UPROPERTY(Transient)
	int32 InitialReserve = 0;

	UPROPERTY(Transient)
	bool bDidBlockFiring = false;

	/** 已决定进入护木收尾段；火力打断不会设置此状态，因为打断必须立即切换到 Fire。 */
	UPROPERTY(Transient)
	bool bEndSectionStarted = false;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> InsertShellTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ReloadDoneTask;

	FGameplayTag TagAbilityWeaponNoFiring;
	FGameplayTag TagActivateFailMagazineFull;
	FGameplayTag TagActivateFailNoSpareAmmo;
};
