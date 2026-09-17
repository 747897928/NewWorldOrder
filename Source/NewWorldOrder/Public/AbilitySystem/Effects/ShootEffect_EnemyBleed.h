// Bleeder archetype 的短时流血：周期写入 IncomingDamage，复用统一伤害后处理。
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_EnemyBleed.generated.h"

/**
 * 首期 Zombie archetype 使用的短暂流血 GE。
 *
 * 每秒从 SetByCaller.Damage 读取一次伤害，共持续 3 秒；同一攻击者重复命中只刷新持续时间，
 * 不无限叠加。友伤过滤、受击 Cue 与死亡仍由 UShootAttributeSet 统一处理；本类不按“流血”
 * 这个效果名硬编码护盾/生命关系，当前未声明特殊路由时沿用统一默认结算。
 * 当前效果就是 Bleeder 的正式纵切实现；以后增加抗性、驱散、多种流血等级或特殊伤害路由时，
 * 应把策略配置在通用 GameplayEffect 数据上，仍沿用 IncomingDamage，不另建扣血通道。
 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_EnemyBleed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_EnemyBleed();
};
