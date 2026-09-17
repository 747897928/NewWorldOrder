// Copyright ZhaoYiJie


#include "System/SaveGameSubsystem.h"

#include "Character/CombatComponent.h"
#include "Engine/Engine.h"
#include "GameModes/ShootGameModeBase.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Inventory/ResourceInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShootPlayerState.h"
#include "System/SaveGameSlotInfo.h"
#include "System/ShootSaveGame.h"


USaveGameSubsystem::USaveGameSubsystem()
{
	SaveInfoDataDelegate.BindUObject(this, &ThisClass::OnSaveInfoDataFinish);
}

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USaveGameSubsystem::Deinitialize()
{
	SaveInfoDataDelegate.Unbind();
	Super::Deinitialize();
}

FString USaveGameSubsystem::GetPIESafeSlotName(const FString& BaseSlotName) const
{
#if WITH_EDITOR
	int32 PIESessionID = INDEX_NONE;
	if (const UWorld* World = GetWorld())
	{
		if (GEngine)
		{
			if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
			{
				if (WorldContext->WorldType == EWorldType::PIE && WorldContext->PIEInstance != INDEX_NONE)
				{
					PIESessionID = WorldContext->PIEInstance;
				}
			}
		}
	}

	if (PIESessionID == INDEX_NONE)
	{
		PIESessionID = UE::GetPlayInEditorID();
	}

	// 多人 PIE 里 listen server 和 client 各自有 WorldContext。
	// 槽位必须按当前 WorldContext::PIEInstance 隔离，否则两个窗口会读写同一份测试存档。
	return FString::Printf(TEXT("%s_PIE_%d"), *BaseSlotName, PIESessionID);
#else
		return BaseSlotName;
#endif
}

void USaveGameSubsystem::AsyncLoadPlayerSaveGame(const int32 SlotIndex)
{
	GetOrCreateSaveInfoData();
	FString SlotName = GetSlotNameByIndex(SlotIndex);
	// 只在PIE编辑器模式下执行特殊处理
#if WITH_EDITOR
	if (GetWorld() && GetWorld()->IsPlayInEditor())
	{
		// PIE 仍使用带 _PIE_<id> 的隔离槽位；这里只在槽位不存在时创建新存档。
		// 不能因为 CurrentSaveGame 为空就覆盖文件，否则每次新 PIE 会话都会把上次捡到的 Persistent 服装清掉。
		if (!UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
		{
			UE_LOG(LogTemp, Log, TEXT("Creating new save for PIE: Slot %d (%s)"), SlotIndex, *SlotName);

			// 创建新存档对象
			CurrentSaveGame = Cast<UShootSaveGame>(
				UGameplayStatics::CreateSaveGameObject(UShootSaveGame::StaticClass()));
			//这里我们用同步操作
			UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, SlotIndex);
		}
	}
#endif
	LoadGameDataStart.Broadcast(SlotName, SlotIndex);
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	LoadedDelegate.BindLambda(
		[this](const FString& InSlotName, const int32 InSlotIndex, USaveGame* Save)
		{
			if (Save)
			{
				if (UShootSaveGame* LoadedSave = Cast<UShootSaveGame>(Save))
				{
					this->CurrentSaveGame = LoadedSave;
					this->CurrentSlotIndex = InSlotIndex;
					this->UpdateSlotInfoData(InSlotName, InSlotIndex);
					FString SaveInfoSlotName = GetPIESafeSlotName(SlotInfoName.ToString());
					UGameplayStatics::AsyncSaveGameToSlot(this->SaveInfoData, SaveInfoSlotName, 0,
					                                      this->SaveInfoDataDelegate);
					this->AsyncLoadGameDataFinish.Broadcast(true, InSlotName, InSlotIndex);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Loaded Save is not UShootSaveGame"));
					this->AsyncLoadGameDataFinish.Broadcast(false, InSlotName, InSlotIndex);
				}
			}
			else
			{
				this->AsyncLoadGameDataFinish.Broadcast(false, InSlotName, InSlotIndex);
			}
		});

	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, SlotIndex, LoadedDelegate);
}

UShootSaveGame* USaveGameSubsystem::LoadPlayerSaveGame(int32 SlotIndex)
{
	GetOrCreateSaveInfoData();
	FString SlotName = GetSlotNameByIndex(SlotIndex);
	// 只在PIE编辑器模式下执行特殊处理
#if WITH_EDITOR
	if (GetWorld() && GetWorld()->IsPlayInEditor())
	{
		// PIE 仍使用带 _PIE_<id> 的隔离槽位；这里只在槽位不存在时创建新存档。
		// 不能因为 CurrentSaveGame 为空就覆盖文件，否则同步加载也会丢掉上一轮调试得到的服装。
		if (!UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
		{
			UE_LOG(LogTemp, Log, TEXT("Creating new save for PIE: Slot %d (%s)"), SlotIndex, *SlotName);

			// 创建新存档对象
			CurrentSaveGame = Cast<UShootSaveGame>(
				UGameplayStatics::CreateSaveGameObject(UShootSaveGame::StaticClass()));
			//这里我们用同步操作
			UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, SlotIndex);
		}
	}
#endif
	USaveGame* Save = UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex);
	if (UShootSaveGame* LoadedSave = Cast<UShootSaveGame>(Save))
	{
		this->CurrentSaveGame = LoadedSave;
		this->CurrentSlotIndex = SlotIndex;
		this->UpdateSlotInfoData(SlotName, SlotIndex);
		FString SaveInfoSlotName = GetPIESafeSlotName(SlotInfoName.ToString());
		UGameplayStatics::SaveGameToSlot(this->SaveInfoData, SaveInfoSlotName, 0);
		return LoadedSave;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Loaded Save is not UShootSaveGame"));
		return nullptr;
	}
}

void USaveGameSubsystem::AsyncPlayerSaveGame(UShootSaveGame* SaveGameObject, const int32 SlotIndex)
{
	if (SaveGameObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveGameObject == nullptr"));
		return;
	}
	GetOrCreateSaveInfoData();
	FString SlotName = GetSlotNameByIndex(SlotIndex);
	SaveGameDataStart.Broadcast(SlotName, SlotIndex);

	this->CurrentSaveGame = SaveGameObject;
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	// 或者绑定一个lambda表达式到委托
	SavedDelegate.BindLambda(
		[this](const FString& InSlotName, const int32 InSlotIndex, bool bWasSuccessful)
		{
			if (bWasSuccessful)
			{
				this->CurrentSlotIndex = InSlotIndex;
				this->UpdateSlotInfoData(InSlotName, InSlotIndex);
				FString SaveInfoSlotName = GetPIESafeSlotName(SlotInfoName.ToString());
				UGameplayStatics::AsyncSaveGameToSlot(this->SaveInfoData, SaveInfoSlotName, 0,
				                                      this->SaveInfoDataDelegate);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to save game to slot: %s"), *InSlotName);
				this->AsyncSaveGameDataFinish.Broadcast(false, InSlotName, InSlotIndex);
			}
		});

	UGameplayStatics::AsyncSaveGameToSlot(SaveGameObject, SlotName, SlotIndex, SavedDelegate);
}

void USaveGameSubsystem::UpdateSlotInfoData(const FString& SlotName, const int32 SlotIndex)
{
	if (SaveInfoData == nullptr)
	{
		return;
	}
	FString CurrentTime = GetCurrentTime();
	FSaveSlotItem* SaveSlotItem = SaveInfoData->SaveSlotInfoMap.Find(SlotName);
	if (SaveSlotItem)
	{
		SaveSlotItem->SlotSaveTime = CurrentTime;
		SaveInfoData->SaveSlotInfoMap.Emplace(SlotName, *SaveSlotItem);
	}
	else
	{
		FSaveSlotItem SlotItem;
		SlotItem.SlotName = SlotName;
		SlotItem.SlotIndex = SlotIndex;
		SlotItem.SaveSlotImage.Reset(); // 可选缩略图默认清空，避免保存运行时资源引用
		SlotItem.SlotSaveTime = CurrentTime;
		SaveInfoData->SaveSlotInfoMap.Add(SlotName, SlotItem);
	}
	SaveInfoData->LastSlotIndex = SlotIndex;
}

bool USaveGameSubsystem::DeleteSaveGame(const int32 SlotIndex)
{
	GetOrCreateSaveInfoData();
	FString SlotName = GetSlotNameByIndex(SlotIndex);
	bool bDeleteGameInSlot = UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
	SaveInfoData->SaveSlotInfoMap.Remove(SlotName);
	UGameplayStatics::SaveGameToSlot(SaveInfoData, SlotInfoName.ToString(), 0);
	//当删除掉当前槽时，再次load会出现控指针异常，所以需要将当前槽的索引重置
	/*if (CurrentSlotIndex == SlotIndex)
	{
		CurrentSlotIndex = 0;
		SaveGameData = nullptr;
		ContinueButtonHide.Broadcast();
	}*/
	//现在删除游戏存档就立马隐藏继续游戏的按钮
	ContinueButtonHide.Broadcast();
	return bDeleteGameInSlot;
}

FString USaveGameSubsystem::GetSlotNameByIndex(int32 InSlotIndex) const
{
	FString BaseName = FString::Printf(TEXT("%s%d"), *SlotNamePrefix.ToString(), InSlotIndex);
	return GetPIESafeSlotName(BaseName);
}

FString USaveGameSubsystem::GetCurrentTime() const
{
	FDateTime Now = FDateTime::Now();
	FString CurrentTimeString = Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
	return CurrentTimeString;
}

UShootSaveGame* USaveGameSubsystem::EnsureCurrentSaveGame()
{
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = LoadPlayerSaveGame(CurrentSlotIndex);
	}
	return CurrentSaveGame;
}

void USaveGameSubsystem::OnSaveInfoDataFinish(const FString& InSlotName, const int32 InSlotIndex, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to Save info data to slot, save data name = : %s"), *InSlotName);
	}
	AsyncSaveGameDataFinish.Broadcast(bWasSuccessful, InSlotName, InSlotIndex);
}

USaveGameSlotInfo* USaveGameSubsystem::GetOrCreateSaveInfoData()
{
	// 如果 SaveInfoData 不为空，直接返回
	if (SaveInfoData != nullptr)
	{
		return SaveInfoData;
	}

	// 获取安全的存档信息槽位名称
	FString SaveInfoSlotName = GetPIESafeSlotName(SlotInfoName.ToString());
	int32 SaveInfoSlotIndex = 0;

	// 检查存档是否存在
	if (UGameplayStatics::DoesSaveGameExist(SaveInfoSlotName, SaveInfoSlotIndex))
	{
		// 加载现有存档
		USaveGame* SaveGame = UGameplayStatics::LoadGameFromSlot(SaveInfoSlotName, SaveInfoSlotIndex);

		// 验证类型转换
		if (USaveGameSlotInfo* LoadedInfo = Cast<USaveGameSlotInfo>(SaveGame))
		{
			SaveInfoData = LoadedInfo;
			UE_LOG(LogTemp, Log, TEXT("Loaded existing save info data from slot: %s"), *SaveInfoSlotName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Corrupted save info data! Creating new one."));
			CreateNewSaveInfoData(SaveInfoSlotName, SaveInfoSlotIndex);
		}
	}
	else
	{
		// 创建新的存档信息
		CreateNewSaveInfoData(SaveInfoSlotName, SaveInfoSlotIndex);
	}

	return SaveInfoData;
}

// 辅助函数：创建新的存档信息数据
void USaveGameSubsystem::CreateNewSaveInfoData(const FString& SlotName, int32 SlotIndex)
{
	USaveGame* NewSaveGame = UGameplayStatics::CreateSaveGameObject(USaveGameSlotInfo::StaticClass());
	SaveInfoData = Cast<USaveGameSlotInfo>(NewSaveGame);

	if (SaveInfoData)
	{
		SaveInfoData->LastSlotIndex = 0;
		UGameplayStatics::SaveGameToSlot(SaveInfoData, SlotName, SlotIndex);
		UE_LOG(LogTemp, Log, TEXT("Created new save info data for slot: %s"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create new save info data!"));
	}
}

void USaveGameSubsystem::CapturePlayerInventoryState(AShootPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	UShootSaveGame* SaveGame = EnsureCurrentSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("CapturePlayerInventoryState: no save game available"));
		return;
	}

	if (UShootInventoryManagerComponent* InventoryManager = PlayerState->GetInventoryManagerComponent())
	{
		InventoryManager->BuildPersistentItemsSaveData(SaveGame->SavedInventoryItems);
	}

	if (UResourceInventoryComponent* ResourceInventory = PlayerState->GetResourceInventoryComponent())
	{
		ResourceInventory->BuildResourceSaveData(SaveGame->SavedResources);
	}

	APawn* Pawn = PlayerState->GetPawn();
	if (!Pawn)
	{
		return;
	}

	if (UCombatComponent* CombatComponent = Pawn->FindComponentByClass<UCombatComponent>())
	{
		TArray<FSavedQuickbarSlot> TempSlots;
		CombatComponent->BuildQuickbarSaveData(TempSlots);

		const ECharacterGender CurrentGender = PlayerState->GetCharacterGender();
		FProtagonistSaveData& TargetData = (CurrentGender == ECharacterGender::MALE)
			                                   ? SaveGame->MaleProtagonist
			                                   : SaveGame->FemaleProtagonist;
		TargetData.QuickbarSlots = TempSlots;
		TargetData.AppearanceTags = PlayerState->GetAppearanceTags(CurrentGender);

		// 若快照已初始化，则以快照为权威数据覆盖
		if (const FCharacterRuntimeSnapshot* Snapshot = PlayerState->GetSnapshot(CurrentGender))
		{
			if (Snapshot->bInitialized)
			{
				TargetData.QuickbarSlots = Snapshot->QuickbarSlots;
				TargetData.AppearanceTags = Snapshot->AppearanceTags;
			}
		}
	}

	if (PlayerState->CanPersistLastActiveGender())
	{
		SaveGame->LastActiveGender = PlayerState->GetCharacterGender();
	}
}

void USaveGameSubsystem::RestorePlayerInventoryState(AShootPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	UShootSaveGame* SaveGame = EnsureCurrentSaveGame();

	// 修复(2026-08-23): 无存档时旧流程直接 return, Experience AbilitySet 永不授予，
	// 导致手雷(UShootGA_ThrowGrenade)等男主/女主技能套件从未授予, 按 T 无反应。
	// 现在无论有无存档都确定性别并授予默认套件; 存档数据恢复仅在存在时执行。
	ECharacterGender InitialGender = SaveGame ? SaveGame->LastActiveGender : ECharacterGender::MALE;
	if (const AShootGameModeBase* ShootGameMode =
		PlayerState->GetWorld() ? PlayerState->GetWorld()->GetAuthGameMode<AShootGameModeBase>() : nullptr)
	{
		// PlayerState::BeginPlay 的恢复事务先解析本地图规则，再一次性恢复外观与能力套件。
		// Character::PossessedBy 随后读取的已经是最终状态，Mutable 不需要下一帧再次切换 COI。
		InitialGender = ShootGameMode->ResolveInitialCharacterGender(PlayerState, InitialGender);
		PlayerState->SetCanPersistLastActiveGender(
			ShootGameMode->ShouldPersistLastActiveGender(PlayerState));
	}
	else
	{
		PlayerState->SetCanPersistLastActiveGender(true);
	}
	PlayerState->SetCharacterGender(InitialGender);

	if (SaveGame)
	{
		if (UShootInventoryManagerComponent* InventoryManager = PlayerState->GetInventoryManagerComponent())
		{
			InventoryManager->ApplyPersistentItemsSaveData(SaveGame->SavedInventoryItems);
		}

		if (UResourceInventoryComponent* ResourceInventory = PlayerState->GetResourceInventoryComponent())
		{
			ResourceInventory->ApplyResourceSaveData(SaveGame->SavedResources);
		}

		if (APawn* Pawn = PlayerState->GetPawn())
		{
			if (UCombatComponent* CombatComponent = Pawn->FindComponentByClass<UCombatComponent>())
			{
				CombatComponent->ClearRuntimeSlots();

				const ECharacterGender CurrentGender = PlayerState->GetCharacterGender();
				const FProtagonistSaveData& ProtagonistData = (CurrentGender == ECharacterGender::MALE)
					                                              ? SaveGame->MaleProtagonist
					                                              : SaveGame->FemaleProtagonist;
				CombatComponent->ApplyQuickbarSaveData(ProtagonistData.QuickbarSlots);
			}
		}

		// 恢复外观标签
		PlayerState->ServerSetAppearanceTags(ECharacterGender::MALE, SaveGame->MaleProtagonist.AppearanceTags);
		PlayerState->ServerSetAppearanceTags(ECharacterGender::FEMALE, SaveGame->FemaleProtagonist.AppearanceTags);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RestorePlayerInventoryState: no save game; granted default gender ability kit only"));
	}

	// 恢复完角色状态后，套用当前性别技能套件
	if (PlayerState->HasAuthority())
	{
		PlayerState->ApplyExperienceAbilitySets();
	}
}
