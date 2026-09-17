// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootMapTravelPortal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Abilities/ShootGA_Interaction_Travel.h"
#include "Kismet/GameplayStatics.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootMapTravelPortal)

DEFINE_LOG_CATEGORY_STATIC(LogShootMapTravelPortal, Log, All);

AShootMapTravelPortal::AShootMapTravelPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(120.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionText = NSLOCTEXT("ShootMapTravelPortal", "DefaultInteractionText", "Enter Expedition");
	InteractionSubText = NSLOCTEXT("ShootMapTravelPortal", "DefaultInteractionSubText", "Go to the Weapon Practice Area");
	InteractionAbilityClass = UShootGA_Interaction_Travel::StaticClass();
}

void AShootMapTravelPortal::GatherInteractionOptions(
	const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract || DestinationMap.IsNull())
	{
		return;
	}

	APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!RequestingPawn)
	{
		const AController* RequestingController = InteractQuery.RequestingController.Get();
		RequestingPawn = RequestingController ? RequestingController->GetPawn() : nullptr;
	}

	if (!CanBeTriggeredBy(RequestingPawn))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_Travel::StaticClass());
	OptionBuilder.AddInteractionOption(Option);
}

void AShootMapTravelPortal::CustomizeInteractionEventData(
	const FGameplayTag& InteractionEventTag,
	FGameplayEventData& InOutEventData)
{
	InOutEventData.Target = this;
}

bool AShootMapTravelPortal::HandleTravel(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !CanBeTriggeredBy(InstigatorPawn) || DestinationMap.IsNull())
	{
		return false;
	}

	const FString DestinationPackage = DestinationMap.ToSoftObjectPath().GetLongPackageName();
	if (DestinationPackage.IsEmpty())
	{
		return false;
	}

	if (bClearAllRuntimeSessionsBeforeTravel)
	{
		ClearAllRuntimeSessions();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UE_LOG(LogShootMapTravelPortal, Log, TEXT("Traveling from %s to %s (NetMode=%d)."),
		*World->GetPackage()->GetName(), *DestinationPackage, static_cast<int32>(World->GetNetMode()));

	if (World->GetNetMode() == NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName(*DestinationPackage));
	}
	else if (bEndOnlineSessionBeforeTravel)
	{
		// 不在 Portal 维护第二套 DestroySession/ClientTravel。Coordinator 会让房主销毁 Session，
		// 通知远端客户端断开，并由每台机器的 ShootGameInstance 统一回到 SessionReturnMap(HomeMap)。
		UGameInstance* GameInstance = World->GetGameInstance();
		UShootSessionCoordinatorSubsystem* Coordinator = GameInstance
			? GameInstance->GetSubsystem<UShootSessionCoordinatorSubsystem>()
			: nullptr;
		APlayerController* PlayerController = InstigatorPawn
			? Cast<APlayerController>(InstigatorPawn->GetController())
			: nullptr;
		if (!Coordinator || !Coordinator->LeaveOrDestroySession(PlayerController))
		{
			UE_LOG(LogShootMapTravelPortal, Error,
				TEXT("Failed to end the online expedition session before returning to %s."),
				*DestinationPackage);
			return false;
		}
	}
	else
	{
		// 相对 ServerTravel 保留现有 Listen URL 与会话选项；客户端不能执行这一分支。
		World->ServerTravel(DestinationPackage, /*bAbsolute=*/false);
	}

	return true;
}

bool AShootMapTravelPortal::CanBeTriggeredBy(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	const bool bIsPlayerControlled = Pawn->IsPlayerControlled();
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

void AShootMapTravelPortal::ClearAllRuntimeSessions()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			if (UShootQuickBarComponent* QuickBar = PlayerController->FindComponentByClass<UShootQuickBarComponent>())
			{
				QuickBar->ClearRuntimeSession();
			}
		}
	}
}
