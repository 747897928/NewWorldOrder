// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/Robot/ShootGA_RobotMelee.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "Interface/CombatInterface.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_RobotMelee)

UShootGA_RobotMelee::UShootGA_RobotMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FShootGameplayTags::Get().Ability_Skill_Robot_Melee);
	SetAssetTags(AssetTags);
	MeleeGameplayCueTag = FShootGameplayTags::Get().GameplayCue_Skill_Robot_Melee;
}

bool UShootGA_RobotMelee::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	return Robot && World && Robot->GetPendingMeleeTarget()
		&& World->GetTimeSeconds() >= NextAllowedMeleeTime;
}

void UShootGA_RobotMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AShootRobotCompanionCharacter* Robot = ActorInfo
		? Cast<AShootRobotCompanionCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	AActor* Target = Robot ? Robot->GetPendingMeleeTarget() : nullptr;
	if (!Robot || !Target || !ActorInfo->IsNetAuthority() || !ValidateTarget(*Robot, *Target)
		|| !Robot->IsFacingAttackTarget(Target))
	{
		if (Robot)
		{
			Robot->ClearPendingMeleeTarget();
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveRobot = Robot;
	ActiveTarget = Target;
	bUseChomp = Robot->ChooseChompAttack();
	bImpactExecuted = false;
	NextAllowedMeleeTime = Robot->GetWorld()->GetTimeSeconds() + Robot->GetMeleeInterval(bUseChomp);
	Robot->OnMeleeAnimationNotify().AddUObject(this, &ThisClass::HandleMeleeNotify);
	Robot->PlayReplicatedMeleeMontage(bUseChomp);
	Robot->GetWorldTimerManager().SetTimer(MissingNotifyTimerHandle, this,
		&ThisClass::ExecuteImpactAndFinish, MissingNotifyFallbackDelay, false);
}

void UShootGA_RobotMelee::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (AShootRobotCompanionCharacter* Robot = ActiveRobot.Get())
	{
		Robot->OnMeleeAnimationNotify().RemoveAll(this);
		Robot->GetWorldTimerManager().ClearTimer(MissingNotifyTimerHandle);
		Robot->ClearPendingMeleeTarget();
	}
	ActiveRobot.Reset();
	ActiveTarget.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_RobotMelee::HandleMeleeNotify()
{
	ExecuteImpactAndFinish();
}

void UShootGA_RobotMelee::ExecuteImpactAndFinish()
{
	if (bImpactExecuted)
	{
		return;
	}
	bImpactExecuted = true;
	AShootRobotCompanionCharacter* Robot = ActiveRobot.Get();
	AActor* Target = ActiveTarget.Get();
	if (Robot && Target && ValidateTarget(*Robot, *Target) && Robot->IsFacingAttackTarget(Target))
	{
		Robot->ApplyMeleeImpact(Target, bUseChomp);
		if (MeleeGameplayCueTag.IsValid())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = Robot->GetActorLocation() + Robot->GetActorForwardVector() * 105.f;
			Parameters.Normal = Robot->GetActorForwardVector();
			Parameters.EffectCauser = Robot;
			Parameters.RawMagnitude = Robot->GetMeleeDamage(bUseChomp);
			K2_ExecuteGameplayCueWithParams(MeleeGameplayCueTag, Parameters);
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UShootGA_RobotMelee::ValidateTarget(const AShootRobotCompanionCharacter& Robot,
	const AActor& Target) const
{
	const bool bTargetAlreadyDead = Target.Implements<UCombatInterface>()
		&& ICombatInterface::Execute_IsDead(&Target);
	return IsValid(&Target) && !bTargetAlreadyDead
		&& UShootAbilitySystemLibrary::GetTeamAttitudeForActors(&Robot, &Target) == ETeamAttitude::Hostile
		&& Target.Implements<UAbilitySystemInterface>()
		&& FVector::DistSquared(Robot.GetActorLocation(), Target.GetActorLocation())
			<= FMath::Square(Robot.GetMeleeRange());
}
