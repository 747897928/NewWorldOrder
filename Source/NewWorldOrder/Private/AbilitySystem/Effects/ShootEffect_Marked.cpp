// 通用标记效果：授予 Status.Marked
#include "AbilitySystem/Effects/ShootEffect_Marked.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

UShootEffect_Marked::UShootEffect_Marked(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 注意：构造函数内不使用 FShootGameplayTags::Get()，避免初始化时机问题
	const FGameplayTag MarkedTag = FGameplayTag::RequestGameplayTag(FName("Status.Marked"), false);

	if (MarkedTag.IsValid())
	{
		UTargetTagsGameplayEffectComponent* TargetTags = ObjectInitializer.CreateDefaultSubobject<
			UTargetTagsGameplayEffectComponent>(this, TEXT("TargetTags"));
		FInheritedTagContainer TagChanges;
		TagChanges.AddTag(MarkedTag);
		TargetTags->SetAndApplyTargetTagChanges(TagChanges);
		GEComponents.Add(TargetTags);
	}
}
