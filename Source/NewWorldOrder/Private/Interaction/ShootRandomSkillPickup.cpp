// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootRandomSkillPickup.h"

#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"
#include "AbilitySystem/Skills/ShootSkillDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Abilities/ShootGA_Interaction_AcquireSkill.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRandomSkillPickup)

AShootRandomSkillPickup::AShootRandomSkillPickup()
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

	InteractionText = NSLOCTEXT("RandomSkillPickup", "InteractionText", "Random Skill");
	InteractionSubText = NSLOCTEXT("RandomSkillPickup", "InteractionSubText", "Gain a new skill or upgrade an existing skill");
	InteractionAbilityClass = UShootGA_Interaction_AcquireSkill::StaticClass();
}

void AShootRandomSkillPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bAvailable);
}

void AShootRandomSkillPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyAvailabilityState();
}

void AShootRandomSkillPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

bool AShootRandomSkillPickup::CanGrantSkillToPawn(const APawn* Pawn) const
{
	if (!bAvailable || !Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}

	// 交互主能力已经用射线负责“玩家正在看向谁”；这里保留服务器距离校验，既避免远程伪造请求，
	// 又不会像旧的 IsOverlappingActor 那样把有效范围意外缩成 120cm，导致看得到却没有交互选项。
	const float MaxDistance = FMath::Max(0.f, MaxInteractionDistance);
	if (MaxDistance <= 0.f || FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) > FMath::Square(MaxDistance))
	{
		return false;
	}

	const AShootPlayerState* ShootPlayerState = Pawn->GetPlayerState<AShootPlayerState>();
	const UShootSkillLoadoutComponent* SkillLoadout =
		ShootPlayerState ? ShootPlayerState->GetSkillLoadoutComponent() : nullptr;
	return SkillLoadout && (OfferedSkillDefinition
		? SkillLoadout->CanAcquireSkill(OfferedSkillDefinition)
		: SkillLoadout->CanAcquireAnySkill());
}

bool AShootRandomSkillPickup::TryGrantSkillToPawn(APawn* Pawn)
{
	if (!HasAuthority() || !CanGrantSkillToPawn(Pawn))
	{
		return false;
	}

	AShootPlayerState* ShootPlayerState = Pawn->GetPlayerState<AShootPlayerState>();
	UShootSkillLoadoutComponent* SkillLoadout =
		ShootPlayerState ? ShootPlayerState->GetSkillLoadoutComponent() : nullptr;
	int32 GrantedSlot = INDEX_NONE;
	EShootSkillAcquireResult Result = EShootSkillAcquireResult::InvalidRequest;
	const bool bGranted = SkillLoadout && (OfferedSkillDefinition
		? SkillLoadout->AcquireSkill(Pawn, OfferedSkillDefinition, GrantedSlot, Result)
		: SkillLoadout->AcquireRandomSkill(Pawn, GrantedSlot, Result));
	if (!bGranted)
	{
		return false;
	}

	ConsumeByPawn(Pawn);
	return true;
}

void AShootRandomSkillPickup::ConsumeByPawn(const APawn* Pawn)
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
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ThisClass::Respawn, RespawnDelay, false);
}

void AShootRandomSkillPickup::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	if (!CanGrantSkillToPawn(Cast<APawn>(InteractQuery.RequestingAvatar.Get())))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.HoldDuration = HoldDuration;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_AcquireSkill::StaticClass());
	OptionBuilder.AddInteractionOption(Option);
}

void AShootRandomSkillPickup::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	FGameplayEventData& InOutEventData)
{
	(void)InteractionEventTag;
	InOutEventData.Target = this;
}

void AShootRandomSkillPickup::OnRep_Available()
{
	ApplyAvailabilityState();
}

void AShootRandomSkillPickup::Respawn()
{
	if (HasAuthority())
	{
		bAvailable = true;
		ApplyAvailabilityState();
	}
}

void AShootRandomSkillPickup::ApplyAvailabilityState()
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
