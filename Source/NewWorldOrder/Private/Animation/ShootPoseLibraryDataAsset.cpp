// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Animation/ShootPoseLibraryDataAsset.h"

const FShootPoseLibraryEntry* UShootPoseLibraryDataAsset::GetPoseEntry(int32 PoseIndex) const
{
	return PoseEntries.IsValidIndex(PoseIndex) ? &PoseEntries[PoseIndex] : nullptr;
}

bool UShootPoseLibraryDataAsset::IsPosePlayableForGender(int32 PoseIndex, ECharacterGender CurrentGender) const
{
	const FShootPoseLibraryEntry* Entry = GetPoseEntry(PoseIndex);
	if (!Entry || !Entry->Animation)
	{
		return false;
	}

	return Entry->CompatibleGender == ECharacterGender::UNKNOWN || Entry->CompatibleGender == CurrentGender;
}
