// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/ShootGameStateBase.h"

#include "GameModes/ShootExperienceManagerComponent.h"
#include "GameModes/ShootExpeditionLobbyComponent.h"
#include "Net/UnrealNetwork.h"

AShootGameStateBase::AShootGameStateBase()
{
	bReplicates = true;
	ExperienceManagerComponent = CreateDefaultSubobject<UShootExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
	ExpeditionLobbyComponent = CreateDefaultSubobject<UShootExpeditionLobbyComponent>(TEXT("ExpeditionLobbyComponent"));
}

void AShootGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
