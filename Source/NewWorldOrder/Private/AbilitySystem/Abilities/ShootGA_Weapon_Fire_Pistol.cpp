// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Pistol.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Weapon_Fire_Pistol)

UShootGA_Weapon_Fire_Pistol::UShootGA_Weapon_Fire_Pistol(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 数值与角色 Montage 由 Pistol AbilitySet/蓝图配置；构造函数只声明该枪种的语义标签。
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Pistol.Fire"), false);
	// Pistol 继承 Rifle 的命中、伤害与表现实现，但扳机语义仍是半自动：一次按压只开一枪。
	// 未来某把手枪需要全自动时，应建立独立 GA 子类或可审计的武器行为配置，不能修改共享手枪默认值。
	ActivationPolicy = EShootAbilityActivationPolicy::OnInputTriggered;
	// Lyra 的手枪、步枪和霰弹枪共用 Rifle.Impact；统一标签可让表面音效只维护一份，
	// 而弹孔、火花和角色命中特效仍由各自 Fire Cue 的 B_WeaponImpacts 链负责，避免重复生成视觉效果。
	ImpactGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rifle.Impact"));
	FireDelayTimeSecs = 0.22f;
}
