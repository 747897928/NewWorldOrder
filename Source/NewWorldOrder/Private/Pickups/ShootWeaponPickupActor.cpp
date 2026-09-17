// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Pickups/ShootWeaponPickupActor.h"

#include "Abilities/GameplayAbility.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Character/CombatComponent.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "Interaction/Abilities/ShootGA_Interaction_Collect.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Interaction/InteractionQuery.h"
#include "Player/ShootPlayerState.h"
#include "Player/ShootPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeaponPickup, Log, All);

namespace
{
UShootInventoryManagerComponent* ResolveInventoryManagerFromPawn(APawn* Pawn)
{
	if (!Pawn)
	{
		return nullptr;
	}

	if (AShootPlayerState* PlayerState = Pawn->GetPlayerState<AShootPlayerState>())
	{
		return PlayerState->GetInventoryManagerComponent();
	}

	if (AController* Controller = Pawn->GetController())
	{
		if (AShootPlayerState* PlayerState = Controller->GetPlayerState<AShootPlayerState>())
		{
			return PlayerState->GetInventoryManagerComponent();
		}
	}

	return nullptr;
}

const TCHAR* GetAuthorityString(const AActor* Actor)
{
	return (Actor && Actor->HasAuthority()) ? TEXT("Server") : TEXT("Client");
}
}

AShootWeaponPickupActor::AShootWeaponPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(60.f);
	// PressToInteract 和 AutoOverlap 都依赖项目统一的交互 profile，避免调试拾取物无法被玩家交互扫描发现。
	CollisionComponent->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionAbilityClass = UShootGA_Interaction_Collect::StaticClass();
}

void AShootWeaponPickupActor::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent && TriggerMode == EShootInteractionTriggerMode::AutoOverlap)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	}
}

void AShootWeaponPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, WeaponItemDefinition);
	DOREPLIFETIME(ThisClass, ItemLifetime);
	DOREPLIFETIME(ThisClass, StoredStatTags);
}

void AShootWeaponPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                             const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	if (TriggerMode == EShootInteractionTriggerMode::AutoOverlap && CanBeTriggeredBy(Pawn))
	{
		HandlePickup(Pawn);
	}
}

bool AShootWeaponPickupActor::HandlePickup(APawn* PickingPawn)
{
	const TCHAR* AuthorityStr = GetAuthorityString(this);

	if (!HasAuthority() || !PickingPawn || !WeaponItemDefinition)
	{
		UE_LOG(LogWeaponPickup, Warning, TEXT("[%s] HandlePickup failed: invalid state."), AuthorityStr);
		return false;
	}

	if (!CanBeTriggeredBy(PickingPawn))
	{
		return false;
	}

	UShootInventoryManagerComponent* InventoryManager = ResolveInventoryManagerFromPawn(PickingPawn);
	if (!InventoryManager)
	{
		UE_LOG(LogWeaponPickup, Warning, TEXT("[%s] HandlePickup failed: missing InventoryManager on %s."), AuthorityStr, *GetNameSafe(PickingPawn));
		return false;
	}

	FShootInventoryItemInitData InitData;
	InitData.Lifetime = ItemLifetime;
	InitData.InitialStatTags = StoredStatTags;

	UShootInventoryItemInstance* NewInstance = nullptr;

	if (ItemLifetime == EShootItemLifetime::Persistent)
	{
		NewInstance = InventoryManager->AddPersistentItem(WeaponItemDefinition);
	}
	else
	{
		NewInstance = InventoryManager->AddRuntimeWeaponItem(WeaponItemDefinition, InitData);
	}

	if (!NewInstance)
	{
		UE_LOG(LogWeaponPickup, Warning, TEXT("[%s] HandlePickup failed: unable to create ItemInstance for %s."), AuthorityStr, *GetNameSafe(WeaponItemDefinition));
		return false;
	}

	if (!AutoEquipIfPossible(PickingPawn, NewInstance))
	{
		// RuntimeOnly 武器只有真正进入 Controller QuickBar 后才算本次拾取成功。
		// 失败时撤销刚创建的身份物品并保留地面 Actor，避免“枪消失但玩家没拿到”。
		InventoryManager->RemoveItemInstance(NewInstance);
		UE_LOG(LogWeaponPickup, Warning,
			TEXT("[%s] HandlePickup kept world pickup because QuickBar assignment failed on %s."),
			AuthorityStr, *GetNameSafe(PickingPawn));
		return false;
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}

	UE_LOG(LogWeaponPickup, Log, TEXT("[%s] Weapon pickup collected by %s (Lifetime=%s, SlotAssigned=%d)."),
		AuthorityStr,
		*GetNameSafe(PickingPawn),
		ItemLifetime == EShootItemLifetime::RuntimeOnly ? TEXT("RuntimeOnly") : TEXT("Persistent"),
		PickingPawn->FindComponentByClass<UCombatComponent>() ? PickingPawn->FindComponentByClass<UCombatComponent>()->GetActiveSlotIndex() : INDEX_NONE);

	return true;
}

bool AShootWeaponPickupActor::AutoEquipIfPossible(APawn* PickingPawn, UShootInventoryItemInstance* NewInstance)
{
	if (!PickingPawn || !NewInstance)
	{
		return false;
	}

	if (NewInstance->GetItemLifetime() == EShootItemLifetime::RuntimeOnly)
	{
		if (AShootPlayerController* PlayerController = Cast<AShootPlayerController>(PickingPawn->GetController()))
		{
			if (UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
			{
				int32 SlotIndex = INDEX_NONE;
				if (QuickBar->AssignRuntimeItemToAvailableSlot(NewInstance, SlotIndex))
				{
					UE_LOG(LogWeaponPickup, Log, TEXT("[%s] Assigned runtime weapon to Controller QuickBar slot %d on %s."),
						GetAuthorityString(this), SlotIndex, *GetNameSafe(PickingPawn));
					return true;
				}
				else
				{
					UE_LOG(LogWeaponPickup, Warning, TEXT("[%s] No free Controller QuickBar slot when picking up weapon on %s."),
						GetAuthorityString(this), *GetNameSafe(PickingPawn));
				}
			}
		}
		return false;
	}

	if (UCombatComponent* CombatComponent = PickingPawn->FindComponentByClass<UCombatComponent>())
	{
		const int32 SlotIndex = CombatComponent->GetNextFreeItemSlot();
		if (SlotIndex != INDEX_NONE)
		{
			CombatComponent->AssignSlotFromInventory(SlotIndex, NewInstance->GetItemInstanceId());
		}
	}

	// Persistent 物品的拾取成功与是否立即进入战斗槽位解耦；它已经由 PlayerState InventoryManager 持有。
	return true;
}

void AShootWeaponPickupActor::InitializeFromDrop(TSubclassOf<UShootInventoryItemDefinition> InDefinition, EShootItemLifetime InLifetime,
                                                 const FShootGameplayTagStackContainer& InStatTags)
{
	WeaponItemDefinition = InDefinition;
	ItemLifetime = InLifetime;
	StoredStatTags = InStatTags;
	// 玩家主动丢出的武器不能沿用原生类的 AutoOverlap 默认值，否则在角色脚下生成后会被同一玩家立即捡回。
	// 固定点拾取物仍由各自蓝图配置 TriggerMode；这里只有掉落初始化链强制改为按键交互。
	TriggerMode = EShootInteractionTriggerMode::PressToInteract;
}

void AShootWeaponPickupActor::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                                       FInteractionOptionBuilder& OptionBuilder)
{
	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract || !WeaponItemDefinition)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!Pawn)
	{
		AController* Controller = InteractQuery.RequestingController.Get();
		Pawn = Controller ? Controller->GetPawn() : nullptr;
	}

	if (!Pawn || !CanBeTriggeredBy(Pawn))
	{
		return;
	}

	FInteractionOption Option;
	const UShootInventoryItemDefinition* ItemCDO = WeaponItemDefinition->GetDefaultObject<UShootInventoryItemDefinition>();
	Option.Text = ItemCDO ? ItemCDO->DisplayName : FText();
	Option.SubText = InteractionSubText;
	const TSubclassOf<UGameplayAbility> AbilityClassToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_Collect::StaticClass());
	Option.InteractionAbilityToGrant = AbilityClassToGrant;

	OptionBuilder.AddInteractionOption(Option);
}

void AShootWeaponPickupActor::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
                                                            FGameplayEventData& InOutEventData)
{
	InOutEventData.Target = this;
}

bool AShootWeaponPickupActor::CanBeTriggeredBy(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	const bool bIsPlayer = IsPlayerActor(Pawn);
	switch (UserFilter)
	{
	case EShootInteractionUserFilter::PlayerOnly:
		return bIsPlayer;
	case EShootInteractionUserFilter::AIOnly:
		return !bIsPlayer;
	case EShootInteractionUserFilter::PlayerAndAI:
	default:
		return true;
	}
}

bool AShootWeaponPickupActor::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}
