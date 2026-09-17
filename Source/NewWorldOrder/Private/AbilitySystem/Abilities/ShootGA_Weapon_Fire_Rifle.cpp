// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Rifle.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "Animation/AnimMontage.h"

DEFINE_LOG_CATEGORY_STATIC(LogShootRifleDamage, Log, All);

UShootGA_Weapon_Fire_Rifle::UShootGA_Weapon_Fire_Rifle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CharacterFireMontage = nullptr;
	MontageTask = nullptr;
	InputReleaseTask = nullptr;
	FieldActorToSpawnOnImpact = nullptr;
	AutoRate = 1.0f;
	FireDelayTimeSecs = 0.1f;

	// Rifle 与 Assault_Rifle_A 都直接授予这个原生 GA。WhileInputActive 标记全自动扳机语义；
	// 一次 Ability 激活内由本类按 FireDelayTimeSecs 循环射击，不能依赖 ASC 反复重新激活。
	// 半自动手枪会在自己的构造函数恢复 OnInputTriggered；Shotgun、Sniper 和投射物 GA
	// 不是本类子类，因此不会因为这项默认值被误改成全自动。
	ActivationPolicy = EShootAbilityActivationPolicy::WhileInputActive;
	
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rifle.Fire"));
	ImpactGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rifle.Impact"));

	// 设置 AbilityTags（等价于 JSON 中 AbilityTags）
	SetAssetTags(FGameplayTagContainer(
		FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.WeaponFire"))));
		
	// 当 ability 激活时会把这些 tag 添加到拥有者 (ActivationOwnedTags)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Movement.WeaponFire")));

	// 激活所需标签 / 被阻挡的标签（ActivationRequiredTags / ActivationBlockedTags）
	//ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Require.SomeTag"))); // 示例
	//ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Blocked.SomeTag")));   // 示例

	// 来源受阻挡（SourceBlockedTags），例如武器上标记“不允许开火”
	SourceBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.NoFiring")));

	// 取消或阻止其他 ability 的标签（CancelAbilitiesWithTag / BlockAbilitiesWithTag）
	//CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Reload"))); // 示例
	//BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Other"))); // 示例
	StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.LMB"));

	// 弹药成本：统一使用 AmmoTagStack（Inventory_Ammo_Magazine / 每次 1 发），支持 Overload 无限弹
	if (!AdditionalCosts.ContainsByPredicate([](const UShootAbilityCost* Cost){ return Cost && Cost->IsA<UShootAbilityCost_AmmoTagStack>(); }))
	{
		UShootAbilityCost_AmmoTagStack* AmmoCost = ObjectInitializer.CreateDefaultSubobject<UShootAbilityCost_AmmoTagStack>(this, TEXT("RifleAmmoCost"));
		AdditionalCosts.Add(AmmoCost);
	}
}

void UShootGA_Weapon_Fire_Rifle::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ExecuteLocallyControlledShot();

	if (ActivationPolicy == EShootAbilityActivationPolicy::WhileInputActive)
	{
		// Rifle 在一次 Ability 激活中持续发射，不能依赖“Montage 结束后由 Held 再激活”。
		// 后一种做法会让长按期间只打一发，并可能在松开附近才产生第二发。
		InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
		InputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::HandleAutomaticFireReleased);
		InputReleaseTask->ReadyForActivation();

		if (IsLocallyControlled())
		{
			// 第一发已在上方立即发射；第二发及后续每发最早在完整 FireDelayTimeSecs 后发生。
			// 枪声音尾可以长于该间隔并自然重叠，不能为了等音频播完而改变武器射速。
			GetWorld()->GetTimerManager().SetTimer(
				AutomaticFireTimerHandle, this, &ThisClass::HandleAutomaticFireTick,
				FMath::Max(FireDelayTimeSecs, 0.01f), false);
		}
	}
	else
	{
		// Pistol 继承本类的命中实现，但保持一次按压一次开火并在自己的节奏端点结束。
		GetWorld()->GetTimerManager().SetTimer(
			AutomaticFireTimerHandle,
			[this]() { if (IsActive()) K2_EndAbility(); },
			FMath::Max(FireDelayTimeSecs, 0.01f), false);
	}
}

void UShootGA_Weapon_Fire_Rifle::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutomaticFireTimerHandle);
	}
	InputReleaseTask = nullptr;
	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Weapon_Fire_Rifle::PlayFireMontage()
{
	// ItemDefinition Fragment 按当前 Pawn Skeleton 选择蒙太奇；GA 属性保留为蓝图兼容兜底。
	const UShootRangedWeaponInstance* WeaponInstance = GetWeaponInstance();
	UAnimMontage* MontageToPlay = WeaponInstance ? WeaponInstance->GetCharacterFireMontage() : nullptr;
	if (!MontageToPlay)
	{
		MontageToPlay = CharacterFireMontage;
	}
	if (!MontageToPlay)
	{
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, AutoRate, NAME_None,
		false, 1.0f, 0.0f, false);
	// 全自动时 Montage 完成或被下一发重播打断都不结束 Ability；只有松开、Cost 失败或取消能停止循环。
	MontageTask->ReadyForActivation();
}

void UShootGA_Weapon_Fire_Rifle::ExecuteLocallyControlledShot()
{
	if (!IsActive() || !IsLocallyControlled())
	{
		return;
	}

	if (UShootRangedWeaponInstance* WeaponInstance = GetWeaponInstance())
	{
		// 基类 Activate 只覆盖第一发；自动射击的每一发都要刷新散布恢复时间端点。
		WeaponInstance->UpdateFiringTime();
	}
	PlayFireMontage();
	StartRangedWeaponTargeting();
}

bool UShootGA_Weapon_Fire_Rifle::IsFireInputPressed() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = AbilitySystem
		? AbilitySystem->FindAbilitySpecFromHandle(CurrentSpecHandle)
		: nullptr;
	return Spec && Spec->InputPressed;
}

void UShootGA_Weapon_Fire_Rifle::HandleAutomaticFireTick()
{
	if (!IsActive())
	{
		return;
	}
	if (!IsFireInputPressed())
	{
		K2_EndAbility();
		return;
	}

	ExecuteLocallyControlledShot();
	if (IsActive())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutomaticFireTimerHandle, this, &ThisClass::HandleAutomaticFireTick,
			FMath::Max(FireDelayTimeSecs, 0.01f), false);
	}
}

void UShootGA_Weapon_Fire_Rifle::HandleAutomaticFireReleased(float TimeHeld)
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}

// ========== 核心逻辑：处理命中数据 ==========

void UShootGA_Weapon_Fire_Rifle::OnRangedWeaponTargetDataReady_Implementation(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	// 远端拥有客户端负责本地预测循环；服务器每收到一发 TargetData 后播放一次权威 Montage，
	// 让模拟代理看到同样的连射节奏。Listen Host 已在本地循环播放，不能重复一次。
	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		PlayFireMontage();
	}
	PlayFireEffects(TargetData);
	ProcessHits(TargetData);
	BroadcastReticleHitNotify(TargetData);
}

void UShootGA_Weapon_Fire_Rifle::PlayFireEffects(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (!FireGameplayCueTag.IsValid()) return;

	// 获取第一个命中结果（即使没命中也会有虚拟点）
	const FHitResult& FirstHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, 0);

	// 从命中结果生成 Cue 参数
	FGameplayCueParameters GCParams = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(FirstHit);

	// 补充武器信息
	if (UShootRangedWeaponInstance* Weapon = GetWeaponInstance())
	{
		GCParams.SourceObject = Weapon;
		// 如果没命中任何东西，使用枪口位置和瞄准方向
		if (!FirstHit.bBlockingHit)
		{
			GCParams.Location = Weapon->GetMuzzleLocation();
			GCParams.Normal = GetAimDirection();
		}
	}

	// 执行 GameplayCue（会在所有客户端播放）
	K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, GCParams);
}

void UShootGA_Weapon_Fire_Rifle::ProcessHits(const FGameplayAbilityTargetDataHandle& TargetData)
{
	const int32 HitCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(TargetData);
	const bool bIsAuthority = K2_HasAuthority();
	UE_LOG(LogShootRifleDamage, Verbose,
		TEXT("ProcessHits: Ability=%s Count=%d Authority=%d"), *GetNameSafe(this), HitCount, bIsAuthority);

	// 一次循环处理所有命中
	for (int32 i = 0; i < HitCount; ++i)
	{
		// 使用 const 引用避免拷贝
		const FHitResult& Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, i);

		// 只处理真实命中（排除虚拟命中点）
		if (!Hit.bBlockingHit) continue;

		// 1. 播放命中特效（所有客户端）
		if (ImpactGameplayCueTag.IsValid())
		{
			FGameplayCueParameters ImpactParams =
				UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(Hit);
			K2_ExecuteGameplayCueWithParams(ImpactGameplayCueTag, ImpactParams);
		}

		// 2. 服务器：生成命中 Actor（可选）
		if (bIsAuthority && FieldActorToSpawnOnImpact)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());

			FTransform SpawnTransform(Hit.ImpactNormal.ToOrientationQuat(), Hit.ImpactPoint);
			GetWorld()->SpawnActor<AActor>(FieldActorToSpawnOnImpact, SpawnTransform, SpawnParams);
		}

		// 3. 服务器：应用伤害
		if (bIsAuthority)
		{
			ApplyWeaponDamageToTarget(Hit);
		}
	}
}
