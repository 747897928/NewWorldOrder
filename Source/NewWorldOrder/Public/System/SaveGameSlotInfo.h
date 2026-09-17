// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/SoftObjectPtr.h"
#include "SaveGameSlotInfo.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FSaveSlotItem
{
	GENERATED_USTRUCT_BODY()

public:
	FSaveSlotItem()
		: SlotName(TEXT(""))
		, SlotIndex(INDEX_NONE)
		, SaveSlotImage(nullptr)
		, SlotSaveTime(TEXT(""))
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FString SlotName;  // 槽位名称

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 SlotIndex;   // 槽位索引

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thumbnail")
	TSoftObjectPtr<UTexture2D> SaveSlotImage;  // 存档缩略图，可选（软引用防止裸指针序列化）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	FString SlotSaveTime;  // 存档时间字符串
};

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API USaveGameSlotInfo : public USaveGame
{
	GENERATED_BODY()

public:
	// Key: 槽位名称，Value: FSaveSlotItem 结构体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	TMap<FString, FSaveSlotItem> SaveSlotInfoMap;

	// 最新使用的槽位索引，用于循环或一键覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 LastSlotIndex;
};
