// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Pickups/ShootResourcePickup.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/CombatComponent.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "Inventory/ResourceInventoryBlueprintLibrary.h"
#include "Inventory/ShootInventoryFragment_EquippableItem.h"
#include "Inventory/SavedInventoryTypes.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Interaction/Abilities/ShootGA_Interaction_Collect.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Interaction/InteractionQuery.h"
#include "Player/ShootPlayerState.h"
#include "Player/ShootPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"

AShootResourcePickup::AShootResourcePickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 根组件使用 Sphere，方便蓝图替换
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(60.f);
	// PressToInteract 和 AutoOverlap 都依赖项目统一的交互 profile，避免调试拾取物无法被玩家交互扫描发现。
	CollisionComponent->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 默认交互能力设置为 C++ 基类，蓝图可覆盖
	InteractionAbilityClass = UShootGA_Interaction_Collect::StaticClass();
}

void AShootResourcePickup::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent && TriggerMode == EShootInteractionTriggerMode::AutoOverlap)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AShootResourcePickup::OnOverlapBegin);
	}
}

void AShootResourcePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                          const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn || !ResourceItemDef)
	{
		return;
	}

	HandlePickup(OverlappingPawn);
}

bool AShootResourcePickup::HandlePickup(APawn* PickingPawn)
{
	if (!PickingPawn || !ResourceItemDef)
	{
		return false;
	}

	if (!CanBeTriggeredBy(PickingPawn))
	{
		return false;
	}

	if (bGrantInventoryItem)
	{
		return GiveInventoryItem(PickingPawn);
	}

	// 优先尝试弹药补给（直接写入当前武器 StatTags，服务器）
	if (bGrantAmmoToActiveWeapon)
	{
		if (UCombatComponent* CombatComponent = PickingPawn->FindComponentByClass<UCombatComponent>())
		{
			const bool bGranted = CombatComponent->GrantAmmoToActiveWeapon(MagazineAmmoDelta, ReserveAmmoDelta);
			if (!bGranted)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		// 通过蓝图函数库访问 ResourceInventoryComponent（数量型资源仓库）
		const bool bAdded = UResourceInventoryBlueprintLibrary::AddResource(PickingPawn, ResourceItemDef, Amount);
		if (!bAdded)
		{
			return false;
		}
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}

	return true;
}

bool AShootResourcePickup::GiveInventoryItem(APawn* PickingPawn)
{
	UShootInventoryManagerComponent* InventoryManager = ResolveInventoryManager(PickingPawn);
	if (!InventoryManager)
	{
		return false;
	}

	UShootInventoryItemInstance* NewInstance = nullptr;
	if (PickupItemLifetime == EShootItemLifetime::RuntimeOnly)
	{
		FShootInventoryItemInitData InitData;
		NewInstance = InventoryManager->AddRuntimeWeaponItem(ResourceItemDef, InitData);
	}
	else
	{
		NewInstance = InventoryManager->AddPersistentItem(ResourceItemDef);
	}

	if (!NewInstance)
	{
		return false;
	}

	// ResourcePickup 也会被用作服装/设计图等 Inventory 物品的测试入口。
	// 只有带 EquippableItem Fragment 的战斗物品才允许自动占用 QuickBar，服装只进入 InventoryManager 等待衣柜 UI 使用。
	if (NewInstance->FindFragmentByClass<UShootInventoryFragment_EquippableItem>())
	{
		if (NewInstance->GetItemLifetime() == EShootItemLifetime::RuntimeOnly)
		{
			if (AShootPlayerController* PlayerController = Cast<AShootPlayerController>(PickingPawn->GetController()))
			{
				if (UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
				{
					int32 SlotIndex = INDEX_NONE;
					QuickBar->AssignRuntimeItemToAvailableSlot(NewInstance, SlotIndex);
				}
			}
		}
		else if (UCombatComponent* CombatComponent = PickingPawn->FindComponentByClass<UCombatComponent>())
		{
			const int32 SlotIndex = CombatComponent->GetNextFreeItemSlot();
			if (SlotIndex != INDEX_NONE)
			{
				CombatComponent->AssignSlotFromInventory(SlotIndex, NewInstance->GetItemInstanceId());
			}
		}
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}

	return true;
}

bool AShootResourcePickup::CanBeTriggeredBy(const APawn* Pawn) const
{
	if (!Pawn)
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

bool AShootResourcePickup::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}

bool AShootResourcePickup::ProcessPickupFromAbility(APawn* PickingPawn)
{
	return HandlePickup(PickingPawn);
}

UShootInventoryManagerComponent* AShootResourcePickup::ResolveInventoryManager(APawn* PickingPawn) const
{
	if (!PickingPawn)
	{
		return nullptr;
	}

	if (AShootPlayerState* ShootPlayerState = PickingPawn->GetPlayerState<AShootPlayerState>())
	{
		return ShootPlayerState->GetInventoryManagerComponent();
	}

	if (AController* Controller = PickingPawn->GetController())
	{
		if (AShootPlayerState* ShootPlayerState = Controller->GetPlayerState<AShootPlayerState>())
		{
			return ShootPlayerState->GetInventoryManagerComponent();
		}
	}

	return nullptr;
}

void AShootResourcePickup::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                                    FInteractionOptionBuilder& OptionBuilder)
{
	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract || !ResourceItemDef)
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
	const UShootInventoryItemDefinition* ResourceCDO = ResourceItemDef ? ResourceItemDef->GetDefaultObject<UShootInventoryItemDefinition>() : nullptr;
	Option.Text = !InteractionText.IsEmpty() ? InteractionText :
		(ResourceCDO ? ResourceCDO->DisplayName : FText());
	Option.SubText = InteractionSubText;
	const TSubclassOf<UGameplayAbility> AbilityClassToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_Collect::StaticClass());
	Option.InteractionAbilityToGrant = AbilityClassToGrant;

	OptionBuilder.AddInteractionOption(Option);
}

void AShootResourcePickup::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
                                                         FGameplayEventData& InOutEventData)
{
	InOutEventData.Target = this;
	InOutEventData.OptionalObject = ResourceItemDef ? ResourceItemDef->GetDefaultObject() : nullptr;
	InOutEventData.EventMagnitude = static_cast<float>(Amount);
}
