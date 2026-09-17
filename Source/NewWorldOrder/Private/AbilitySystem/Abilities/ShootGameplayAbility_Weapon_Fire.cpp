// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.h"

#include "Components/SkeletalMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "CollisionQueryParams.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Physics/LyraCollisionChannels.h"
#include "ShootGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogShootWeaponTargeting, Log, All);
#include "Weapons/ShootRangedWeaponInstance.h"
#include "Animation/AnimMontage.h"

/**

 * 生成在以 Dir 为中心、半角为 ConeHalfAngleRad 的圆锥内的随机方向，使用指数分布控制偏离中心的聚集程度。
 * Generate a random direction inside a cone centered on Dir with half-angle ConeHalfAngleRad,
 *      using an exponent to bias samples toward the cone center.
 *
 * 参数 / Parameters:
 * - Dir: 中心方向向量（可非单位向量），函数会在必要时归一化。 
 *        / Center direction vector (may be non-unit; normalized as needed).
 * - ConeHalfAngleRad: 圆锥半角，单位为弧度。若 <= 0，将直接返回归一化的 Dir。
 *        / Cone half-angle in radians. If <= 0, the function returns the normalized Dir.
 * - Exponent: 控制从中心线偏移分布的指数。Exponent 越大，生成的方向越集中于中心线；Exponent == 1 表示均匀分布。
 *        / Exponent controls bias from the center line. Larger Exponent -> more clustering near center;
 *          Exponent == 1 gives a uniform distribution.
 *
 * 返回 / Returns:
 * - 随机方向（单位向量）。
 *   / A random unit direction vector within the cone.
 *
 * 说明 / Notes:
 * - 使用 FMath::FRand() 生成 [0,1) 随机数，再用 Pow 调整到期望分布。
 * - 先将 Dir 转为旋转（FRotator）并构造四元数，然后围绕该轴旋转以生成最终方向。
 */
/**
 * 在圆锥内生成随机方向，使用指数分布控制聚集程度。
 * 
 * 这是 Lyra 的核心算法，支持所有枪械类型：
 * - 步枪：小扩散角 + 高指数 = 集中射击
 * - 霰弹枪：大扩散角 + 低指数 = 均匀分布
 * - 狙击枪：极小扩散角 + 极高指数 = 几乎无扩散
 * 
 * 数学原理：
 * 1. 用 Pow(Rand, Exponent) 生成偏向中心的分布
 *    - Exponent = 1.0：均匀分布
 *    - Exponent > 1.0：越来越集中于中心
 * 2. 用四元数组合三个旋转：
 *    - DirQuat：基准方向
 *    - AroundQuat：绕中心线旋转（0-360度随机）
 *    - FromCenterQuat：偏离中心线（0-ConeHalfAngle）
 */
FVector VRandConeNormalDistribution(const FVector& Dir, const float ConeHalfAngleRad, const float Exponent)
{
	// 如果锥体半角大于0，才进行随机方向计算
	if (ConeHalfAngleRad > 0.f)
	{
		// 将弧度转换为角度，方便后续旋转计算
		const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRad);

		// consider the cone a concatenation of two rotations. one "away" from the center line, and another "around" the circle
		// apply the exponent to the away-from-center rotation. a larger exponent will cluster points more tightly around the center
		// 生成一个0~1之间的随机数，并用指数Exponent调整分布，Exponent越大，越集中在中心
		const float FromCenter = FMath::Pow(FMath::FRand(), Exponent);

		// 计算离中心线的角度，越靠近中心概率越大
		const float AngleFromCenter = FromCenter * ConeHalfAngleDegrees;

		// 在圆周上随机一个角度（0~360度），用于绕中心线旋转
		const float AngleAround = FMath::FRand() * 360.0f;

		// 将输入方向向量Dir转换为旋转（FRotator）
		FRotator Rot = Dir.Rotation();

		// 用旋转构造一个四元数，表示中心线方向
		FQuat DirQuat(Rot);

		// 构造一个四元数，表示离中心线AngleFromCenter度的旋转（在Pitch方向上偏移）
		FQuat FromCenterQuat(FRotator(0.0f, AngleFromCenter, 0.0f));

		// 构造一个四元数，表示绕中心线AngleAround度的旋转（在Yaw方向上偏移）
		FQuat AroundQuat(FRotator(0.0f, 0.0, AngleAround));

		// 组合三个旋转，得到最终的方向
		FQuat FinalDirectionQuat = DirQuat * AroundQuat * FromCenterQuat;

		// 归一化四元数，确保方向有效
		FinalDirectionQuat.Normalize();

		// 用最终四元数旋转一个单位向前向量，得到随机方向
		return FinalDirectionQuat.RotateVector(FVector::ForwardVector);
	}
	else
	{
		// 如果锥体半角为0，直接返回归一化的输入方向
		return Dir.GetSafeNormal();
	}
}

UShootGameplayAbility_Weapon_Fire::UShootGameplayAbility_Weapon_Fire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	bServerRespectsRemoteAbilityCancellation = true;
	bRetriggerInstancedAbility = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	// 基类只负责 Lyra 风格的预测射线与提交时机，不创建通用弹药 Cost。
	// Rifle/Pistol/Shotgun 等最终开火类各自挂载唯一的 UShootAbilityCost_AmmoTagStack，
	// 由它从 SourceObject 关联的 ItemInstance 固定扣除一个弹匣 StatTag。
	// 若基类再创建一个未配置 Tag 的 ItemTagStack，GAS 会同时检查两份 Cost，
	// 导致有弹药的武器也因旧 Cost 失败而无法激活。
}

UShootRangedWeaponInstance* UShootGameplayAbility_Weapon_Fire::GetWeaponInstance() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UShootRangedWeaponInstance>(Spec->SourceObject.Get());
	}
	return nullptr;
}

bool UShootGameplayAbility_Weapon_Fire::ApplyWeaponDamageToTarget(const FHitResult& Hit)
{
	UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	AActor* TargetActor = Hit.GetActor();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = TargetActor
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor)
		: nullptr;

	if (!Weapon || !TargetActor || !SourceASC || !TargetASC || !Weapon->GetDamageGameplayEffect())
	{
		UE_LOG(LogShootWeaponTargeting, Warning,
			TEXT("Damage skipped: Weapon=%s Target=%s SourceASC=%s TargetASC=%s Effect=%s"),
			*GetNameSafe(Weapon), *GetNameSafe(TargetActor), *GetNameSafe(SourceASC), *GetNameSafe(TargetASC),
			*GetNameSafe(Weapon ? Weapon->GetDamageGameplayEffect() : nullptr));
		return false;
	}

	// 伤害公式属于 GE Execution。这里保留完整命中上下文，确保距离、弱点物理材质和后续伤害类型都能数据驱动。
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(Weapon);
	EffectContext.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
	EffectContext.AddHitResult(Hit);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		Weapon->GetDamageGameplayEffect(), GetAbilityLevel(), EffectContext);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogShootWeaponTargeting, Warning, TEXT("Damage skipped: unable to create GE spec for %s."),
			*GetNameSafe(Weapon->GetDamageGameplayEffect()));
		return false;
	}

	// MakeOutgoingSpec 使用自定义 Context，因此显式补上 Ability 的动态标签，保持与 UGameplayAbility 原生应用路径一致。
	ApplyAbilityTagsToGameplayEffectSpec(*SpecHandle.Data.Get(), GetCurrentAbilitySpec());
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

void UShootGameplayAbility_Weapon_Fire::BroadcastReticleHitNotify(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	if (!IsLocallyControlled())
	{
		// 只有本地端需要推送命中提示，服务器稍后会复制真实数据
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->PlayerController.IsValid())
	{
		return;
	}

	FShootReticleHitNotifyMessage Message;
	Message.SourceActor = ActorInfo->AvatarActor.Get();

	const int32 HitCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(TargetData);
	for (int32 HitIndex = 0; HitIndex < HitCount; ++HitIndex)
	{
		const FHitResult& Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, HitIndex);
		if (!Hit.bBlockingHit)
		{
			continue;
		}

		FShootReticleHitLocation Location;
		// 兼容虚拟命中点，ImpactPoint 为空时退回 TraceEnd
		Location.WorldPosition = Hit.ImpactPoint.IsZero() ? Hit.TraceEnd : Hit.ImpactPoint;
		Location.bHasWorldPosition = true;
		Location.bShowAsSuccess = true;

		if (const UPhysicalMaterialWithTags* PhysMat = Cast<UPhysicalMaterialWithTags>(Hit.PhysMaterial.Get()))
		{
			if (PhysMat->Tags.HasTag(FShootGameplayTags::Get().Gameplay_Zone_WeakSpot))
			{
				Location.HitZone = FShootGameplayTags::Get().Gameplay_Zone_WeakSpot;
			}
		}

		if (ActorInfo->PlayerController.IsValid())
		{
			FVector2D ScreenPos;
			if (ActorInfo->PlayerController->ProjectWorldLocationToScreen(Location.WorldPosition, ScreenPos, true))
			{
				Location.ScreenPosition = ScreenPos;
			}
		}

		Message.HitMarkers.Add(Location);
	}

	Message.bHasSuccessfulHit = Message.HitMarkers.Num() > 0;
	if (Message.HitMarkers.Num() > 0)
	{
		// 使用 Gameplay Message Subsystem 将消息交给 HUD
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			FShootGameplayTags::Get().Msg_UI_Reticle_HitNotify, Message);
	}
}

void UShootGameplayAbility_Weapon_Fire::AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		// Ignore any actors attached to the avatar doing the shooting
		TArray<AActor*> AttachedActors;
		Avatar->GetAttachedActors(/*out*/ AttachedActors);
		TraceParams.AddIgnoredActors(AttachedActors);
	}
}

/**
 * 在命中结果列表中查找第一个命中到APawn（角色）或其附属物体的命中结果，常用于射击类游戏的武器命中判定。
 */
int32 UShootGameplayAbility_Weapon_Fire::FindFirstPawnHitResult(const TArray<FHitResult>& HitResults)
{
	for (int32 Idx = 0; Idx < HitResults.Num(); ++Idx)
	{
		const FHitResult& CurHitResult = HitResults[Idx];
		//判断当前命中的对象句柄（HitObjectHandle）是否代表APawn类
		//HitObjectHandle是Unreal的通用对象引用，可以指向Actor或Component。这个方法用于快速判断命中对象是不是角色本身。
		//Represent代表；为……代言（辩护）；等于，相当于；（符号或象征）代表
		if (CurHitResult.HitObjectHandle.DoesRepresentClass(APawn::StaticClass()))
		{
			// If we hit a pawn, we're good
			//如果我们击中了一个角色（Pawn），就可以了
			return Idx;
		}
		else
		{
			//CurHitResult.HitObjectHandle.FetchActor(), 从命中对象句柄中获取实际的AActor指针。如果命中的是Actor，
			//这里会返回对应的Actor对象；如果是Component，则返回其所属Actor。常用于需要进一步操作命中对象时。
			AActor* HitActor = CurHitResult.HitObjectHandle.FetchActor();
			//HitActor->GetAttachParentActor() 获取当前Actor的父级挂载Actor。比如武器、装备、饰品等通常会挂载在角色身上，
			//这时它们的父Actor就是角色。这个方法用于判断命中的物体是否是角色的附属物。
			if ((HitActor != nullptr) && (HitActor->GetAttachParentActor() != nullptr) && (Cast<APawn>(
				HitActor->GetAttachParentActor()) != nullptr))
			{
				// If we hit something attached to a pawn, we're good
				//如果我们击中了附着在角色上的物体，也可以了。
				return Idx;
			}
		}
	}

	return INDEX_NONE;
}

ECollisionChannel UShootGameplayAbility_Weapon_Fire::DetermineTraceChannel(FCollisionQueryParams& TraceParams,
                                                                           bool bIsSimulated) const
{
	return Lyra_TraceChannel_Weapon;
}

FVector UShootGameplayAbility_Weapon_Fire::GetWeaponTargetingSourceLocation() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	return AvatarPawn ? AvatarPawn->GetActorLocation() : FVector::ZeroVector;
}

FTransform UShootGameplayAbility_Weapon_Fire::GetTargetingTransform(APawn* SourcePawn) const
{
	check(SourcePawn);

	const FVector ActorLocation = SourcePawn->GetActorLocation();
	const FQuat ActorAimQuat = SourcePawn->GetActorQuat();
	AController* Controller = SourcePawn->GetController();
	if (!Controller)
	{
		return FTransform(ActorAimQuat, ActorLocation);
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->GetPlayerViewPoint(/*out*/ CameraLocation, /*out*/ CameraRotation);

		// Lyra CameraTowardsFocus：先取相机前方焦点，再把起点沿瞄准轴投影到 Pawn 所在位置。
		// 这样第三人称相机不会从角色身后三百厘米开始判定命中，同时仍保持准星方向不变。
		constexpr double FocalDistance = 1024.0;
		const FVector AimDirection = CameraRotation.Vector().GetSafeNormal();
		FVector FocalLocation = CameraLocation + AimDirection * FocalDistance;
		const FVector WeaponLocation = GetWeaponTargetingSourceLocation();
		CameraLocation = FocalLocation + (((WeaponLocation - FocalLocation) | AimDirection) * AimDirection);
		FocalLocation = CameraLocation + AimDirection * FocalDistance;
		return FTransform((FocalLocation - CameraLocation).Rotation(), CameraLocation);
	}

	// Lyra 的 AI 分支从头部/眼睛位置开始，并使用 Controller 的权威 ControlRotation。
	CameraLocation = ActorLocation + FVector(0.0, 0.0, SourcePawn->BaseEyeHeight);
	CameraRotation = Controller->GetControlRotation();
	return FTransform(CameraRotation, CameraLocation);
}

FVector UShootGameplayAbility_Weapon_Fire::GetAimDirection() const
{
	APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	return AvatarPawn
		? GetTargetingTransform(AvatarPawn).GetUnitAxis(EAxis::X)
		: FVector::ForwardVector;
}

bool UShootGameplayAbility_Weapon_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                           const FGameplayAbilityActorInfo* ActorInfo,
                                                           const FGameplayTagContainer* SourceTags,
                                                           const FGameplayTagContainer* TargetTags,
                                                           FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		// 必须有武器实例才能激活
		if (GetWeaponInstance() == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Weapon ability %s cannot be activated because there is no weapon instance)"),
			       *GetPathName());
			bResult = false;
		}
	}

	return bResult;
}

void UShootGameplayAbility_Weapon_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                        const FGameplayAbilityActorInfo* ActorInfo,
                                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                                        const FGameplayEventData* TriggerEventData)
{
	// 绑定 TargetData 回调（Lyra 的做法）
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(
		CurrentSpecHandle,
		CurrentActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// 更新开火时间（用于扩散恢复计算）
	UShootRangedWeaponInstance* WeaponData = GetWeaponInstance();
	check(WeaponData);
	WeaponData->UpdateFiringTime();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UShootGameplayAbility_Weapon_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		// 如果当前有锁（ScopeLock），延迟结束
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(
				this, &ThisClass::EndAbility,
				Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled
			));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// 清理 TargetData 回调和数据
		MyAbilityComponent->AbilityTargetDataSetDelegate(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey()
		).Remove(OnTargetDataReadyCallbackDelegateHandle);

		MyAbilityComponent->ConsumeClientReplicatedTargetData(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey()
		);

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

// ========== 核心函数：目标检测流程 ==========

void UShootGameplayAbility_Weapon_Fire::StartRangedWeaponTargeting()
{
	check(CurrentActorInfo);

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	check(AvatarActor);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	// 创建预测窗口（用于客户端预测）
	FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, CurrentActivationInfo.GetActivationPredictionKey());

	// 1. 执行本地射线检测
	TArray<FHitResult> FoundHits;
	PerformLocalTargeting(/*out*/ FoundHits);

	// 2. 将命中结果打包成 TargetData
	FGameplayAbilityTargetDataHandle TargetData;
	// UniqueId 用于服务器验证（Lyra 用 WeaponStateComponent 生成，我们简化为 0）
	TargetData.UniqueId = 0;

	if (FoundHits.Num() > 0)
	{
		// 为每个命中创建一个 TargetData 条目
		for (const FHitResult& FoundHit : FoundHits)
		{
			FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new
				FGameplayAbilityTargetData_SingleTargetHit();
			NewTargetData->HitResult = FoundHit;
			TargetData.Add(NewTargetData);
		}
	}

	// 3. 触发回调（会调用 OnTargetDataReadyCallback）
	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}

void UShootGameplayAbility_Weapon_Fire::OnTargetDataReadyCallback(
	const FGameplayAbilityTargetDataHandle& InData,
	FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	// 确保 Ability 还在运行
	if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);

		// 客户端保留本地 Trace，立即驱动枪口表现与准星预览；它绝不决定服务器伤害。
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(
			MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		// PVE 联机不引入 Lyra WeaponStateComponent 的严格命中校验，但服务器仍必须用自己的
		// 当前视角、碰撞场景和武器散布重做 Trace。这样客户端上送的数据只是一条预测输入链路，
		// 最终伤害、命中特效和服务器侧散布都不会信任客户端提供的 HitResult。
		if (CurrentActorInfo->IsNetAuthority() && !ShouldUseClientTargetDataOnServer())
		{
			TArray<FHitResult> AuthoritativeHits;
			PerformLocalTargeting(AuthoritativeHits);
			UE_LOG(LogShootWeaponTargeting, Verbose,
				TEXT("Authority targeting: Ability=%s Avatar=%s Hits=%d"),
				*GetNameSafe(this), *GetNameSafe(GetAvatarActorFromActorInfo()), AuthoritativeHits.Num());

			FGameplayAbilityTargetDataHandle AuthoritativeTargetData;
			AuthoritativeTargetData.UniqueId = 0;
			for (const FHitResult& AuthoritativeHit : AuthoritativeHits)
			{
				FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
				NewTargetData->HitResult = AuthoritativeHit;
				AuthoritativeTargetData.Add(NewTargetData);
			}

			LocalTargetDataHandle = MoveTemp(AuthoritativeTargetData);
		}

		// 如果是客户端，发送到服务器验证
		const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
		if (bShouldNotifyServer)
		{
			MyAbilityComponent->CallServerSetReplicatedTargetData(
				CurrentSpecHandle,
				CurrentActivationInfo.GetActivationPredictionKey(),
				LocalTargetDataHandle,
				ApplicationTag,
				MyAbilityComponent->ScopedPredictionKey
			);
		}

		// 尝试消耗成本（弹药、Mana 等）
		if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			UE_LOG(LogShootWeaponTargeting, Verbose,
				TEXT("Weapon commit succeeded: Ability=%s TargetData=%d Authority=%d"),
				*GetNameSafe(this), LocalTargetDataHandle.Num(), CurrentActorInfo->IsNetAuthority());
			// 成功消耗成本后才改变散布并触发本地相机表现，空弹或 Cost 失败不会产生后坐力。
			UShootRangedWeaponInstance* WeaponData = GetWeaponInstance();
			check(WeaponData);
			WeaponData->AddSpread();
			ApplyLocalCameraRecoil(*WeaponData);

			// 触发蓝图事件（子类在这里应用伤害 GE 和 GameplayCue）
			OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
		}
		else
		{
			// 无法消耗成本（比如没弹药了），结束 Ability
			UE_LOG(LogTemp, Warning,
			       TEXT("Weapon ability %s failed to commit (no ammo or cost not met)"),
			       *GetPathName());
			K2_EndAbility();
		}
	}

	// 消耗客户端复制的 TargetData
	MyAbilityComponent->ConsumeClientReplicatedTargetData(
		CurrentSpecHandle,
		CurrentActivationInfo.GetActivationPredictionKey()
	);
}

void UShootGameplayAbility_Weapon_Fire::ApplyLocalCameraRecoil(const UShootRangedWeaponInstance& WeaponData) const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsLocallyControlled())
	{
		return;
	}

	APlayerController* const PlayerController = ActorInfo->PlayerController.Get();
	if (!PlayerController)
	{
		return;
	}

	// Lyra 的 WeaponInstance 把跨设备反馈留在本地输入设备层；项目额外需要鼠标相机后坐力，
	// 因此也只在拥有者 Controller 消费 Fragment 数据。这里绝不发送 RPC、绝不写入武器散布。
	const FVector2D Recoil = WeaponData.GetLocalCameraRecoil();
	PlayerController->AddYawInput(Recoil.X);
	PlayerController->AddPitchInput(Recoil.Y);
}

void UShootGameplayAbility_Weapon_Fire::OnRangedWeaponTargetDataReady_Implementation(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	
}

// ========== 射线检测实现 ==========

void UShootGameplayAbility_Weapon_Fire::PerformLocalTargeting(TArray<FHitResult>& OutHits)
{
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UShootRangedWeaponInstance* WeaponData = GetWeaponInstance();

	if (!AvatarPawn || !WeaponData)
	{
		return;
	}

	// 只在本地控制端执行（玩家客户端或服务器）
	// if (AvatarPawn->IsLocallyControlled())
	// {
	// 	const FVector StartTrace = GetWeaponTargetingSourceLocation();
	// 	const FVector AimDir = GetAimDirection();
	//
	// 	// 执行所有子弹的射线检测
	// 	TraceBulletsInCartridge(WeaponData, StartTrace, AimDir, /*out*/ OutHits);
	// }
	// 客户端用于即时预测；服务器用于最终伤害结算。远端客户端 Pawn 在服务器上通常不是
	// LocallyControlled，因此这里必须允许 Authority，否则服务器会错误地复用客户端命中数据。
	if (!AvatarPawn->IsLocallyControlled() && !AvatarPawn->HasAuthority())
	{
		return;
	}

	// 严格回归 Lyra 主线：CameraTowardsFocus 只决定射击起点和中心方向，
	// 每颗 pellet 都必须进入 TraceBulletsInCartridge。旧的“相机命中后枪口单次 Trace 提前返回”
	// 会把霰弹枪的九颗 pellet 退化成一次命中，因此不能保留。
	const FTransform TargetingTransform = GetTargetingTransform(AvatarPawn);
	const FVector StartTrace = TargetingTransform.GetTranslation();
	const FVector AimDirection = TargetingTransform.GetUnitAxis(EAxis::X);
	TraceBulletsInCartridge(WeaponData, StartTrace, AimDirection, /*out*/ OutHits);
}


void UShootGameplayAbility_Weapon_Fire::TraceBulletsInCartridge(
	UShootRangedWeaponInstance* WeaponData,
	const FVector& StartTrace,
	const FVector& AimDir,
	TArray<FHitResult>& OutHits)
{
	check(WeaponData);

	const int32 BulletsPerCartridge = WeaponData->GetBulletsPerCartridge();
	const float BaseSpreadAngle = WeaponData->GetCalculatedSpreadAngle();
	const float SpreadAngleMultiplier = WeaponData->GetCalculatedSpreadAngleMultiplier();
	const float ActualSpreadAngle = BaseSpreadAngle * SpreadAngleMultiplier;
	const float HalfSpreadAngleInRadians = FMath::DegreesToRadians(ActualSpreadAngle * 0.5f);
	const float SpreadExponent = WeaponData->GetSpreadExponent();
	const float SweepRadius = WeaponData->GetBulletTraceSweepRadius();
	const float MaxDamageRange = WeaponData->GetMaxDamageRange();

	// 射击多颗子弹（霰弹枪 = 8，步枪 = 1）
	for (int32 BulletIndex = 0; BulletIndex < BulletsPerCartridge; ++BulletIndex)
	{
		// 每颗子弹独立计算扩散方向
		const FVector BulletDir = VRandConeNormalDistribution(AimDir, HalfSpreadAngleInRadians, SpreadExponent);

		// 计算射线终点
		const FVector EndTrace = StartTrace + (BulletDir * MaxDamageRange);

		// 执行单颗子弹的射线检测
		TArray<FHitResult> SingleBulletHits;
		const FHitResult Impact = DoSingleBulletTrace(StartTrace, EndTrace, SweepRadius, /*bIsSimulated=*/ false,
		                                              /*out*/ SingleBulletHits);

		// 与 Lyra 一致：真实命中才追加这一颗 pellet 的全部阻挡结果。
		if (Impact.GetActor())
		{
			OutHits.Append(SingleBulletHits);
		}

		// Lyra 只保证整发弹药至少有一条 TargetData 供曳光/枪口表现使用；
		// 不为每颗落空 pellet 伪造命中，避免一次霰弹产生九条无意义远端 TargetData。
		if (OutHits.Num() == 0)
		{
			FHitResult VirtualHit = Impact;
			if (!VirtualHit.bBlockingHit)
			{
				VirtualHit.Location = EndTrace;
				VirtualHit.ImpactPoint = EndTrace;
			}
			OutHits.Add(VirtualHit);
		}
	}
}

FHitResult UShootGameplayAbility_Weapon_Fire::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace,
                                                          float SweepRadius, bool bIsSimulated,
                                                          TArray<FHitResult>& OutHitResults) const
{
	TArray<FHitResult> HitResults;

	// 设置射线检测参数
	FCollisionQueryParams TraceParams(
		SCENE_QUERY_STAT(WeaponTrace),
		/*bTraceComplex=*/ true, // 使用复杂碰撞（精确网格）
		/*IgnoreActor=*/ GetAvatarActorFromActorInfo()
	);
	TraceParams.bReturnPhysicalMaterial = true; // 需要物理材质来计算爆头等
	AddAdditionalTraceIgnoreActors(TraceParams);
	//TraceParams.bDebugQuery = true;

	const ECollisionChannel TraceChannel = DetermineTraceChannel(TraceParams, bIsSimulated);

	// 根据是否有扫描半径选择检测方式
	if (SweepRadius > 0.0f)
	{
		// 球体扫描（霰弹枪等）
		GetWorld()->SweepMultiByChannel(
			HitResults,
			StartTrace,
			EndTrace,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(SweepRadius),
			TraceParams
		);
	}
	else
	{
		// 线性射线（步枪、狙击枪等）
		GetWorld()->LineTraceMultiByChannel(
			HitResults,
			StartTrace,
			EndTrace,
			TraceChannel,
			TraceParams
		);
	}

	// 过滤重复命中（同一个 Actor 只记录一次）
	FHitResult Hit(ForceInit);
	if (HitResults.Num() > 0)
	{
		for (FHitResult& CurHitResult : HitResults)
		{
			// 检查是否已经记录过这个 Actor
			if (!OutHitResults.ContainsByPredicate(
				[&](const FHitResult& Other) { return Other.HitObjectHandle == CurHitResult.HitObjectHandle; }))
			{
				OutHitResults.Add(CurHitResult);
			}
		}
		Hit = OutHitResults.Last();
	}
	else
	{
		// 没有命中任何东西，记录射线起点和终点
		Hit.TraceStart = StartTrace;
		Hit.TraceEnd = EndTrace;
	}

	return Hit;
}

FHitResult UShootGameplayAbility_Weapon_Fire::DoSingleBulletTrace(
	const FVector& StartTrace,
	const FVector& EndTrace,
	float SweepRadius,
	bool bIsSimulated,
	TArray<FHitResult>& OutHits) const
{
	FHitResult Impact;

	// 第一步：尝试精确的线性射线检测
	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE)
	{
		Impact = WeaponTrace(StartTrace, EndTrace, /*SweepRadius=*/ 0.0f, bIsSimulated, /*out*/ OutHits);
	}

	// 第二步：如果没命中 Pawn 且支持扫描半径，尝试球体扫描
	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE && SweepRadius > 0.0f)
	{
		TArray<FHitResult> SweepHits;
		Impact = WeaponTrace(StartTrace, EndTrace, SweepRadius, bIsSimulated, /*out*/ SweepHits);

		// 检查扫描是否命中了 Pawn
		const int32 FirstPawnIdx = FindFirstPawnHitResult(SweepHits);
		if (SweepHits.IsValidIndex(FirstPawnIdx))
		{
			// 验证：如果线性检测已经有阻挡物在 Pawn 前面，就不使用扫描结果
			bool bUseSweepHits = true;
			for (int32 Idx = 0; Idx < FirstPawnIdx; ++Idx)
			{
				const FHitResult& CurHitResult = SweepHits[Idx];
				if (CurHitResult.bBlockingHit && OutHits.ContainsByPredicate(
					[&](const FHitResult& Other) { return Other.HitObjectHandle == CurHitResult.HitObjectHandle; }))
				{
					bUseSweepHits = false;
					break;
				}
			}

			if (bUseSweepHits)
			{
				OutHits = SweepHits;
			}
		}
	}

	return Impact;
}
