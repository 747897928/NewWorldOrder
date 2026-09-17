// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Wardrobe/ShootWardrobeTypes.h"
#include "ShootWardrobeCatalogDataAsset.generated.h"

class UShootInventoryItemDefinition;

/** 一个一级页签的内容与默认二级分类。页面视觉容器仍由 W_Cloth 蓝图维护。 */
USTRUCT(BlueprintType)
struct FShootWardrobePageDefinition
{
	GENERATED_BODY()

	bool operator==(const FShootWardrobePageDefinition& Other) const
	{
		return CategoryTag == Other.CategoryTag
			&& DisplayText.EqualTo(Other.DisplayText)
			&& PreviewCameraMode == Other.PreviewCameraMode
			&& DefaultSubCategoryTag == Other.DefaultSubCategoryTag;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTag CategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobePreviewCameraMode PreviewCameraMode = EShootWardrobePreviewCameraMode::FullBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTag DefaultSubCategoryTag;
};

/** 一个二级分类的显示、预览和筛选规则。物品归类只匹配 WardrobeItem Fragment 的 SubCategoryTags。 */
USTRUCT(BlueprintType)
struct FShootWardrobeSubCategoryDefinition
{
	GENERATED_BODY()

	bool operator==(const FShootWardrobeSubCategoryDefinition& Other) const
	{
		return ParentCategoryTag == Other.ParentCategoryTag
			&& CategoryTag == Other.CategoryTag
			&& DisplayText.EqualTo(Other.DisplayText)
			&& PreviewCameraMode == Other.PreviewCameraMode
			&& bIncludeChildCategories == Other.bIncludeChildCategories;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTag ParentCategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FGameplayTag CategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobePreviewCameraMode PreviewCameraMode = EShootWardrobePreviewCameraMode::FullBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe")
	bool bIncludeChildCategories = false;
};

/**
 * 衣柜正式图鉴目录。
 *
 * 该资产只负责列出可在衣柜中展示或调试发放的 ItemDefinition。
 * 单件服装的名称、图标、适用性别、部位和 Mutable 标签仍定义在 ItemDefinition 及其 WardrobeItem Fragment 上。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootWardrobeCatalogDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 正式服装图鉴。后续新增服装时优先改这个 DataAsset，不再改 C++ 硬编码列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe")
	TArray<TSubclassOf<UShootInventoryItemDefinition>> WardrobeItems;

	/** 一级分类定义。新增页面时，在 W_Cloth 增加对应内容容器后配置这里。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Navigation")
	TArray<FShootWardrobePageDefinition> PageDefinitions;

	/** 二级分类定义。禁止在 Widget C++ 中维护分类 ID、文本或前缀规则。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Navigation")
	TArray<FShootWardrobeSubCategoryDefinition> SubCategoryDefinitions;

	/** 将目录内容追加到目标数组，并用 AddUnique 去重。 */
	void AppendWardrobeItemsTo(TArray<TSubclassOf<UShootInventoryItemDefinition>>& InOutItems) const;
	const FShootWardrobePageDefinition* FindPageDefinition(FName TabId) const;
	const FShootWardrobeSubCategoryDefinition* FindSubCategoryDefinition(
		FGameplayTag ParentCategoryTag,
		FGameplayTag CategoryTag) const;
	void GetSubCategoryDefinitions(
		FGameplayTag ParentCategoryTag,
		TArray<const FShootWardrobeSubCategoryDefinition*>& OutDefinitions) const;

};
