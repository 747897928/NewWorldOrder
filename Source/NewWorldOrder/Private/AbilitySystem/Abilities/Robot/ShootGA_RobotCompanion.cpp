// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/Robot/ShootGA_RobotCompanion.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"
#include "AbilitySystem/Skills/ShootRobotCompanionComponent.h"
#include "AbilitySystem/Skills/ShootSkillDefinition.h"
#include "GameFramework/Pawn.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_RobotCompanion)

UShootGA_RobotCompanion::UShootGA_RobotCompanion()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FShootGameplayTags::Get().Ability_Skill_RobotCompanion);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(FShootGameplayTags::Get().Cooldown_Skill_RobotCompanion);
	DestroyedCooldownEffectClass = UShootEffect_RobotCompanionCooldown::StaticClass();
}

void UShootGA_RobotCompanion::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APawn* Pawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	AShootPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AShootPlayerState>() : nullptr;
	UShootRobotCompanionComponent* Component = PlayerState
		? PlayerState->GetRobotCompanionComponent()
		: nullptr;
	const UShootSkillDefinition* Definition = Cast<UShootSkillDefinition>(GetSourceObject(Handle, ActorInfo));
	const int32 AbilityLevel = FMath::Max(1, GetAbilityLevel(Handle, ActorInfo));
	const float EffectiveLifetime = CompanionLifetime
		+ CompanionLifetimePerLevel * static_cast<float>(AbilityLevel - 1);
	const bool bSucceeded = ActorInfo && ActorInfo->IsNetAuthority() && Component && Definition
		&& Component->ActivateOrCycleRobot(Pawn, Definition, RobotCharacterClass,
			DestroyedCooldownEffectClass, SpawnOffset, SummonDropHeight, EffectiveLifetime,
			bOwnerDeathStartsCooldown);

	// 此能力不能 CommitAbility：机器人存在期间再次按键是模式指令；只有战斗击毁时由 PlayerState
	// 组件单独施加冷却 GE。把冷却绑在普通 Commit 上会让第一次召唤后立刻无法切模式。
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bSucceeded);
}
