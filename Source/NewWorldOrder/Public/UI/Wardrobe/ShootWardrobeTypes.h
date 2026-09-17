// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/Fragments/ShootInventoryFragment_WardrobeItem.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Styling/SlateBrush.h"
#include "ShootWardrobeTypes.generated.h"

UENUM(BlueprintType)
enum class EShootWardrobePreviewCameraMode : uint8
{
	FullBody,
	UpperBody,
	Head,
	Footwear
};

USTRUCT(BlueprintType)
struct FShootWardrobeItemViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	TObjectPtr<UShootInventoryItemInstance> ItemInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	TSubclassOf<UShootInventoryItemDefinition> ItemDefinition;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FGuid ItemInstanceId;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FSlateBrush Icon;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FGameplayTagContainer AppearanceTags;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe|Preview")
	TSoftObjectPtr<UAnimSequenceBase> PreviewEquipAnimation;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	ECharacterGender SuitableGender = ECharacterGender::UNKNOWN;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeCategory Category = EShootWardrobeCategory::UpperBody;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	FGameplayTagContainer SubCategoryTags;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeRarity Rarity = EShootWardrobeRarity::Common;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	int32 SortPriority = 0;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bOwned = false;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bLocked = true;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bCanEquip = false;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bEquipped = false;
};
