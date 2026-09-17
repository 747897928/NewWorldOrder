// Match Skill 冷却 GE：每个主动技能使用独立拥有标签，供 GAS 阻断激活并供四槽 HUD 查询进度。
#pragma once

#include "GameplayEffect.h"

#include "ShootEffect_MatchSkillCooldowns.generated.h"

UCLASS(Abstract)
class NEWWORLDORDER_API UShootEffect_MatchSkillCooldownBase : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	UShootEffect_MatchSkillCooldownBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}

	/** 只能在派生 GE 构造期间调用；命名子对象保证 CDO 初始化稳定。 */
	void ConfigureCooldown(const FObjectInitializer& ObjectInitializer, const TCHAR* TagName,
		float DurationSeconds);
};

UCLASS()
class NEWWORLDORDER_API UShootEffect_TacticalAssaultCooldown final : public UShootEffect_MatchSkillCooldownBase
{
	GENERATED_BODY()

public:
	UShootEffect_TacticalAssaultCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS()
class NEWWORLDORDER_API UShootEffect_TacticalOverloadCooldown final : public UShootEffect_MatchSkillCooldownBase
{
	GENERATED_BODY()

public:
	UShootEffect_TacticalOverloadCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS()
class NEWWORLDORDER_API UShootEffect_MedicalStationCooldown final : public UShootEffect_MatchSkillCooldownBase
{
	GENERATED_BODY()

public:
	UShootEffect_MedicalStationCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

/** 只在机器人被战斗击毁后施加；主动清理和 Owner 死亡默认不进入此冷却。 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_RobotCompanionCooldown final : public UShootEffect_MatchSkillCooldownBase
{
	GENERATED_BODY()

public:
	UShootEffect_RobotCompanionCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
