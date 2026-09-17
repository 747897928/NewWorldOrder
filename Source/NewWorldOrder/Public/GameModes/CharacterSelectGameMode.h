// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CharacterSelectGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API ACharacterSelectGameMode : public AGameMode
{
	GENERATED_BODY()

protected:
	
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;
};
