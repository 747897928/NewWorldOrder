// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "MVVMViewModelBase.h"
#include "UI/Wardrobe/ShootWardrobeCatalogDataAsset.h"
#include "UI/Wardrobe/ShootWardrobeTypes.h"
#include "WardrobeViewModel.generated.h"

class APlayerController;
class AShootPlayerController;
class AShootPlayerState;
class UShootInventoryItemInstance;
class UShootInventoryManagerComponent;
class UAnimSequenceBase;
struct FShootInventoryChangeMessage;

/**
 * 二级分类按钮使用的只读展示对象。
 *
 * GameplayTag 是目录身份，DisplayName 是蓝图显示数据；条目 Widget 不解析文本，也不持有衣柜页面。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootWardrobeSubCategoryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize(const FShootWardrobeSubCategoryDefinition& Definition);

	UFUNCTION(BlueprintPure, Category="Wardrobe|Navigation")
	FGameplayTag GetCategoryTag() const { return CategoryTag; }

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe|Navigation")
	FGameplayTag CategoryTag;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe|Navigation")
	FText DisplayName;

};

/**
 * 衣柜物品格使用的单件只读展示对象。
 *
 * 对象只保存一次刷新得到的只读快照，不持有 Widget，也不发送服务器请求。
 * W_Cloth_Item 从这里读取业务状态，并在蓝图中决定文案、颜色、图标和动画。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootWardrobeItemViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void InitializeFromViewData(const FShootWardrobeItemViewData& InViewData);

	const FShootWardrobeItemViewData& GetViewData() const { return ViewData; }

	/** 蓝图节点使用无歧义函数名读取画刷；颜色、尺寸和状态文案仍由条目 Widget 决定。 */
	UFUNCTION(BlueprintPure, Category="Wardrobe")
	FSlateBrush GetWardrobeIconBrush() const { return Icon; }

	UFUNCTION(BlueprintPure, Category="Wardrobe")
	bool IsWardrobeItemOwned() const { return bOwned; }

	UFUNCTION(BlueprintPure, Category="Wardrobe")
	bool IsWardrobeItemEquipped() const { return bEquipped; }

	UFUNCTION(BlueprintPure, Category="Wardrobe")
	EShootWardrobeRarity GetWardrobeItemRarity() const { return Rarity; }

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
	ECharacterGender SuitableGender = ECharacterGender::UNKNOWN;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeCategory Category = EShootWardrobeCategory::UpperBody;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	EShootWardrobeRarity Rarity = EShootWardrobeRarity::Common;

	/** 只供衣柜预览 Actor 使用，不传给正式角色 AnimBP。 */
	UPROPERTY(BlueprintReadOnly, Category="Wardrobe|Preview")
	TSoftObjectPtr<UAnimSequenceBase> PreviewEquipAnimation;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bOwned = false;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bCanEquip = false;

	UPROPERTY(BlueprintReadOnly, Category="Wardrobe")
	bool bEquipped = false;

private:
	FShootWardrobeItemViewData ViewData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShootWardrobeViewStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShootWardrobePreviewModeChanged);

/**
 * W_Cloth 的页面级展示状态。
 *
 * 调用链：W_Cloth 激活 -> Initialize -> 查询 Catalog、Persistent Inventory 和 PlayerState ->
 * FieldNotify/委托通知页面刷新 Tab、GridPanel 和 Tooltip。蓝图点击装备时只把 ItemViewModel
 * 交回这里，最终仍由 AShootPlayerState 的服务器 RPC 重新校验所有权和性别。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootWardrobeViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	/** W_Cloth 激活时调用。Catalog 由 W_Cloth 蓝图显式配置，不在 C++ 中加载资产路径。 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe")
	void Initialize(APlayerController* InOwningPlayer, UShootWardrobeCatalogDataAsset* InCatalog,
	                bool bInFilterByCurrentGender = true);

	/** W_Cloth 反激活时调用，解除 PlayerState、PlayerController 和 GameplayMessage 监听。 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe")
	void Shutdown();

	/** 顶部 TabList 的 FName 只存在于 CommonUI 接口边界，内部立即还原为 GameplayTag。 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Navigation")
	bool SelectTopLevelTab(FName TabId);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Navigation")
	bool SelectSubCategory(FGameplayTag CategoryTag);

	UFUNCTION(BlueprintCallable, Category="Wardrobe")
	void Refresh();

	/** 页面协调层动态创建格子时使用；这里只做 UObject 数组适配，不持有或操作任何 Widget。 */
	UFUNCTION(BlueprintPure, Category="Wardrobe")
	TArray<UObject*> GetVisibleListItems() const;

	/** 当前一级页面的二级分类快照，由该页面自己的 VerticalBox 显示。 */
	UFUNCTION(BlueprintPure, Category="Wardrobe|Navigation")
	TArray<UObject*> GetCurrentSubCategoryListItems() const;

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Selection")
	void SelectItem(UShootWardrobeItemViewModel* Item);

	/** 点击已装备条目时卸下，否则装备；服务器仍以 ItemInstanceId 重新验证。 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Actions")
	bool RequestToggleEquip(UShootWardrobeItemViewModel* Item);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Actions")
	void SaveOutfit();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Actions")
	void SwitchCharacter();

	UPROPERTY(BlueprintAssignable, Category="Wardrobe|Events")
	FShootWardrobeViewStateChanged OnViewStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Wardrobe|Events")
	FShootWardrobePreviewModeChanged OnPreviewModeChanged;

	/** Catalog 数组顺序与 W_Cloth 中 WardrobeTopLevelContentSwitcher 的子页面顺序一致。 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	TArray<FShootWardrobePageDefinition> PageDefinitions;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	TArray<FShootWardrobeSubCategoryDefinition> CurrentSubCategories;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	TArray<TObjectPtr<UShootWardrobeSubCategoryViewModel>> CurrentSubCategoryItems;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	FGameplayTag ActiveTopLevelCategoryTag;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	FGameplayTag ActiveSubCategoryTag;

	/** 蓝图把这个索引交给 WardrobeTopLevelContentSwitcher；C++ 不引用任何具体页面控件。 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Navigation")
	int32 ActivePageIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe")
	TArray<TObjectPtr<UShootWardrobeItemViewModel>> VisibleItems;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Selection")
	TObjectPtr<UShootWardrobeItemViewModel> SelectedItem;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe")
	bool bHasVisibleItems = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe")
	ECharacterGender CurrentGender = ECharacterGender::UNKNOWN;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Wardrobe|Preview")
	EShootWardrobePreviewCameraMode PreviewCameraMode = EShootWardrobePreviewCameraMode::FullBody;

private:
	void BindRuntimeSources();
	void UnbindRuntimeSources();
	void RefreshSubCategories(const FShootWardrobePageDefinition& PageDefinition);
	void RefreshItems();
	bool BuildViewDataForDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDefinition,
	                                UShootInventoryItemInstance* OwnedInstance,
	                                const FGameplayTagContainer& CurrentAppearanceTags,
	                                FShootWardrobeItemViewData& OutData) const;
	bool PassesCurrentFilters(const FShootWardrobeItemViewData& ViewData) const;
	const FShootWardrobeSubCategoryDefinition* FindActiveSubCategoryDefinition() const;
	AShootPlayerState* ResolveShootPlayerState() const;
	AShootPlayerController* ResolveShootPlayerController() const;
	UShootInventoryManagerComponent* ResolveInventoryManager() const;

	void HandleInventoryChanged(FGameplayTag Channel, const FShootInventoryChangeMessage& Message);
	void HandleAppearanceTagsChanged(ECharacterGender InGender, const FGameplayTagContainer& NewTags);

	UFUNCTION()
	void HandleCharacterSwitchResult(bool bSuccess, ECharacterGender InCurrentGender, FText Message);

	UPROPERTY(Transient)
	TObjectPtr<UShootWardrobeCatalogDataAsset> Catalog;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> OwningPlayer;

	UPROPERTY(Transient)
	TWeakObjectPtr<AShootPlayerState> BoundPlayerState;

	FGameplayMessageListenerHandle InventoryChangedHandle;
	bool bFilterByCurrentGender = true;
};
