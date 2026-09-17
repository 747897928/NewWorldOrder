// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "GameplayTagContainer.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryFragment_WardrobeItem.generated.h"

class UAnimSequenceBase;

UENUM(BlueprintType)
enum class EShootWardrobeCategory : uint8
{
	Outfit UMETA(DisplayName="Outfit"),
	Head UMETA(DisplayName="Head"),
	UpperBody UMETA(DisplayName="Upper Body"),
	LowerBody UMETA(DisplayName="Lower Body"),
	Dress UMETA(DisplayName="Dress"),
	Footwear UMETA(DisplayName="Footwear"),
	Hair UMETA(DisplayName="Hair"),
	Accessory UMETA(DisplayName="Accessory"),
	Preset UMETA(DisplayName="Preset")
};

UENUM(BlueprintType)
enum class EShootWardrobeRarity : uint8
{
	Common UMETA(DisplayName="Common"),
	Rare UMETA(DisplayName="Rare"),
	Epic UMETA(DisplayName="Epic"),
	Legendary UMETA(DisplayName="Legendary")
};

/**
 * 衣柜物品片段。
 *
 * InventoryManager 只知道“玩家拥有了一个有身份物品”，本 Fragment 负责说明这件物品
 * 是否是一件可换装内容，以及装备时应写入当前主角 PlayerState.AppearanceTags 的哪些 Mutable 标签。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootInventoryFragment_WardrobeItem : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** 这件服装适用于哪个主角；UNKNOWN 表示男女都可用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	ECharacterGender SuitableGender = ECharacterGender::UNKNOWN;

	/** UI 分类，用于衣柜页签和子分类筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeCategory Category = EShootWardrobeCategory::UpperBody;

	/** 可同时归属多个衣柜分类；使用 Wardrobe.Category.* 标签，不再维护字符串 ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTagContainer SubCategoryTags;

	/** UI 稀有度展示，不参与数值逻辑。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeRarity Rarity = EShootWardrobeRarity::Common;

	/** UI 排序，数字越小越靠前。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	int32 SortPriority = 0;

	/** 服装描述，留空时 UI 只展示 ItemDefinition.DisplayName。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe", meta=(MultiLine="true"))
	FText Description;

	/** 装备时写入当前主角外观的 Mutable Gameplay Tags。正式资产只维护这一处。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTagContainer AppearanceTags;

	/**
	 * 可选的衣柜预览表演。玩家在 W_Cloth 中装备本物品并完成 Mutable 换装后播放；
	 * 它不作用于游戏场景角色，也不进入存档或网络复制。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Preview")
	TSoftObjectPtr<UAnimSequenceBase> PreviewEquipAnimation;

	/** true 时，装备前先移除当前标签中与新标签相同 Mutable 参数位的旧选项。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	bool bReplaceTagsInSameMutableSlot = true;

	/** true 时，装备前清空当前性别所有外观标签，适合完整套装或预设。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	bool bReplaceAllCurrentAppearanceTags = false;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Wardrobe")
	bool CanApplyToGender(ECharacterGender CurrentGender) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Wardrobe")
	bool IsEquippedInTags(const FGameplayTagContainer& CurrentAppearanceTags) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Wardrobe")
	FGameplayTagContainer BuildTagsAfterEquip(const FGameplayTagContainer& CurrentAppearanceTags, ECharacterGender CurrentGender) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Wardrobe")
	FGameplayTagContainer BuildTagsAfterUnequip(const FGameplayTagContainer& CurrentAppearanceTags, ECharacterGender CurrentGender) const;

private:
	static FString BuildMutableSlotKey(const FGameplayTag& Tag);
	static bool DoesTagMatchGender(const FGameplayTag& Tag, ECharacterGender Gender);
};
