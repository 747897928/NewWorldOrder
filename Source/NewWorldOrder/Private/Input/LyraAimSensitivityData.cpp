// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/LyraAimSensitivityData.h"

#include "Settings/LyraSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAimSensitivityData)

ULyraAimSensitivityData::ULyraAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 由 Data Asset 配置；项目可以在不改 C++ 的情况下调节手柄曲线。
}

const float ULyraAimSensitivityData::SensitivtyEnumToFloat(const ELyraGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}

