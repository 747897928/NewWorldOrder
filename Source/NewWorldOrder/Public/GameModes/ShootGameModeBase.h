// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "System/ShootSaveGame.h"

#include "ShootGameModeBase.generated.h"

class UCharacterClassInfo;
class ILyraTeamAgentInterface;
class UShootExperienceDefinition;
class AShootPlayerState;
class AController;
class APlayerController;
class AEnemyBotCharacter;

struct FLyraInteractionDurationMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShootCharacterKilled, AActor*, Killer, AActor*, Victim);
/**
 * 
 */
UENUM(BlueprintType)
enum class EShootDifficulty : uint8
{
	Easy UMETA(DisplayName="Easy"),
	Normal UMETA(DisplayName="Normal"),
	Hard UMETA(DisplayName="Hard"),
	Nightmare UMETA(DisplayName="Nightmare")
};

/**
 * 地图级本地多人策略由 GameMode 蓝图选择，避免 GameInstance 硬编码地图资产路径。
 * 该策略只管理当前机器的 ULocalPlayer；Listen Server 的远端网络玩家没有 ULocalPlayer，
 * 不会被 HomeMap 收敛或本机分屏主角分配误伤。
 */
UENUM(BlueprintType)
enum class EShootLocalPlayerMapPolicy : uint8
{
	KeepCurrent UMETA(DisplayName="Keep Current Local Players"),
	PrimaryOnly UMETA(DisplayName="Primary Local Player Only"),
	SplitProtagonists UMETA(DisplayName="Keep Split Players And Assign Protagonists")
};

UCLASS()
class NEWWORLDORDER_API AShootGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Character Class Defaults")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;
	
	AShootGameModeBase();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }

	void PlayerDied(ACharacter* DeadCharacter);

	/**
	 * Lyra 风格的下一帧重启入口，供未来模式级 AutoRespawn Ability 使用。
	 * 当前最终死亡仍由本 GameMode 的单一延迟计时器调用，避免 Ability 与 GameMode 各自重生一次。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Player Respawn")
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);

	/**
	 * 终结死亡后的玩家重生规则。Hub 等非战斗地图保持关闭；具体 PVE GameMode 可启用。
	 * “倒地救起”不由本开关表达，后续由模式 Experience/AbilitySet 在 Die 前接管。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Player Respawn")
	bool bRespawnPlayers = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Player Respawn", meta=(ClampMin="0.0", Units="s", EditCondition="bRespawnPlayers"))
	float PlayerRespawnDelay = 5.0f;

	/**
	 * 重生已启用时旧 Pawn 的保留时间；Controller 会立即解绑，QuickBar 会话由 Controller 暂存。
	 * 默认立即销毁旧 Pawn，死亡期间的表现交给 HUD 的 OnEliminated 动画和 Respawn UI，
	 * 避免已经失去控制的角色模型在场景中额外停留一秒。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Player Respawn", meta=(ClampMin="0.0", Units="s", EditCondition="bRespawnPlayers"))
	float PlayerCorpseLifeSpan = 0.0f;

	/**
	 * 回合开始/重新开始时由 GameMode 蓝图调用(服务器)：重置并初始化所有玩家与敌人属性。
	 * Experience 技能属于 Match，不在这里重新授予，避免跨 Round 丢失冷却和构筑状态。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Round")
	void InitializeRoundForAll();

	/** 仅由启用该规则的副本 GameMode 重建敌人；Hub 和未配置地图保持关闭。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Enemy Respawn")
	bool bRespawnEnemyBots = false;

	/** 从死亡到重新生成同一敌人蓝图类的等待时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Enemy Respawn", meta=(ClampMin="0.0", Units="s", EditCondition="bRespawnEnemyBots"))
	float EnemyRespawnDelay = 5.0f;

	/** 死亡 Actor 保留时间；当前没有尸体表现时可设为 0 立即清理。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Enemy Respawn", meta=(ClampMin="0.0", Units="s"))
	float EnemyCorpseLifeSpan = 1.0f;
	
	UShootSaveGame* RetrieveInGameSaveData();

	// 友伤倍率：0 = 无友伤，1 = 完全友伤，可根据难度配置
	UPROPERTY(EditDefaultsOnly, Category="Difficulty")
	float FriendlyFireScalar;

	// 当前难度（可用于绑定数值/友伤等）
	UPROPERTY(EditDefaultsOnly, Category="Difficulty")
	EShootDifficulty CurrentDifficulty;

	// 各难度对应的友伤倍率（优先使用映射，未配置则使用 FriendlyFireScalar）
	UPROPERTY(EditDefaultsOnly, Category="Difficulty")
	TMap<EShootDifficulty, float> FriendlyFireByDifficulty;

	/** GameMode 在 PostLogin 为玩家 PlayerState 分配的默认队伍；PlayerState 只保存和复制结果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 DefaultPlayerTeamId = 1;

	/**
	 * 当前 GameMode 使用的 Experience。蓝图子类只选择资产，不在图表里硬编码 HUD 数组。
	 * 服务器会把该资产标识交给 GameState ExperienceManager 复制到所有客户端。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience")
	TSoftObjectPtr<UShootExperienceDefinition> ExperienceDefinition;

	/**
	 * 本地图如何处理同一进程内的 LocalPlayer。
	 * HomeMap 蓝图使用 PrimaryOnly；需要分屏主角配对的副本模式使用 SplitProtagonists。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Multiplayer")
	EShootLocalPlayerMapPolicy LocalPlayerMapPolicy = EShootLocalPlayerMapPolicy::KeepCurrent;

	EShootLocalPlayerMapPolicy GetLocalPlayerMapPolicy() const { return LocalPlayerMapPolicy; }

	/**
	 * PlayerState 恢复 SaveGame 时调用的地图规则入口。
	 * 基类不改变账号保存的主角；具体玩法 GameMode 可以在同一次恢复事务中选择本局初始主角，
	 * 避免 Character/Mutable 已初始化后再用 Timer 重切性别。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Local Multiplayer")
	ECharacterGender ResolveInitialCharacterGender(
		const AShootPlayerState* PlayerState, ECharacterGender SavedGender) const;

	/**
	 * 返回该 PlayerState 是否可以更新账号级 LastActiveGender。
	 * 基类默认允许；本地双人选择不属于单人“最后切换主角”，两位 LocalPlayer 都不写入该字段；
	 * Listen Server 远端玩家没有 ULocalPlayer，仍按各自进程和存档处理。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Local Multiplayer")
	bool ShouldPersistLastActiveGender(const AShootPlayerState* PlayerState) const;

	float GetFriendlyFireScalar() const;
	// 计算友伤倍率：若同队返回友伤系数，否则返回 1
	UFUNCTION(BlueprintCallable, Category="Difficulty|FriendlyFire")
	float GetFriendlyFireScalarForActors(const AActor* InstigatorActor, const AActor* TargetActor) const;

	// 通知击杀（可在子类/蓝图重写处理积分、充能等）
	UFUNCTION(BlueprintNativeEvent, Category="Combat")
	void NotifyCharacterKilled(AActor* Killer, AActor* Victim);

	// 全局击杀委托（服务器广播，便于技能/系统监听）
	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnShootCharacterKilled OnCharacterKilled;

protected:
	virtual void BeginPlay() override;

private:
	/** InitGame 解析后的本局 Experience；显式 URL 选择优先于蓝图默认值。 */
	TSoftObjectPtr<UShootExperienceDefinition> ResolvedExperienceDefinition;
	FPrimaryAssetId PendingExpeditionMapId;
	FPrimaryAssetId PendingExpeditionExperienceId;
	bool bPendingAllowJoinInProgress = true;
	bool bPendingFillEmptySlotsWithBots = false;

	void RespawnEnemyBot(TSubclassOf<AEnemyBotCharacter> EnemyClass, FTransform SpawnTransform);
	bool RespawnPlayer(AController* PlayerController);
	void BroadcastRespawnDuration(AController* PlayerController, float Duration);
	void BroadcastRespawnCompleted(AController* PlayerController);

	//进入Level的TimeSeconds
	double LevelStartingTime = 0.f;
};
