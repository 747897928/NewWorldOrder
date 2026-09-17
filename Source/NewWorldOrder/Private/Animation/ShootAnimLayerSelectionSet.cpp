// Copyright ZhaoYiJie

#include "Animation/ShootAnimLayerSelectionSet.h"
#include "Animation/AnimInstance.h"

TSubclassOf<UAnimInstance> FShootAnimLayerSelectionSet::SelectBestLayer(
	const FGameplayTagContainer& CosmeticTags) const
{
	for (const FShootAnimLayerSelectionEntry& Rule : LayerRules)
	{
		if (Rule.Layer && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Layer;
		}
	}

	return DefaultLayer;
}
