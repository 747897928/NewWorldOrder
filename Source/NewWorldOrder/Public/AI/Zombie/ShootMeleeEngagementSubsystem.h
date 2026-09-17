// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "ShootMeleeEngagementSubsystem.generated.h"

/**
 * 服务器侧近战接敌槽位。
 *
 * 目标 Actor、追击位置和攻击事件必须分开管理：Controller 决定追谁，
 * 本 Subsystem 只负责同一目标周围的有限站位，Gameplay Ability 仍负责真正的攻击和伤害。
 * 预留槽位不写入 Blackboard，也不复制到客户端；它是当前服务器世界内的调度状态。
 */
UCLASS()
class NEWWORLDORDER_API UShootMeleeEngagementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 为攻击者在目标周围申请一个环形槽位。已有槽位会返回原索引，满员时返回 INDEX_NONE。
	 * PreferredDirection 只用于在多个空槽中选择离攻击者当前方位最近的槽位。
	 */
	int32 AcquireSlot(AActor* Target, AActor* Attacker, int32 SlotCount,
		const FVector& PreferredDirection);

	/** 释放攻击者在指定目标上的槽位。 */
	void ReleaseSlot(AActor* Target, AActor* Attacker);

	/** 释放攻击者在本世界内的全部槽位，供死亡、UnPossess 和重生清理使用。 */
	void ReleaseAllForAttacker(AActor* Attacker);

	/** 判断攻击者当前是否持有指定目标的有效槽位。 */
	bool HasSlot(AActor* Target, AActor* Attacker) const;

	/** 根据槽位索引返回相对目标位置的水平偏移。 */
	FVector GetSlotOffset(AActor* Target, int32 SlotIndex, int32 SlotCount, float Radius) const;

protected:
	virtual void Deinitialize() override;

private:
	struct FReservation
	{
		TWeakObjectPtr<AActor> Attacker;
		int32 SlotIndex = INDEX_NONE;
	};

	TMap<TWeakObjectPtr<AActor>, TArray<FReservation>> Reservations;

	void RemoveInvalidReservations();
};
