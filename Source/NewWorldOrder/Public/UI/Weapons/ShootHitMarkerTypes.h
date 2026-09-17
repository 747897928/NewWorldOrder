#pragma once

#include "GameplayTagContainer.h"
#include "ShootHitMarkerTypes.generated.h"

/**
 * 屏幕空间命中点数据：由武器/能力在命中时生成，并转换为屏幕像素坐标
 * 说明：
 * - ScreenPosition 默认使用 PlayerController::ProjectWorldLocationToScreen 得到的绝对屏幕坐标
 * - HitZone 用于区分弱点/护甲等区域，便于 UI 替换贴图
 * - bShowAsSuccess 可做命中/未命中区分（默认 true，后续可扩展不同材质）
 */
USTRUCT(BlueprintType)
struct FShootReticleHitLocation
{
	GENERATED_BODY()

	/** 屏幕空间坐标（像素）。若无效则 HUD 组件会根据 WorldPosition 重新投影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** 命中世界坐标；用于客户端自行转换屏幕坐标或用于调试。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	FVector WorldPosition = FVector::ZeroVector;

	/** 是否包含有效的世界坐标，可供 HUD 再投影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	bool bHasWorldPosition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	FGameplayTag HitZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	bool bShowAsSuccess = true;
};

/**
 * 命中提示消息体：未来可通过 UGameplayMessageSubsystem 发送到 HUD → 准星控件
 * Reticle 控件收到后应调用 HandleHitNotification，把命中点缓存到 Slate 层进行淡出展示。
 */
USTRUCT(BlueprintType)
struct FShootReticleHitNotifyMessage
{
	GENERATED_BODY()

	/** 推送 UI 的攻击者（玩家角色 / 武器拥有者），HUD 会据此筛选是否属于本地玩家。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	TWeakObjectPtr<AActor> SourceActor;

	/** 本次命中的所有屏幕空间标记 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	TArray<FShootReticleHitLocation> HitMarkers;

	/** 是否有至少一个成功命中（用于中心命中提示的快速判断） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	bool bHasSuccessfulHit = true;
};

/**
 * ADS 状态切换消息：来源为玩家角色 / 武器，bIsAds 指明是否正在瞄准
 */
USTRUCT(BlueprintType)
struct FShootReticleADSMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	bool bIsAds = false;
};

/**
 * 击杀消息：本地玩家击杀敌人时播放特殊动画
 */
USTRUCT(BlueprintType)
struct FShootReticleEliminationMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reticle")
	TWeakObjectPtr<AActor> Victim;
};
