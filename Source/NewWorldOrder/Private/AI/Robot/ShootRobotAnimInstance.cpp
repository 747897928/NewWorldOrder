// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootRobotAnimInstance.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRobotAnimInstance)

void UShootRobotAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RobotCharacter = Cast<AShootRobotCompanionCharacter>(TryGetPawnOwner());
}

void UShootRobotAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	AShootRobotCompanionCharacter* Robot = RobotCharacter.Get();
	if (!Robot)
	{
		Robot = Cast<AShootRobotCompanionCharacter>(TryGetPawnOwner());
		RobotCharacter = Robot;
	}
	if (!Robot)
	{
		return;
	}
	GroundSpeed = Robot->GetVelocity().Size2D();
	bIsMoving = GroundSpeed > 5.f;
	bIsFalling = Robot->GetCharacterMovement() && Robot->GetCharacterMovement()->IsFalling();
	bIsDead = Robot->GetDeathState() != EShootDeathState::NotDead;
}
