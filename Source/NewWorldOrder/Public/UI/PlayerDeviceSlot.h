// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PlayerDeviceSlot.generated.h"

class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UPlayerDeviceSlot : public UCommonUserWidget
{
	GENERATED_BODY()

	friend class AShootHUD;
	
public:
	UPlayerDeviceSlot();
	
protected:

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	
};
