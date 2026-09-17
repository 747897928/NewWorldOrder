// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CustomBaseHUD.generated.h"

class UPrimaryGameLayout;
class UCommonActivatableWidget;
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API ACustomBaseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void PreInitializeComponents() override;

	virtual void BeginDestroy() override;

protected:
	virtual void BeginPlay() override;
};
