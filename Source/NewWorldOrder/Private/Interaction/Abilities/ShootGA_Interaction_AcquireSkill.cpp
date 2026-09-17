// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_AcquireSkill.h"

#include "GameFramework/Pawn.h"
#include "Interaction/ShootSkillGrantSource.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_AcquireSkill)

UShootGA_Interaction_AcquireSkill::UShootGA_Interaction_AcquireSkill(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UShootGA_Interaction_AcquireSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = TriggerEventData
		? const_cast<AActor*>(TriggerEventData->Target.Get())
		: nullptr;
	IShootSkillGrantSource* SkillSource = Cast<IShootSkillGrantSource>(TargetActor);

	// 本地预测只负责即时完成交互动画；技能槽、AbilitySet 和来源消费始终由服务器提交。
	const bool bGranted = ActorInfo && ActorInfo->IsNetAuthority() && InstigatorPawn && SkillSource &&
		SkillSource->CanGrantSkillToPawn(InstigatorPawn) && SkillSource->TryGrantSkillToPawn(InstigatorPawn);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true,
		ActorInfo && ActorInfo->IsNetAuthority() && !bGranted);
}
