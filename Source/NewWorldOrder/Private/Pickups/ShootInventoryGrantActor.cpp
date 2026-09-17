// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Pickups/ShootInventoryGrantActor.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "Blueprint/UserWidget.h"
#include "Character/CombatComponent.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Abilities/ShootGA_Interaction_Collect.h"
#include "Inventory/ResourceInventoryBlueprintLibrary.h"
#include "Inventory/ShootInventoryFragment_EquippableItem.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Player/ShootPlayerState.h"
#include "Player/ShootPlayerController.h"
#include "System/SaveGameSubsystem.h"
#include "System/ShootSaveGame.h"
#include "TimerManager.h"
#include "UI/Wardrobe/ShootWardrobeCatalogDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryGrantActor)

AShootInventoryGrantActor::AShootInventoryGrantActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(90.f);
	// 玩家交互能力默认用 Interactable_OverlapDynamic profile 进行线性扫描和近距离授予。
	// 调试发放 Actor 必须使用同一 profile，否则 HomeMap 里看得到模型但按交互键扫不到目标。
	CollisionComponent->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.35f));

	// Visual 只负责让调试物在场景里可见；网格由 BP_ShootInventoryGrantActor 或关卡实例配置，
	// 真正的交互命中始终交给 Collision 根组件。
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	VisualComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

	InteractionPromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	InteractionPromptComponent->SetupAttachment(CollisionComponent);
	InteractionPromptComponent->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPromptComponent->SetDrawSize(FVector2D(160.f, 64.f));
	InteractionPromptComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	InteractionPromptComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPromptComponent->SetGenerateOverlapEvents(false);
	InteractionPromptComponent->SetHiddenInGame(true);
	InteractionPromptComponent->SetVisibility(false);

	// PressToInteract 的按键图标由蓝图/关卡实例选择 CommonUI Widget。
	// 交互能力仍只负责扫描和触发，C++ 不加载具体 UI 资产，也不保留失效路径兜底。

	InteractionText = NSLOCTEXT("InventoryGrant", "DefaultInteractionText", "Claim Item");
	InteractionSubText = NSLOCTEXT("InventoryGrant", "DefaultInteractionSubText", "Outfit, weapon, or resource reward");
	InteractionAbilityClass = UShootGA_Interaction_Collect::StaticClass();

#if UE_BUILD_SHIPPING
	bDebugGrantEnabled = false;
#endif
}

void AShootInventoryGrantActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionPromptComponent)
	{
		if (InteractionPromptWidgetClass)
		{
			InteractionPromptComponent->SetWidgetClass(InteractionPromptWidgetClass);
		}
		InteractionPromptComponent->SetHiddenInGame(true);
		InteractionPromptComponent->SetVisibility(false);
	}

	if (CollisionComponent)
	{
		// AutoOverlap 和 PressToInteract 都需要 Overlap。
		// AutoOverlap 在服务器直接授予；PressToInteract 在本地只显示 W_Icon_Interact 提示，真正拾取仍由交互 GA 按 E 触发。
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnOverlapEnd);
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ProcessCurrentOverlaps);
	}
}

void AShootInventoryGrantActor::ConfigureForSpawner(
	const TArray<FShootInventoryGrantItemEntry>& InItemGrants,
	EShootInteractionTriggerMode InTriggerMode,
	EShootInteractionUserFilter InUserFilter,
	bool bInDestroyOnGrant,
	FText InInteractionText,
	FText InInteractionSubText)
{
	// 生成器只配置本 Actor，不直接写库存；真正授予仍走 GrantToPawn，
	// 这样 PressToInteract 和 AutoOverlap 都复用同一条服务器权威库存入口。
	ItemGrants = InItemGrants;
	TriggerMode = InTriggerMode;
	UserFilter = InUserFilter;
	bDestroyOnGrant = bInDestroyOnGrant;
	InteractionText = MoveTemp(InInteractionText);
	InteractionSubText = MoveTemp(InInteractionSubText);
}

void AShootInventoryGrantActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                               AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp,
                                               int32 OtherBodyIndex,
                                               bool bFromSweep,
                                               const FHitResult& SweepResult)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	SetLocalPromptVisibleForPawn(Pawn, true);

	if (HasAuthority() && TriggerMode == EShootInteractionTriggerMode::AutoOverlap)
	{
		GrantToPawn(Pawn);
	}
}

void AShootInventoryGrantActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp,
                                             AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp,
                                             int32 OtherBodyIndex)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;

	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		SetLocalPromptVisibleForPawn(Pawn, false);
	}
}

void AShootInventoryGrantActor::ProcessCurrentOverlaps()
{
	if (!CollisionComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	CollisionComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (APawn* Pawn = Cast<APawn>(OverlappingActor))
		{
			SetLocalPromptVisibleForPawn(Pawn, true);

			if (HasAuthority() && TriggerMode == EShootInteractionTriggerMode::AutoOverlap)
			{
				GrantToPawn(Pawn);
				if (IsActorBeingDestroyed())
				{
					return;
				}
			}
		}
	}
}

void AShootInventoryGrantActor::SetLocalPromptVisibleForPawn(APawn* Pawn, bool bVisible)
{
	if (!InteractionPromptComponent || TriggerMode != EShootInteractionTriggerMode::PressToInteract)
	{
		return;
	}

	if (!Pawn || !Pawn->IsLocallyControlled() || !CanBeTriggeredBy(Pawn))
	{
		return;
	}

	// WidgetComponent 的可见性是本机 Actor 实例状态；只在本地受控 Pawn 触发时改，避免多人 PIE 中别人的 E 提示跑到自己屏幕上。
	InteractionPromptComponent->SetHiddenInGame(!bVisible);
	InteractionPromptComponent->SetVisibility(bVisible);
}

void AShootInventoryGrantActor::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                                         FInteractionOptionBuilder& OptionBuilder)
{
	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract)
	{
		return;
	}

	APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!RequestingPawn)
	{
		AController* RequestingController = InteractQuery.RequestingController.Get();
		RequestingPawn = RequestingController ? RequestingController->GetPawn() : nullptr;
	}

	if (!RequestingPawn || !CanBeTriggeredBy(RequestingPawn))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_Collect::StaticClass());
	OptionBuilder.AddInteractionOption(Option);
}

void AShootInventoryGrantActor::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
                                                              FGameplayEventData& InOutEventData)
{
	(void)InteractionEventTag;

	InOutEventData.Target = this;
	InOutEventData.EventMagnitude = 1.f;
}

bool AShootInventoryGrantActor::ProcessGrantFromAbility(APawn* ReceivingPawn)
{
	return GrantToPawn(ReceivingPawn);
}

bool AShootInventoryGrantActor::GrantToPawn(APawn* ReceivingPawn)
{
	if (!HasAuthority() || !ReceivingPawn || !CanBeTriggeredBy(ReceivingPawn))
	{
		return false;
	}

	if (DebugOperation == EShootInventoryGrantDebugOperation::ClearWardrobeOwnership)
	{
		// 调用链仍由 ShootGA_Interaction_Collect 进入这里，但 Clear 模式只改 InventoryManager。
		// WardrobeCatalogAsset 是唯一范围边界，禁止为了调试误删武器、资源或其他 Persistent 物品。
		const bool bClearedWardrobe = ClearWardrobeOwnership(ReceivingPawn);
		PersistInventoryToSave(ReceivingPawn);
		return bClearedWardrobe;
	}

	const bool bGrantedItems = GrantInventoryItems(ReceivingPawn);
	const bool bGrantedResources = GrantResources(ReceivingPawn);
	const bool bGrantedAnything = bGrantedItems || bGrantedResources;

	if (bGrantedAnything && (bGrantedResources || (bGrantedItems && HasPersistentItemGrantConfigured())))
	{
		PersistInventoryToSave(ReceivingPawn);
	}

	if (bGrantedAnything && bDestroyOnGrant)
	{
		Destroy();
	}

	return bGrantedAnything;
}

bool AShootInventoryGrantActor::ClearWardrobeOwnership(APawn* ReceivingPawn)
{
	UShootInventoryManagerComponent* InventoryManager = ResolveInventoryManager(ReceivingPawn);
	if (!InventoryManager || !WardrobeCatalogAsset)
	{
		return false;
	}

	TArray<TSubclassOf<UShootInventoryItemDefinition>> WardrobeDefinitions;
	WardrobeCatalogAsset->AppendWardrobeItemsTo(WardrobeDefinitions);
	if (WardrobeDefinitions.IsEmpty())
	{
		return false;
	}

	TArray<UShootInventoryItemInstance*> InstancesToRemove;
	for (UShootInventoryItemInstance* Instance : InventoryManager->GetAllItems())
	{
		if (Instance && Instance->GetItemLifetime() == EShootItemLifetime::Persistent
			&& WardrobeDefinitions.Contains(Instance->GetItemDef()))
		{
			InstancesToRemove.Add(Instance);
		}
	}

	for (UShootInventoryItemInstance* Instance : InstancesToRemove)
	{
		InventoryManager->RemoveItemInstance(Instance);
	}

	// 即使本来就没有服装也视为成功：调试入口表达的是“现在处于全部未获取状态”。
	return true;
}

bool AShootInventoryGrantActor::GrantInventoryItems(APawn* ReceivingPawn)
{
	UShootInventoryManagerComponent* InventoryManager = ResolveInventoryManager(ReceivingPawn);
	if (!InventoryManager)
	{
		return false;
	}

	TArray<FShootInventoryGrantItemEntry> EffectiveEntries;
	BuildEffectiveItemGrants(EffectiveEntries);

	bool bGrantedAny = false;
	for (const FShootInventoryGrantItemEntry& Entry : EffectiveEntries)
	{
		if (!Entry.ItemDefinition || Entry.StackCount <= 0)
		{
			continue;
		}

		UShootInventoryItemInstance* NewInstance = nullptr;
		if (Entry.Lifetime == EShootItemLifetime::RuntimeOnly)
		{
			NewInstance = InventoryManager->AddRuntimeItem(Entry.ItemDefinition, Entry.StackCount);
		}
		else
		{
			NewInstance = InventoryManager->AddPersistentItem(Entry.ItemDefinition, Entry.StackCount);
		}

		if (!NewInstance)
		{
			if (bTreatAlreadyOwnedAsSuccess && InventoryManager->FindFirstItemStackByDefinition(Entry.ItemDefinition))
			{
				bGrantedAny = true;
			}
			continue;
		}

		bGrantedAny = true;

		// QuickBar/Equipment 只能引用 InventoryManager 里的有身份物品。
		// 这里仍需验证 EquippableItem Fragment，服装等 WardrobeItem 不能占用战斗槽。
		if (Entry.bAutoAssignToQuickbarIfEquippable && NewInstance->FindFragmentByClass<UShootInventoryFragment_EquippableItem>())
		{
			if (NewInstance->GetItemLifetime() == EShootItemLifetime::RuntimeOnly)
			{
				if (AShootPlayerController* PlayerController = Cast<AShootPlayerController>(ReceivingPawn->GetController()))
				{
					if (UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
					{
						int32 SlotIndex = INDEX_NONE;
						QuickBar->AssignRuntimeItemToAvailableSlot(NewInstance, SlotIndex);
					}
				}
			}
			else if (UCombatComponent* CombatComponent = ReceivingPawn->FindComponentByClass<UCombatComponent>())
			{
				const int32 SlotIndex = CombatComponent->GetNextFreeItemSlot();
				if (SlotIndex != INDEX_NONE)
				{
					CombatComponent->AssignSlotFromInventory(SlotIndex, NewInstance->GetItemInstanceId());
				}
			}
		}
	}

	return bGrantedAny;
}

bool AShootInventoryGrantActor::GrantResources(APawn* ReceivingPawn)
{
	bool bGrantedAny = false;
	for (const FShootInventoryGrantResourceEntry& Entry : ResourceGrants)
	{
		if (!Entry.ResourceItemDef || Entry.Amount <= 0)
		{
			continue;
		}

		bGrantedAny |= UResourceInventoryBlueprintLibrary::AddResource(ReceivingPawn, Entry.ResourceItemDef, Entry.Amount);
	}
	return bGrantedAny;
}

bool AShootInventoryGrantActor::CanBeTriggeredBy(const APawn* Pawn) const
{
	if (!bDebugGrantEnabled || !Pawn)
	{
		return false;
	}

	const bool bIsPlayerControlled = IsPlayerActor(Pawn);
	switch (UserFilter)
	{
	case EShootInteractionUserFilter::PlayerOnly:
		return bIsPlayerControlled;
	case EShootInteractionUserFilter::AIOnly:
		return !bIsPlayerControlled;
	case EShootInteractionUserFilter::PlayerAndAI:
	default:
		return true;
	}
}

bool AShootInventoryGrantActor::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}

UShootInventoryManagerComponent* AShootInventoryGrantActor::ResolveInventoryManager(APawn* ReceivingPawn) const
{
	if (!ReceivingPawn)
	{
		return nullptr;
	}

	if (AShootPlayerState* ShootPlayerState = ReceivingPawn->GetPlayerState<AShootPlayerState>())
	{
		return ShootPlayerState->GetInventoryManagerComponent();
	}

	if (AController* Controller = ReceivingPawn->GetController())
	{
		if (AShootPlayerState* ShootPlayerState = Controller->GetPlayerState<AShootPlayerState>())
		{
			return ShootPlayerState->GetInventoryManagerComponent();
		}
	}

	return nullptr;
}

bool AShootInventoryGrantActor::HasPersistentItemGrantConfigured() const
{
	TArray<FShootInventoryGrantItemEntry> EffectiveEntries;
	BuildEffectiveItemGrants(EffectiveEntries);
	for (const FShootInventoryGrantItemEntry& Entry : EffectiveEntries)
	{
		if (Entry.ItemDefinition && Entry.StackCount > 0 && Entry.Lifetime == EShootItemLifetime::Persistent)
		{
			return true;
		}
	}

	return false;
}

void AShootInventoryGrantActor::PersistInventoryToSave(APawn* ReceivingPawn) const
{
	if (!HasAuthority() || !ReceivingPawn)
	{
		return;
	}

	AShootPlayerState* ShootPlayerState = ReceivingPawn->GetPlayerState<AShootPlayerState>();
	if (!ShootPlayerState)
	{
		if (AController* Controller = ReceivingPawn->GetController())
		{
			ShootPlayerState = Controller->GetPlayerState<AShootPlayerState>();
		}
	}

	if (!ShootPlayerState)
	{
		return;
	}

	// Grant 与 Clear 都走正式 SaveGameSubsystem 捕获入口：前者保存新增所有权，后者用空衣柜覆盖旧存档。
	// 这里不能直接编辑 UShootSaveGame::SavedInventoryItems，否则会绕过 InventoryManager 当前权威状态。
	if (UGameInstance* GameInstance = ReceivingPawn->GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
		{
			SaveSubsystem->CapturePlayerInventoryState(ShootPlayerState);
			if (UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame())
			{
				SaveSubsystem->AsyncPlayerSaveGame(SaveGame, SaveSubsystem->GetCurrentSlotIndex());
			}
		}
	}
}

void AShootInventoryGrantActor::BuildEffectiveItemGrants(TArray<FShootInventoryGrantItemEntry>& OutEntries) const
{
	OutEntries = ItemGrants;
	if (OutEntries.Num() > 0)
	{
		return;
	}

	TArray<TSubclassOf<UShootInventoryItemDefinition>> CatalogItems;
	if (WardrobeCatalogAsset)
	{
		// 调试 Actor 也读正式目录 DataAsset，确保“领到的服装”和“衣柜展示图鉴”来自同一数据源。
		WardrobeCatalogAsset->AppendWardrobeItemsTo(CatalogItems);
	}

	for (TSubclassOf<UShootInventoryItemDefinition> ItemDefinition : CatalogItems)
	{
		FShootInventoryGrantItemEntry Entry;
		Entry.ItemDefinition = ItemDefinition;
		Entry.StackCount = 1;
		Entry.Lifetime = EShootItemLifetime::Persistent;
		Entry.bAutoAssignToQuickbarIfEquippable = false;
		OutEntries.Add(Entry);
	}
}
