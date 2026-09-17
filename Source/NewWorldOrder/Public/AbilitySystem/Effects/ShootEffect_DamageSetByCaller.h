// 通用即时伤害：IncomingDamage += SetByCaller.Damage
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_DamageSetByCaller.generated.h"

/**
 * 武器、投射物和技能共享的轻量伤害入口。
 *
 * 射击 GA 仍负责服务器权威命中、距离衰减和物理材质倍率；本 GE 只把已经算好的
 * SetByCaller.Damage 写入 UShootAttributeSet::IncomingDamage，随后由 AttributeSet 统一处理
 * 友伤、扣血和死亡。项目不引入 LyraWeaponStateComponent，也不重复 Lyra 的严格命中校验。
 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_DamageSetByCaller : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_DamageSetByCaller();
};
