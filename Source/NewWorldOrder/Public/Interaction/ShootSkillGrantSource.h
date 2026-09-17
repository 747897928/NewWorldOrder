// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "ShootSkillGrantSource.generated.h"

class APawn;

/**
 * 局内技能来源的统一事务接口。
 * 世界拾取物、Round 奖励代理和商人都通过同一入口提交技能获取；交互 GA 不依赖任何具体 NPC 或拾取物类型。
 */
UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UShootSkillGrantSource : public UInterface
{
	GENERATED_BODY()
};

class IShootSkillGrantSource
{
	GENERATED_BODY()

public:
	/** 客户端用于展示交互选项，服务器会在真正提交前再次校验。 */
	virtual bool CanGrantSkillToPawn(const APawn* Pawn) const = 0;

	/** 只允许服务器提交；成功时来源自行完成消费、扣费或库存更新。 */
	virtual bool TryGrantSkillToPawn(APawn* Pawn) = 0;
};
