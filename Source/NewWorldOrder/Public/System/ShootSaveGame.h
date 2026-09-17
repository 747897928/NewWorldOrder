// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "Character/CharacterGender.h"
#include "Inventory/ResourceInventoryComponent.h"
#include "Inventory/SavedInventoryTypes.h"
#include "ShootSaveGame.generated.h"

class UGameplayAbility;

USTRUCT()
struct FProtagonistSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSavedQuickbarSlot> QuickbarSlots;

	UPROPERTY()
	FGameplayTagContainer AppearanceTags;
};

/**
 * 存档数据：账号级共享仓库 + 双主角各自的 QuickBar/外观快照
 */
UCLASS()
class NEWWORLDORDER_API UShootSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	
	UShootSaveGame();

	// Basic player info
	UPROPERTY(EditAnywhere, Category="SaveData")
	FString PlayerName = FString("Default Name");

	UPROPERTY()
	bool bFirstTimeLoadIn = true;

	// 2026-08-23 架构调整：属性/等级/技能不再写入存档(副本数据, 由 Experience/CharacterClassInfo 授予)。
	// 存档仅保留账号级战利品: Persistent 物品/资源/双主角外观与 Quickbar。

	// 账号级共享仓库（Persistent 物品）
	UPROPERTY()
	TArray<FSavedInventoryItem> SavedInventoryItems;

	// 账号级资源仓库（材料/货币/徽章/设计图）
	UPROPERTY()
	TArray<FResourceEntry> SavedResources;

	// 双主角独立的 QuickBar/外观
	UPROPERTY()
	FProtagonistSaveData MaleProtagonist;

	UPROPERTY()
	FProtagonistSaveData FemaleProtagonist;

	// 上次使用的主角
	UPROPERTY()
	ECharacterGender LastActiveGender = ECharacterGender::MALE;

	// 存档版本号（破坏性升级时递增; v3: 移除属性/等级/技能字段)
	UPROPERTY()
	int32 SaveVersion = 3;
};
