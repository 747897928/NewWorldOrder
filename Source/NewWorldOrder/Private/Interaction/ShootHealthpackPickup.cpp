// 一次性治疗包拾取物实现。
#include "Interaction/ShootHealthpackPickup.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Abilities/ShootGA_Interaction_HealthPack.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootHealthpackPickup)

AShootHealthpackPickup::AShootHealthpackPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->InitSphereRadius(120.f);
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	InteractionCollision->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(InteractionCollision);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionText = NSLOCTEXT("HealthpackPickup", "InteractionText", "Medkit");
	InteractionSubText = NSLOCTEXT("HealthpackPickup", "InteractionSubText", "Use to heal");
	InteractionAbilityClass = UShootGA_Interaction_HealthPack::StaticClass();
}

void AShootHealthpackPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bAvailable);
}

void AShootHealthpackPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyAvailabilityState();
}

void AShootHealthpackPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AShootHealthpackPickup::OnRep_Available()
{
	ApplyAvailabilityState();
}

void AShootHealthpackPickup::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	bAvailable = true;
	ApplyAvailabilityState();
}

void AShootHealthpackPickup::ApplyAvailabilityState()
{
	SetActorHiddenInGame(!bAvailable);

	if (VisualComponent)
	{
		VisualComponent->SetVisibility(bAvailable, true);
	}

	if (InteractionCollision)
	{
		InteractionCollision->SetCollisionEnabled(
			bAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		InteractionCollision->SetGenerateOverlapEvents(bAvailable);
	}
}

bool AShootHealthpackPickup::CanUsePawn(const APawn* Pawn) const
{
	if (!bAvailable || !Pawn || !Pawn->IsPlayerControlled() || !InteractionCollision ||
		!InteractionCollision->IsOverlappingActor(Pawn))
	{
		return false;
	}

	// 满血时不提供交互选项，避免治疗包被白捡浪费。
	// GetAbilitySystemComponent 要求非 const 参数：先去掉 const APawn* 的限定符，再隐式转换为 AActor*（本调用只读，安全）。
	const UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(Pawn));
	if (!PawnASC)
	{
		return false;
	}
	const float CurrentHealth = PawnASC->GetNumericAttribute(UShootAttributeSet::GetHealthAttribute());
	const float MaxHealth = PawnASC->GetNumericAttribute(UShootAttributeSet::GetMaxHealthAttribute());
	return MaxHealth > 0.f && CurrentHealth < MaxHealth;
}

void AShootHealthpackPickup::ConsumeByPawn(const APawn* Pawn)
{
	(void)Pawn;

	if (!HasAuthority() || !bAvailable)
	{
		return;
	}

	if (RespawnDelay <= 0.f)
	{
		Destroy();
		return;
	}

	bAvailable = false;
	ApplyAvailabilityState();
	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&ThisClass::Respawn,
		RespawnDelay,
		false);
}

void AShootHealthpackPickup::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	if (!CanUsePawn(Cast<APawn>(InteractQuery.RequestingAvatar.Get())))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.HoldDuration = HoldDuration;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_HealthPack::StaticClass());
	OptionBuilder.AddInteractionOption(Option);
}

void AShootHealthpackPickup::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	FGameplayEventData& InOutEventData)
{
	(void)InteractionEventTag;
	InOutEventData.Target = this;
}
