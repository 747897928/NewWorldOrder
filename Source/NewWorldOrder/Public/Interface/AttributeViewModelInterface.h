// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttributeViewModelInterface.generated.h"

class UAttributeViewModel;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UAttributeViewModelInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class NEWWORLDORDER_API IAttributeViewModelInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ViewModel")
	/** Returns the ability system component to use for this actor. It may live on another actor, such as a Pawn using the PlayerState's component */
	UAttributeViewModel* GetAttributeViewModel();
};
