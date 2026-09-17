// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "LyraAimSensitivityData.generated.h"

enum class ELyraGamepadSensitivity : uint8;

class UObject;

/** Defines a set of gamepad sensitivity to a float value. */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Aim Sensitivity Data", ShortTooltip = "Data asset mapping gamepad sensitivity presets to scalar values."))
class NEWWORLDORDER_API ULyraAimSensitivityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULyraAimSensitivityData(const FObjectInitializer& ObjectInitializer);
	
	const float SensitivtyEnumToFloat(const ELyraGamepadSensitivity InSensitivity) const;
	
protected:
	/** Map of SensitivityMap settings to their corresponding float */
	// 数值目录由 DA_AimSensitivity 在编辑器中配置，禁止在构造函数里复制 Lyra 的硬编码表。
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ELyraGamepadSensitivity, float> SensitivityMap;
};
