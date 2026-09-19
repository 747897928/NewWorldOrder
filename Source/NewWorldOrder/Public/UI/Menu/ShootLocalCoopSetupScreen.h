// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "CommonUserTypes.h"
#include "UI/LyraActivatableWidget.h"

#include "ShootLocalCoopSetupScreen.generated.h"

class UCommonUserInfo;
class UCommonUserSubsystem;
class UInputAction;
class UInputMappingContext;
class ULyraUserFacingExperienceDefinition;
class UShootSessionCoordinatorSubsystem;

UENUM(BlueprintType)
enum class EShootLocalCoopSetupStage : uint8
{
	DeviceJoin,
	CharacterAssignment,
	Launching
};

USTRUCT(BlueprintType)
struct FShootLocalCoopPlayerSetupState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 LocalPlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	int32 InputDeviceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	bool bJoined = false;

	UPROPERTY(BlueprintReadOnly)
	ECharacterGender SelectedProtagonist = ECharacterGender::UNKNOWN;

	UPROPERTY(BlueprintReadOnly)
	bool bConfirmed = false;
};

/**
 * 本地双人设置页后端。
 *
 * 页面分成“第二设备加入”和“两台设备独立选择主角”两个明确阶段。CommonUser 负责把物理设备
 * 初始化为第二位 LocalPlayer；页面只通过配置的 InputAction/IMC 解释左右、确认和取消，
 * 不根据具体键位或设备类型写分支。两位玩家确认后写入 GameInstance 的一次性分配并启动所选 UFE。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootLocalCoopSetupScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	void InitializeForExperience(ULyraUserFacingExperienceDefinition* Experience);

	UFUNCTION(BlueprintCallable, Category="Local Coop")
	bool ContinueToCharacterAssignment();

	UFUNCTION(BlueprintCallable, Category="Local Coop")
	void CancelSetup();

	UFUNCTION(BlueprintPure, Category="Local Coop")
	EShootLocalCoopSetupStage GetSetupStage() const { return SetupStage; }

	UFUNCTION(BlueprintPure, Category="Local Coop")
	bool CanContinueToCharacterAssignment() const;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Local Coop", meta=(DisplayName="On Local Coop Setup Changed"))
	void BP_OnSetupChanged(EShootLocalCoopSetupStage Stage,
		const FShootLocalCoopPlayerSetupState& Player01,
		const FShootLocalCoopPlayerSetupState& Player02,
		bool bCanContinue);

	UFUNCTION(BlueprintImplementableEvent, Category="Local Coop", meta=(DisplayName="On Character Selection Conflict"))
	void BP_OnCharacterSelectionConflict(int32 LocalPlayerIndex, ECharacterGender OccupiedProtagonist);

	UFUNCTION(BlueprintImplementableEvent, Category="Local Coop", meta=(DisplayName="On Local Coop Setup Error"))
	void BP_OnSetupError(const FText& Error);

	/** 同一份 IMC 同时用于显示按键提示和解析每个输入设备的动作。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Coop|Input")
	TObjectPtr<UInputMappingContext> SetupInputMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Coop|Input")
	TObjectPtr<UInputAction> SelectLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Coop|Input")
	TObjectPtr<UInputAction> SelectRightAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Coop|Input")
	TObjectPtr<UInputAction> ConfirmAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Coop|Input")
	TObjectPtr<UInputAction> CancelAction;

private:
	void RefreshInitialPlayers();
	void BeginListeningForSecondPlayer();
	void StopListeningForSecondPlayer();
	void ApplyInputMappingToLocalPlayers(bool bAdd);
	void BroadcastSetupState();
	void HandleViewportInputKey(const FInputKeyEventArgs& EventArgs);
	void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState ConnectionState,
		FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);
	void HandleCharacterAction(int32 LocalPlayerIndex, const UInputAction* Action);
	bool DoesKeyTriggerAction(FKey Key, const UInputAction* Action) const;
	int32 ResolveLocalPlayerIndex(FInputDeviceId InputDeviceId) const;
	void TryLaunchWhenReady();
	void LaunchSelectedExperience();
	void CleanupSecondLocalPlayer();

	UFUNCTION()
	void HandleUserInitializeComplete(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
		ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

	UPROPERTY(Transient)
	TObjectPtr<ULyraUserFacingExperienceDefinition> SelectedExperience;

	UPROPERTY(Transient)
	TObjectPtr<UCommonUserSubsystem> CommonUserSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UShootSessionCoordinatorSubsystem> Coordinator;

	FShootLocalCoopPlayerSetupState PlayerStates[2];
	EShootLocalCoopSetupStage SetupStage = EShootLocalCoopSetupStage::DeviceJoin;
	FTimerHandle LaunchTimerHandle;
	bool bCreatedSecondLocalPlayer = false;
	bool bTravelStarted = false;
};
