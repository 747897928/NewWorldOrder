// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShootCharacterBase.h"
#include "Animation/ShootAnimLayerSelectionSet.h"
#include "Interface/PlayerInterface.h"
#include "Logging/LogMacros.h"
#include "ShootCharacter.generated.h"

class UCombatComponent;
class ULyraContextEffectComponent;
class UMutableAppearanceComponent;
class UShootAbilitySystemComponent;
class UShootInventoryManagerComponent;
class UShootEquipmentManagerComponent;
class UGameplayAbility;
class UCustomizableSkeletalComponent;
class UCustomizableObjectInstance;
class UCustomizableObject;
struct FOnAttributeChangeData;

class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UShootCameraModeStackComponent;
class UInputMappingContext;
class UInputAction;
class UShootInputConfig;
class UAnimInstance;
class UAnimMontage;
class UAnimSequence;
class IRepChangedPropertyTracker;
struct FGameplayTag;
struct FInputActionValue;
enum class ECharacterGender : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * 模拟代理动画使用的压缩加速度。
 *
 * 速度由 ACharacter 复制，但加速度默认不会作为稳定动画事实到达远端；
 * Lyra 的 Start/Stop/Pivot 状态依赖它，因此在 Pawn 上单独复制给模拟代理。
 */
USTRUCT()
struct FShootReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;

	UPROPERTY()
	int8 AccelZ = 0;
};

UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class AShootCharacter : public AShootCharacterBase, public IPlayerInterface
{
	GENERATED_BODY()

protected:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/**
	 * 独立第一人称相机。按项目决定附加到 Head 骨骼，直接跟随头部动画；
	 * BP_ShootCharacter 可调整相对位置、旋转、FOV、PostProcess 和未来第一人称专属渲染配置。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** 双相机模式栈：选择第三/第一人称输出，并把临时 ADS 应用到当前相机。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShootCameraModeStackComponent> CameraModeStack;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Run Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> RunAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CrouchAction;

	/** Open Menu Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> OpenMenuAction;

public:
	AShootCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	//PossessedBy只会在服务器端触发，客户端不会触发，我们在这个方法初始化服务端的AbilitySystemComponent
	virtual void PossessedBy(AController* NewController) override;

	//由于PossessedBy只会在服务器端触发，OnRep_PlayerState只在客户端触发,
	//初始化服务端的AbilitySystemComponent则是用OnRep_PlayerState替代
	virtual void OnRep_PlayerState() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName BodyMeshComponentName;

	/** Player Interface */
	// virtual void AddToXP_Implementation(int32 InXP) override;
	// virtual void LevelUp_Implementation() override;
	// virtual int32 GetXP_Implementation() const override;
	// virtual int32 FindLevelForXP_Implementation(int32 InXP) const override;
	// virtual int32 GetAttributePointsReward_Implementation(int32 Level) const override;
	// virtual int32 GetSpellPointsReward_Implementation(int32 Level) const override;
	// virtual void AddToPlayerLevel_Implementation(int32 InPlayerLevel) override;
	virtual void AddToAttributePoints_Implementation(int32 InAttributePoints) override;
	virtual void AddToSpellPoints_Implementation(int32 InSpellPoints) override;
	virtual int32 GetAttributePoints_Implementation() const override;
	// virtual int32 GetSpellPoints_Implementation() const override;
	// virtual void ShowMagicCircle_Implementation(UMaterialInterface* DecalMaterial) override;
	// virtual void HideMagicCircle_Implementation() override;
	// virtual void SaveProgress_Implementation(const FName& CheckpointTag) override;
	/** end Player Interface */
	
	/** Combat Interface */
	virtual int32 GetPlayerLevel_Implementation() override;
	virtual void Die(const FVector& DeathImpulse) override;
	/** end Combat Interface */
	
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	virtual void BeginPlay() override;

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 与 Lyra Character 一致，蹲伏不能在 CanJump 的预校验阶段直接拒绝跳跃。
	 * Jump GA 通过校验后会先 UnCrouch 再调用 Jump，避免“蹲伏时永远无法激活 Jump GA”的死锁。
	 */
	virtual bool CanJumpInternal_Implementation() const override;

	void StartRunning();

	void EndRunning();

	void CrouchButtonPressed();

public:
	/**
	 * 由当前 GameState 的 Experience 决定是否开放游泳；没有加载 Experience 时保留引擎默认行为，
	 * 避免 Pawn 在初始化竞态期间因为水体查询为空而卡在错误移动模式。
	 */
	bool IsSwimmingAllowedByExperience() const;

	/** 当前角色是否处于引擎权威的 MOVE_Swimming。 */
	UFUNCTION(BlueprintPure, Category="Shoot|Swimming")
	bool IsSwimming() const;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowMenuWidget();

public:
	/** 服务器校验姿势条目后调用，多播到所有机器播放同一个 Slot 动态 Montage。 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPose(UAnimSequence* Animation, FName SlotName, float PlayRate, float BlendInTime, float BlendOutTime,
	                       int32 LoopCount);
	
protected:
	UFUNCTION(Server, Reliable)
	void ServerSetMaxSpeed(float NewMaxSpeed);

	UFUNCTION(Client, Reliable)
	void ClientSyncMaxSpeed(float NewMaxSpeed);

	// MoveSpeedMultiplier 变更时刷新移动速度（含客户端预测与服务器权威同步）
	void OnMoveSpeedMultiplierChanged(const FOnAttributeChangeData& Data);

	// 统一从倍率刷新移动速度
	void UpdateMovementSpeedFromMultiplier(float NewMultiplier);

public:
	UPROPERTY(Category="Character Movement speed", EditAnywhere, BlueprintReadWrite,
		meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float BaseWalkSpeed = 300.f;

	UPROPERTY(Category="Character Movement speed", EditAnywhere, BlueprintReadWrite,
		meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float BaseRunSpeed = 600.f;

	/** Returns CameraBoom subobject **/
	FORCEINLINE TObjectPtr<USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE TObjectPtr<UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	/** Returns the independently configurable first-person camera subobject. */
	FORCEINLINE TObjectPtr<UCameraComponent> GetFirstPersonCamera() const { return FirstPersonCamera; }
	FORCEINLINE UShootCameraModeStackComponent* GetCameraModeStack() const { return CameraModeStack; }

	/**
	 * CameraModeStack 的本地表现入口。只对 Owner 视图隐藏 Mutable Head Mesh，
	 * 并把空手第一人称也切到控制器朝向；不复制、不修改其他分屏视图。
	 */
	void SetFirstPersonCameraActive(bool bActive);

	/**
	 * ADS GA 的移速接入点。这里只保存通用移动倍率，不保存武器、相机或狙击镜状态；
	 * UpdateMovementSpeedFromMultiplier 会把它与属性 Buff、走/跑状态相乘，避免结束瞄准时覆盖期间发生的移速变化。
	 */
	void SetAimingMovementSpeedMultiplier(float NewMultiplier);

	/** Returns Mesh subobject **/
	FORCEINLINE TObjectPtr<USkeletalMeshComponent> GetBodyMesh() const { return BodyMesh; }

	/**
	 * EquipmentInstance 装卸时调用：按当前 PlayerState 外观标签链接唯一动画层，
	 * 目标是 Mutable 正式 CC Mesh 上的主 AnimInstance；Hair、Shoe 使用独立接口，不会被武器层覆盖。
	 * 同时原子切换空手/持枪移动朝向。调用者属于 Pawn，不依赖远端 PlayerController。
	 */
	void ApplyWeaponPresentation(const FShootAnimLayerSelectionSet& SelectionSet, bool bArmed);

	/** 外观或 Pawn 初始化后从本 Pawn 的 EquipmentManager 恢复当前表现。 */
	void RefreshWeaponPresentationFromEquipment();

	FORCEINLINE TObjectPtr<UCustomizableSkeletalComponent> GetBodyCSkeletalComponent() const
	{
		return BodyCSkeletalComponent;
	}

	FORCEINLINE TObjectPtr<UCustomizableSkeletalComponent> GetHeadCSkeletalComponent() const
	{
		return HeadCSkeletalComponent;
	}

	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }

	UShootInventoryManagerComponent* GetInventoryManagerComponent() const;

	FORCEINLINE UShootEquipmentManagerComponent* GetEquipmentManagerComponent() const { return EquipmentManagerComponent; }
	FORCEINLINE ULyraContextEffectComponent* GetContextEffectComponent() const { return ContextEffectComponent; }

	UFUNCTION(BlueprintCallable)
	UMutableAppearanceComponent* GetAppearanceComponent();

	void InitAppearanceComponent();
	
	virtual void InitAbilityActorInfo() override;

	void LoadProgress();
	bool GetOrientRotationToMovement();
	
private:
	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UPROPERTY(Transient, ReplicatedUsing=OnRep_ReplicatedAcceleration)
	FShootReplicatedAcceleration ReplicatedAcceleration;

	/** 根据当前 QuickBar 是否持枪切换空手自由转向与 Lyra 控制器朝向侧移。 */
	void ApplyArmedMovementMode(bool bArmed);
	void RefreshMovementFacingMode();

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UShootInputConfig> InputConfig;

	UPROPERTY(Category=GAS, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UShootAbilitySystemComponent> ShootAbilitySystemComponent;

	/** The body skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Combat, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;

	/**
	 * 装备管理组件（Equipment Manager Component）
	 *
	 * 用途：
	 *   - 管理 Pawn 当前装备的物品（武器、护甲、工具等）
	 *   - 处理装备/卸载流程
	 *   - 支持 FastArray 增量复制和 SubObject 复制
	 *
	 * 与 InventoryManager 的区别：
	 *   - InventoryManager：存储所有拥有的物品（库存）
	 *   - EquipmentManager：管理当前装备的物品（装备）
	 *
	 * Phase 2 集成：
	 *   - 创建组件，连接库存系统和装备系统
	 *   - 在 .cpp 构造函数中初始化
	 *
	 * 工作流程：
	 *   1. 玩家从库存中选择物品（ItemInstance）
	 *   2. QuickBar 获取 EquippableItem Fragment
	 *   3. EquipmentManager->EquipItem(Fragment->EquipmentDefinition)
	 *   4. 创建 EquipmentInstance，生成 Mesh，授予 Ability
	 *   5. EquipmentInstance->SetInstigator(ItemInstance)（连接库存和装备）
	 */
	UPROPERTY(Category=Equipment, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UShootEquipmentManagerComponent> EquipmentManagerComponent;

	/**
	 * Lyra ContextEffects 的 Actor 侧入口。
	 * AnimNotify 通过接口找到该组件；具体脚步 Library 由 BP_ShootCharacter 配置，不在 C++ 硬编码资产。
	 */
	UPROPERTY(Category="Feedback|Context Effects", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<ULyraContextEffectComponent> ContextEffectComponent;

	/** The body skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	/** 没有当前武器时使用的男女 Unarmed 层选择集，由 BP_ShootCharacter 配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Weapon", meta=(AllowPrivateAccess = "true"))
	FShootAnimLayerSelectionSet DefaultUnarmedAnimSet;

	UPROPERTY(Category = CustomizableSkeletalComponent, BlueprintReadWrite, EditAnywhere,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableSkeletalComponent> BodyCSkeletalComponent;

	UPROPERTY(Category = CustomizableSkeletalComponent, BlueprintReadWrite, EditAnywhere,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableSkeletalComponent> HeadCSkeletalComponent;

	UPROPERTY(Category=CustomizableObjectInstance, VisibleAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UMutableAppearanceComponent> AppearanceComponent;

	/** PlayerState 性别或外观标签变化后重新选择当前武器层，避免同 Pawn 切性别仍保留旧层。 */
	void HandleAppearanceAnimationTagsChanged(ECharacterGender InGender,
		const FGameplayTagContainer& NewAppearanceTags);

	/** 组合 PlayerState 外观标签，并为女性角色补充 Lyra 的 AnimationStyle 标签。 */
	FGameplayTagContainer BuildWeaponAnimationCosmeticTags() const;

	
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	UShootAbilitySystemComponent* GetASC();

	// 当前移动倍率（来自 AttributeSet）
	float CurrentMoveSpeedMultiplier = 1.0f;

	// 当前 ADS GA 注入的临时移动倍率；能力结束或武器卸下时恢复为 1。
	float CurrentAimingMoveSpeedMultiplier = 1.0f;

	// 跑步状态，用于决定 BaseWalkSpeed/BaseRunSpeed
	bool bIsRunning = false;

	// 武器表现与本地 CameraMode 分别更新，统一在 RefreshMovementFacingMode 合并朝向规则。
	bool bArmedPresentationActive = false;
	bool bFirstPersonCameraActive = false;

	/**
	 * 游泳动画使用当前正式男女 AnimBP 已有的 FullBody Slot 动态播放，
	 * 不改动既有 locomotion 状态机；具体序列由 BP_ShootCharacter 配置，不能在 C++ 构造函数中加载 /Game 路径。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Swimming", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimSequence> SwimmingIdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Swimming", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimSequence> SwimmingForwardAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Swimming", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimSequence> SwimmingStartAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Swimming", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimSequence> SwimmingDiveStartAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Swimming", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimSequence> SwimmingDiveLoopAnimation;

	void UpdateSwimmingAnimation();
	void UpdateFallingDiveAnimation();
	void StopFallingDiveAnimation();
	bool TryGetPredictedWaterEntryTime(float& OutTimeToWater) const;
	void StopSwimmingAnimation();
	UAnimMontage* PlaySwimmingAnimation(UAnimSequence* Animation, int32 LoopCount, float BlendInTime,
	                                    float BlendOutTime);

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveSwimmingMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveSwimmingSequence;

	bool bSwimmingEntryAnimationPending = false;
	bool bSwimmingDiveEntry = false;
	bool bSwimmingEntryMontagePlaying = false;
	// 预测即将落入水体时，先在 MOVE_Falling 中播放跳水姿势；真正入水后交还给游泳状态处理。
	bool bSwimmingPreDiveAnimationActive = false;
};
