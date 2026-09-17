// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Character/CharacterGender.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "Inventory/SavedInventoryTypes.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Abilities/GameplayAbility.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "AbilitySystem/ShootAbilitySet.h"
#include "ShootPlayerState.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
class UAttributeSet;
class ULevelUpInfo;
class UGameplayEffect;
class UShootAbilitySystemComponent;
class UShootExperienceDefinition;
class UShootInventoryFragment_WardrobeItem;
class UShootInventoryManagerComponent;
class UResourceInventoryComponent;
class UShootSkillLoadoutComponent;
class UShootRobotCompanionComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChanged, int32 /*StatValue*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLevelChanged, int32 /*StatValue*/, bool /*bLevelUp*/)

// 标签变化委托
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAppearanceTagsChanged, ECharacterGender /*InGender*/,
                                     const FGameplayTagContainer& /*NewTags*/);

USTRUCT()
struct FRuntimeInventoryItemSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ItemInstanceId;

	UPROPERTY()
	TSoftClassPtr<UShootInventoryItemDefinition> ItemDefinition;

	UPROPERTY()
	TArray<FSavedTagStack> TagStacks;

	UPROPERTY()
	int32 StackCount = 1;

	UPROPERTY()
	EShootItemLifetime Lifetime = EShootItemLifetime::Persistent;
};

USTRUCT()
struct FSavedSetByCallerTagMagnitude
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	float Magnitude = 0.f;
};

USTRUCT()
struct FActiveGameplayEffectSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY()
	float RemainingTime = 0.f;

	UPROPERTY()
	float EffectLevel = 1.f;

	UPROPERTY()
	int32 StackCount = 1;

	// SetByCaller 标签快照（用于还原动态数值）
	UPROPERTY()
	TArray<FSavedSetByCallerTagMagnitude> SetByCallerTagMagnitudes;
};

USTRUCT()
struct FCharacterRuntimeSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	bool bInitialized = false;

	UPROPERTY()
	TArray<FRuntimeInventoryItemSnapshot> InventoryItems;

	UPROPERTY()
	TArray<FSavedQuickbarSlot> QuickbarSlots;

	// 主动效果/冷却快照（切换角色时保持技能状态）
	UPROPERTY()
	TArray<FActiveGameplayEffectSnapshot> ActiveEffectSnapshots;

	UPROPERTY()
	FGameplayTagContainer AppearanceTags;

	UPROPERTY()
	float HealthSnapshot = -1.f;

	UPROPERTY()
	float ShieldSnapshot = -1.f;

	UPROPERTY()
	float UltimateChargeSnapshot = -1.f;
};
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API AShootPlayerState : public AModularPlayerState, public IAbilitySystemInterface,
                                            public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	AShootPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	//~ILyraTeamAgentInterface interface
	void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(MyTeamID);
	}

	virtual void Reset() override;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<ULevelUpInfo> LevelUpInfo;

	FOnPlayerStatChanged OnXPChangedDelegate;
	FOnLevelChanged OnLevelChangedDelegate;
	FOnPlayerStatChanged OnAttributePointsChangedDelegate;
	FOnPlayerStatChanged OnSpellPointsChangedDelegate;

	FORCEINLINE int32 GetPlayerLevel() const { return Level; }
	FORCEINLINE int32 GetXP() const { return XP; }
	FORCEINLINE int32 GetAttributePoints() const { return AttributePoints; }
	FORCEINLINE int32 GetSpellPoints() const { return SpellPoints; }
	FORCEINLINE UShootInventoryManagerComponent* GetInventoryManagerComponent() const { return InventoryManagerComponent; }
	FORCEINLINE UResourceInventoryComponent* GetResourceInventoryComponent() const { return ResourceInventoryComponent; }
	UFUNCTION(BlueprintPure, Category="Match Skills")
	UShootSkillLoadoutComponent* GetSkillLoadoutComponent() const { return SkillLoadoutComponent; }

	UFUNCTION(BlueprintPure, Category="Match Skills|Robot")
	UShootRobotCompanionComponent* GetRobotCompanionComponent() const { return RobotCompanionComponent; }

	/**
	 * 幂等地应用当前 Experience 的公共套件与当前性别套件（服务器）。
	 * 两类来源分别持有撤销句柄，公共输入能力不会因性别切换或回合重置反复撤销/授予。
	 */
	UFUNCTION(BlueprintCallable, Category="Abilities|Experience")
	void ApplyExperienceAbilitySets();

	/** 仅刷新当前性别套件；运行时切换男女主时调用，不触碰公共套件。 */
	UFUNCTION(BlueprintCallable, Category="Abilities|Experience")
	void RefreshExperienceGenderAbilitySet();

	/** Experience 卸载时按“性别 -> 公共”的逆序完整取回授予内容。 */
	UFUNCTION(BlueprintCallable, Category="Abilities|Experience")
	void TakeExperienceAbilitySets();

	void AddToXP(int32 InXP);
	void AddToLevel(int32 InLevel);
	void AddToAttributePoints(int32 InPoints);
	void AddToSpellPoints(int32 InPoints);

	void SetXP(int32 InXP);
	void SetLevel(int32 InLevel);
	void SetAttributePoints(int32 InPoints);
	void SetSpellPoints(int32 InPoints);

	// 添加可复制的标签容器
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_FemaleAppearanceTags)
	FGameplayTagContainer FemaleAppearanceTags;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_MaleAppearanceTags)
	FGameplayTagContainer MaleAppearanceTags;

	// 标签变化委托
	FOnAppearanceTagsChanged OnAppearanceTagsChanged;

	// 设置标签容器
	UFUNCTION(Server, Reliable)
	void ServerSetAppearanceTags(ECharacterGender InGender, const FGameplayTagContainer& NewTags);

	/** 衣柜装备入口：客户端只传物品实例 Guid，服务器验证背包所有权和 Wardrobe Fragment 后写入当前性别外观。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Appearance|Wardrobe")
	void ServerEquipWardrobeItem(FGuid ItemInstanceId);

	/** 衣柜试穿入口：只改当前外观预览，不写存档；玩家点保存搭配或正式装备后才落盘。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Appearance|Wardrobe")
	void ServerPreviewWardrobeItem(FGuid ItemInstanceId);

	/** 衣柜卸下入口：移除该服装写入的 Mutable 外观标签，并立即保存当前搭配。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Appearance|Wardrobe")
	void ServerUnequipWardrobeItem(FGuid ItemInstanceId);

	/** 衣柜保存入口：把当前性别外观写入快照并落盘。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Appearance|Wardrobe")
	void ServerSaveCurrentWardrobe();

	// 添加获取当前标签的方法
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	const FGameplayTagContainer& GetAppearanceTags(ECharacterGender InGender) const;
	
	void SetCharacterGender(ECharacterGender InGender);

	FORCEINLINE ECharacterGender GetCharacterGender() const { return CharacterGender; }

	/**
	 * SaveGame 恢复时由当前 GameMode 一次性写入。
	 * EndPlay 阶段 Controller/ULocalPlayer 可能已经解绑，禁止届时再猜该 PlayerState 是否属于分屏子玩家。
	 */
	void SetCanPersistLastActiveGender(bool bInCanPersist) { bCanPersistLastActiveGender = bInCanPersist; }
	bool CanPersistLastActiveGender() const { return bCanPersistLastActiveGender; }

	// 运行时切换性别（仅服务器）
	UFUNCTION(BlueprintCallable, Category="Character|Switch", BlueprintAuthorityOnly)
	bool SwitchToCharacter(ECharacterGender TargetGender);

	/**
	 * 在 HUB/主菜单中，当玩家确认当前性别的 Loadout/QuickBar 修改时调用。
	 * 蓝图只负责修改当前 Pawn/CombatComponent 的槽位，然后调用本函数让 C++ 刷新快照并更新存档。
	 */
	UFUNCTION(BlueprintCallable, Category="Character|Loadout", BlueprintAuthorityOnly)
	void CommitCurrentGenderLoadoutToSave();

	/**
	 * PVE GameMode 在副本开始、重生或回合重置时调用。
	 * 当前性别的 Persistent 出战配置会物化为独立 RuntimeOnly 战斗武器；家园编辑装备不能调用此函数。
	 */
	UFUNCTION(BlueprintCallable, Category="Character|Loadout", BlueprintAuthorityOnly)
	bool InitializeRuntimeLoadoutForCurrentCharacter();
	const FCharacterRuntimeSnapshot* GetSnapshot(ECharacterGender Gender) const;

	/** 开发调试：打印男女主快照 */
	UFUNCTION(Exec)
	void DumpCharacterSnapshots();

	/** 开发调试：服务器快速切换性别（0=男，1=女） */
	UFUNCTION(Exec)
	void SwitchCharacterDebug(int32 GenderAsInt);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(Category=Inventory, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UShootInventoryManagerComponent> InventoryManagerComponent;

	UPROPERTY(Category=Inventory, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UResourceInventoryComponent> ResourceInventoryComponent;

	/** 四槽技能属于 Match，会复制给拥有客户端，但不进入 SaveGame。 */
	UPROPERTY(Category="Match Skills", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UShootSkillLoadoutComponent> SkillLoadoutComponent;

	/** 机器人是 PlayerState 持有的 Match 临时实体；角色重生时重新绑定，离开 Experience 时销毁。 */
	UPROPERTY(Category="Match Skills|Robot", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootRobotCompanionComponent> RobotCompanionComponent;

private:

	UPROPERTY(Category=Character, EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CharacterGender,
		meta=(AllowPrivateAccess = "true"))
	ECharacterGender CharacterGender;

	// 单人地图默认允许；本地分屏的 Player01/02 都不得覆盖单人模式使用的 LastActiveGender。
	UPROPERTY(Transient)
	bool bCanPersistLastActiveGender = true;
	
	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	/** Experience 公共套件与性别套件必须分源持有句柄，避免刷新性别时误撤销相机、交互等公共能力。 */
	FShootAbilitySet_GrantedHandles ExperienceCommonAbilityHandles;
	FShootAbilitySet_GrantedHandles ExperienceGenderAbilityHandles;

private:
	/** 通过 GameState 上的 ExperienceManager 读取当前 Experience（可能为空）。 */
	const UShootExperienceDefinition* GetCurrentExperienceDefinition() const;

public:

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_XP)
	int32 XP = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_AttributePoints)
	int32 AttributePoints = 99;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_SpellPoints)
	int32 SpellPoints = 0;

	// 运行时快照（内存，不持久化）
	UPROPERTY()
	FCharacterRuntimeSnapshot MaleSnapshot;

	UPROPERTY()
	FCharacterRuntimeSnapshot FemaleSnapshot;

	// Helper
	bool CanSwitchCharacter(ECharacterGender TargetGender) const;
	bool SaveCurrentCharacterSnapshot(FCharacterRuntimeSnapshot& OutSnapshot, ECharacterGender SnapshotGender);
	bool LoadCharacterFromSnapshot(const FCharacterRuntimeSnapshot& InSnapshot, ECharacterGender TargetGender);
	bool InitializeNewCharacterForGender(ECharacterGender TargetGender, FCharacterRuntimeSnapshot& OutSnapshot);
	void SaveActiveEffectSnapshot(UShootAbilitySystemComponent* ShootASC, FCharacterRuntimeSnapshot& OutSnapshot) const;
	void RestoreActiveEffectSnapshot(UShootAbilitySystemComponent* ShootASC, const FCharacterRuntimeSnapshot& InSnapshot) const;
	void ClearActiveEffectsForSwitch(UShootAbilitySystemComponent* ShootASC) const;
	bool BuildDefaultQuickbarFromInventory(class UCombatComponent* Combat, UShootInventoryManagerComponent* InventoryManager) const;
	const UShootInventoryFragment_WardrobeItem* ResolveOwnedPersistentWardrobeFragment(FGuid ItemInstanceId, UShootInventoryItemInstance*& OutItemInstance);
	void PersistCurrentWardrobeState();

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_XP(int32 OldXP);

	UFUNCTION()
	void OnRep_AttributePoints(int32 OldAttributePoints);

	UFUNCTION()
	void OnRep_SpellPoints(int32 OldSpellPoints);

	// 复制回调女主角服装标签
	UFUNCTION()
	void OnRep_FemaleAppearanceTags();

	// 复制回调男主角服装标签
	UFUNCTION()
	void OnRep_MaleAppearanceTags();

	UFUNCTION()
	void OnRep_CharacterGender(ECharacterGender LastCharacterGender);
};
