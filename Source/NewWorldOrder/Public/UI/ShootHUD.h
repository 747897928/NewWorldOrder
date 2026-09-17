// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "CustomBaseHUD.h"
#include "ShootHUD.generated.h"

class UPlayerDeviceSlot;
class AShootCharacter;
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API AShootHUD : public ACustomBaseHUD
{
	GENERATED_BODY()
	
	friend class AShootCharacter;

public:

	AShootHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual void PreInitializeComponents() override;
	
		
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~End of UObject interface
protected:
	//~AActor interface
	virtual void BeginPlay() override;

private:
	
	
	
};
