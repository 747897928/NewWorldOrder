// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Wardrobe/ShootWardrobeCatalogDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWardrobeCatalogDataAsset)

void UShootWardrobeCatalogDataAsset::AppendWardrobeItemsTo(
	TArray<TSubclassOf<UShootInventoryItemDefinition>>& InOutItems) const
{
	for (const TSubclassOf<UShootInventoryItemDefinition>& ItemDefinition : WardrobeItems)
	{
		if (ItemDefinition)
		{
			InOutItems.AddUnique(ItemDefinition);
		}
	}
}

const FShootWardrobePageDefinition* UShootWardrobeCatalogDataAsset::FindPageDefinition(FName TabId) const
{
	return PageDefinitions.FindByPredicate([TabId](const FShootWardrobePageDefinition& Definition)
	{
		return Definition.CategoryTag.IsValid() && Definition.CategoryTag.GetTagName() == TabId;
	});
}

const FShootWardrobeSubCategoryDefinition* UShootWardrobeCatalogDataAsset::FindSubCategoryDefinition(
	FGameplayTag ParentCategoryTag,
	FGameplayTag CategoryTag) const
{
	return SubCategoryDefinitions.FindByPredicate([ParentCategoryTag, CategoryTag](
		const FShootWardrobeSubCategoryDefinition& Definition)
	{
		return Definition.ParentCategoryTag.MatchesTagExact(ParentCategoryTag)
			&& Definition.CategoryTag.MatchesTagExact(CategoryTag);
	});
}

void UShootWardrobeCatalogDataAsset::GetSubCategoryDefinitions(
	FGameplayTag ParentCategoryTag,
	TArray<const FShootWardrobeSubCategoryDefinition*>& OutDefinitions) const
{
	OutDefinitions.Reset();
	for (const FShootWardrobeSubCategoryDefinition& Definition : SubCategoryDefinitions)
	{
		if (Definition.ParentCategoryTag.MatchesTagExact(ParentCategoryTag))
		{
			OutDefinitions.Add(&Definition);
		}
	}
}
