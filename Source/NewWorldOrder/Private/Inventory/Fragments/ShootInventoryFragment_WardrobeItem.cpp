// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/Fragments/ShootInventoryFragment_WardrobeItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryFragment_WardrobeItem)

bool UShootInventoryFragment_WardrobeItem::CanApplyToGender(ECharacterGender CurrentGender) const
{
	if (CurrentGender == ECharacterGender::UNKNOWN)
	{
		return false;
	}

	if (SuitableGender != ECharacterGender::UNKNOWN && SuitableGender != CurrentGender)
	{
		return false;
	}

	if (AppearanceTags.Num() == 0)
	{
		return false;
	}

	TArray<FGameplayTag> TagArray;
	AppearanceTags.GetGameplayTagArray(TagArray);
	for (const FGameplayTag& Tag : TagArray)
	{
		if (!DoesTagMatchGender(Tag, CurrentGender))
		{
			return false;
		}
	}

	return true;
}

bool UShootInventoryFragment_WardrobeItem::IsEquippedInTags(const FGameplayTagContainer& CurrentAppearanceTags) const
{
	return AppearanceTags.Num() > 0 && CurrentAppearanceTags.HasAllExact(AppearanceTags);
}

FGameplayTagContainer UShootInventoryFragment_WardrobeItem::BuildTagsAfterEquip(
	const FGameplayTagContainer& CurrentAppearanceTags,
	ECharacterGender CurrentGender) const
{
	FGameplayTagContainer NextTags;
	const FGameplayTagContainer& NewTags = AppearanceTags;
	if (NewTags.Num() == 0 || !CanApplyToGender(CurrentGender))
	{
		return CurrentAppearanceTags;
	}

	if (!bReplaceAllCurrentAppearanceTags)
	{
		NextTags = CurrentAppearanceTags;
	}

	if (bReplaceTagsInSameMutableSlot)
	{
		TSet<FString> SlotsToReplace;
		TArray<FGameplayTag> NewTagArray;
		NewTags.GetGameplayTagArray(NewTagArray);
		for (const FGameplayTag& NewTag : NewTagArray)
		{
			const FString SlotKey = BuildMutableSlotKey(NewTag);
			if (!SlotKey.IsEmpty())
			{
				SlotsToReplace.Add(SlotKey);
			}
		}

		TArray<FGameplayTag> CurrentTagArray;
		NextTags.GetGameplayTagArray(CurrentTagArray);
		for (const FGameplayTag& ExistingTag : CurrentTagArray)
		{
			const FString ExistingSlotKey = BuildMutableSlotKey(ExistingTag);
			if (!ExistingSlotKey.IsEmpty() && SlotsToReplace.Contains(ExistingSlotKey))
			{
				NextTags.RemoveTag(ExistingTag);
			}
		}
	}

	NextTags.AppendTags(NewTags);
	return NextTags;
}

FGameplayTagContainer UShootInventoryFragment_WardrobeItem::BuildTagsAfterUnequip(
	const FGameplayTagContainer& CurrentAppearanceTags,
	ECharacterGender CurrentGender) const
{
	const FGameplayTagContainer& TagsToRemove = AppearanceTags;
	if (TagsToRemove.Num() == 0 || !CanApplyToGender(CurrentGender))
	{
		return CurrentAppearanceTags;
	}

	FGameplayTagContainer NextTags = CurrentAppearanceTags;

	TArray<FGameplayTag> TagArray;
	TagsToRemove.GetGameplayTagArray(TagArray);
	for (const FGameplayTag& Tag : TagArray)
	{
		NextTags.RemoveTag(Tag);
	}

	return NextTags;
}

FString UShootInventoryFragment_WardrobeItem::BuildMutableSlotKey(const FGameplayTag& Tag)
{
	TArray<FString> Parts;
	Tag.ToString().ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() < 3)
	{
		return FString();
	}

	return FString::Printf(TEXT("%s.%s.%s"), *Parts[0], *Parts[1], *Parts[2]);
}

bool UShootInventoryFragment_WardrobeItem::DoesTagMatchGender(const FGameplayTag& Tag, ECharacterGender Gender)
{
	TArray<FString> Parts;
	Tag.ToString().ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() == 0)
	{
		return false;
	}

	if (Gender == ECharacterGender::MALE)
	{
		return Parts[0].Equals(TEXT("Male"), ESearchCase::IgnoreCase);
	}

	if (Gender == ECharacterGender::FEMALE)
	{
		return Parts[0].Equals(TEXT("Female"), ESearchCase::IgnoreCase);
	}

	return false;
}
