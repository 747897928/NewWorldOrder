// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootAnimNotify_RobotMelee.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_RobotMelee)

void UShootAnimNotify_RobotMelee::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (AShootRobotCompanionCharacter* Robot = MeshComp
		? Cast<AShootRobotCompanionCharacter>(MeshComp->GetOwner())
		: nullptr)
	{
		Robot->NotifyMeleeAnimationEvent();
	}
}
