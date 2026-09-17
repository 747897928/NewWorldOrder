// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSubsystem.generated.h"

class USaveGameSlotInfo;
class UShootSaveGame;
class AShootPlayerState;

// 通知加载完成：是否成功、槽位名称、槽位索引
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAsyncLoadGameDataFinish, bool, bWasSuccessful, FString, SlotName,
                                               int32, SlotIndex);

// 通知保存完成：是否成功、槽位名称、槽位索引
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAsyncSaveGameDataFinish, bool, bWasSuccessful, FString, SlotName,
                                               int32, SlotIndex);

// 保存/加载开始的广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameDataStart, FString, SlotName, int32, SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadGameDataStart, FString, SlotName, int32, SlotIndex);

// 删除当前继续按钮的隐藏通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FContinueButtonHide);

/**
 * 负责管理游戏存档，支持多存档槽位。
 * SlotName 与 SlotIndex 会在初始化时读取并缓存，之后可直接调用 Load/Save。
 */
UCLASS()
class NEWWORLDORDER_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USaveGameSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category="SaveGame")
	FOnAsyncLoadGameDataFinish AsyncLoadGameDataFinish;

	UPROPERTY(BlueprintAssignable, Category="SaveGame")
	FOnAsyncSaveGameDataFinish AsyncSaveGameDataFinish;

	UPROPERTY(BlueprintAssignable, Category="SaveGame")
	FOnSaveGameDataStart SaveGameDataStart;

	UPROPERTY(BlueprintAssignable, Category="SaveGame")
	FOnLoadGameDataStart LoadGameDataStart;

	FAsyncSaveGameToSlotDelegate SaveInfoDataDelegate;

	UPROPERTY(BlueprintAssignable, Category="SaveGame")
	FContinueButtonHide ContinueButtonHide;

private:
	/** 存档信息槽名 */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	FName SlotInfoName{TEXT("SaveInfo")};

	/** 每个存档前缀（Save） */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	FName SlotNamePrefix{TEXT("Save")};

	/** 当前使用的存档名称 */
	UPROPERTY(BlueprintReadOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	FString CurrentSlotName;

	/** 当前使用的存档索引 */
	UPROPERTY(BlueprintReadOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	int32 CurrentSlotIndex = 0;

	/** 当前保存的数据对象 */
	UPROPERTY(BlueprintReadOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	UShootSaveGame* CurrentSaveGame;

	/** 所有存档槽位的元数据 */
	UPROPERTY(BlueprintReadOnly, Category = "SaveGame", meta=(AllowPrivateAccess = "true"))
	USaveGameSlotInfo* SaveInfoData;

	UShootSaveGame* EnsureCurrentSaveGame();

public:
	
	UFUNCTION(BlueprintCallable, Category = "Debug|PIE")
	FString GetPIESafeSlotName(const FString& BaseSlotName) const;

	/** 异步加载指定索引的玩家存档 */
	UFUNCTION(BlueprintCallable)
	void AsyncLoadPlayerSaveGame(int32 SlotIndex);

	/** 同步加载指定索引的玩家存档 */
	UFUNCTION(BlueprintCallable)
	UShootSaveGame* LoadPlayerSaveGame(int32 SlotIndex);
	
	/**
	 * 异步保存玩家存档
	 * 开始和结束后，请绑定FOnSaveGameDataStart和FOnAsyncSaveGameDataFinish获取结果
	 * 
	 * @param SaveGameObject 槽对象
	 * @param SlotIndex 槽的索引
	 */
	UFUNCTION(BlueprintCallable)
	void AsyncPlayerSaveGame(UShootSaveGame* SaveGameObject, int32 SlotIndex);

	/** 删除指定索引的存档 */
	UFUNCTION(BlueprintCallable)
	bool DeleteSaveGame(int32 SlotIndex);

	/** 根据索引获取格式化的存档名 */
	UFUNCTION(BlueprintCallable)
	FString GetSlotNameByIndex(int32 InSlotIndex) const;

	/** 获取当前系统时间，格式 yyyy-MM-dd HH:mm:ss */
	UFUNCTION(BlueprintCallable)
	FString GetCurrentTime() const;

protected:
	// 内部用：更新存档元数据，并立即写入 SaveInfo
	void UpdateSlotInfoData(const FString& SlotName, int32 SlotIndex);

	// 存档信息保存完成的回调
	void OnSaveInfoDataFinish(const FString& InSlotName, int32 InSlotIndex, bool bWasSuccessful);
	
	void CreateNewSaveInfoData(const FString& SlotName, int32 SlotIndex);
	
public:
	/** 存档信息槽名 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FName GetSlotInfoName() { return SlotInfoName; }

	/** 每个存档前缀*/
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FName GetSlotNamePrefix() { return SlotNamePrefix; }

	/** 当前使用的存档名称 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FString GetCurrentSlotName() { return CurrentSlotName; }

	/** 当前使用的存档索引 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	int32 GetCurrentSlotIndex() { return CurrentSlotIndex; }

	/** 当前保存的数据对象 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	UShootSaveGame* GetCurrentSaveGame() { return CurrentSaveGame; }
	
	/** 获取存档信息数据，如果不存在则加载或创建 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	USaveGameSlotInfo* GetOrCreateSaveInfoData();

	UFUNCTION(BlueprintCallable, Category="SaveGame")
	void CapturePlayerInventoryState(AShootPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category="SaveGame")
	void RestorePlayerInventoryState(AShootPlayerState* PlayerState);
	
};
