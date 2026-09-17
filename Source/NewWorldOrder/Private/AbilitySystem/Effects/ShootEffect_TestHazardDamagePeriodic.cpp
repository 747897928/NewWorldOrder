// 仅用于 PIE 验证回血包和护盾优先结算：用周期 GE 写入 IncomingDamage，复用正式伤害后处理链。
#include "AbilitySystem/Effects/ShootEffect_TestHazardDamagePeriodic.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_TestHazardDamagePeriodic::UShootEffect_TestHazardDamagePeriodic()
{
	// 测试区域在 Pawn 离开 BoxOverlap 时由 AShootTestDamageHazardVolume 主动移除，
	// 因此这里必须是 Infinite；否则玩家站在火焰里 12 秒后会悄悄停止掉血。
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = 0.5f;
	bExecutePeriodicEffectOnApplication = true;

	FGameplayModifierInfo Modifier;
	// 这是地图测试夹具，不代表正式火焰伤害实现。写入 IncomingDamage 后，护盾优先、
	// 免死、伤害 Cue 等规则仍由 UShootAttributeSet::HandleIncomingDamage 统一处理。
	Modifier.Attribute = UShootAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	// IncomingDamage 使用正数表示伤害；负值会被 AttributeSet 当作无效输入并直接清零。
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
	Modifiers.Add(Modifier);
}
