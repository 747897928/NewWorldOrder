// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Interaction/Abilities/ShootGA_Interaction_AcquireSkill.h"

#include "ShootGA_Interaction_GrantRandomSkill.generated.h"

/**
 * 旧资产兼容名。新交互来源应引用 UShootGA_Interaction_AcquireSkill；实际事务已收敛到统一技能来源接口。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_GrantRandomSkill : public UShootGA_Interaction_AcquireSkill
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_GrantRandomSkill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};
