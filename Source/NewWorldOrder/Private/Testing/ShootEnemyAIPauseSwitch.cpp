// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Testing/ShootEnemyAIPauseSwitch.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Abilities/ShootGA_Interaction_ToggleEnemyAI.h"
#include "Net/UnrealNetwork.h"
#include "Testing/ShootEnemyTestSpawner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEnemyAIPauseSwitch)

AShootEnemyAIPauseSwitch::AShootEnemyAIPauseSwitch()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->InitSphereRadius(180.0f);
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	InteractionCollision->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(InteractionCollision);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PauseInteractionText = NSLOCTEXT("EnemyAIPauseSwitch", "Pause", "Pause AI Behavior");
	ResumeInteractionText = NSLOCTEXT("EnemyAIPauseSwitch", "Resume", "Resume AI Behavior");
	InteractionSubText = NSLOCTEXT("EnemyAIPauseSwitch", "TestOnly", "Controls enemy behavior");
	InteractionAbilityClass = UShootGA_Interaction_ToggleEnemyAI::StaticClass();
}

void AShootEnemyAIPauseSwitch::GatherInteractionOptions(
	const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	const APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!CanToggleForPawn(RequestingPawn))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = bEnemyBehaviorEnabled ? PauseInteractionText : ResumeInteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_ToggleEnemyAI::StaticClass());
	Option.InteractionWidgetClass = InteractionWidgetClass;
	OptionBuilder.AddInteractionOption(Option);
}

void AShootEnemyAIPauseSwitch::CustomizeInteractionEventData(
	const FGameplayTag& InteractionEventTag,
	FGameplayEventData& InOutEventData)
{
	(void)InteractionEventTag;
	InOutEventData.Target = this;
}

bool AShootEnemyAIPauseSwitch::CanToggleForPawn(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled() && InteractionCollision
		&& InteractionCollision->IsOverlappingActor(Pawn);
}

void AShootEnemyAIPauseSwitch::ToggleEnemyBehavior()
{
	if (!HasAuthority())
	{
		return;
	}

	bEnemyBehaviorEnabled = !bEnemyBehaviorEnabled;
	for (TActorIterator<AShootEnemyTestSpawner> It(GetWorld()); It; ++It)
	{
		It->SetEnemyBehaviorEnabled(bEnemyBehaviorEnabled);
	}
	ForceNetUpdate();
}

void AShootEnemyAIPauseSwitch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bEnemyBehaviorEnabled);
}
