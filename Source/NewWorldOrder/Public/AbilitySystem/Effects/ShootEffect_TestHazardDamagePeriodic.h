// 仅用于 PIE 验证回血包和护盾优先结算：每半秒向 IncomingDamage 写入固定负值。
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_TestHazardDamagePeriodic.generated.h"

/**
 * 测试用持续伤害 GE，不是正式火焰系统。
 *
 * 这是地图测试夹具，不是正式火焰伤害实现；通过 IncomingDamage 进入正式后处理，专门验证：
 * 角色踩入火焰后先消耗 Shield、再扣 Health，回血包随后能否恢复 Health，以及治疗 Cue 是否能播放。
 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_TestHazardDamagePeriodic : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_TestHazardDamagePeriodic();
};
