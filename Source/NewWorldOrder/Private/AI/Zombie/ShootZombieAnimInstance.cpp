// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Zombie/ShootZombieAnimInstance.h"

#include "AI/EnemyBotCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootZombieAnimInstance)

void UShootZombieAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ZombieCharacter = Cast<AEnemyBotCharacter>(TryGetPawnOwner());
}

void UShootZombieAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AEnemyBotCharacter* Zombie = ZombieCharacter.Get();
	if (!IsValid(Zombie))
	{
		ZombieCharacter = Cast<AEnemyBotCharacter>(TryGetPawnOwner());
		Zombie = ZombieCharacter.Get();
	}

	if (!IsValid(Zombie))
	{
		GroundSpeed = 0.0f;
		bIsMoving = false;
		bIsFalling = false;
		bIsDead = false;
		bIsAttacking = false;
		return;
	}

	GroundSpeed = Zombie->GetVelocity().Size2D();
	bIsMoving = GroundSpeed > KINDA_SMALL_NUMBER;
	bIsFalling = Zombie->GetCharacterMovement() && Zombie->GetCharacterMovement()->IsFalling();
	bIsDead = Zombie->GetDeathState() != EShootDeathState::NotDead;
	bIsAttacking = Zombie->IsArchetypeAttackAnimationActive();
}
