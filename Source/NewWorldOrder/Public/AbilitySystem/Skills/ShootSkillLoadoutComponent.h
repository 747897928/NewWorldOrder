// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/ShootAbilitySet.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "ShootSkillLoadoutComponent.generated.h"

class APawn;
class APlayerState;
class AShootPlayerState;
class UShootAbilitySystemComponent;
class UShootSkillDefinition;
class UShootSkillLoadoutConfig;
class UInputAction;

UENUM(BlueprintType)
enum class EShootSkillAcquireResult : uint8
{
	GrantedNewSkill,
	UpgradedExistingSkill,
	NoEligibleSkill,
	InvalidRequest
};

USTRUCT(BlueprintType)
struct FShootSkillSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<const UShootSkillDefinition> SkillDefinition;

	UPROPERTY(BlueprintReadOnly)
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CurrentModeTag;

	bool IsEmpty() const { return !SkillDefinition || Level <= 0; }
};

/** GameplayMessageSubsystem 消息：Widget 必须按自己的 Owning PlayerState 过滤，保证分屏不串槽。 */
USTRUCT(BlueprintType)
struct FShootSkillLoadoutChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState;

	/** INDEX_NONE 表示整表刷新。 */
	UPROPERTY(BlueprintReadOnly)
	int32 ChangedSlotIndex = INDEX_NONE;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FShootSkillSlotsChanged, class UShootSkillLoadoutComponent*, int32);

/**
 * PlayerState 上的四槽 Match Skill 权威组件。
 * 服务器写槽位并持有每槽 AbilitySet 句柄；拥有客户端只接收展示快照。Experience 卸载时统一清空，绝不进入 SaveGame。
 */
UCLASS(BlueprintType, ClassGroup=(Abilities), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootSkillLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 SkillSlotCount = 4;

	UShootSkillLoadoutComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Match Skills")
	int32 GetSlotCount() const { return SkillSlotCount; }

	UFUNCTION(BlueprintPure, Category="Match Skills")
	FShootSkillSlot GetSlot(int32 SlotIndex) const;

	/** 从当前 Experience 的 SkillLoadoutConfig 读取槽位输入，不在 UI 中复制一份键位表。 */
	UFUNCTION(BlueprintPure, Category="Match Skills")
	UInputAction* GetSlotInputAction(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Match Skills")
	FGameplayTag GetSlotInputTag(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Match Skills")
	bool CanAcquireAnySkill() const;

	/** 查询指定技能在当前 Experience 目录中是否可新增或升级，供商人/三选一先过滤不可购买项。 */
	UFUNCTION(BlueprintPure, Category="Match Skills")
	bool CanAcquireSkill(const UShootSkillDefinition* Definition) const;

	/**
	 * 服务器权威的指定技能事务入口。商人、Round 三选一和技能融合都必须走这里，不能自行写槽或直接授予 GA。
	 */
	bool AcquireSkill(APawn* RequestingPawn, const UShootSkillDefinition* Definition,
		int32& OutSlotIndex, EShootSkillAcquireResult& OutResult);

	/** 交互 GA 的服务器入口：从 Experience 配置的候选池随机获得新技能或升级已有技能。 */
	bool AcquireRandomSkill(APawn* RequestingPawn, int32& OutSlotIndex, EShootSkillAcquireResult& OutResult);

	/** 服务器切换多模式技能；模式必须属于 Definition.ModeTags。 */
	bool SetSkillMode(const UShootSkillDefinition* Definition, FGameplayTag NewModeTag);

	/** 按 Definition.ModeTags 顺序循环，返回切换后的模式。 */
	bool CycleSkillMode(const UShootSkillDefinition* Definition, FGameplayTag& OutNewModeTag);

	/** Experience 卸载或 PlayerState 离开 Match 时调用。 */
	void ClearMatchSkills();

	/** UI 可轮询冷却进度；返回 false 表示该槽没有可查询的冷却。 */
	UFUNCTION(BlueprintPure, Category="Match Skills")
	bool GetCooldownRemaining(int32 SlotIndex, float& OutTimeRemaining, float& OutDuration) const;

	FShootSkillSlotsChanged& OnSkillSlotsChanged() { return SkillSlotsChangedDelegate; }

private:
	UFUNCTION()
	void OnRep_Slots();

	const UShootSkillLoadoutConfig* GetCurrentConfig() const;
	bool IsCurrentConfigValid(const UShootSkillLoadoutConfig* Config) const;
	UShootAbilitySystemComponent* GetShootASC() const;
	int32 FindSlotForDefinition(const UShootSkillDefinition* Definition) const;
	int32 FindFirstEmptySlot() const;
	bool ApplyDefinitionToSlot(const UShootSkillDefinition* Definition, int32& OutSlotIndex,
		EShootSkillAcquireResult& OutResult);
	void RegrantSlot(int32 SlotIndex);
	void NotifySlotsChanged(int32 ChangedSlotIndex);

	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<FShootSkillSlot> Slots;

	/** 只存在服务器；每槽只撤销自己的 Ability/GE/AttributeSet，禁止按 AbilityClass 全局扫描。 */
	UPROPERTY(Transient)
	TArray<FShootAbilitySet_GrantedHandles> GrantedSlotHandles;

	FShootSkillSlotsChanged SkillSlotsChangedDelegate;
};
