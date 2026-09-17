// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/ViewModel/WardrobeViewModel.h"

#include "Inventory/Fragments/ShootInventoryFragment_WardrobeItem.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Player/ShootPlayerController.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WardrobeViewModel)

void UShootWardrobeSubCategoryViewModel::Initialize(const FShootWardrobeSubCategoryDefinition& Definition)
{
	CategoryTag = Definition.CategoryTag;
	DisplayName = Definition.DisplayText;
}

void UShootWardrobeItemViewModel::InitializeFromViewData(const FShootWardrobeItemViewData& InViewData)
{
	ViewData = InViewData;
	ItemDefinition = InViewData.ItemDefinition;
	ItemInstanceId = InViewData.ItemInstanceId;
	DisplayName = InViewData.DisplayName;
	Description = InViewData.Description;
	Icon = InViewData.Icon;
	SuitableGender = InViewData.SuitableGender;
	Category = InViewData.Category;
	Rarity = InViewData.Rarity;
	PreviewEquipAnimation = InViewData.PreviewEquipAnimation;
	bOwned = InViewData.bOwned;
	bCanEquip = InViewData.bCanEquip;
	bEquipped = InViewData.bEquipped;
}

void UShootWardrobeViewModel::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UShootWardrobeViewModel::Initialize(APlayerController* InOwningPlayer,
	UShootWardrobeCatalogDataAsset* InCatalog, bool bInFilterByCurrentGender)
{
	Shutdown();
	OwningPlayer = InOwningPlayer;
	Catalog = InCatalog;
	bFilterByCurrentGender = bInFilterByCurrentGender;

	UE_MVVM_SET_PROPERTY_VALUE(PageDefinitions, Catalog ? Catalog->PageDefinitions : TArray<FShootWardrobePageDefinition>());
	BindRuntimeSources();

	for (int32 PageIndex = 0; PageIndex < PageDefinitions.Num(); ++PageIndex)
	{
		if (PageDefinitions[PageIndex].CategoryTag.IsValid())
		{
			SelectTopLevelTab(PageDefinitions[PageIndex].CategoryTag.GetTagName());
			return;
		}
	}

	Refresh();
}

void UShootWardrobeViewModel::Shutdown()
{
	UnbindRuntimeSources();
	OwningPlayer = nullptr;
	Catalog = nullptr;
	UE_MVVM_SET_PROPERTY_VALUE(PageDefinitions, TArray<FShootWardrobePageDefinition>());
	UE_MVVM_SET_PROPERTY_VALUE(CurrentSubCategories, TArray<FShootWardrobeSubCategoryDefinition>());
	UE_MVVM_SET_PROPERTY_VALUE(CurrentSubCategoryItems,
		TArray<TObjectPtr<UShootWardrobeSubCategoryViewModel>>());
	UE_MVVM_SET_PROPERTY_VALUE(VisibleItems, TArray<TObjectPtr<UShootWardrobeItemViewModel>>());
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(bHasVisibleItems, false);
	UE_MVVM_SET_PROPERTY_VALUE(ActiveTopLevelCategoryTag, FGameplayTag());
	UE_MVVM_SET_PROPERTY_VALUE(ActiveSubCategoryTag, FGameplayTag());
	UE_MVVM_SET_PROPERTY_VALUE(ActivePageIndex, INDEX_NONE);
}

bool UShootWardrobeViewModel::SelectTopLevelTab(FName TabId)
{
	const int32 PageIndex = PageDefinitions.IndexOfByPredicate([TabId](const FShootWardrobePageDefinition& Definition)
	{
		return Definition.CategoryTag.IsValid() && Definition.CategoryTag.GetTagName() == TabId;
	});
	if (!PageDefinitions.IsValidIndex(PageIndex))
	{
		return false;
	}
	if (ActivePageIndex == PageIndex && !CurrentSubCategoryItems.IsEmpty())
	{
		return true;
	}

	const FShootWardrobePageDefinition& PageDefinition = PageDefinitions[PageIndex];
	UE_MVVM_SET_PROPERTY_VALUE(ActivePageIndex, PageIndex);
	UE_MVVM_SET_PROPERTY_VALUE(ActiveTopLevelCategoryTag, PageDefinition.CategoryTag);
	UE_MVVM_SET_PROPERTY_VALUE(PreviewCameraMode, PageDefinition.PreviewCameraMode);
	RefreshSubCategories(PageDefinition);
	RefreshItems();
	OnPreviewModeChanged.Broadcast();
	return true;
}

bool UShootWardrobeViewModel::SelectSubCategory(FGameplayTag CategoryTag)
{
	const FShootWardrobeSubCategoryDefinition* Definition = CurrentSubCategories.FindByPredicate(
		[CategoryTag](const FShootWardrobeSubCategoryDefinition& Candidate)
		{
			return Candidate.CategoryTag.MatchesTagExact(CategoryTag);
		});
	if (!Definition)
	{
		return false;
	}

	if (ActiveSubCategoryTag.MatchesTagExact(Definition->CategoryTag))
	{
		return true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(ActiveSubCategoryTag, Definition->CategoryTag);
	UE_MVVM_SET_PROPERTY_VALUE(PreviewCameraMode, Definition->PreviewCameraMode);
	RefreshItems();
	OnPreviewModeChanged.Broadcast();
	return true;
}

void UShootWardrobeViewModel::Refresh()
{
	AShootPlayerState* PlayerState = ResolveShootPlayerState();
	UE_MVVM_SET_PROPERTY_VALUE(CurrentGender,
		PlayerState ? PlayerState->GetCharacterGender() : ECharacterGender::UNKNOWN);
	RefreshItems();
}

TArray<UObject*> UShootWardrobeViewModel::GetVisibleListItems() const
{
	TArray<UObject*> Result;
	Result.Reserve(VisibleItems.Num());
	for (UShootWardrobeItemViewModel* Item : VisibleItems)
	{
		Result.Add(Item);
	}
	return Result;
}

TArray<UObject*> UShootWardrobeViewModel::GetCurrentSubCategoryListItems() const
{
	TArray<UObject*> Result;
	Result.Reserve(CurrentSubCategoryItems.Num());
	for (UShootWardrobeSubCategoryViewModel* Item : CurrentSubCategoryItems)
	{
		Result.Add(Item);
	}
	return Result;
}

void UShootWardrobeViewModel::SelectItem(UShootWardrobeItemViewModel* Item)
{
	if (Item && !VisibleItems.Contains(Item))
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, Item);
}

bool UShootWardrobeViewModel::RequestToggleEquip(UShootWardrobeItemViewModel* Item)
{
	SelectItem(Item);
	if (!Item || !Item->bOwned || !Item->bCanEquip || !Item->ItemInstanceId.IsValid())
	{
		return false;
	}

	AShootPlayerState* PlayerState = ResolveShootPlayerState();
	if (!PlayerState)
	{
		return false;
	}

	// UI 只提交实例 Guid；PlayerState 在服务器重新验证 Persistent Inventory、性别和 Fragment。
	if (Item->bEquipped)
	{
		PlayerState->ServerUnequipWardrobeItem(Item->ItemInstanceId);
	}
	else
	{
		PlayerState->ServerEquipWardrobeItem(Item->ItemInstanceId);
	}
	return true;
}

void UShootWardrobeViewModel::SaveOutfit()
{
	if (AShootPlayerState* PlayerState = ResolveShootPlayerState())
	{
		PlayerState->ServerSaveCurrentWardrobe();
	}
}

void UShootWardrobeViewModel::SwitchCharacter()
{
	AShootPlayerController* PlayerController = ResolveShootPlayerController();
	AShootPlayerState* PlayerState = ResolveShootPlayerState();
	if (!PlayerController || !PlayerState)
	{
		return;
	}

	const ECharacterGender TargetGender = PlayerState->GetCharacterGender() == ECharacterGender::MALE
		? ECharacterGender::FEMALE
		: ECharacterGender::MALE;
	PlayerController->RequestSwitchCharacter(TargetGender, nullptr);
}

void UShootWardrobeViewModel::BindRuntimeSources()
{
	if (AShootPlayerController* PlayerController = ResolveShootPlayerController())
	{
		PlayerController->OnCharacterSwitchResult.RemoveDynamic(this, &ThisClass::HandleCharacterSwitchResult);
		PlayerController->OnCharacterSwitchResult.AddDynamic(this, &ThisClass::HandleCharacterSwitchResult);
	}

	if (AShootPlayerState* PlayerState = ResolveShootPlayerState())
	{
		BoundPlayerState = PlayerState;
		PlayerState->OnAppearanceTagsChanged.RemoveAll(this);
		PlayerState->OnAppearanceTagsChanged.AddUObject(this, &ThisClass::HandleAppearanceTagsChanged);
	}

	if (!InventoryChangedHandle.IsValid() && UGameplayMessageSubsystem::HasInstance(this))
	{
		InventoryChangedHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FShootInventoryChangeMessage>(
			FShootGameplayTags::Get().Inventory_Message_StackChanged,
			this,
			&ThisClass::HandleInventoryChanged);
	}
}

void UShootWardrobeViewModel::UnbindRuntimeSources()
{
	if (AShootPlayerController* PlayerController = ResolveShootPlayerController())
	{
		PlayerController->OnCharacterSwitchResult.RemoveDynamic(this, &ThisClass::HandleCharacterSwitchResult);
	}

	if (AShootPlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnAppearanceTagsChanged.RemoveAll(this);
	}
	BoundPlayerState = nullptr;

	if (InventoryChangedHandle.IsValid() && UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(InventoryChangedHandle);
		InventoryChangedHandle = FGameplayMessageListenerHandle();
	}
}

void UShootWardrobeViewModel::RefreshSubCategories(const FShootWardrobePageDefinition& PageDefinition)
{
	TArray<FShootWardrobeSubCategoryDefinition> NewDefinitions;
	if (Catalog)
	{
		for (const FShootWardrobeSubCategoryDefinition& Definition : Catalog->SubCategoryDefinitions)
		{
			if (Definition.ParentCategoryTag.MatchesTagExact(PageDefinition.CategoryTag))
			{
				NewDefinitions.Add(Definition);
			}
		}
	}
	UE_MVVM_SET_PROPERTY_VALUE(CurrentSubCategories, MoveTemp(NewDefinitions));

	FGameplayTag NewActiveTag = PageDefinition.DefaultSubCategoryTag;
	if (!CurrentSubCategories.ContainsByPredicate([NewActiveTag](const FShootWardrobeSubCategoryDefinition& Definition)
	{
		return Definition.CategoryTag.MatchesTagExact(NewActiveTag);
	}))
	{
		NewActiveTag = CurrentSubCategories.IsEmpty() ? FGameplayTag() : CurrentSubCategories[0].CategoryTag;
	}
	UE_MVVM_SET_PROPERTY_VALUE(ActiveSubCategoryTag, NewActiveTag);

	TArray<TObjectPtr<UShootWardrobeSubCategoryViewModel>> NewItems;
	NewItems.Reserve(CurrentSubCategories.Num());
	for (const FShootWardrobeSubCategoryDefinition& Definition : CurrentSubCategories)
	{
		UShootWardrobeSubCategoryViewModel* Item = NewObject<UShootWardrobeSubCategoryViewModel>(this);
		Item->Initialize(Definition);
		NewItems.Add(Item);
	}
	UE_MVVM_SET_PROPERTY_VALUE(CurrentSubCategoryItems, MoveTemp(NewItems));

	if (const FShootWardrobeSubCategoryDefinition* Definition = FindActiveSubCategoryDefinition())
	{
		UE_MVVM_SET_PROPERTY_VALUE(PreviewCameraMode, Definition->PreviewCameraMode);
	}
}

void UShootWardrobeViewModel::RefreshItems()
{
	AShootPlayerState* PlayerState = ResolveShootPlayerState();
	UShootInventoryManagerComponent* InventoryManager = ResolveInventoryManager();
	if (!Catalog || !PlayerState || !InventoryManager || !ActiveSubCategoryTag.IsValid())
	{
		UE_MVVM_SET_PROPERTY_VALUE(VisibleItems, TArray<TObjectPtr<UShootWardrobeItemViewModel>>());
		UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, nullptr);
		UE_MVVM_SET_PROPERTY_VALUE(bHasVisibleItems, false);
		OnViewStateChanged.Broadcast();
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(CurrentGender, PlayerState->GetCharacterGender());
	const FGameplayTagContainer& AppearanceTags = PlayerState->GetAppearanceTags(CurrentGender);
	const TSubclassOf<UShootInventoryItemDefinition> PreviousSelection = SelectedItem
		? SelectedItem->ItemDefinition
		: nullptr;

	TMap<TSubclassOf<UShootInventoryItemDefinition>, UShootInventoryItemInstance*> OwnedItems;
	for (UShootInventoryItemInstance* ItemInstance : InventoryManager->GetAllItems())
	{
		if (ItemInstance && ItemInstance->GetItemDef()
			&& ItemInstance->FindFragmentByClass<UShootInventoryFragment_WardrobeItem>())
		{
			OwnedItems.FindOrAdd(ItemInstance->GetItemDef()) = ItemInstance;
		}
	}

	TArray<FShootWardrobeItemViewData> ViewDataItems;
	for (const TSubclassOf<UShootInventoryItemDefinition>& ItemDefinition : Catalog->WardrobeItems)
	{
		UShootInventoryItemInstance* const* OwnedInstance = OwnedItems.Find(ItemDefinition);
		FShootWardrobeItemViewData ViewData;
		if (BuildViewDataForDefinition(ItemDefinition, OwnedInstance ? *OwnedInstance : nullptr,
			AppearanceTags, ViewData) && PassesCurrentFilters(ViewData))
		{
			ViewDataItems.Add(MoveTemp(ViewData));
		}
	}

	ViewDataItems.Sort([](const FShootWardrobeItemViewData& Left, const FShootWardrobeItemViewData& Right)
	{
		return Left.SortPriority != Right.SortPriority
			? Left.SortPriority < Right.SortPriority
			: Left.DisplayName.ToString() < Right.DisplayName.ToString();
	});

	TArray<TObjectPtr<UShootWardrobeItemViewModel>> NewItems;
	UShootWardrobeItemViewModel* NewSelection = nullptr;
	UShootWardrobeItemViewModel* EquippedSelection = nullptr;
	for (const FShootWardrobeItemViewData& ViewData : ViewDataItems)
	{
		UShootWardrobeItemViewModel* ItemViewModel = NewObject<UShootWardrobeItemViewModel>(this);
		ItemViewModel->InitializeFromViewData(ViewData);
		NewItems.Add(ItemViewModel);
		if (ItemViewModel->ItemDefinition == PreviousSelection)
		{
			NewSelection = ItemViewModel;
		}
		if (!EquippedSelection && ItemViewModel->bEquipped)
		{
			EquippedSelection = ItemViewModel;
		}
	}

	if (!NewSelection)
	{
		if (EquippedSelection)
		{
			NewSelection = EquippedSelection;
		}
		else if (!NewItems.IsEmpty())
		{
			NewSelection = NewItems[0].Get();
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(VisibleItems, MoveTemp(NewItems));
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, NewSelection);
	UE_MVVM_SET_PROPERTY_VALUE(bHasVisibleItems, !VisibleItems.IsEmpty());
	OnViewStateChanged.Broadcast();
}

bool UShootWardrobeViewModel::BuildViewDataForDefinition(
	TSubclassOf<UShootInventoryItemDefinition> ItemDefinition,
	UShootInventoryItemInstance* OwnedInstance,
	const FGameplayTagContainer& CurrentAppearanceTags,
	FShootWardrobeItemViewData& OutData) const
{
	const UShootInventoryItemDefinition* ItemCDO = ItemDefinition
		? ItemDefinition->GetDefaultObject<UShootInventoryItemDefinition>()
		: nullptr;
	const UShootInventoryFragment_WardrobeItem* Fragment = ItemCDO
		? Cast<UShootInventoryFragment_WardrobeItem>(
			ItemCDO->FindFragmentByClass(UShootInventoryFragment_WardrobeItem::StaticClass()))
		: nullptr;
	if (!ItemCDO || !Fragment)
	{
		return false;
	}

	OutData = FShootWardrobeItemViewData();
	OutData.ItemInstance = OwnedInstance;
	OutData.ItemDefinition = ItemDefinition;
	OutData.ItemInstanceId = OwnedInstance ? OwnedInstance->GetItemInstanceId() : FGuid();
	OutData.DisplayName = OwnedInstance ? OwnedInstance->GetItemDisplayName() : ItemCDO->DisplayName;
	OutData.Description = Fragment->Description;
	OutData.Icon = OwnedInstance ? OwnedInstance->GetItemDisplayIcon() : ItemCDO->Icon;
	OutData.AppearanceTags = Fragment->AppearanceTags;
	OutData.PreviewEquipAnimation = Fragment->PreviewEquipAnimation;
	OutData.SuitableGender = Fragment->SuitableGender;
	OutData.Category = Fragment->Category;
	OutData.SubCategoryTags = Fragment->SubCategoryTags;
	OutData.Rarity = Fragment->Rarity;
	OutData.SortPriority = Fragment->SortPriority;
	OutData.bOwned = OwnedInstance != nullptr;
	OutData.bLocked = !OutData.bOwned;
	OutData.bCanEquip = OutData.bOwned && Fragment->CanApplyToGender(CurrentGender);
	OutData.bEquipped = OutData.bOwned && Fragment->IsEquippedInTags(CurrentAppearanceTags);
	return true;
}

bool UShootWardrobeViewModel::PassesCurrentFilters(const FShootWardrobeItemViewData& ViewData) const
{
	const FShootWardrobeSubCategoryDefinition* Definition = FindActiveSubCategoryDefinition();
	if (!Definition)
	{
		return false;
	}

	const bool bMatchesCategory = Definition->bIncludeChildCategories
		? ViewData.SubCategoryTags.HasTag(Definition->CategoryTag)
		: ViewData.SubCategoryTags.HasTagExact(Definition->CategoryTag);
	if (!bMatchesCategory || !bFilterByCurrentGender)
	{
		return bMatchesCategory;
	}

	return ViewData.SuitableGender == ECharacterGender::UNKNOWN || ViewData.SuitableGender == CurrentGender;
}

const FShootWardrobeSubCategoryDefinition* UShootWardrobeViewModel::FindActiveSubCategoryDefinition() const
{
	return CurrentSubCategories.FindByPredicate([this](const FShootWardrobeSubCategoryDefinition& Definition)
	{
		return Definition.CategoryTag.MatchesTagExact(ActiveSubCategoryTag);
	});
}

AShootPlayerState* UShootWardrobeViewModel::ResolveShootPlayerState() const
{
	return OwningPlayer.IsValid() ? OwningPlayer->GetPlayerState<AShootPlayerState>() : nullptr;
}

AShootPlayerController* UShootWardrobeViewModel::ResolveShootPlayerController() const
{
	return Cast<AShootPlayerController>(OwningPlayer.Get());
}

UShootInventoryManagerComponent* UShootWardrobeViewModel::ResolveInventoryManager() const
{
	const AShootPlayerState* PlayerState = ResolveShootPlayerState();
	return PlayerState ? PlayerState->GetInventoryManagerComponent() : nullptr;
}

void UShootWardrobeViewModel::HandleInventoryChanged(FGameplayTag Channel,
	const FShootInventoryChangeMessage& Message)
{
	(void)Channel;
	if (Message.InventoryOwner == ResolveInventoryManager())
	{
		RefreshItems();
	}
}

void UShootWardrobeViewModel::HandleAppearanceTagsChanged(ECharacterGender InGender,
	const FGameplayTagContainer& NewTags)
{
	(void)NewTags;
	if (InGender == CurrentGender)
	{
		RefreshItems();
	}
}

void UShootWardrobeViewModel::HandleCharacterSwitchResult(bool bSuccess,
	ECharacterGender InCurrentGender, FText Message)
{
	(void)Message;
	if (bSuccess)
	{
		UE_MVVM_SET_PROPERTY_VALUE(CurrentGender, InCurrentGender);
		RefreshItems();
	}
}
