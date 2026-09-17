// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"

#include "ShootSkillDefinition.generated.h"

class UShootAbilitySet;
class UInputAction;

/**
 * 单个 Match Skill 的权威目录条目。
 * 图标、文案、最大等级与 AbilitySet 都由 DataAsset 配置；SaveGame 只允许保存账号物品，不能保存本对象的局内槽位状态。
 */
UCLASS(BlueprintType, Const)
class NEWWORLDORDER_API UShootSkillDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill", meta=(Categories="Ability"))
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill")
	FSlateBrush Icon;

	/** 技能的能力、长期效果与可选 AttributeSet 包；槽位组件持有其完整撤销句柄。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill")
	TObjectPtr<const UShootAbilitySet> AbilitySet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill", meta=(ClampMin="1", ClampMax="3"))
	int32 MaxLevel = 3;

	/** UI 查询剩余冷却时匹配的拥有标签。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill", meta=(Categories="Cooldown"))
	FGameplayTag CooldownTag;

	/** 机器人等多模式技能的可选模式目录；首项是首次获取时的默认模式。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill", meta=(Categories="Ability.Mode"))
	TArray<FGameplayTag> ModeTags;

	/** 可选模式图标；机器人切换模式时槽位仍是同一技能，只替换图标帮助玩家确认指令。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill")
	TMap<FGameplayTag, FSlateBrush> ModeIcons;

	/** 可选模式显示名；未配置时继续显示技能总名。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill")
	TMap<FGameplayTag, FText> ModeDisplayNames;

	const FSlateBrush& GetIconForMode(FGameplayTag ModeTag) const
	{
		if (const FSlateBrush* ModeIcon = ModeIcons.Find(ModeTag))
		{
			return *ModeIcon;
		}
		return Icon;
	}

	FText GetDisplayNameForMode(FGameplayTag ModeTag) const
	{
		if (const FText* ModeName = ModeDisplayNames.Find(ModeTag))
		{
			return *ModeName;
		}
		return DisplayName;
	}
};

/**
 * Experience 级四槽技能配置。
 * SlotInputTags 的顺序只表达槽位语义，具体键鼠/手柄键位仍由 Enhanced Input 资产决定。
 */
UCLASS(BlueprintType, Const)
class NEWWORLDORDER_API UShootSkillLoadoutConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Slots", meta=(Categories="InputTag"))
	TArray<FGameplayTag> SlotInputTags;

	/**
	 * 与 SlotInputTags 同序的 Enhanced Input Action。
	 * InputTag 决定 ASC 将技能授予哪个槽，InputAction 只给 CommonUI/触摸按钮显示当前改键并注入同一输入动作；
	 * 具体键盘、手柄和触摸映射仍全部留在 IMC，禁止在 Widget C++ 中硬编码 C/Q/E/X。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Slots")
	TArray<TObjectPtr<UInputAction>> SlotInputActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Random Acquisition")
	TArray<TObjectPtr<const UShootSkillDefinition>> RandomSkillPool;
};
