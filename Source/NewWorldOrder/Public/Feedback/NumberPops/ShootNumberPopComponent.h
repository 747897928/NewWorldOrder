// Copyright ZhaoYiJie

#pragma once

#include "Components/ControllerComponent.h"
#include "GameplayTagContainer.h"

#include "ShootNumberPopComponent.generated.h"

/**
 * 一次伤害数字表现请求。
 * 结构与 Lyra 的 FLyraNumberPopRequest 对齐，但网络只传给造成伤害的玩家 Controller，
 * 不广播给受击者、Listen Server 主机或其他本地分屏玩家。
 */
USTRUCT(BlueprintType)
struct NEWWORLDORDER_API FShootNumberPopRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot|Number Pops")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot|Number Pops")
	FGameplayTagContainer SourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot|Number Pops")
	FGameplayTagContainer TargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot|Number Pops")
	int32 NumberToDisplay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot|Number Pops")
	bool bIsCriticalDamage = false;
};

/**
 * Controller 私有的伤害数字表现入口。
 * 与 Lyra 一样把生命周期放在 PlayerController，而不是 Pawn 或每次命中的临时 Actor 上。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootNumberPopComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Shoot|Number Pops")
	virtual void AddNumberPop(const FShootNumberPopRequest& NewRequest) {}
};
