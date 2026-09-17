// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"
#include "Interface/AttributeViewModelInterface.h"
#include "Teams/LyraTeamAgentInterface.h"

#include "LyraLocalPlayer.generated.h"

struct FGenericTeamId;

class APlayerController;
class UInputMappingContext;
class ULyraSettingsLocal;
class ULyraSettingsShared;
class UObject;
class UWorld;
struct FFrame;
struct FSwapAudioOutputResult;

/**
 * ULyraLocalPlayer
 */
UCLASS(Config=Game)
class NEWWORLDORDER_API ULyraLocalPlayer : public UCommonLocalPlayer, public ILyraTeamAgentInterface,
                                           public IAttributeViewModelInterface
{
	GENERATED_BODY()

public:
	ULyraLocalPlayer();

	//~UObject interface
	virtual void PostInitProperties() override;
	//~End of UObject interface

	//~UPlayer interface
	virtual void SwitchController(class APlayerController* PC) override;
	//~End of UPlayer interface

	//~ULocalPlayer interface
	virtual bool SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld) override;
	virtual void InitOnlineSession() override;
	//~End of ULocalPlayer interface

	//~ILyraTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface

	/** 进程级设备设置，来自 UGameUserSettings，启动后始终可用。 */
	UFUNCTION()
	ULyraSettingsLocal* GetLocalSettings() const;

	/** 当前 LocalPlayer 的玩家设置，通过 SaveGame 持久化。首次访问时会按平台能力同步或临时创建。 */
	UFUNCTION()
	ULyraSettingsShared* GetSharedSettings() const;

	/** Starts an async request to load the shared settings, this will call OnSharedSettingsLoaded after loading or creating new ones */
	void LoadSharedSettingsFromDisk(bool bForceLoad = false);

	virtual UAttributeViewModel* GetAttributeViewModel_Implementation() override;

protected:
	void OnSharedSettingsLoaded(ULyraSettingsShared* LoadedOrCreatedSettings);

	void OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId);

	UFUNCTION()
	void OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult);

	void OnPlayerControllerChanged(APlayerController* NewController);

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

private:
	UPROPERTY(Transient)
	mutable TObjectPtr<ULyraSettingsShared> SharedSettings;

	FUniqueNetIdRepl NetIdForSharedSettings;

	/**
	 * 只注册项目自己的 IMC_Default，使 Settings 能在 FrontEndMap 尚未激活玩法输入时列出可改键位。
	 * 具体资产通过 DefaultGame.ini 配置，不能在 C++ 构造函数硬编码 /Game 路径。
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<UInputMappingContext> UserMappableInputMappingContext;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TWeakObjectPtr<APlayerController> LastBoundPC;

	UPROPERTY()
	TObjectPtr<UAttributeViewModel> AttributeViewModel;
};
