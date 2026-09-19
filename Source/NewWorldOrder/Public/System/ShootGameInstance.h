// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"

#include "Character/CharacterGender.h"
#include "CommonGameInstance.h"
#include "ShootGameInstance.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class NEWWORLDORDER_API UShootGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()
public:
	UShootGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetDisableSplitScreen(bool bDisableSplitScreen);

	virtual void ReturnToMainMenu() override;
	virtual void OnUserRequestedSession(const FPlatformUserId& PlatformUserId,
		UCommonSession_SearchResult* InRequestedSession,
		const FOnlineResultInformation& RequestedSessionResult) override;
	virtual bool CanJoinRequestedSession() const override;
	virtual void JoinRequestedSession() override;
	virtual void ResetGameAndJoinRequestedSession() override;

	/** 当前世界是否就是会话返回地图。游戏菜单用它区分 HomeMap 与副本/Lobby，不在 UI 中硬编码地图路径。 */
	bool IsSessionReturnMap(const UWorld* World) const;

	/**
	 * 按当前权威 GameMode 配置收敛本机 LocalPlayer。
	 * PostLoadMapWithWorld 负责正常 Travel；GameMode::BeginPlay 还会调用一次，
	 * 因为 PIE 复制世界不会稳定触发 PostLoadMapWithWorld。函数可重复调用且结果幂等。
	 */
	void ApplyLocalPlayerMapPolicy(UWorld* LoadedWorld);

	/** 本地双人选择页写入的一次性主角分配；只供下一张 SplitProtagonists 地图读取。 */
	void SetPendingLocalCoopProtagonists(ECharacterGender Player01Gender, ECharacterGender Player02Gender);
	bool GetPendingLocalCoopProtagonist(int32 LocalPlayerIndex, ECharacterGender& OutGender) const;
	void ClearPendingLocalCoopProtagonists();
	
protected:
	virtual void Init() override;

	virtual void Shutdown() override;

	/** 会话离开或主机销毁后的家园地图；BP_ShootGameInstance 可用 World 资产选择器覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category="Session|Travel")
	TSoftObjectPtr<UWorld> SessionReturnMap;

private:
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
		const FString& ErrorString);
	bool TravelToSessionReturnMap(UWorld* World);
	void TryJoinRequestedSessionAfterTravel();
	ULocalPlayer* ResolveRequestedSessionLocalPlayer() const;

	/** Steam/平台邀请属于接受邀请的本地用户；不能无条件把分屏玩家的邀请交给 Player 0。 */
	FPlatformUserId RequestedSessionPlatformUser;
	bool bSessionReturnTravelPending = false;
	bool bSessionReturnRedirectScheduled = false;
	bool bJoinRequestedSessionAfterTravel = false;
	int32 JoinRequestedSessionRetryCount = 0;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle NetworkFailureHandle;
	FTimerHandle JoinRequestedSessionRetryTimer;
	ECharacterGender PendingPlayer01Gender = ECharacterGender::UNKNOWN;
	ECharacterGender PendingPlayer02Gender = ECharacterGender::UNKNOWN;
	bool bHasPendingLocalCoopProtagonists = false;
};
