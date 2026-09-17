// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootCharacterSwitchStation.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "Player/ShootPlayerState.h"

AShootCharacterSwitchStation::AShootCharacterSwitchStation()
{
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(90.f);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AShootCharacterSwitchStation::BeginPlay()
{
	Super::BeginPlay();
}

bool AShootCharacterSwitchStation::ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const
{
	OutTargetGender = ECharacterGender::UNKNOWN;
	if (!RequestingPawn)
	{
		return false;
	}

	const AShootPlayerState* ShootPS = RequestingPawn->GetPlayerState<AShootPlayerState>();
	if (!ShootPS)
	{
		if (const AController* Controller = RequestingPawn->GetController())
		{
			ShootPS = Controller->GetPlayerState<AShootPlayerState>();
		}
	}

	if (!ShootPS)
	{
		return false;
	}

	const ECharacterGender CurrentGender = ShootPS->GetCharacterGender();
	if (CurrentGender == ECharacterGender::MALE)
	{
		OutTargetGender = ECharacterGender::FEMALE;
		return true;
	}
	if (CurrentGender == ECharacterGender::FEMALE)
	{
		OutTargetGender = ECharacterGender::MALE;
		return true;
	}

	return false;
}
