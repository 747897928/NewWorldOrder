// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootAnimNotify_RobotFire.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_RobotFire)

void UShootAnimNotify_RobotFire::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (AShootRobotCompanionCharacter* Robot = MeshComp
		? Cast<AShootRobotCompanionCharacter>(MeshComp->GetOwner())
		: nullptr)
	{
		Robot->NotifyFireAnimationEvent();
	}
}
