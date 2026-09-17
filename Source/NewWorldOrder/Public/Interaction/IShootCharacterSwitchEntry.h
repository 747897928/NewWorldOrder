// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "UObject/Interface.h"
#include "IShootCharacterSwitchEntry.generated.h"

class APawn;

UINTERFACE(MinimalAPI)
class UShootCharacterSwitchEntry : public UInterface
{
	GENERATED_BODY()
};

/**
 * 世界内角色切换入口接口。
 * 入口物或另一位主角 NPC 只要实现这个接口，就能复用同一条“世界入口桥接到共享切换后端”的 C++ 链路。
 * 当前仓库里有两套可直接复用的骨架：
 * - `AShootCharacterSwitchEntryActorBase`：适合站点 / 切换点这类独立 Actor
 * - `AShootCharacterSwitchNPCBase`：适合另一位主角 NPC 入口
 * 这里故意不把切换业务写在入口对象里，入口对象只负责告诉系统“当前应该切到谁”。
 */
class NEWWORLDORDER_API IShootCharacterSwitchEntry
{
	GENERATED_BODY()

public:
	virtual bool ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const = 0;
};
