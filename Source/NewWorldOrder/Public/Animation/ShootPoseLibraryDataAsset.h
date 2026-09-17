// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "Engine/DataAsset.h"
#include "ShootPoseLibraryDataAsset.generated.h"

class UAnimSequence;
class UTexture2D;

USTRUCT(BlueprintType)
struct FShootPoseLibraryEntry
{
	GENERATED_BODY()

	/** 姿势库 UI 展示名；按钮、列表文本都从这里读，不在 Widget 里写死。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose")
	FText DisplayName;

	/** 可选缩略图；UMG 只负责展示这个配置，不负责决定姿势播放规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** UNKNOWN 表示男女都允许；指定性别时服务器会按当前 PlayerState 性别校验。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose")
	ECharacterGender CompatibleGender = ECharacterGender::UNKNOWN;

	/** 姿势或短动画资源。当前 /Game/Assets/Animations/Girl 扫描到的是 AnimSequence。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose")
	TObjectPtr<UAnimSequence> Animation = nullptr;

	/** AnimBP 中必须有同名 Slot 节点承接动态 Montage；默认使用 UE 常见的 DefaultSlot。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose")
	FName SlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose", meta=(ClampMin="0.01"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose", meta=(ClampMin="0.0"))
	float BlendInTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose", meta=(ClampMin="0.0"))
	float BlendOutTime = 0.2f;

	/** 1 表示播完回到 AnimBP 原本 locomotion；大于 1 可用于循环动作调试。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pose", meta=(ClampMin="1"))
	int32 LoopCount = 1;
};

/**
 * 姿势库数据资产。
 * C++ 只定义可复制、可校验的数据结构；具体姿势列表由蓝图/DataAsset 配置，避免把动画资源路径写死进代码。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootPoseLibraryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PoseLibrary")
	TArray<FShootPoseLibraryEntry> GetPoseEntries() const { return PoseEntries; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PoseLibrary")
	bool IsValidPoseIndex(int32 PoseIndex) const { return PoseEntries.IsValidIndex(PoseIndex); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PoseLibrary")
	bool IsPosePlayableForGender(int32 PoseIndex, ECharacterGender CurrentGender) const;

	const FShootPoseLibraryEntry* GetPoseEntry(int32 PoseIndex) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PoseLibrary", meta=(AllowPrivateAccess="true", TitleProperty="DisplayName"))
	TArray<FShootPoseLibraryEntry> PoseEntries;
};
