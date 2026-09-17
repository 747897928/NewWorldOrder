// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/CollisionProfile.h"
#include "Interaction/InteractionOption.h"

#include "AbilityTask_WaitForInteractableTargets.generated.h"

class AActor;
class IInteractableTarget;
class UObject;
class UWorld;
struct FCollisionQueryParams;
struct FHitResult;
struct FInteractionQuery;
template <typename InterfaceType> class TScriptInterface;
// 交互选项改变时的委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractableObjectsChangedEvent, const TArray<FInteractionOption>&, InteractableOptions);

/**
 * 等待可交互目标的能力任务基类
 * AbilityTask是GAS中用于执行异步操作的任务类，可以等待特定条件满足
 */
UCLASS(Abstract)
class UAbilityTask_WaitForInteractableTargets : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

public:
	// 当可交互对象发生变化时广播的委托
	UPROPERTY(BlueprintAssignable)
	FInteractableObjectsChangedEvent InteractableObjectsChanged;

protected:
	// 静态射线检测函数
	static void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params);

	// 使用玩家控制器进行瞄准计算
	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	// 将相机射线裁剪到能力范围内
	static bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition);

	// 更新可交互选项
	void UpdateInteractableOptions(const FInteractionQuery& InteractQuery, const TArray<TScriptInterface<IInteractableTarget>>& InteractableTargets);

	// 射线检测使用的碰撞配置文件
	FCollisionProfileName TraceProfile;

	// Does the trace affect the aiming pitch
	// 射线检测是否影响瞄准俯仰角
	bool bTraceAffectsAimPitch = true;

	// 当前的可交互选项列表
	TArray<FInteractionOption> CurrentOptions;
};
