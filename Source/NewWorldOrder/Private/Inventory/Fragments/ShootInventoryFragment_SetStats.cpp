// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/Fragments/ShootInventoryFragment_SetStats.h"

#include "Inventory/ShootInventoryItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryFragment_SetStats)

void UShootInventoryFragment_SetStats::OnInstanceCreated(UShootInventoryItemInstance* Instance) const
{
	if (!Instance)
	{
		return;
	}

	for (const TPair<FGameplayTag, int32>& InitialStat : InitialItemStats)
	{
		if (InitialStat.Key.IsValid() && InitialStat.Value > 0)
		{
			Instance->AddStatTagStack(InitialStat.Key, InitialStat.Value);
		}
	}
}

int32 UShootInventoryFragment_SetStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* Value = InitialItemStats.Find(Tag))
	{
		return *Value;
	}

	return 0;
}
