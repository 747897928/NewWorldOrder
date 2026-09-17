// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Skills/ShootRobotCompanionComponent.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/Skills/ShootSkillDefinition.h"
#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"
#include "Character/ShootCharacterBase.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRobotCompanionComponent)

DEFINE_LOG_CATEGORY_STATIC(LogShootRobotCompanion, Log, All);

UShootRobotCompanionComponent::UShootRobotCompanionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UShootRobotCompanionComponent::BeginPlay()
{
	Super::BeginPlay();

	AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	if (!ShootPlayerState)
	{
		return;
	}

	ShootPlayerState->OnPawnSet.AddDynamic(this, &ThisClass::HandleOwnerPawnChanged);
	BindOwnerPawn(ShootPlayerState->GetPawn(), nullptr);
	if (UShootSkillLoadoutComponent* Loadout = ShootPlayerState->GetSkillLoadoutComponent())
	{
		Loadout->OnSkillSlotsChanged().AddUObject(this, &ThisClass::HandleSkillSlotsChanged);
	}
}

void UShootRobotCompanionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner()))
	{
		ShootPlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::HandleOwnerPawnChanged);
		if (UShootSkillLoadoutComponent* Loadout = ShootPlayerState->GetSkillLoadoutComponent())
		{
			Loadout->OnSkillSlotsChanged().RemoveAll(this);
		}
	}
	BindOwnerPawn(nullptr, BoundOwnerPawn);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// EndPlay/Experience 清理不能制造一段无意义的跨地图冷却。
		DestroyActiveRobot(ECleanupReason::ExperienceUnload);
	}
	Super::EndPlay(EndPlayReason);
}

void UShootRobotCompanionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, ActiveRobot, COND_OwnerOnly);
}

bool UShootRobotCompanionComponent::ActivateOrCycleRobot(APawn* RequestingPawn,
	const UShootSkillDefinition* Definition, TSubclassOf<AShootRobotCompanionCharacter> RobotClass,
	TSubclassOf<UGameplayEffect> DestroyedCooldownEffect, const FVector& SpawnOffset,
	const float SummonDropHeight, const float CompanionLifetime, const bool bOwnerDeathStartsCooldown)
{
	AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	if (!ShootPlayerState || !ShootPlayerState->HasAuthority() || !RequestingPawn
		|| RequestingPawn->GetPlayerState() != ShootPlayerState || !Definition || !RobotClass)
	{
		return false;
	}

	UShootSkillLoadoutComponent* Loadout = ShootPlayerState->GetSkillLoadoutComponent();
	if (!Loadout)
	{
		return false;
	}

	if (IsValid(ActiveRobot))
	{
		FGameplayTag NewMode;
		if (!Loadout->CycleSkillMode(Definition, NewMode))
		{
			return false;
		}
		ActiveRobot->SetCommandMode(NewMode, RequestingPawn);
		return true;
	}

	int32 SkillLevel = 0;
	FGameplayTag InitialMode;
	ActiveDefinition = Definition;
	if (!ResolveActiveSlot(SkillLevel, InitialMode))
	{
		ActiveDefinition = nullptr;
		return false;
	}

	const FTransform OwnerTransform = RequestingPawn->GetActorTransform();
	const FVector WorldOffset = OwnerTransform.TransformVectorNoScale(SpawnOffset);
	const float ClampedDropHeight = FMath::Max(0.f, SummonDropHeight);
	FTransform SpawnTransform(OwnerTransform.GetRotation(),
		OwnerTransform.GetLocation() + WorldOffset + FVector::UpVector * ClampedDropHeight);
	AShootRobotCompanionCharacter* NewRobot = GetWorld()->SpawnActorDeferred<AShootRobotCompanionCharacter>(
		RobotClass, SpawnTransform, RequestingPawn, RequestingPawn,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!NewRobot)
	{
		ActiveDefinition = nullptr;
		return false;
	}

	ActiveDestroyedCooldownEffect = DestroyedCooldownEffect;
	bActiveOwnerDeathStartsCooldown = bOwnerDeathStartsCooldown;
	bCombatDeathAlreadyHandled = false;
	NewRobot->InitializeCompanion(ShootPlayerState, SkillLevel, InitialMode);
	NewRobot->MarkSummonDropPending(ClampedDropHeight > 0.f);
	NewRobot->FinishSpawning(SpawnTransform);
	NewRobot->SpawnDefaultController();
	NewRobot->OnDestroyed.AddDynamic(this, &ThisClass::HandleRobotDestroyed);
	ActiveRobot = NewRobot;
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimerHandle, this,
		&ThisClass::HandleRobotLifetimeExpired, FMath::Max(1.f, CompanionLifetime), false);
	ShootPlayerState->ForceNetUpdate();
	return true;
}

void UShootRobotCompanionComponent::NotifyRobotCombatDestroyed(AShootRobotCompanionCharacter* DestroyedRobot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || DestroyedRobot != ActiveRobot
		|| bCombatDeathAlreadyHandled)
	{
		return;
	}

	bCombatDeathAlreadyHandled = true;
	ApplyDestroyedCooldown();
}

void UShootRobotCompanionComponent::HandleOwnerPawnChanged(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn)
{
	if (PlayerState != GetOwner())
	{
		return;
	}
	BindOwnerPawn(NewPawn, OldPawn);
	if (GetOwner()->HasAuthority() && IsValid(ActiveRobot) && NewPawn)
	{
		ActiveRobot->SetCompanionOwnerPawn(NewPawn);
	}
}

void UShootRobotCompanionComponent::BindOwnerPawn(APawn* NewPawn, APawn* OldPawn)
{
	if (AShootCharacterBase* OldCharacter = Cast<AShootCharacterBase>(OldPawn ? OldPawn : BoundOwnerPawn.Get()))
	{
		OldCharacter->GetOnDeathDelegate().RemoveDynamic(this, &ThisClass::HandleOwnerPawnDeath);
	}
	BoundOwnerPawn = Cast<AShootCharacterBase>(NewPawn);
	if (BoundOwnerPawn)
	{
		BoundOwnerPawn->GetOnDeathDelegate().AddUniqueDynamic(this, &ThisClass::HandleOwnerPawnDeath);
	}
}

void UShootRobotCompanionComponent::HandleOwnerPawnDeath(AActor* DeadActor)
{
	if (GetOwner() && GetOwner()->HasAuthority() && DeadActor == BoundOwnerPawn)
	{
		// Owner 死亡是技能所有权失效，不等价于机器人被敌人击毁。默认不惩罚冷却，可由 GA 蓝图显式开启。
		DestroyActiveRobot(ECleanupReason::OwnerDeath);
	}
}

void UShootRobotCompanionComponent::HandleRobotDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != ActiveRobot)
	{
		return;
	}
	ActiveRobot = nullptr;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	}
	ActiveDefinition = nullptr;
	ActiveDestroyedCooldownEffect = nullptr;
	bCombatDeathAlreadyHandled = false;
	if (GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
}

void UShootRobotCompanionComponent::HandleRobotLifetimeExpired()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(ActiveRobot)
		|| bCombatDeathAlreadyHandled)
	{
		return;
	}

	// 同一技能键在机器人存续时负责切模式，因此不能在召唤瞬间用普通 Commit 锁死 GA。
	// 存续结束才开始重召冷却：规则清楚、UI 可解释，也不需要为“冷却中仍允许指令”开后门。
	bCombatDeathAlreadyHandled = true;
	ApplyDestroyedCooldown();
	DestroyActiveRobot(ECleanupReason::LifetimeExpired);
}

void UShootRobotCompanionComponent::HandleSkillSlotsChanged(UShootSkillLoadoutComponent* ChangedComponent,
	int32 ChangedSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(ActiveRobot) || !ActiveDefinition)
	{
		return;
	}

	int32 SkillLevel = 0;
	FGameplayTag Mode;
	if (!ResolveActiveSlot(SkillLevel, Mode))
	{
		// 技能替换、四槽清理与 Experience 卸载统一从这里回收机器人，不开始冷却。
		DestroyActiveRobot(ECleanupReason::SkillRemoved);
		return;
	}
	ActiveRobot->SetCompanionLevel(SkillLevel);
	ActiveRobot->SetCommandMode(Mode, BoundOwnerPawn);
}

void UShootRobotCompanionComponent::DestroyActiveRobot(const ECleanupReason Reason)
{
	AShootRobotCompanionCharacter* Robot = ActiveRobot;
	if (!IsValid(Robot))
	{
		return;
	}
	const bool bOwnerDeath = Reason == ECleanupReason::OwnerDeath;
	const bool bLifetimeExpired = Reason == ECleanupReason::LifetimeExpired;
	if (bOwnerDeath && bActiveOwnerDeathStartsCooldown && !bCombatDeathAlreadyHandled)
	{
		ApplyDestroyedCooldown();
		bCombatDeathAlreadyHandled = true;
	}
	if (bOwnerDeath)
	{
		Robot->SelfDestructFromOwnerLoss();
	}
	else if (bLifetimeExpired)
	{
		Robot->SelfDestructFromLifetimeExpiry();
	}
	else
	{
		Robot->Destroy();
	}
}

void UShootRobotCompanionComponent::ApplyDestroyedCooldown()
{
	AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	UShootAbilitySystemComponent* ASC = ShootPlayerState
		? Cast<UShootAbilitySystemComponent>(ShootPlayerState->GetAbilitySystemComponent())
		: nullptr;
	if (!ASC || !ActiveDestroyedCooldownEffect)
	{
		UE_LOG(LogShootRobotCompanion, Warning,
			TEXT("Robot was destroyed but %s has no cooldown GameplayEffect configured."), *GetNameSafe(GetOwner()));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(const_cast<UShootSkillDefinition*>(ActiveDefinition.Get()));
	if (const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ActiveDestroyedCooldownEffect, 1.f, Context);
		Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

bool UShootRobotCompanionComponent::ResolveActiveSlot(int32& OutLevel, FGameplayTag& OutMode) const
{
	OutLevel = 0;
	OutMode = FGameplayTag();
	const AShootPlayerState* ShootPlayerState = Cast<AShootPlayerState>(GetOwner());
	const UShootSkillLoadoutComponent* Loadout = ShootPlayerState
		? ShootPlayerState->GetSkillLoadoutComponent()
		: nullptr;
	if (!Loadout || !ActiveDefinition)
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < Loadout->GetSlotCount(); ++SlotIndex)
	{
		const FShootSkillSlot Slot = Loadout->GetSlot(SlotIndex);
		if (Slot.SkillDefinition == ActiveDefinition)
		{
			OutLevel = Slot.Level;
			OutMode = Slot.CurrentModeTag;
			return !Slot.IsEmpty();
		}
	}
	return false;
}

void UShootRobotCompanionComponent::OnRep_ActiveRobot()
{
	// 指针复制只供拥有客户端和蓝图读取；世界表现由机器人 Actor 自身复制。
}
