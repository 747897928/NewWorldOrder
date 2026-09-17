// Copyright ZhaoYiJie


#include "Player/ShootPlayerState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAbilitySet.h"
#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"
#include "AbilitySystem/Skills/ShootRobotCompanionComponent.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "GameModes/ShootExperienceManagerComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Effects/ShootEffect_SnapshotRestore.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_MedicalStation.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_RapidCharge.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_RescueCloak.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_TacticalScan.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Passive_MedicalExpertise.h"
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Passive_SmartAssist.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_ShockGrenade.h"
#include "AbilitySystem/Abilities/ShootGA_ThrowGrenade.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_SteelBulwark.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalAssault.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalOverload.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Passive_ArmorEnhancement.h"
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Passive_MarkHunter.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Inventory/Fragments/ShootInventoryFragment_WardrobeItem.h"
#include "Inventory/ShootInventoryFragment_EquippableItem.h"
#include "Character/CombatComponent.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Inventory/ResourceInventoryComponent.h"
#include "Character/QuickbarMessageTypes.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "System/SaveGameSubsystem.h"
#include "System/ShootSaveGame.h"
#include "Engine/GameInstance.h"
#include "ShootGameplayTags.h"
#include "GameplayEffect.h"

AShootPlayerState::AShootPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UShootAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	//`Mixed`同步模式需要`OwnerActor`的`Owner`是`Controller`. `PlayerState`的`Owner`默认是`Controller`但是`Character`不是.
	//如果`OwnerActor`不是`PlayerState`时使用`Mixed`同步模式, 那么需要在`OwnerActor`中调用`SetOwner()`设置`Controller`.  
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	//`AttributeSet`用于定义, 保存以及管理对`Attribute`的修改. 开发者应该继承UAttributeSet.
	//在OwnerActor的构造函数中创建`AttributeSet`会自动注册到其`ASC`. **这必须在C++中完成.**  
	AttributeSet = CreateDefaultSubobject<UShootAttributeSet>("AttributeSet");

	InventoryManagerComponent = CreateDefaultSubobject<UShootInventoryManagerComponent>(TEXT("InventoryManagerComponent"));
	InventoryManagerComponent->SetIsReplicated(true);

	ResourceInventoryComponent = CreateDefaultSubobject<UResourceInventoryComponent>(TEXT("ResourceInventoryComponent"));
	ResourceInventoryComponent->SetIsReplicated(true);

	SkillLoadoutComponent = CreateDefaultSubobject<UShootSkillLoadoutComponent>(TEXT("SkillLoadoutComponent"));
	SkillLoadoutComponent->SetIsReplicated(true);

	RobotCompanionComponent = CreateDefaultSubobject<UShootRobotCompanionComponent>(TEXT("RobotCompanionComponent"));
	RobotCompanionComponent->SetIsReplicated(true);

	SetNetUpdateFrequency(100.f);
	MyTeamID = FGenericTeamId::NoTeam;
	//设置默认性别，后续可从存档加载覆盖
	CharacterGender = ECharacterGender::MALE;

	// 默认男女主技能套件（可在编辑器覆盖，切换时按 KitTag 授予/移除）
	// 技能套件已迁移到 Experience 数据驱动的 AbilitySet(GameMode 授予),不再在此硬编码。
}

void AShootPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);

	DOREPLIFETIME(ThisClass, Level);
	DOREPLIFETIME(ThisClass, XP);
	DOREPLIFETIME(ThisClass, AttributePoints);
	DOREPLIFETIME(ThisClass, SpellPoints);
	DOREPLIFETIME(ThisClass, FemaleAppearanceTags);
	DOREPLIFETIME(ThisClass, MaleAppearanceTags);
	DOREPLIFETIME(ThisClass, CharacterGender);
}

UAbilitySystemComponent* AShootPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AShootPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
			{
				// PlayerState 管理账号级 ResourceInventory/InventoryManager：进入关卡时先还原持久化背包与 QuickBar
				SaveSubsystem->RestorePlayerInventoryState(this);
			}
		}
	}
}

void AShootPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
			{
				// 离开关卡/销毁前保存玩家的 Persistent 物品，并清理 RuntimeOnly 数据
				SaveSubsystem->CapturePlayerInventoryState(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AShootPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		// 标记动态数组、结构体、对象、普通变量
		// #define MARK_PROPERTY_DIRTY_FROM_NAME(ClassName, PropertyName, Object) 
		// 标记静态数组的某个元素（对于静态数组，每一个元素都有独立的ReplIndex， 所以可以只标记其中某一个元素发生了改变）
		// #define MARK_PROPERTY_DIRTY_FROM_NAME_STATIC_ARRAY_INDEX(ClassName, PropertyName, ArrayIndex, Object) 
		// 标记静态数组发生了改变
		// #define MARK_PROPERTY_DIRTY_FROM_NAME_STATIC_ARRAY(ClassName, PropertyName, ArrayIndex, Object) 
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId AShootPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLyraTeamIndexChangedDelegate* AShootPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AShootPlayerState::Reset()
{
	Super::Reset();
}


void AShootPlayerState::AddToXP(int32 InXP)
{
	XP += InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AShootPlayerState::AddToLevel(int32 InLevel)
{
	Level += InLevel;
	OnLevelChangedDelegate.Broadcast(Level, true);
}

void AShootPlayerState::SetXP(int32 InXP)
{
	XP = InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AShootPlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnLevelChangedDelegate.Broadcast(Level, false);
}

void AShootPlayerState::SetAttributePoints(int32 InPoints)
{
	AttributePoints = InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AShootPlayerState::SetSpellPoints(int32 InPoints)
{
	SpellPoints = InPoints;
	OnSpellPointsChangedDelegate.Broadcast(SpellPoints);
}

void AShootPlayerState::AddToAttributePoints(int32 InPoints)
{
	AttributePoints += InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AShootPlayerState::AddToSpellPoints(int32 InPoints)
{
	SpellPoints += InPoints;
	OnSpellPointsChangedDelegate.Broadcast(SpellPoints);
}

void AShootPlayerState::ApplyExperienceAbilitySets()
{
	UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent);
	if (!ShootASC || !HasAuthority())
	{
		return;
	}

	// 进入副本、迟到加入和读档恢复都可能重入。每个来源只撤销自己的句柄，保证幂等且不串源。
	ExperienceGenderAbilityHandles.TakeFromAbilitySystem(ShootASC);
	ExperienceCommonAbilityHandles.TakeFromAbilitySystem(ShootASC);

	const UShootExperienceDefinition* Experience = GetCurrentExperienceDefinition();
	if (!Experience)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyExperienceAbilitySets: no loaded Experience (gender=%d)"),
			static_cast<int32>(CharacterGender));
		return;
	}

	if (Experience->CommonAbilitySet)
	{
		Experience->CommonAbilitySet->GiveToAbilitySystem(ShootASC, &ExperienceCommonAbilityHandles);
	}

	RefreshExperienceGenderAbilitySet();
}

void AShootPlayerState::RefreshExperienceGenderAbilitySet()
{
	UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent);
	if (!ShootASC || !HasAuthority())
	{
		return;
	}

	ExperienceGenderAbilityHandles.TakeFromAbilitySystem(ShootASC);

	const UShootExperienceDefinition* Experience = GetCurrentExperienceDefinition();
	if (!Experience)
	{
		return;
	}

	const bool bIsMale = CharacterGender == ECharacterGender::MALE;
	const UShootAbilitySet* AbilitySet = bIsMale
		? Experience->MaleProtagonistAbilitySet
		: Experience->FemaleProtagonistAbilitySet;
	if (AbilitySet)
	{
		AbilitySet->GiveToAbilitySystem(ShootASC, &ExperienceGenderAbilityHandles);
		UE_LOG(LogTemp, Verbose, TEXT("RefreshExperienceGenderAbilitySet: granted %s (set=%s)"),
			bIsMale ? TEXT("MALE") : TEXT("FEMALE"), *GetNameSafe(AbilitySet));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("RefreshExperienceGenderAbilitySet: no set for gender=%d"),
			static_cast<int32>(CharacterGender));
	}
}

void AShootPlayerState::TakeExperienceAbilitySets()
{
	UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent);
	if (!ShootASC || !HasAuthority())
	{
		return;
	}
	// Match Skill 是 Experience 下更具体的运行时来源，必须先清理，避免技能 Actor/GE 跨地图残留。
	if (SkillLoadoutComponent)
	{
		SkillLoadoutComponent->ClearMatchSkills();
	}

	// 与授予顺序相反，先取回更具体的性别套件，再取回公共套件。
	ExperienceGenderAbilityHandles.TakeFromAbilitySystem(ShootASC);
	ExperienceCommonAbilityHandles.TakeFromAbilitySystem(ShootASC);
}

const UShootExperienceDefinition* AShootPlayerState::GetCurrentExperienceDefinition() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
	if (!GameState)
	{
		return nullptr;
	}
	const UShootExperienceManagerComponent* ExpComponent =
		GameState->FindComponentByClass<UShootExperienceManagerComponent>();
	return ExpComponent ? ExpComponent->GetCurrentExperience() : nullptr;
}

void AShootPlayerState::CommitCurrentGenderLoadoutToSave()
{
	if (!HasAuthority())
	{
		return;
	}

	FCharacterRuntimeSnapshot* CurrentSnapshot = (CharacterGender == ECharacterGender::MALE) ? &MaleSnapshot : &FemaleSnapshot;
	if (!CurrentSnapshot)
	{
		UE_LOG(LogTemp, Error, TEXT("CommitCurrentGenderLoadoutToSave: CurrentSnapshot is null for gender %d"), (int32)CharacterGender);
		return;
	}

	if (!SaveCurrentCharacterSnapshot(*CurrentSnapshot, CharacterGender))
	{
		UE_LOG(LogTemp, Error, TEXT("CommitCurrentGenderLoadoutToSave: failed to save snapshot for gender %d"), (int32)CharacterGender);
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
		{
			SaveSubsystem->CapturePlayerInventoryState(this);
		}
	}
}

bool AShootPlayerState::InitializeRuntimeLoadoutForCurrentCharacter()
{
	if (!HasAuthority())
	{
		return false;
	}

	const FCharacterRuntimeSnapshot* CurrentSnapshot = GetSnapshot(CharacterGender);
	APawn* Pawn = GetPawn();
	UCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UCombatComponent>() : nullptr;
	if (!CurrentSnapshot || !Combat)
	{
		return false;
	}

	// Persistent 快照只提供账号出战配置；Combat 会为每个槽位创建独立 RuntimeOnly 实例。
	Combat->InitializeRuntimeLoadoutFromPersistentSlots(CurrentSnapshot->QuickbarSlots);
	return true;
}

void AShootPlayerState::DumpCharacterSnapshots()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("DumpCharacterSnapshots only valid on authority"));
		return;
	}

	auto LogSnapshot = [](const TCHAR* Label, const FCharacterRuntimeSnapshot& Snap)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Initialized=%d, Inventory=%d, Quickbar=%d, Health=%.1f, Shield=%.1f, Ult=%.1f"),
			Label, Snap.bInitialized ? 1 : 0,
			Snap.InventoryItems.Num(),
			Snap.QuickbarSlots.Num(),
			Snap.HealthSnapshot,
			Snap.ShieldSnapshot,
			Snap.UltimateChargeSnapshot);

		for (int32 SlotIdx = 0; SlotIdx < Snap.QuickbarSlots.Num(); ++SlotIdx)
		{
			const FSavedQuickbarSlot& Slot = Snap.QuickbarSlots[SlotIdx];
			UE_LOG(LogTemp, Log, TEXT("  Slot %d -> ItemInstanceId=%s"), SlotIdx, *Slot.ItemInstanceId.ToString());
		}
	};

	LogSnapshot(TEXT("Male"), MaleSnapshot);
	LogSnapshot(TEXT("Female"), FemaleSnapshot);
}

void AShootPlayerState::SwitchCharacterDebug(int32 GenderAsInt)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchCharacterDebug only valid on authority"));
		return;
	}

	ECharacterGender TargetGender = ECharacterGender::FEMALE;
	if (GenderAsInt == 0)
	{
		TargetGender = ECharacterGender::MALE;
	}
	else if (GenderAsInt == 1)
	{
		TargetGender = ECharacterGender::FEMALE;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchCharacterDebug invalid gender %d (0=Male,1=Female)"), GenderAsInt);
		return;
	}

	SwitchToCharacter(TargetGender);
}

bool AShootPlayerState::CanSwitchCharacter(ECharacterGender TargetGender) const
{
	if (!HasAuthority())
	{
		return false;
	}

	if (TargetGender == CharacterGender)
	{
		return false;
	}

	// 当前开发阶段先不把角色切换写死到 Hub/安全区。
	// 后续需要真正补的是战斗状态、剧情演出状态、特殊任务状态等更细粒度门禁。
	// 等这些状态系统接好后，再把切换限制从“地图类型判断”升级成“真实玩法状态判断”。
	// TODO: 战斗状态检查（需接入 Combat 状态系统）
	// TODO: 特殊剧情/演出/任务状态检查
	return true;
}

bool AShootPlayerState::SwitchToCharacter(ECharacterGender TargetGender)
{
	if (!CanSwitchCharacter(TargetGender))
	{
		return false;
	}

	const ECharacterGender PreviousGender = CharacterGender;
	FCharacterRuntimeSnapshot* CurrentSnapshot = CharacterGender == ECharacterGender::MALE ? &MaleSnapshot : &FemaleSnapshot;
	FCharacterRuntimeSnapshot* TargetSnapshot = TargetGender == ECharacterGender::MALE ? &MaleSnapshot : &FemaleSnapshot;

	if (!CurrentSnapshot || !TargetSnapshot)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning,
	       TEXT("SwitchToCharacter Begin: PlayerState=%s, CurrentGender=%d, TargetGender=%d, CurrentSnapshotInit=%d, TargetSnapshotInit=%d, CurrentTagsNum=%d, TargetTagsNum=%d"),
	       *GetNameSafe(this), static_cast<int32>(CharacterGender), static_cast<int32>(TargetGender),
	       CurrentSnapshot->bInitialized ? 1 : 0, TargetSnapshot->bInitialized ? 1 : 0,
	       CurrentSnapshot->AppearanceTags.Num(), TargetSnapshot->AppearanceTags.Num());

	if (!SaveCurrentCharacterSnapshot(*CurrentSnapshot, CharacterGender))
	{
		UE_LOG(LogTemp, Error, TEXT("SwitchToCharacter: failed to save current snapshot, aborting"));
		return false;
	}

	bool bLoaded = false;
	if (TargetSnapshot->bInitialized)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("SwitchToCharacter LoadExistingSnapshot: TargetGender=%d, Items=%d, QuickbarSlots=%d, TagsNum=%d"),
		       static_cast<int32>(TargetGender), TargetSnapshot->InventoryItems.Num(), TargetSnapshot->QuickbarSlots.Num(),
		       TargetSnapshot->AppearanceTags.Num());
		bLoaded = LoadCharacterFromSnapshot(*TargetSnapshot, TargetGender);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchToCharacter InitNewSnapshot: TargetGender=%d"),
		       static_cast<int32>(TargetGender));
		if (InitializeNewCharacterForGender(TargetGender, *TargetSnapshot))
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("SwitchToCharacter InitDone: TargetGender=%d, TargetSnapshotInit=%d, TagsNum=%d"),
			       static_cast<int32>(TargetGender), TargetSnapshot->bInitialized ? 1 : 0,
			       TargetSnapshot->AppearanceTags.Num());
			bLoaded = LoadCharacterFromSnapshot(*TargetSnapshot, TargetGender);
		}
	}

	if (!bLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("SwitchToCharacter: failed to load/initialize target snapshot"));
		return false;
	}

	SetCharacterGender(TargetGender);

	UE_LOG(LogTemp, Log, TEXT("SwitchToCharacter: %d -> %d"), (int32)PreviousGender, (int32)TargetGender);
	return true;
}

bool AShootPlayerState::SaveCurrentCharacterSnapshot(FCharacterRuntimeSnapshot& OutSnapshot, ECharacterGender SnapshotGender)
{
	OutSnapshot = FCharacterRuntimeSnapshot();

	APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveCurrentCharacterSnapshot: no pawn for PlayerState %s"), *GetName());
		return false;
	}

	if (!InventoryManagerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveCurrentCharacterSnapshot: missing InventoryManagerComponent"));
		return false;
	}

	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveCurrentCharacterSnapshot: missing CombatComponent on pawn %s"), *Pawn->GetName());
		return false;
	}

	OutSnapshot.InventoryItems.Reset();
	for (UShootInventoryItemInstance* Instance : InventoryManagerComponent->GetAllItems())
	{
		if (!Instance)
		{
			continue;
		}
		FRuntimeInventoryItemSnapshot Snap;
		Snap.ItemInstanceId = Instance->GetItemInstanceId();
		Snap.ItemDefinition = Instance->GetItemDef();
		Snap.StackCount = 1;
		Snap.Lifetime = Instance->GetItemLifetime();
		Instance->GetStatTags().ForEachStack(
			[&Snap](const FGameplayTag& Tag, int32 Count)
			{
				if (Tag.IsValid() && Count > 0)
				{
					FSavedTagStack Stack;
					Stack.Tag = Tag;
					Stack.Count = Count;
					Snap.TagStacks.Add(Stack);
				}
			});
		OutSnapshot.InventoryItems.Add(Snap);
	}

	OutSnapshot.QuickbarSlots.Reset();
	Combat->BuildQuickbarSaveData(OutSnapshot.QuickbarSlots);

	OutSnapshot.AppearanceTags = (SnapshotGender == ECharacterGender::MALE) ? MaleAppearanceTags : FemaleAppearanceTags;

	if (UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent))
	{
		if (const UShootAttributeSet* AttrSet = ShootASC->GetSet<UShootAttributeSet>())
		{
			OutSnapshot.HealthSnapshot = AttrSet->GetHealth();
			OutSnapshot.ShieldSnapshot = AttrSet->GetShield();
			OutSnapshot.UltimateChargeSnapshot = AttrSet->GetUltimateCharge();
		}

		SaveActiveEffectSnapshot(ShootASC, OutSnapshot);
	}

	OutSnapshot.bInitialized = true;
	return true;
}

bool AShootPlayerState::LoadCharacterFromSnapshot(const FCharacterRuntimeSnapshot& InSnapshot, ECharacterGender TargetGender)
{
	APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadCharacterFromSnapshot: no pawn for PlayerState %s"), *GetName());
		return false;
	}

	if (!InventoryManagerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadCharacterFromSnapshot: missing InventoryManagerComponent"));
		return false;
	}

	if (UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent))
	{
		ClearActiveEffectsForSwitch(ShootASC);
	}

	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadCharacterFromSnapshot: missing CombatComponent on pawn %s"), *Pawn->GetName());
		return false;
	}

	if (InventoryManagerComponent)
	{
		const TArray<UShootInventoryItemInstance*> ExistingItems = InventoryManagerComponent->GetAllItems();
		for (UShootInventoryItemInstance* Item : ExistingItems)
		{
			InventoryManagerComponent->RemoveItemInstance(Item);
		}
	}

	Combat->ClearRuntimeSlots();
	TArray<FSavedQuickbarSlot> EmptySlots;
	Combat->ApplyQuickbarSaveData(EmptySlots);

	for (const FRuntimeInventoryItemSnapshot& Snap : InSnapshot.InventoryItems)
	{
		if (!Snap.ItemDefinition.IsValid() || !InventoryManagerComponent)
		{
			continue;
		}
		TSubclassOf<UShootInventoryItemDefinition> Def = Snap.ItemDefinition.LoadSynchronous();
		if (!Def)
		{
			continue;
		}

		// 优先恢复实例 Guid：武器类物品通常 StackCount=1，可通过 InitData.DesiredInstanceId 保持 QuickBar 绑定一致。
		if (Snap.StackCount != 1)
		{
			UShootInventoryItemInstance* InventoryItemInstance = InventoryManagerComponent->AddPersistentItem(Def, Snap.StackCount);
			if (InventoryItemInstance)
			{
				FShootGameplayTagStackContainer StatSnapshot;
				for (const FSavedTagStack& Stack : Snap.TagStacks)
				{
					StatSnapshot.AddStack(Stack.Tag, Stack.Count);
				}
				InventoryItemInstance->ApplyStatTagSnapshot(StatSnapshot);
			}
			continue;
		}

		FShootInventoryItemInitData InitData;
		InitData.Lifetime = Snap.Lifetime;
		InitData.DesiredInstanceId = Snap.ItemInstanceId;
		for (const FSavedTagStack& Stack : Snap.TagStacks)
		{
			InitData.InitialStatTags.AddStack(Stack.Tag, Stack.Count);
		}
		InventoryManagerComponent->AddRuntimeWeaponItem(Def, InitData);
	}

	Combat->ApplyQuickbarSaveData(InSnapshot.QuickbarSlots);

	// 通过统一入口写入外观标签，确保服务端本地也会触发标签变化回调。
	ServerSetAppearanceTags(TargetGender, InSnapshot.AppearanceTags);

	if (UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent))
	{
		if (const UShootAttributeSet* ShootAttributeSet = ShootASC->GetSet<UShootAttributeSet>())
		{
			const float CurrentHealth = ShootAttributeSet->GetHealth();
			const float CurrentShield = ShootAttributeSet->GetShield();
			const float CurrentUltimate = ShootAttributeSet->GetUltimateCharge();

			const bool bHasHealth = InSnapshot.HealthSnapshot >= 0.f;
			const bool bHasShield = InSnapshot.ShieldSnapshot >= 0.f;
			const bool bHasUltimate = InSnapshot.UltimateChargeSnapshot >= 0.f;

			const float HealthDelta = bHasHealth ? (InSnapshot.HealthSnapshot - CurrentHealth) : 0.f;
			const float ShieldDelta = bHasShield ? (InSnapshot.ShieldSnapshot - CurrentShield) : 0.f;
			const float UltimateDelta = bHasUltimate ? (InSnapshot.UltimateChargeSnapshot - CurrentUltimate) : 0.f;

			if (!FMath::IsNearlyZero(HealthDelta) || !FMath::IsNearlyZero(ShieldDelta) || !FMath::IsNearlyZero(UltimateDelta))
			{
				FGameplayEffectContextHandle Ctx = ShootASC->MakeEffectContext();
				Ctx.AddSourceObject(this);

				if (FGameplayEffectSpecHandle Spec = ShootASC->MakeOutgoingSpec(UShootEffect_SnapshotRestore::StaticClass(), 1.f, Ctx); Spec.IsValid())
				{
					const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
					if (bHasHealth)
					{
						FShootGameplayTags::SetSetByCallerMagnitude(
							*Spec.Data.Get(), GameplayTags.SetByCaller_HealthSnapshotDelta, HealthDelta);
					}
					if (bHasShield)
					{
						FShootGameplayTags::SetSetByCallerMagnitude(
							*Spec.Data.Get(), GameplayTags.SetByCaller_ShieldSnapshotDelta, ShieldDelta);
					}
					if (bHasUltimate)
					{
						FShootGameplayTags::SetSetByCallerMagnitude(
							*Spec.Data.Get(), GameplayTags.SetByCaller_UltimateChargeSnapshotDelta, UltimateDelta);
					}
					ShootASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}

		RestoreActiveEffectSnapshot(ShootASC, InSnapshot);
	}

	return true;
}

bool AShootPlayerState::InitializeNewCharacterForGender(ECharacterGender TargetGender, FCharacterRuntimeSnapshot& OutSnapshot)
{
	OutSnapshot = FCharacterRuntimeSnapshot();
	OutSnapshot.AppearanceTags = (TargetGender == ECharacterGender::MALE) ? MaleAppearanceTags : FemaleAppearanceTags;

	APawn* Pawn = GetPawn();
	if (!Pawn || !InventoryManagerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeNewCharacterForGender: missing pawn or inventory"));
		return false;
	}

	// 默认装配：优先使用存档中的性别专属 QuickBar/外观；若存档不存在则沿用当前持久化背包与 QuickBar
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
		{
			UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame();
			if (!SaveGame)
			{
				SaveGame = SaveSubsystem->LoadPlayerSaveGame(SaveSubsystem->GetCurrentSlotIndex());
			}

			if (SaveGame)
			{
				FProtagonistSaveData& ProtagonistData = (TargetGender == ECharacterGender::MALE)
					                                         ? SaveGame->MaleProtagonist
					                                         : SaveGame->FemaleProtagonist;

				if (UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>())
				{
					Combat->ClearRuntimeSlots();
					if (ProtagonistData.QuickbarSlots.Num() > 0)
					{
						Combat->ApplyQuickbarSaveData(ProtagonistData.QuickbarSlots);
					}
					else
					{
						BuildDefaultQuickbarFromInventory(Combat, InventoryManagerComponent);
					}
				}

				if (ProtagonistData.AppearanceTags.Num() > 0)
				{
					ServerSetAppearanceTags(TargetGender, ProtagonistData.AppearanceTags);
					OutSnapshot.AppearanceTags = ProtagonistData.AppearanceTags;
				}
			}
		}
	}

	if (!SaveCurrentCharacterSnapshot(OutSnapshot, TargetGender))
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeNewCharacterForGender: failed to save fresh snapshot"));
		return false;
	}

	return true;
}

const FCharacterRuntimeSnapshot* AShootPlayerState::GetSnapshot(ECharacterGender Gender) const
{
	return (Gender == ECharacterGender::MALE) ? &MaleSnapshot : &FemaleSnapshot;
}

void AShootPlayerState::SaveActiveEffectSnapshot(UShootAbilitySystemComponent* ShootASC, FCharacterRuntimeSnapshot& OutSnapshot) const
{
	if (!ShootASC)
	{
		return;
	}

	OutSnapshot.ActiveEffectSnapshots.Reset();

	const FGameplayEffectQuery Query;
	const TArray<FActiveGameplayEffectHandle> Handles = ShootASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* ActiveEffect = ShootASC->GetActiveGameplayEffect(Handle);
		if (!ActiveEffect || !ActiveEffect->Spec.Def)
		{
			continue;
		}

		const UGameplayEffect* EffectCDO = ActiveEffect->Spec.Def;
		if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant)
		{
			continue;
		}

		const float RemainingTime = ActiveEffect->GetTimeRemaining(ShootASC->GetWorld()->GetTimeSeconds());
		if (RemainingTime <= 0.f && EffectCDO->DurationPolicy == EGameplayEffectDurationType::HasDuration)
		{
			continue;
		}

		FActiveGameplayEffectSnapshot Snapshot;
		Snapshot.EffectClass = EffectCDO->GetClass();
		Snapshot.RemainingTime = RemainingTime;
		Snapshot.EffectLevel = ActiveEffect->Spec.GetLevel();
		Snapshot.StackCount = ActiveEffect->Spec.GetStackCount();
		// 保存 SetByCaller 标签数值，避免切换角色后动态 Buff 丢失
		for (const TPair<FGameplayTag, float>& Pair : ActiveEffect->Spec.SetByCallerTagMagnitudes)
		{
			if (!Pair.Key.IsValid())
			{
				continue;
			}

			FSavedSetByCallerTagMagnitude Saved;
			Saved.Tag = Pair.Key;
			Saved.Magnitude = Pair.Value;
			Snapshot.SetByCallerTagMagnitudes.Add(Saved);
		}
		OutSnapshot.ActiveEffectSnapshots.Add(Snapshot);
	}
}

void AShootPlayerState::RestoreActiveEffectSnapshot(UShootAbilitySystemComponent* ShootASC, const FCharacterRuntimeSnapshot& InSnapshot) const
{
	if (!ShootASC)
	{
		return;
	}

	for (const FActiveGameplayEffectSnapshot& Snapshot : InSnapshot.ActiveEffectSnapshots)
	{
		if (!Snapshot.EffectClass || Snapshot.RemainingTime <= 0.f)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = ShootASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = ShootASC->MakeOutgoingSpec(Snapshot.EffectClass, Snapshot.EffectLevel, ContextHandle);
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		// 使用剩余时间恢复持续效果，保证切换后冷却与状态一致
		SpecHandle.Data->SetDuration(Snapshot.RemainingTime, true);
		SpecHandle.Data->SetStackCount(FMath::Max(1, Snapshot.StackCount));
		// 还原 SetByCaller 动态数值（治疗/伤害/护盾等）
		for (const FSavedSetByCallerTagMagnitude& Saved : Snapshot.SetByCallerTagMagnitudes)
		{
			if (!Saved.Tag.IsValid())
			{
				continue;
			}

			FShootGameplayTags::SetSetByCallerMagnitude(*SpecHandle.Data.Get(), Saved.Tag, Saved.Magnitude);
		}
		ShootASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AShootPlayerState::ClearActiveEffectsForSwitch(UShootAbilitySystemComponent* ShootASC) const
{
	if (!ShootASC)
	{
		return;
	}

	const FGameplayEffectQuery Query;
	const TArray<FActiveGameplayEffectHandle> Handles = ShootASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			ShootASC->RemoveActiveGameplayEffect(Handle);
		}
	}
}

bool AShootPlayerState::BuildDefaultQuickbarFromInventory(UCombatComponent* Combat, UShootInventoryManagerComponent* InventoryManager) const
{
	if (!Combat || !InventoryManager)
	{
		return false;
	}

	const TArray<AActor*> Slots = Combat->GetSlots();
	const int32 SlotCount = Slots.Num();
	if (SlotCount <= 0)
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		Combat->Server_ClearQuickbarSlot(SlotIndex);
	}

	TArray<UShootInventoryItemInstance*> Candidates;
	for (UShootInventoryItemInstance* ItemInstance : InventoryManager->GetAllItems())
	{
		if (!ItemInstance || ItemInstance->GetItemLifetime() != EShootItemLifetime::Persistent)
		{
			continue;
		}

		if (ItemInstance->FindFragmentByClass<UShootInventoryFragment_EquippableItem>())
		{
			Candidates.Add(ItemInstance);
		}
	}

	int32 AssignedCount = 0;
	for (UShootInventoryItemInstance* ItemInstance : Candidates)
	{
		if (AssignedCount >= SlotCount)
		{
			break;
		}

		TArray<FQuickbarSlotData> SlotData;
		Combat->GetQuickbarSlotsData(SlotData);

		int32 FreeSlot = INDEX_NONE;
		for (int32 SlotIndex = 0; SlotIndex < SlotData.Num(); ++SlotIndex)
		{
			if (!SlotData[SlotIndex].ItemInstanceId.IsValid())
			{
				FreeSlot = SlotIndex;
				break;
			}
		}

		if (FreeSlot == INDEX_NONE)
		{
			break;
		}

		Combat->Server_SetQuickbarSlot(FreeSlot, ItemInstance);
		AssignedCount++;
	}

	return AssignedCount > 0;
}

const FGameplayTagContainer& AShootPlayerState::GetAppearanceTags(ECharacterGender InGender) const
{
	switch (InGender)
	{
	case ECharacterGender::MALE:
		return MaleAppearanceTags;
	case ECharacterGender::FEMALE:
		return FemaleAppearanceTags;
	default:
		//C++ 不允许把一个非 const 引用（FGameplayTagContainer&）绑定到临时对象
		return FemaleAppearanceTags;
	}
}

void AShootPlayerState::SetCharacterGender(ECharacterGender InGender)
{
	if (CharacterGender == InGender)
	{
		return;
	}

	const ECharacterGender LastCharacterGender = CharacterGender;
	CharacterGender = InGender;
	// 服务端不会自动触发 OnRep，这里手动调用一次，确保本地监听链路和客户端行为一致。
	OnRep_CharacterGender(LastCharacterGender);

	// SaveGame 恢复可能早于 Experience 加载，此时函数安全返回；加载完成后 Manager 会统一补授予。
	// 运行中的角色切换则只刷新性别来源，公共 Experience 能力保持不动。
	RefreshExperienceGenderAbilitySet();
}

void AShootPlayerState::ServerSetAppearanceTags_Implementation(ECharacterGender InGender,
                                                               const FGameplayTagContainer& NewTags)
{
	// 调试辅助：将性别与标签容器转成可读字符串，便于直接从日志核对 RPC 入参和最终写入值。
	const auto GenderToString = [](ECharacterGender InGenderValue) -> const TCHAR*
	{
		switch (InGenderValue)
		{
		case ECharacterGender::MALE:
			return TEXT("MALE");
		case ECharacterGender::FEMALE:
			return TEXT("FEMALE");
		default:
			return TEXT("UNKNOWN");
		}
	};

	const auto TagsToDebugString = [](const FGameplayTagContainer& InTags) -> FString
	{
		TArray<FGameplayTag> TagArray;
		InTags.GetGameplayTagArray(TagArray);

		TArray<FString> TagNames;
		TagNames.Reserve(TagArray.Num());
		for (const FGameplayTag& Tag : TagArray)
		{
			TagNames.Add(Tag.ToString());
		}

		return TagNames.Num() > 0 ? FString::Join(TagNames, TEXT(", ")) : TEXT("None");
	};

	UE_LOG(LogTemp, Warning,
	       TEXT("ServerSetAppearanceTags RPC: PlayerState=%s, PlayerName=%s, Gender=%s(%d), NewTagsNum=%d, NewTags=[%s]"),
	       *GetNameSafe(this), *GetPlayerName(), GenderToString(InGender), static_cast<int32>(InGender), NewTags.Num(),
	       *TagsToDebugString(NewTags));

	switch (InGender)
	{
	case ECharacterGender::MALE:
		MaleAppearanceTags = NewTags;
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerSetAppearanceTags Apply(MALE): SavedTagsNum=%d, SavedTags=[%s]"),
		       MaleAppearanceTags.Num(), *TagsToDebugString(MaleAppearanceTags));
	// 手动触发复制回调（服务端不会自动触发OnRep）
		OnRep_MaleAppearanceTags();
		break;
	case ECharacterGender::FEMALE:
		FemaleAppearanceTags = NewTags;
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerSetAppearanceTags Apply(FEMALE): SavedTagsNum=%d, SavedTags=[%s]"),
		       FemaleAppearanceTags.Num(), *TagsToDebugString(FemaleAppearanceTags));
	// 手动触发复制回调（服务端不会自动触发OnRep）
		OnRep_FemaleAppearanceTags();
		break;
	default:
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerSetAppearanceTags Apply(UNKNOWN): Gender=%d, TagsNum=%d, Tags=[%s]"),
		       static_cast<int32>(InGender), NewTags.Num(), *TagsToDebugString(NewTags));
		break;
	}
}

const UShootInventoryFragment_WardrobeItem* AShootPlayerState::ResolveOwnedPersistentWardrobeFragment(
	FGuid ItemInstanceId,
	UShootInventoryItemInstance*& OutItemInstance)
{
	OutItemInstance = nullptr;
	if (!InventoryManagerComponent || !ItemInstanceId.IsValid())
	{
		return nullptr;
	}

	UShootInventoryItemInstance* ItemInstance = InventoryManagerComponent->FindItemByInstanceId(ItemInstanceId);
	if (!ItemInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolveOwnedPersistentWardrobeFragment: item not found, Id=%s"), *ItemInstanceId.ToString());
		return nullptr;
	}

	// 衣柜只允许装备玩家已经拥有的 Persistent 服装。
	// RuntimeOnly 是副本临时物品，不能写入账号衣柜外观存档。
	if (ItemInstance->GetItemLifetime() != EShootItemLifetime::Persistent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolveOwnedPersistentWardrobeFragment: ignore non-persistent item %s"), *GetNameSafe(ItemInstance));
		return nullptr;
	}

	const UShootInventoryFragment_WardrobeItem* WardrobeFragment =
		ItemInstance->FindFragmentByClass<UShootInventoryFragment_WardrobeItem>();
	if (!WardrobeFragment)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolveOwnedPersistentWardrobeFragment: item has no Wardrobe fragment %s"), *GetNameSafe(ItemInstance));
		return nullptr;
	}

	OutItemInstance = ItemInstance;
	return WardrobeFragment;
}

void AShootPlayerState::ServerEquipWardrobeItem_Implementation(FGuid ItemInstanceId)
{
	UShootInventoryItemInstance* ItemInstance = nullptr;
	const UShootInventoryFragment_WardrobeItem* WardrobeFragment =
		ResolveOwnedPersistentWardrobeFragment(ItemInstanceId, ItemInstance);
	if (!WardrobeFragment)
	{
		return;
	}

	if (!WardrobeFragment->CanApplyToGender(CharacterGender))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerEquipWardrobeItem: item %s cannot apply to current gender %d"),
		       *GetNameSafe(ItemInstance), static_cast<int32>(CharacterGender));
		return;
	}

	const FGameplayTagContainer NextTags = WardrobeFragment->BuildTagsAfterEquip(
		GetAppearanceTags(CharacterGender),
		CharacterGender);
	ServerSetAppearanceTags(CharacterGender, NextTags);
	PersistCurrentWardrobeState();
}

void AShootPlayerState::ServerPreviewWardrobeItem_Implementation(FGuid ItemInstanceId)
{
	UShootInventoryItemInstance* ItemInstance = nullptr;
	const UShootInventoryFragment_WardrobeItem* WardrobeFragment =
		ResolveOwnedPersistentWardrobeFragment(ItemInstanceId, ItemInstance);
	if (!WardrobeFragment)
	{
		return;
	}

	if (!WardrobeFragment->CanApplyToGender(CharacterGender))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerPreviewWardrobeItem: item %s cannot apply to current gender %d"),
		       *GetNameSafe(ItemInstance), static_cast<int32>(CharacterGender));
		return;
	}

	// 试穿只更新当前 PlayerState 外观标签，让 Mutable 组件立即刷新预览；
	// 不调用 PersistCurrentWardrobeState，避免玩家误点试穿就污染存档。
	const FGameplayTagContainer NextTags = WardrobeFragment->BuildTagsAfterEquip(
		GetAppearanceTags(CharacterGender),
		CharacterGender);
	ServerSetAppearanceTags(CharacterGender, NextTags);
}

void AShootPlayerState::ServerUnequipWardrobeItem_Implementation(FGuid ItemInstanceId)
{
	UShootInventoryItemInstance* ItemInstance = nullptr;
	const UShootInventoryFragment_WardrobeItem* WardrobeFragment =
		ResolveOwnedPersistentWardrobeFragment(ItemInstanceId, ItemInstance);
	if (!WardrobeFragment)
	{
		return;
	}

	if (!WardrobeFragment->CanApplyToGender(CharacterGender))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("ServerUnequipWardrobeItem: item %s cannot apply to current gender %d"),
		       *GetNameSafe(ItemInstance), static_cast<int32>(CharacterGender));
		return;
	}

	const FGameplayTagContainer NextTags = WardrobeFragment->BuildTagsAfterUnequip(
		GetAppearanceTags(CharacterGender),
		CharacterGender);
	ServerSetAppearanceTags(CharacterGender, NextTags);
	PersistCurrentWardrobeState();
}

void AShootPlayerState::ServerSaveCurrentWardrobe_Implementation()
{
	PersistCurrentWardrobeState();
}

void AShootPlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChangedDelegate.Broadcast(Level, true);
}

void AShootPlayerState::OnRep_XP(int32 OldXP)
{
	OnXPChangedDelegate.Broadcast(XP);
}

void AShootPlayerState::OnRep_AttributePoints(int32 OldAttributePoints)
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AShootPlayerState::OnRep_SpellPoints(int32 OldSpellPoints)
{
	OnSpellPointsChangedDelegate.Broadcast(SpellPoints);
}

void AShootPlayerState::OnRep_FemaleAppearanceTags()
{
	OnAppearanceTagsChanged.Broadcast(ECharacterGender::FEMALE, FemaleAppearanceTags);
}

void AShootPlayerState::OnRep_MaleAppearanceTags()
{
	OnAppearanceTagsChanged.Broadcast(ECharacterGender::MALE, MaleAppearanceTags);
}

void AShootPlayerState::OnRep_CharacterGender(ECharacterGender LastCharacterGender)
{
	// 性别切换后主动广播当前性别外观标签，驱动外观组件立刻刷新到对应主角。
	OnAppearanceTagsChanged.Broadcast(CharacterGender, GetAppearanceTags(CharacterGender));
}

void AShootPlayerState::PersistCurrentWardrobeState()
{
	if (!HasAuthority())
	{
		return;
	}

	FCharacterRuntimeSnapshot* CurrentSnapshot = CharacterGender == ECharacterGender::MALE ? &MaleSnapshot : &FemaleSnapshot;
	if (CurrentSnapshot && CurrentSnapshot->bInitialized)
	{
		// CapturePlayerInventoryState 会优先使用已初始化快照，因此衣柜装备后必须同步更新当前快照里的外观标签。
		CurrentSnapshot->AppearanceTags = GetAppearanceTags(CharacterGender);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>())
		{
			SaveSubsystem->CapturePlayerInventoryState(this);
			if (UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame())
			{
				SaveSubsystem->AsyncPlayerSaveGame(SaveGame, SaveSubsystem->GetCurrentSlotIndex());
			}
		}
	}
}

void AShootPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}
