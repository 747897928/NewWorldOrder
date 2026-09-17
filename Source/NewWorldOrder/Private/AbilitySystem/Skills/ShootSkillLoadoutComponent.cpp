// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"

#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/Skills/ShootSkillDefinition.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "GameModes/ShootExperienceManagerComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootSkillLoadoutComponent)

DEFINE_LOG_CATEGORY_STATIC(LogShootSkillLoadout, Log, All);

UShootSkillLoadoutComponent::UShootSkillLoadoutComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Slots.SetNum(SkillSlotCount);
	GrantedSlotHandles.SetNum(SkillSlotCount);
}

void UShootSkillLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClearMatchSkills();
	}
	Super::EndPlay(EndPlayReason);
}

void UShootSkillLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, Slots, COND_OwnerOnly);
}

FShootSkillSlot UShootSkillLoadoutComponent::GetSlot(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FShootSkillSlot();
}

UInputAction* UShootSkillLoadoutComponent::GetSlotInputAction(int32 SlotIndex) const
{
	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	return Config && Config->SlotInputActions.IsValidIndex(SlotIndex)
		? Config->SlotInputActions[SlotIndex]
		: nullptr;
}

FGameplayTag UShootSkillLoadoutComponent::GetSlotInputTag(int32 SlotIndex) const
{
	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	return Config && Config->SlotInputTags.IsValidIndex(SlotIndex)
		? Config->SlotInputTags[SlotIndex]
		: FGameplayTag();
}

const UShootSkillLoadoutConfig* UShootSkillLoadoutComponent::GetCurrentConfig() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState<AGameStateBase>() : nullptr;
	const UShootExperienceManagerComponent* ExperienceManager =
		GameState ? GameState->FindComponentByClass<UShootExperienceManagerComponent>() : nullptr;
	const UShootExperienceDefinition* Experience =
		ExperienceManager ? ExperienceManager->GetCurrentExperience() : nullptr;
	return Experience ? Experience->SkillLoadoutConfig : nullptr;
}

bool UShootSkillLoadoutComponent::IsCurrentConfigValid(const UShootSkillLoadoutConfig* Config) const
{
	if (!Config || Config->SlotInputTags.Num() != SkillSlotCount ||
		Config->SlotInputActions.Num() != SkillSlotCount)
	{
		return false;
	}

	FGameplayTagContainer UniqueInputTags;
	for (int32 SlotIndex = 0; SlotIndex < SkillSlotCount; ++SlotIndex)
	{
		const FGameplayTag SlotInputTag = Config->SlotInputTags[SlotIndex];
		if (!SlotInputTag.IsValid() || UniqueInputTags.HasTagExact(SlotInputTag))
		{
			return false;
		}
		if (!Config->SlotInputActions[SlotIndex])
		{
			return false;
		}
		UniqueInputTags.AddTag(SlotInputTag);
	}
	return true;
}

UShootAbilitySystemComponent* UShootSkillLoadoutComponent::GetShootASC() const
{
	const AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	return ShootPlayerState
		? Cast<UShootAbilitySystemComponent>(ShootPlayerState->GetAbilitySystemComponent())
		: nullptr;
}

int32 UShootSkillLoadoutComponent::FindSlotForDefinition(const UShootSkillDefinition* Definition) const
{
	if (!Definition)
	{
		return INDEX_NONE;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		const UShootSkillDefinition* Existing = Slots[SlotIndex].SkillDefinition;
		if (Existing == Definition ||
			(Existing && Definition->SkillTag.IsValid() && Existing->SkillTag.MatchesTagExact(Definition->SkillTag)))
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

int32 UShootSkillLoadoutComponent::FindFirstEmptySlot() const
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (Slots[SlotIndex].IsEmpty())
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

bool UShootSkillLoadoutComponent::CanAcquireAnySkill() const
{
	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	if (!IsCurrentConfigValid(Config))
	{
		return false;
	}

	const bool bHasEmptySlot = FindFirstEmptySlot() != INDEX_NONE;
	for (const UShootSkillDefinition* Definition : Config->RandomSkillPool)
	{
		if (!Definition || !Definition->AbilitySet)
		{
			continue;
		}

		const int32 ExistingSlot = FindSlotForDefinition(Definition);
		if ((ExistingSlot == INDEX_NONE && bHasEmptySlot) ||
			(ExistingSlot != INDEX_NONE && Slots[ExistingSlot].Level < FMath::Max(1, Definition->MaxLevel)))
		{
			return true;
		}
	}
	return false;
}

bool UShootSkillLoadoutComponent::CanAcquireSkill(const UShootSkillDefinition* Definition) const
{
	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	if (!IsCurrentConfigValid(Config) || !Definition || !Definition->AbilitySet ||
		!Config->RandomSkillPool.Contains(Definition))
	{
		return false;
	}

	const int32 ExistingSlot = FindSlotForDefinition(Definition);
	return ExistingSlot != INDEX_NONE
		? Slots[ExistingSlot].Level < FMath::Max(1, Definition->MaxLevel)
		: FindFirstEmptySlot() != INDEX_NONE;
}

bool UShootSkillLoadoutComponent::AcquireSkill(APawn* RequestingPawn,
	const UShootSkillDefinition* Definition, int32& OutSlotIndex, EShootSkillAcquireResult& OutResult)
{
	OutSlotIndex = INDEX_NONE;
	OutResult = EShootSkillAcquireResult::InvalidRequest;

	AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	if (!ShootPlayerState || !ShootPlayerState->HasAuthority() || !RequestingPawn ||
		RequestingPawn->GetPlayerState() != ShootPlayerState || !CanAcquireSkill(Definition))
	{
		return false;
	}

	return ApplyDefinitionToSlot(Definition, OutSlotIndex, OutResult);
}

bool UShootSkillLoadoutComponent::AcquireRandomSkill(APawn* RequestingPawn, int32& OutSlotIndex,
	EShootSkillAcquireResult& OutResult)
{
	OutSlotIndex = INDEX_NONE;
	OutResult = EShootSkillAcquireResult::InvalidRequest;

	AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	if (!ShootPlayerState || !ShootPlayerState->HasAuthority() || !RequestingPawn ||
		RequestingPawn->GetPlayerState() != ShootPlayerState)
	{
		return false;
	}

	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	if (!IsCurrentConfigValid(Config))
	{
		UE_LOG(LogShootSkillLoadout, Error,
			TEXT("Experience SkillLoadoutConfig must provide exactly %d valid InputTags and InputActions."), SkillSlotCount);
		return false;
	}

	TArray<const UShootSkillDefinition*> EligibleDefinitions;
	const bool bHasEmptySlot = FindFirstEmptySlot() != INDEX_NONE;
	for (const UShootSkillDefinition* Definition : Config->RandomSkillPool)
	{
		if (!Definition || !Definition->AbilitySet)
		{
			continue;
		}

		const int32 ExistingSlot = FindSlotForDefinition(Definition);
		// 构筑阶段优先把四个空槽填满；只有四槽已满后才进入升级池。
		// 这样玩家不会在还有空槽时连续抽到同一技能升级，也让四次获取稳定覆盖四个输入槽。
		if ((bHasEmptySlot && ExistingSlot == INDEX_NONE) ||
			(!bHasEmptySlot && ExistingSlot != INDEX_NONE &&
				Slots[ExistingSlot].Level < FMath::Max(1, Definition->MaxLevel)))
		{
			EligibleDefinitions.AddUnique(Definition);
		}
	}

	if (EligibleDefinitions.IsEmpty())
	{
		OutResult = EShootSkillAcquireResult::NoEligibleSkill;
		return false;
	}

	const UShootSkillDefinition* Selected = EligibleDefinitions[FMath::RandHelper(EligibleDefinitions.Num())];
	return AcquireSkill(RequestingPawn, Selected, OutSlotIndex, OutResult);
}

bool UShootSkillLoadoutComponent::SetSkillMode(const UShootSkillDefinition* Definition,
	const FGameplayTag NewModeTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Definition || !NewModeTag.IsValid()
		|| !Definition->ModeTags.Contains(NewModeTag))
	{
		return false;
	}

	const int32 SlotIndex = FindSlotForDefinition(Definition);
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return false;
	}

	if (Slots[SlotIndex].CurrentModeTag != NewModeTag)
	{
		Slots[SlotIndex].CurrentModeTag = NewModeTag;
		NotifySlotsChanged(SlotIndex);
		GetOwner()->ForceNetUpdate();
	}
	return true;
}

bool UShootSkillLoadoutComponent::CycleSkillMode(const UShootSkillDefinition* Definition,
	FGameplayTag& OutNewModeTag)
{
	OutNewModeTag = FGameplayTag();
	if (!Definition || Definition->ModeTags.IsEmpty())
	{
		return false;
	}

	const int32 SlotIndex = FindSlotForDefinition(Definition);
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return false;
	}

	const int32 CurrentIndex = Definition->ModeTags.IndexOfByKey(Slots[SlotIndex].CurrentModeTag);
	const int32 NextIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % Definition->ModeTags.Num();
	OutNewModeTag = Definition->ModeTags[NextIndex];
	return SetSkillMode(Definition, OutNewModeTag);
}

bool UShootSkillLoadoutComponent::ApplyDefinitionToSlot(const UShootSkillDefinition* Definition, int32& OutSlotIndex,
	EShootSkillAcquireResult& OutResult)
{
	if (!Definition || !Definition->AbilitySet || !GetShootASC())
	{
		OutResult = EShootSkillAcquireResult::InvalidRequest;
		return false;
	}

	int32 SlotIndex = FindSlotForDefinition(Definition);
	if (SlotIndex != INDEX_NONE)
	{
		FShootSkillSlot& Slot = Slots[SlotIndex];
		const int32 MaxLevel = FMath::Max(1, Definition->MaxLevel);
		if (Slot.Level >= MaxLevel)
		{
			OutResult = EShootSkillAcquireResult::NoEligibleSkill;
			return false;
		}
		Slot.Level++;
		OutResult = EShootSkillAcquireResult::UpgradedExistingSkill;
	}
	else
	{
		SlotIndex = FindFirstEmptySlot();
		if (SlotIndex == INDEX_NONE)
		{
			OutResult = EShootSkillAcquireResult::NoEligibleSkill;
			return false;
		}

		FShootSkillSlot& Slot = Slots[SlotIndex];
		Slot.SkillDefinition = Definition;
		Slot.Level = 1;
		Slot.CurrentModeTag = Definition->ModeTags.IsEmpty() ? FGameplayTag() : Definition->ModeTags[0];
		OutResult = EShootSkillAcquireResult::GrantedNewSkill;
	}

	RegrantSlot(SlotIndex);
	OutSlotIndex = SlotIndex;
	NotifySlotsChanged(SlotIndex);
	GetOwner()->ForceNetUpdate();
	return true;
}

void UShootSkillLoadoutComponent::RegrantSlot(int32 SlotIndex)
{
	UShootAbilitySystemComponent* ShootASC = GetShootASC();
	const UShootSkillLoadoutConfig* Config = GetCurrentConfig();
	if (!ShootASC || !Slots.IsValidIndex(SlotIndex) || !GrantedSlotHandles.IsValidIndex(SlotIndex))
	{
		return;
	}

	GrantedSlotHandles[SlotIndex].TakeFromAbilitySystem(ShootASC);
	const FShootSkillSlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty() || !Slot.SkillDefinition->AbilitySet || !Config ||
		!Config->SlotInputTags.IsValidIndex(SlotIndex) || !Config->SlotInputTags[SlotIndex].IsValid())
	{
		return;
	}

	Slot.SkillDefinition->AbilitySet->GiveToAbilitySystem(
		ShootASC,
		&GrantedSlotHandles[SlotIndex],
		const_cast<UShootSkillDefinition*>(Slot.SkillDefinition.Get()),
		Slot.Level,
		Config->SlotInputTags[SlotIndex]);
}

void UShootSkillLoadoutComponent::ClearMatchSkills()
{
	UShootAbilitySystemComponent* ShootASC = GetShootASC();
	if (ShootASC)
	{
		for (FShootAbilitySet_GrantedHandles& Handles : GrantedSlotHandles)
		{
			Handles.TakeFromAbilitySystem(ShootASC);
		}
	}

	Slots.SetNum(SkillSlotCount);
	for (FShootSkillSlot& Slot : Slots)
	{
		Slot = FShootSkillSlot();
	}
	NotifySlotsChanged(INDEX_NONE);
	if (GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
}

bool UShootSkillLoadoutComponent::GetCooldownRemaining(int32 SlotIndex, float& OutTimeRemaining,
	float& OutDuration) const
{
	OutTimeRemaining = 0.f;
	OutDuration = 0.f;
	const FShootSkillSlot Slot = GetSlot(SlotIndex);
	const UShootAbilitySystemComponent* ShootASC = GetShootASC();
	if (Slot.IsEmpty() || !Slot.SkillDefinition->CooldownTag.IsValid() || !ShootASC)
	{
		return false;
	}

	FGameplayTagContainer CooldownTags(Slot.SkillDefinition->CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<TPair<float, float>> RemainingAndDuration =
		ShootASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	for (const TPair<float, float>& Pair : RemainingAndDuration)
	{
		if (Pair.Key > OutTimeRemaining)
		{
			OutTimeRemaining = Pair.Key;
			OutDuration = Pair.Value;
		}
	}
	return OutDuration > 0.f;
}

void UShootSkillLoadoutComponent::OnRep_Slots()
{
	if (Slots.Num() != SkillSlotCount)
	{
		Slots.SetNum(SkillSlotCount);
	}
	NotifySlotsChanged(INDEX_NONE);
}

void UShootSkillLoadoutComponent::NotifySlotsChanged(int32 ChangedSlotIndex)
{
	SkillSlotsChangedDelegate.Broadcast(this, ChangedSlotIndex);

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		FShootSkillLoadoutChangedMessage Message;
		Message.PlayerState = Cast<APlayerState>(GetOwner());
		Message.ChangedSlotIndex = ChangedSlotIndex;
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			FShootGameplayTags::Get().Msg_Skill_LoadoutChanged, Message);
	}
}
