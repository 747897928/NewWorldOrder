// Copyright Epic Games, Inc. All Rights Reserved.


#include "Character/ShootCharacter.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Character/ShootCharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Camera/ShootCameraModeStackComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ShootGameplayTags.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "Character/CombatComponent.h"
#include "Character/MutableAppearanceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Equipment/ShootEquipmentManagerComponent.h"
#include "Feedback/ContextEffects/LyraContextEffectComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameModes/ShootGameModeBase.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "GameModes/ShootExperienceManagerComponent.h"
#include "Input/ShootInputComponent.h"
#include "Interface/AttributeViewModelInterface.h"
#include "Kismet/GameplayStatics.h"

#include "MuCO/CustomizableSkeletalComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"
#include "System/ShootSaveGame.h"
#include "UI/ViewModel/AttributeViewModel.h"
#include "Weapons/ShootWeaponInstance.h"


FName AShootCharacter::BodyMeshComponentName(TEXT("CharacterBodyMesh0"));

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

namespace ShootSwimming
{
	static const FName FullBodySlot(TEXT("FullBody"));
	static constexpr int32 LoopCount = 1000;
	static constexpr float MovementThreshold = 15.0f;
	static constexpr float PreDivePredictionStep = 0.05f;
	static constexpr float PreDivePredictionHorizon = 3.0f;
	static constexpr float PreDiveTriggerLeadTime = 0.1f;

	bool IsPointInsidePhysicsVolume(const APhysicsVolume& Volume, const FVector& Location)
	{
		const UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Volume.GetRootComponent());
		if (!RootPrimitive)
		{
			return true;
		}

		float DistanceToCollisionSquared = -1.0f;
		FVector ClosestPointOnCollision = FVector::ZeroVector;
		if (!RootPrimitive->GetSquaredDistanceToCollision(
			Location,
			DistanceToCollisionSquared,
			ClosestPointOnCollision))
		{
			return false;
		}

		return DistanceToCollisionSquared <= KINDA_SMALL_NUMBER;
	}

	bool IsWaterVolumeAtLocation(const UWorld& World, const FVector& Location)
	{
		const APhysicsVolume* BestVolume = World.GetDefaultPhysicsVolume();
		int32 BestPriority = BestVolume ? BestVolume->Priority : MIN_int32;

		for (auto VolumeIt = World.GetNonDefaultPhysicsVolumeIterator(); VolumeIt; ++VolumeIt)
		{
			const APhysicsVolume* Volume = VolumeIt->Get();
			if (!Volume || Volume->Priority <= BestPriority ||
				!IsPointInsidePhysicsVolume(*Volume, Location))
			{
				continue;
			}

			BestVolume = Volume;
			BestPriority = Volume->Priority;
		}

		return BestVolume && BestVolume->bWaterVolume;
	}
}

//////////////////////////////////////////////////////////////////////////
// AShootCharacter
AShootCharacter::AShootCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShootCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	//GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.0f);

	// 空手允许角色朝移动方向转身，玩家可绕到正面观察角色。
	// 装备武器后由 Pawn EquipmentInstance 生命周期切换为 Lyra 的控制器朝向/四向侧移模式。
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->MaxAcceleration = 1200.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1400.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->NavAgentProps.bCanJump = true;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->SetAutoActivate(true);

	// 第一人称相机与第三人称 FollowCamera 保留两套独立组件配置。按当前项目决定附加到 Head 骨骼，
	// 接受复用第三人称动画可能带来的头部晃动或局部穿模；后续协作者不要擅自改回 Root，也不要增加专用手臂网格。
	// 具体相对位置、旋转、FOV、PostProcess 等视觉参数由 BP_ShootCharacter 的组件 Details 调整。
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh(), TEXT("head"));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetAutoActivate(false);

	CameraModeStack = CreateDefaultSubobject<UShootCameraModeStackComponent>(TEXT("CameraModeStack"));

	BodyMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(AShootCharacter::BodyMeshComponentName);
	if (BodyMesh)
	{
		BodyMesh->AlwaysLoadOnClient = true;
		BodyMesh->AlwaysLoadOnServer = true;
		BodyMesh->bOwnerNoSee = false;
		BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		BodyMesh->bCastDynamicShadow = true;
		BodyMesh->bAffectDynamicIndirectLighting = true;
		BodyMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		BodyMesh->SetupAttachment(GetMesh());
		BodyMesh->SetCollisionProfileName(TEXT("CharacterMesh"));
		BodyMesh->SetGenerateOverlapEvents(false);
		BodyMesh->SetCanEverAffectNavigation(false);
		// Mutable 的 Head/Body 输出在异步更新前都可能仍是同一套完整兜底人物。
		// Body 由 UMutableAppearanceComponent 在最终 LeaderPose 校验成功后显示，避免出生首帧叠出两个人物。
		BodyMesh->SetVisibility(false, true);
	}

	BodyCSkeletalComponent = CreateDefaultSubobject<UCustomizableSkeletalComponent>(TEXT("BodyCSkeletalComponent"));
	// Select the Mesh Component or Passthrough Mesh Component declared in the Customizable Object graph
	//BodyCSkeletalComponent->SetComponentName(TEXT("Body"));
	// Attach the CustomizableSkeletalComponent to the Actor's SkeletalMeshComponent
	BodyCSkeletalComponent->SetupAttachment(BodyMesh);
	BodyCSkeletalComponent->SetIsReplicated(true);

	HeadCSkeletalComponent = CreateDefaultSubobject<UCustomizableSkeletalComponent>(TEXT("HeadCSkeletalComponent"));
	// Select the Mesh Component or Passthrough Mesh Component declared in the Customizable Object graph
	//HeadCSkeletalComponent->SetComponentName(TEXT("Head"));
	// Attach the CustomizableSkeletalComponent to the Actor's SkeletalMeshComponent
	HeadCSkeletalComponent->SetupAttachment(GetMesh());
	HeadCSkeletalComponent->SetIsReplicated(true);

	AppearanceComponent = CreateDefaultSubobject<UMutableAppearanceComponent>(TEXT("AppearanceComponent"));
	AppearanceComponent->SetIsReplicated(true);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);

	// 创建装备管理组件（Phase 2 集成）
	// 用于管理 Pawn 当前装备的物品（武器、护甲、工具等）
	// 支持 FastArray 增量复制和 SubObject 复制
	EquipmentManagerComponent = CreateDefaultSubobject<UShootEquipmentManagerComponent>(TEXT("EquipmentManagerComponent"));
	EquipmentManagerComponent->SetIsReplicated(true);

	// 动画通知只面向 ILyraContextEffectsInterface；组件负责按 BP 配置的 Library 播放脚步反馈。
	// 它不复制、不进入 PlayerState/SaveGame，也不会影响 RuntimeOnly 武器会话边界。
	ContextEffectComponent = CreateDefaultSubobject<ULyraContextEffectComponent>(TEXT("ContextEffectComponent"));

}

void AShootCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAppearanceComponent();
	// Init ability actor info for the Server
	InitAbilityActorInfo();
	LoadProgress();
}

void AShootCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAppearanceComponent();
	// Init ability actor info for the Client
	InitAbilityActorInfo();
}

void AShootCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
}

void AShootCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || Movement->MaxAcceleration <= UE_SMALL_NUMBER)
	{
		ReplicatedAcceleration = FShootReplicatedAcceleration();
		return;
	}

	// XY 用极坐标压缩方向和大小，Z 保留正负号；只发送给模拟代理，
	// 本地 AutonomousProxy 继续使用自己的预测加速度。
	const FVector CurrentAcceleration = Movement->GetCurrentAcceleration();
	double AccelXYMagnitude = 0.0;
	double AccelXYRadians = 0.0;
	FMath::CartesianToPolar(
		CurrentAcceleration.X,
		CurrentAcceleration.Y,
		AccelXYMagnitude,
		AccelXYRadians);

	const double MaxAcceleration = Movement->MaxAcceleration;
	ReplicatedAcceleration.AccelXYRadians = static_cast<uint8>(
		FMath::Clamp(FMath::FloorToInt(AccelXYRadians / TWO_PI * 255.0), 0, 255));
	ReplicatedAcceleration.AccelXYMagnitude = static_cast<uint8>(
		FMath::Clamp(FMath::FloorToInt(AccelXYMagnitude / MaxAcceleration * 255.0), 0, 255));
	ReplicatedAcceleration.AccelZ = static_cast<int8>(
		FMath::Clamp(FMath::FloorToInt(CurrentAcceleration.Z / MaxAcceleration * 127.0), -127, 127));
}

void AShootCharacter::OnRep_ReplicatedAcceleration()
{
	UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!Movement || Movement->MaxAcceleration <= UE_SMALL_NUMBER)
	{
		return;
	}

	const double MaxAcceleration = Movement->MaxAcceleration;
	const double AccelXYMagnitude =
		static_cast<double>(ReplicatedAcceleration.AccelXYMagnitude) * MaxAcceleration / 255.0;
	const double AccelXYRadians =
		static_cast<double>(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;

	FVector UnpackedAcceleration = FVector::ZeroVector;
	FMath::PolarToCartesian(
		AccelXYMagnitude,
		AccelXYRadians,
		UnpackedAcceleration.X,
		UnpackedAcceleration.Y);
	UnpackedAcceleration.Z =
		static_cast<double>(ReplicatedAcceleration.AccelZ) * MaxAcceleration / 127.0;

	Movement->SetReplicatedAcceleration(UnpackedAcceleration);
}

void AShootCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// MovementMode 可能在网络校正、重生或 Experience 初始化后才第一次变为 Swimming；
	// Tick 作为兜底只负责补启动画，不参与移动判定，移动仍完全由 CharacterMovement 权威处理。
	UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!Movement)
	{
		return;
	}

	if (Movement->IsSwimming())
	{
		if (!ActiveSwimmingMontage && !bSwimmingEntryAnimationPending)
		{
			bSwimmingEntryAnimationPending = true;
		}
		UpdateSwimmingAnimation();
	}
	else if (Movement->IsFalling())
	{
		UpdateFallingDiveAnimation();
	}
	else if (bSwimmingPreDiveAnimationActive)
	{
		// OnMovementModeChanged 是主路径；这里处理网络校正或初始化顺序导致的漏回调。
		StopFallingDiveAnimation();
	}
}

void AShootCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!Movement)
	{
		return;
	}

	if (Movement->IsSwimming())
	{
		const bool bWasPreDiveAnimationActive = bSwimmingPreDiveAnimationActive;
		bSwimmingPreDiveAnimationActive = false;
		bSwimmingDiveEntry = PrevMovementMode == MOVE_Falling;

		if (bWasPreDiveAnimationActive)
		{
			// 预播放的跳水 Montage 已经是同一套 FullBody 动画，入水时不能重新播放 Start，
			// 否则角色会在水面处重新做一次起跳动作。Start 未播完时继续让它自然收尾。
			bSwimmingEntryAnimationPending = false;
			bSwimmingEntryMontagePlaying = false;
			if (ActiveSwimmingMontage && ActiveSwimmingSequence == SwimmingDiveStartAnimation)
			{
				if (USkeletalMeshComponent* MeshComponent = GetMesh())
				{
					if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
					{
						bSwimmingEntryMontagePlaying =
							AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage);
					}
				}
			}
		}
		else
		{
			bSwimmingEntryAnimationPending = true;
			bSwimmingEntryMontagePlaying = false;
		}

		// 进入水体时引擎可能已经把下落速度吸收到水面处理里，因此用前一移动模式
		// 判断“跳入/坠入”，不能依赖回调时刻的 Velocity.Z。
		UpdateSwimmingAnimation();
	}
	else if (PrevMovementMode == MOVE_Swimming)
	{
		StopSwimmingAnimation();
	}
	else if (bSwimmingPreDiveAnimationActive)
	{
		StopFallingDiveAnimation();
	}
}

void AShootCharacter::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
	AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	ShootPlayerState->AddToAttributePoints(InAttributePoints);
}


void AShootCharacter::AddToSpellPoints_Implementation(int32 InSpellPoints)
{
	AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	ShootPlayerState->AddToSpellPoints(InSpellPoints);
}

int32 AShootCharacter::GetAttributePoints_Implementation() const
{
	AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	return ShootPlayerState->GetAttributePoints();
}

int32 AShootCharacter::GetPlayerLevel_Implementation()
{
	const AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	return ShootPlayerState->GetPlayerLevel();
}

UShootInventoryManagerComponent* AShootCharacter::GetInventoryManagerComponent() const
{
	if (AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>())
	{
		return ShootPlayerState->GetInventoryManagerComponent();
	}
	return nullptr;
}

void AShootCharacter::Die(const FVector& DeathImpulse)
{
	Super::Die(DeathImpulse);

	/*FTimerDelegate DeathTimerDelegate;
	DeathTimerDelegate.BindLambda([this]()
	{
		AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
		if (AuraGM)
		{
			AuraGM->PlayerDied(this);
		}
	});
	GetWorldTimerManager().SetTimer(DeathTimer, DeathTimerDelegate, DeathTime, false);
	TopDownCameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);*/
}

void AShootCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Mutable 会在初始化时给正式 CC Mesh 设置对应性别的主 AnimBP。
	// BeginPlay 可能早于该步骤或 Equipment 复制；这里先尝试，后续外观初始化和 OnEquipped 都会再次刷新。
	RefreshWeaponPresentationFromEquipment();
}


//////////////////////////////////////////////////////////////////////////
// Input

void AShootCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AShootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UShootInputComponent* ShootInputComponent = Cast<UShootInputComponent>(PlayerInputComponent))
	{

		//Running
		ShootInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &AShootCharacter::StartRunning);
		ShootInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &AShootCharacter::EndRunning);

		//Crouch
		ShootInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this,
		                                &AShootCharacter::CrouchButtonPressed);

		// Moving
		ShootInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShootCharacter::Move);

		// Looking
		ShootInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShootCharacter::Look);
		ShootInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed,
		                                        &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);

		//openMenu
		ShootInputComponent->BindAction(OpenMenuAction, ETriggerEvent::Started, this, &AShootCharacter::ShowMenuWidget);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C+ file."
		       ), *GetNameSafe(this));
	}
}

void AShootCharacter::StartRunning()
{
	bIsRunning = true;
	//客户端预测
	UpdateMovementSpeedFromMultiplier(CurrentMoveSpeedMultiplier);
}

void AShootCharacter::EndRunning()
{
	bIsRunning = false;
	//客户端预测
	UpdateMovementSpeedFromMultiplier(CurrentMoveSpeedMultiplier);
}

void AShootCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

bool AShootCharacter::CanJumpInternal_Implementation() const
{
	// ACharacter 默认会在蹲伏时让 CanJump 返回 false，导致 Jump GA 无法激活，
	// 也就永远执行不到 GA 内部的 UnCrouch。Lyra 在 Character 层只移除这项检查，
	// 其余落地、二段跳次数和 JumpMaxHoldTime 等规则仍由 JumpIsAllowedInternal 统一校验。
	return JumpIsAllowedInternal();
}

bool AShootCharacter::IsSwimmingAllowedByExperience() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const UShootExperienceManagerComponent* ExperienceManager =
		GameState ? GameState->FindComponentByClass<UShootExperienceManagerComponent>() : nullptr;
	const UShootExperienceDefinition* Experience =
		ExperienceManager ? ExperienceManager->GetCurrentExperience() : nullptr;

	// Experience 加载前保留引擎默认行为，避免 Pawn/Experience 的初始化顺序造成一次性错误状态。
	return !Experience || Experience->bAllowSwimming;
}

bool AShootCharacter::IsSwimming() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	return Movement && Movement->IsSwimming();
}

UAnimMontage* AShootCharacter::PlaySwimmingAnimation(UAnimSequence* Animation, int32 InLoopCount,
	float BlendInTime, float BlendOutTime)
{
	if (!Animation)
	{
		return nullptr;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return nullptr;
	}

	return AnimInstance->PlaySlotAnimationAsDynamicMontage(
		Animation,
		ShootSwimming::FullBodySlot,
		FMath::Max(BlendInTime, 0.0f),
		FMath::Max(BlendOutTime, 0.0f),
		1.0f,
		FMath::Max(InLoopCount, 1));
}

bool AShootCharacter::TryGetPredictedWaterEntryTime(float& OutTimeToWater) const
{
	OutTimeToWater = 0.0f;

	const UWorld* World = GetWorld();
	const UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!World || !Movement || !Movement->IsFalling() || !IsSwimmingAllowedByExperience())
	{
		return false;
	}

	const UCapsuleComponent* CharacterCapsule = GetCapsuleComponent();
	if (!CharacterCapsule)
	{
		return false;
	}

	const FVector StartLocation = GetActorLocation();
	const FVector InitialVelocity = Movement->Velocity;
	const float GravityZ = Movement->GetGravityZ();
	const FCollisionShape CharacterShape = FCollisionShape::MakeCapsule(
		CharacterCapsule->GetScaledCapsuleRadius(),
		CharacterCapsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ShootCharacter_PredictDive),
		false,
		this);

	FVector PreviousLocation = StartLocation;
	for (float Time = ShootSwimming::PreDivePredictionStep;
		 Time <= ShootSwimming::PreDivePredictionHorizon;
		 Time += ShootSwimming::PreDivePredictionStep)
	{
		const FVector PredictedLocation = StartLocation + InitialVelocity * Time + FVector(
			0.0f,
			0.0f,
			0.5f * GravityZ * FMath::Square(Time));
		const float PredictedVerticalVelocity = InitialVelocity.Z + GravityZ * Time;

		FHitResult BlockingHit;
		if (World->SweepSingleByChannel(
			BlockingHit,
			PreviousLocation,
			PredictedLocation,
			GetActorQuat(),
			ECC_Pawn,
			CharacterShape,
			QueryParams))
		{
			// 先撞到地面、墙或其他阻挡物时，实际轨迹不会到达水体。
			return false;
		}

		if (PredictedVerticalVelocity <= 0.0f &&
			ShootSwimming::IsWaterVolumeAtLocation(*World, PredictedLocation))
		{
			OutTimeToWater = Time;
			return true;
		}

		PreviousLocation = PredictedLocation;
	}

	return false;
}

void AShootCharacter::UpdateFallingDiveAnimation()
{
	UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!Movement || !Movement->IsFalling() || !IsSwimmingAllowedByExperience())
	{
		StopFallingDiveAnimation();
		return;
	}

	float TimeToWater = 0.0f;
	if (!TryGetPredictedWaterEntryTime(TimeToWater))
	{
		StopFallingDiveAnimation();
		return;
	}

	if (!SwimmingDiveStartAnimation)
	{
		StopFallingDiveAnimation();
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (!bSwimmingPreDiveAnimationActive)
	{
		const float DiveStartDuration = SwimmingDiveStartAnimation->GetPlayLength();
		if (TimeToWater > DiveStartDuration + ShootSwimming::PreDiveTriggerLeadTime)
		{
			return;
		}

		if (ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage))
		{
			AnimInstance->Montage_Stop(0.1f, ActiveSwimmingMontage);
		}

		ActiveSwimmingMontage = PlaySwimmingAnimation(SwimmingDiveStartAnimation, 1, 0.1f, 0.1f);
		ActiveSwimmingSequence = SwimmingDiveStartAnimation;
		bSwimmingPreDiveAnimationActive = ActiveSwimmingMontage != nullptr;
		return;
	}

	const bool bActiveMontagePlaying =
		ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage);
	if (bActiveMontagePlaying && ActiveSwimmingSequence == SwimmingDiveStartAnimation)
	{
		return;
	}
	if (bActiveMontagePlaying && ActiveSwimmingSequence == SwimmingDiveLoopAnimation)
	{
		return;
	}

	if (bActiveMontagePlaying)
	{
		AnimInstance->Montage_Stop(0.1f, ActiveSwimmingMontage);
	}

	if (SwimmingDiveLoopAnimation)
	{
		ActiveSwimmingMontage = PlaySwimmingAnimation(
			SwimmingDiveLoopAnimation,
			ShootSwimming::LoopCount,
			0.1f,
			0.1f);
		ActiveSwimmingSequence = SwimmingDiveLoopAnimation;
	}
}

void AShootCharacter::StopFallingDiveAnimation()
{
	if (!bSwimmingPreDiveAnimationActive)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (AnimInstance && ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage))
	{
		AnimInstance->Montage_Stop(0.1f, ActiveSwimmingMontage);
	}

	ActiveSwimmingMontage = nullptr;
	ActiveSwimmingSequence = nullptr;
	bSwimmingPreDiveAnimationActive = false;
}

void AShootCharacter::UpdateSwimmingAnimation()
{
	UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(GetCharacterMovement());
	if (!Movement || !Movement->IsSwimming())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (bSwimmingEntryAnimationPending)
	{
		// 重入水体前先清掉离水过渡，避免同一个 FullBody Slot 同时叠两个动态 Montage。
		if (ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage))
		{
			AnimInstance->Montage_Stop(0.1f, ActiveSwimmingMontage);
		}
		ActiveSwimmingMontage = nullptr;
		ActiveSwimmingSequence = nullptr;

		UAnimSequence* EntryAnimation =
			bSwimmingDiveEntry && SwimmingDiveStartAnimation
				? SwimmingDiveStartAnimation
				: SwimmingStartAnimation;
		bSwimmingEntryAnimationPending = false;
		bSwimmingEntryMontagePlaying = false;

		if (EntryAnimation)
		{
			ActiveSwimmingMontage = PlaySwimmingAnimation(EntryAnimation, 1, 0.15f, 0.15f);
			ActiveSwimmingSequence = EntryAnimation;
			bSwimmingEntryMontagePlaying = ActiveSwimmingMontage != nullptr;
			if (bSwimmingEntryMontagePlaying)
			{
				return;
			}
		}
	}

	const bool bEntryMontagePlaying =
		bSwimmingEntryMontagePlaying && ActiveSwimmingMontage &&
		AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage);
	if (bEntryMontagePlaying)
	{
		return;
	}
	bSwimmingEntryMontagePlaying = false;

	const bool bMoving =
		Movement->Velocity.SizeSquared2D() > FMath::Square(ShootSwimming::MovementThreshold) ||
		Movement->GetCurrentAcceleration().SizeSquared2D() > FMath::Square(ShootSwimming::MovementThreshold);

	UAnimSequence* DesiredAnimation = nullptr;
	if (bSwimmingDiveEntry && SwimmingDiveLoopAnimation && Movement->Velocity.Z < -50.0f)
	{
		DesiredAnimation = SwimmingDiveLoopAnimation;
	}
	else if (bMoving)
	{
		DesiredAnimation = SwimmingForwardAnimation;
	}
	else
	{
		DesiredAnimation = SwimmingIdleAnimation;
	}

	if (!DesiredAnimation)
	{
		return;
	}

	const bool bActiveMontagePlaying =
		ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage);
	if (bActiveMontagePlaying && ActiveSwimmingSequence == DesiredAnimation)
	{
		return;
	}

	if (bActiveMontagePlaying)
	{
		AnimInstance->Montage_Stop(0.15f, ActiveSwimmingMontage);
	}

	ActiveSwimmingMontage = PlaySwimmingAnimation(
		DesiredAnimation,
		ShootSwimming::LoopCount,
		0.2f,
		0.2f);
	ActiveSwimmingSequence = DesiredAnimation;
}

void AShootCharacter::StopSwimmingAnimation()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (AnimInstance && ActiveSwimmingMontage && AnimInstance->Montage_IsPlaying(ActiveSwimmingMontage))
	{
		AnimInstance->Montage_Stop(0.15f, ActiveSwimmingMontage);
	}

	ActiveSwimmingMontage = nullptr;
	ActiveSwimmingSequence = nullptr;
	bSwimmingEntryAnimationPending = false;
	bSwimmingDiveEntry = false;
	bSwimmingEntryMontagePlaying = false;
	bSwimmingPreDiveAnimationActive = false;

	// 离开 MOVE_Swimming 后立即把 FullBody 游泳 Montage 交还给正式 locomotion。
	// 旧的 Swimming_Crawl_Fwd_Stop 约 3 秒且覆盖全身，即使移动模式已经回到 Walking，
	// 也会继续把角色锁在水平游泳姿势，因此这里不能再播放离水 Montage。
}

void AShootCharacter::MulticastPlayPose_Implementation(UAnimSequence* Animation, FName SlotName, float PlayRate,
	float BlendInTime, float BlendOutTime, int32 LoopCount)
{
	if (!Animation)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	// 姿势按 AnimSequence 动态包装成 Montage 播放；播完后 AnimBP 会自然回到原来的 locomotion。
	// 前提是男女正式 CC 主 AnimBP 继承 ABP_ShootMannequinBase；
	// 该项目基类的 AnimGraph 固定保留同名 Slot，不能由武器子层覆盖。
	AnimInstance->PlaySlotAnimationAsDynamicMontage(
		Animation,
		SlotName.IsNone() ? FName(TEXT("DefaultSlot")) : SlotName,
		FMath::Max(BlendInTime, 0.f),
		FMath::Max(BlendOutTime, 0.f),
		FMath::Max(PlayRate, 0.01f),
		FMath::Max(LoopCount, 1));
}

void AShootCharacter::ClientSyncMaxSpeed_Implementation(float NewMaxSpeed)
{
	if (!IsLocallyControlled())
	{
		GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;
	}
}

void AShootCharacter::ServerSetMaxSpeed_Implementation(float NewMaxSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;
	ClientSyncMaxSpeed(NewMaxSpeed); // 广播给所有客户端
}

UMutableAppearanceComponent* AShootCharacter::GetAppearanceComponent()
{
	return AppearanceComponent;
}

void AShootCharacter::InitAppearanceComponent()
{
	AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	ShootPlayerState->OnAppearanceTagsChanged.RemoveAll(this);
	ShootPlayerState->OnAppearanceTagsChanged.AddUObject(
		this, &ThisClass::HandleAppearanceAnimationTagsChanged);

	// Mutable 的网格更新异步完成，SwitchGender 中设置主 AnimBP 后仍可能继续替换 HeadMesh。
	// 只有 UpdatedDelegate 到达时，正式 CC Mesh 与主 AnimInstance 才是最终实例；
	// 在这里重链当前 Equipment 武器层，避免同步初始化阶段 Link 到随后被销毁的旧 AnimInstance。
	AppearanceComponent->OnMutableSkeletalMeshUpdated.RemoveAll(this);
	AppearanceComponent->OnMutableSkeletalMeshUpdated.AddUObject(
		this, &ThisClass::RefreshWeaponPresentationFromEquipment);

	AppearanceComponent->InitializeComponents(
		HeadCSkeletalComponent,
		BodyCSkeletalComponent,
		GetMesh(),
		BodyMesh,
		ShootPlayerState);

	// Mutable 完成当前性别选择后，以同一 PlayerState 身份刷新武器 AnimLayer；不依赖下一帧 SaveGame 重应用。
	RefreshWeaponPresentationFromEquipment();
}

FGameplayTagContainer AShootCharacter::BuildWeaponAnimationCosmeticTags() const
{
	FGameplayTagContainer CosmeticTags;
	if (const AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>())
	{
		const ECharacterGender Gender = ShootPlayerState->GetCharacterGender();
		CosmeticTags.AppendTags(ShootPlayerState->GetAppearanceTags(Gender));

		if (Gender == ECharacterGender::FEMALE)
		{
			// 与 Lyra 的 B_WeaponInstance_* 配置保持同一标签语义；请求失败时仍安全回退 DefaultLayer。
			const FGameplayTag FeminineStyleTag = FGameplayTag::RequestGameplayTag(
				FName(TEXT("Cosmetic.AnimationStyle.Feminine")), false);
			if (FeminineStyleTag.IsValid())
			{
				CosmeticTags.AddTag(FeminineStyleTag);
			}
		}
	}

	return CosmeticTags;
}

void AShootCharacter::ApplyWeaponPresentation(const FShootAnimLayerSelectionSet& SelectionSet, bool bArmed)
{
	// 移动朝向不依赖动画资产是否已经加载；即使正式 CC AnimInstance 尚未创建，也先维持正确玩法状态。
	ApplyArmedMovementMode(bArmed);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	UAnimInstance* CharacterAnimInstance = CharacterMesh->GetAnimInstance();
	if (!CharacterAnimInstance)
	{
		return;
	}

	const TSubclassOf<UAnimInstance> SelectedLayer =
		SelectionSet.SelectBestLayer(BuildWeaponAnimationCosmeticTags());
	if (!SelectedLayer)
	{
		return;
	}

	// 武器层、Hair 层、Shoe 层实现不同接口；LinkAnimClassLayers 只替换同接口实现。
	// 与 Lyra 一致，不先 Unlink，避免装卸时制造一帧空姿势。
	// 这里不能只按 Layer 类缓存并跳过：Mutable 会重建 CharacterMesh0 的 AnimInstance，
	// 新实例即使需要相同类也尚未链接任何 Item Layer。装备、卸装和 Mutable 完成回调
	// 都必须把唯一选中层明确应用到当前可见 AnimInstance。
	CharacterAnimInstance->LinkAnimClassLayers(SelectedLayer);
}

void AShootCharacter::RefreshWeaponPresentationFromEquipment()
{
	// Lyra 的 QuickBar OnRep 只发 UI 消息；跨网络动画由 Pawn EquipmentList 的
	// PreReplicatedRemove/PostReplicatedAdd 调用 WeaponInstance 装卸回调。
	if (EquipmentManagerComponent)
	{
		if (UShootWeaponInstance* WeaponInstance = Cast<UShootWeaponInstance>(
			EquipmentManagerComponent->GetFirstInstanceOfType(UShootWeaponInstance::StaticClass())))
		{
			WeaponInstance->RefreshAnimLayer(true);
			return;
		}
	}

	ApplyWeaponPresentation(DefaultUnarmedAnimSet, false);
}

void AShootCharacter::ApplyArmedMovementMode(bool bArmed)
{
	bArmedPresentationActive = bArmed;
	RefreshMovementFacingMode();
}

void AShootCharacter::SetFirstPersonCameraActive(bool bActive)
{
	bFirstPersonCameraActive = bActive;
	if (USkeletalMeshComponent* HeadMesh = GetMesh())
	{
		// Mutable 的 GetMesh() 是 Head 渲染组件，BodyMesh 是独立身体组件。
		// Owner No See 是按每个视图的 ViewActor 过滤，不会让其他本地分屏玩家看不到该角色。
		HeadMesh->SetOwnerNoSee(bActive);
	}
	RefreshMovementFacingMode();
}

void AShootCharacter::RefreshMovementFacingMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// 当前 QuickBar 有枪时，角色必须面向控制器/准星并使用 Lyra 四向侧移；
	// 第一人称即使空手也必须跟随控制器。只有第三人称空手恢复朝移动方向转身，
	// 保留家园观察角色正脸的体验。
	const bool bFaceController = bArmedPresentationActive || bFirstPersonCameraActive;
	bUseControllerRotationYaw = bFaceController;
	Movement->bOrientRotationToMovement = !bFaceController;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, bFaceController ? 720.0f : 500.0f, 0.0f);
}

void AShootCharacter::HandleAppearanceAnimationTagsChanged(ECharacterGender InGender,
	const FGameplayTagContainer& NewAppearanceTags)
{
	const AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	if (!ShootPlayerState || ShootPlayerState->GetCharacterGender() != InGender)
	{
		return;
	}

	RefreshWeaponPresentationFromEquipment();
}

void AShootCharacter::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();
	AShootPlayerState* ShootPlayerState = GetPlayerState<AShootPlayerState>();
	check(ShootPlayerState);
	ShootPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(ShootPlayerState, this);
	Cast<UShootAbilitySystemComponent>(ShootPlayerState->GetAbilitySystemComponent())->AbilityActorInfoSet();
	AbilitySystemComponent = ShootPlayerState->GetAbilitySystemComponent();
	AttributeSet = ShootPlayerState->GetAttributeSet();

	// 绑定移动速度倍率变化，保持移速与 Buff 同步
	if (AbilitySystemComponent && AttributeSet)
	{
		if (const UShootAttributeSet* ShootAttrSet = Cast<UShootAttributeSet>(AttributeSet))
		{
			CurrentMoveSpeedMultiplier = ShootAttrSet->GetMoveSpeedMultiplier();
			UpdateMovementSpeedFromMultiplier(CurrentMoveSpeedMultiplier);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				ShootAttrSet->GetMoveSpeedMultiplierAttribute()).AddUObject(this, &ThisClass::OnMoveSpeedMultiplierChanged);

			// 默认死亡链是终结死亡，不把 Health=0 偷换成所有模式共有的“倒地”。
			// 需要救援玩法的 PVE Experience 后续由模式 AbilitySet/GA 显式开启交互组件。
			SetReviveInteractableEnabled(false);
		}
	}

	if (AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(FShootGameplayTags::Get().Faction_Player))
	{
		AbilitySystemComponent->AddLooseGameplayTag(FShootGameplayTags::Get().Faction_Player);
	}
	if (Controller && Controller->IsLocalController())
	{
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (LocalPlayer && LocalPlayer->Implements<UAttributeViewModelInterface>())
		{
			UAttributeViewModel* AttributeViewModel = IAttributeViewModelInterface::Execute_GetAttributeViewModel(
				LocalPlayer);
			if (AttributeViewModel)
			{
				AttributeViewModel->InitializeWithPlayerState(ShootPlayerState);
			}
		}
	}
	//OnAscRegistered.Broadcast(AbilitySystemComponent);
}

void AShootCharacter::OnMoveSpeedMultiplierChanged(const FOnAttributeChangeData& Data)
{
	CurrentMoveSpeedMultiplier = FMath::Max(0.f, Data.NewValue);
	UpdateMovementSpeedFromMultiplier(CurrentMoveSpeedMultiplier);
}

void AShootCharacter::SetAimingMovementSpeedMultiplier(float NewMultiplier)
{
	CurrentAimingMoveSpeedMultiplier = FMath::Max(0.f, NewMultiplier);
	UpdateMovementSpeedFromMultiplier(CurrentMoveSpeedMultiplier);
}

void AShootCharacter::UpdateMovementSpeedFromMultiplier(float NewMultiplier)
{
	// 说明：服务器权威设置 MaxWalkSpeed，同时同步给所有客户端；
	// 本地玩家先做预测，随后由服务器广播校正。
	CurrentMoveSpeedMultiplier = FMath::Max(0.f, NewMultiplier);
	const float BaseSpeed = bIsRunning ? BaseRunSpeed : BaseWalkSpeed;
	const float NewMaxSpeed = BaseSpeed * CurrentMoveSpeedMultiplier * CurrentAimingMoveSpeedMultiplier;

	if (HasAuthority())
	{
		GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;
		ClientSyncMaxSpeed(NewMaxSpeed);
		return;
	}

	if (IsLocallyControlled())
	{
		GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;
		ServerSetMaxSpeed(NewMaxSpeed);
	}
}

void AShootCharacter::LoadProgress()
{
	// 属性属于副本数据：每次 Pawn 生成统一初始化，升级/属性成长不落存档。
	// 玩家公共能力与模式技能统一由 Experience AbilitySet 授予；这里禁止再从 Character 数组形成平行授予链。
	// AShootCharacterBase::StartupAbilities 仍保留给 ASC 归属于 Pawn 的 AI 使用。
	// 2026-08-23: 移除存档恢复路径(旧实现从存档恢复 Level/XP/点数/属性/技能，且属性从未写入存档，
	// 恢复出的 Strength/Vitality=0 会把 MMC 算出的 MaxHealth 归零——切换男女血条清零的根因)。
	InitializeDefaultAttributes();
}

bool AShootCharacter::GetOrientRotationToMovement()
{
	return GetCharacterMovement()->bOrientRotationToMovement;
}

void AShootCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
										 FString::Printf(TEXT("AbilityInputTagPressed: %s"), *InputTag.ToString()));
	}*/
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FShootGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}
	if (GetASC()) GetASC()->AbilityInputTagPressed(InputTag);
}

void AShootCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
										 FString::Printf(TEXT("AbilityInputTagPressed: %s"), *InputTag.ToString()));
	}*/
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FShootGameplayTags::Get().Player_Block_InputReleased))
	{
		return;
	}
	/*if (!InputTag.MatchesTagExact(FShootGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
		return;
	}*/

	if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
}

void AShootCharacter::AbilityInputTagHeld(FGameplayTag InputTag)
{
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue,
		                                 FString::Printf(TEXT("AbilityInputTagHeld: %s"), *InputTag.ToString()));
	}*/
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FShootGameplayTags::Get().Player_Block_InputHeld))
	{
		return;
	}
	/*if (!InputTag.MatchesTagExact(FShootGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
		return;
	}*/
	if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
}

UShootAbilitySystemComponent* AShootCharacter::GetASC()
{
	if (ShootAbilitySystemComponent == nullptr)
	{
		ShootAbilitySystemComponent = Cast<UShootAbilitySystemComponent>(GetAbilitySystemComponent());
	}
	return ShootAbilitySystemComponent;
}

void AShootCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 陆地移动保持水平朝向；游泳时复用同一个 MoveAction，并把镜头俯仰带入前进向量，
		// 这样不增加一套设备相关输入分支，抬头前进上浮、低头前进下潜。
		const FVector ForwardDirection = IsSwimming()
			? FRotationMatrix(Rotation).GetUnitAxis(EAxis::X)
			: FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AShootCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
