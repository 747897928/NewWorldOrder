// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SavedInventoryTypes.generated.h"

class UShootInventoryItemDefinition;

USTRUCT(BlueprintType)
struct FSavedTagStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FSavedInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TSoftClassPtr<UShootInventoryItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<FSavedTagStack> TagStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 StackCount = 1;
};

USTRUCT(BlueprintType)
struct FSavedQuickbarSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ItemInstanceId;
};
