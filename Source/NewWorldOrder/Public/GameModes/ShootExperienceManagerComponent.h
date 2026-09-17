// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "UIExtensionSystem.h"

#include "ShootExperienceManagerComponent.generated.h"

class AHUD;
class AShootHUD;
class UCommonActivatableWidget;
class UShootExperienceDefinition;
struct FComponentRequestHandle;

DECLARE_MULTICAST_DELEGATE_OneParam(FShootExperienceLoadedDelegate, const UShootExperienceDefinition*);

/**
 * GameState 上的 Experience 生命周期入口。
 *
 * 服务器由 GameMode 选择 Experience 并复制资产路径；每台机器加载同一份定义。
 * 客户端按 Lyra AddWidgets 的做法，以 HUD 对应的 LocalPlayer 为上下文推入布局和注册 UIExtension，
 * 因而单人、分屏和 Listen Server 不会把玩家私有 QuickBar 注入到错误屏幕。
 */
UCLASS()
class NEWWORLDORDER_API UShootExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UShootExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetCurrentExperience(TSoftObjectPtr<UShootExperienceDefinition> Experience);

	UFUNCTION(BlueprintPure, Category="Experience")
	const UShootExperienceDefinition* GetCurrentExperience() const { return LoadedExperience; }

	/** 与 Lyra Experience 一致：已加载时立即回调，否则在本机资产完成加载后回调。 */
	void CallOrRegister_OnExperienceLoaded(FShootExperienceLoadedDelegate::FDelegate&& Delegate);
	FShootExperienceLoadedDelegate& OnExperienceLoaded() { return ExperienceLoadedDelegate; }

private:
	struct FPerHUDData
	{
		TArray<TWeakObjectPtr<UCommonActivatableWidget>> LayoutsAdded;
		TArray<FUIExtensionHandle> ExtensionHandles;
	};

	UPROPERTY(ReplicatedUsing=OnRep_CurrentExperience)
	TSoftObjectPtr<UShootExperienceDefinition> CurrentExperience;

	UPROPERTY(Transient)
	TObjectPtr<const UShootExperienceDefinition> LoadedExperience;

	UFUNCTION()
	void OnRep_CurrentExperience();

	void LoadAndActivateExperience();
	void DeactivateExperience();
	/** 服务器：给现有玩家授予当前 Experience 的公共与性别 AbilitySet。 */
	void GrantPlayerExperienceAbilities();
	/** 服务器：Experience 卸载前完整取回所有玩家的 AbilitySet 授予。 */
	void TakePlayerExperienceAbilities();
	void HandleHUDExtension(AActor* Actor, FName EventName);
	void AddWidgetsForHUD(AShootHUD* HUD);
	void RemoveWidgetsForHUD(AShootHUD* HUD);

	TSharedPtr<FComponentRequestHandle> HUDRequestHandle;
	TMap<TWeakObjectPtr<AShootHUD>, FPerHUDData> ActiveHUDData;
	FShootExperienceLoadedDelegate ExperienceLoadedDelegate;
};
