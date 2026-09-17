// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/Robot/ShootGA_RobotFire.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Interface/CombatInterface.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_RobotFire)

UShootGA_RobotFire::UShootGA_RobotFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FShootGameplayTags::Get().Ability_Skill_Robot_Fire);
	SetAssetTags(AssetTags);
	FireGameplayCueTag = FShootGameplayTags::Get().GameplayCue_Skill_Robot_Fire;
}

bool UShootGA_RobotFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const AShootRobotCompanionCharacter* Robot = ActorInfo
		? Cast<AShootRobotCompanionCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	const UWorld* World = Robot ? Robot->GetWorld() : nullptr;
	return Robot && World && Robot->GetPendingFireTarget()
		&& World->GetTimeSeconds() >= NextAllowedFireTime;
}

void UShootGA_RobotFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AShootRobotCompanionCharacter* Robot = ActorInfo
		? Cast<AShootRobotCompanionCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	AActor* Target = Robot ? Robot->GetPendingFireTarget() : nullptr;
	if (!Robot || !Target || !ActorInfo->IsNetAuthority() || !ValidateTarget(*Robot, *Target)
		|| !Robot->IsFacingAttackTarget(Target))
	{
		if (Robot)
		{
			Robot->ClearPendingFireTarget();
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveRobot = Robot;
	ActiveTarget = Target;
	bShotExecuted = false;
	bool bLeftMuzzle = false;
	ActiveMuzzle = Robot->GetNextMuzzle(bLeftMuzzle);
	NextAllowedFireTime = Robot->GetWorld()->GetTimeSeconds() + Robot->GetFireInterval();
	Robot->OnFireAnimationNotify().AddUObject(this, &ThisClass::HandleFireNotify);
	Robot->PlayReplicatedFireMontage(bLeftMuzzle);

	// 正式 Montage 必须放 UShootAnimNotify_RobotFire。兜底只防止错误资产配置把 GA 永久挂起。
	Robot->GetWorldTimerManager().SetTimer(MissingNotifyTimerHandle, this,
		&ThisClass::ExecuteShotAndFinish, MissingNotifyFallbackDelay, false);
}

void UShootGA_RobotFire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (AShootRobotCompanionCharacter* Robot = ActiveRobot.Get())
	{
		Robot->OnFireAnimationNotify().RemoveAll(this);
		Robot->GetWorldTimerManager().ClearTimer(MissingNotifyTimerHandle);
		Robot->ClearPendingFireTarget();
	}
	ActiveRobot.Reset();
	ActiveTarget.Reset();
	ActiveMuzzle.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_RobotFire::HandleFireNotify()
{
	ExecuteShotAndFinish();
}

void UShootGA_RobotFire::ExecuteShotAndFinish()
{
	if (bShotExecuted)
	{
		return;
	}
	bShotExecuted = true;
	AShootRobotCompanionCharacter* Robot = ActiveRobot.Get();
	AActor* Target = ActiveTarget.Get();
	USceneComponent* Muzzle = ActiveMuzzle.Get();
	if (Robot && Target && Muzzle && ValidateTarget(*Robot, *Target)
		&& Robot->IsFacingAttackTarget(Target))
	{
		FVector TargetOrigin;
		FVector TargetExtent;
		Target->GetActorBounds(true, TargetOrigin, TargetExtent);
		const FVector Start = Muzzle->GetComponentLocation();
		const FVector End = TargetOrigin;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RobotCompanionFire), true, Robot);
		Params.AddIgnoredActor(Robot->GetCompanionOwnerPawn());
		const bool bHit = Robot->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		AActor* HitActor = bHit ? Hit.GetActor() : Target;
		// 枪口反馈属于“本枪已发射”，不依赖是否最终命中 Hostile；否则被墙挡住时会像技能没有执行。
		if (FireGameplayCueTag.IsValid())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = Start;
			CueParameters.Normal = (End - Start).GetSafeNormal();
			CueParameters.TargetAttachComponent = Muzzle;
			CueParameters.EffectCauser = Robot;
			CueParameters.RawMagnitude = Robot->GetShotDamage();
			K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, CueParameters);
		}
		// 拥挤尸群中，瞄准目标前方可能站着另一只 Hostile；子弹应命中最前面的合法敌人，不能整发静默丢失。
		if (HitActor && ValidateTarget(*Robot, *HitActor))
		{
			IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor);
			UAbilitySystemComponent* TargetASC = TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;
			UAbilitySystemComponent* SourceASC = Robot->GetAbilitySystemComponent();
			if (TargetASC && SourceASC && Robot->GetDamageEffectClass())
			{
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				// 召唤者是伤害 Instigator，机器人是 EffectCauser。玩家因此能收到伤害数字与击杀归属，
				// 同时命中与技能执行仍由机器人自己的服务器 ASC 负责。
				AActor* DamageInstigator = IsValid(Robot->GetCompanionOwnerPawn())
					? Robot->GetCompanionOwnerPawn()
					: Robot;
				Context.AddInstigator(DamageInstigator, Robot);
				Context.AddSourceObject(Robot);
				if (bHit)
				{
					Context.AddHitResult(Hit);
				}
				if (const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
					Robot->GetDamageEffectClass(), Robot->GetPlayerLevel_Implementation(), Context); Spec.IsValid())
				{
					FShootGameplayTags::SetSetByCallerMagnitude(
						*Spec.Data.Get(), FShootGameplayTags::Get().SetByCaller_Damage, Robot->GetShotDamage());
					SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UShootGA_RobotFire::ValidateTarget(const AShootRobotCompanionCharacter& Robot,
	const AActor& Target) const
{
	const bool bTargetAlreadyDead = Target.Implements<UCombatInterface>()
		&& ICombatInterface::Execute_IsDead(&Target);
	return IsValid(&Target) && !bTargetAlreadyDead
		&& UShootAbilitySystemLibrary::GetTeamAttitudeForActors(&Robot, &Target) == ETeamAttitude::Hostile
		&& Target.Implements<UAbilitySystemInterface>()
		&& FVector::DistSquared(Robot.GetActorLocation(), Target.GetActorLocation())
			<= FMath::Square(Robot.GetAttackRange());
}
