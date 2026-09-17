// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InputModifiers.h"

#include "UObject/UnrealType.h"
#include "LyraInputModifiers.generated.h"

struct FInputActionValue;

class FProperty;
class UEnhancedPlayerInput;
class ULyraAimSensitivityData;
class UObject;

/** 
*  Scales input basedon a double property in the SharedUserSettings
*/
UCLASS(NotBlueprintable, meta = (DisplayName = "Setting Based Scalar"))
class NEWWORLDORDER_API ULyraSettingBasedScalar : public UInputModifier
{
	GENERATED_BODY()

public:

	/** Name of the property that will be used to clamp the X Axis of this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FName XAxisScalarSettingName = NAME_None;

	/** Name of the property that will be used to clamp the Y Axis of this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FName YAxisScalarSettingName = NAME_None;

	/** Name of the property that will be used to clamp the Z Axis of this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FName ZAxisScalarSettingName = NAME_None;
	
	/** Set the maximium value of this setting on each axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FVector MaxValueClamp = FVector(10.0, 10.0, 10.0);
	
	/** Set the minimum value of this setting on each axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FVector MinValueClamp = FVector::ZeroVector;

	/** 用于鼠标 Look 映射；ADS 时再乘 SharedSettings.TargetingMultiplier。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	bool bApplyTargetingMultiplierWhenADS = false;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

	/** FProperty Cache that will be populated with any found FProperty's on the settings class so that we don't need to look them up each frame */
	TArray<const FProperty*> PropertyCache;
};

/** Represents which stick that this deadzone is for, either the move or the look stick */
UENUM()
enum class EDeadzoneStick : uint8
{
	/** Deadzone for the movement stick */
	MoveStick = 0,

	/** Deadzone for the looking stick */
	LookStick = 1,
};

/**
 * This is a deadzone input modifier that will have it's thresholds driven by what is in the Lyra Shared game settings. 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Settings Driven Dead Zone"))
class NEWWORLDORDER_API ULyraInputModifierDeadZone : public UInputModifier
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, Config)
	EDeadZoneType Type = EDeadZoneType::Radial;
	
	// Threshold above which input is clamped to 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, Config)
	float UpperThreshold = 1.0f;

	/** Which stick this deadzone is for. This controls which setting will be used when calculating the deadzone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, Config)
	EDeadzoneStick DeadzoneStick = EDeadzoneStick::MoveStick;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

	// Visualize as black when unmodified. Red when blocked (with differing intensities to indicate axes)
	// Mirrors visualization in https://www.gamasutra.com/blogs/JoshSutphin/20130416/190541/Doing_Thumbstick_Dead_Zones_Right.php.
	virtual FLinearColor GetVisualizationColor_Implementation(FInputActionValue SampleValue, FInputActionValue FinalValue) const override;
};

/** The type of targeting sensitity that should be considered */
UENUM()
enum class ELyraTargetingType : uint8
{
	/** Sensitivity to be applied why normally looking around */
	Normal = 0,

	/** The sensitivity that should be applied while Aiming Down Sights */
	ADS = 1,
};

/** Applies a scalar modifier based on the current gamepad settings in Lyra Shared game settings.  */
UCLASS(NotBlueprintable, meta = (DisplayName = "Gamepad Sensitivity"))
class NEWWORLDORDER_API ULyraInputModifierGamepadSensitivity : public UInputModifier
{
	GENERATED_BODY()
public:
	
	/** The type of targeting to use for this Sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, Config)
	ELyraTargetingType TargetingType = ELyraTargetingType::Normal;

	/** 单一 IA_Look 同时服务普通与 ADS；启用后根据项目 GAS 的 ADS Tag 动态选择预设。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, Config)
	bool bUseADSPresetWhenAiming = true;

	/** Asset that gives us access to the float scalar value being used for sensitivty */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AssetBundles="Client,Server"))
	TObjectPtr<const ULyraAimSensitivityData> SensitivityLevelTable;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
};

/** Applies an inversion of axis values based on a setting in the Lyra Shared game settings */
UCLASS(NotBlueprintable, meta = (DisplayName = "Aim Inversion Setting"))
class NEWWORLDORDER_API ULyraInputModifierAimInversion : public UInputModifier
{
	GENERATED_BODY()
	
protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;	
};
