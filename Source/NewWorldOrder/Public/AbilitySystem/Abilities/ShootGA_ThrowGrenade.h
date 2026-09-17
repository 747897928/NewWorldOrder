// 项目化手雷能力：复用现有服务器权威 Projectile 与 GameplayMessage UI 链路，不依赖 LyraGame 类或资产。
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"
#include "ShootGA_ThrowGrenade.generated.h"

class AShootProjectileBase;
class UGameplayEffect;
class USoundAttenuation;
class USoundBase;

/**
 * 所有角色共有的投掷手雷。
 * 蓝图子类负责设置投射物表现、Niagara、伤害数值、蒙太奇与冷却 GE；本类仅负责网络权威生成与 UI 时长消息。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_ThrowGrenade : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_ThrowGrenade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** 仅服务器生成；客户端通过复制投射物和 GameplayCue 接收表现。 */
	void SpawnGrenade();

	/** 仅本地拥有者广播，供 W_GrenadeCooldown 内的 W_ActionTouchButton 启动同一冷却动画。 */
	void BroadcastCooldownDuration() const;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	TSubclassOf<AShootProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	FProjectileWeaponConfig ProjectileConfig;

	/** 对应 /Game/UI/Weapon/W_GrenadeCooldown 的 DurationMessageTag。 */
	UPROPERTY(EditDefaultsOnly, Category="Grenade|UI")
	FGameplayTag DurationMessageTag;

	/** 投掷蒙太奇：按主角性别选择（男 MM / 女 MF），由 GA_Grenade 蓝图子类配置资产。 */
	UPROPERTY(EditDefaultsOnly, Category="Grenade|Animation")
	TObjectPtr<UAnimMontage> ThrowMontageMale;

	UPROPERTY(EditDefaultsOnly, Category="Grenade|Animation")
	TObjectPtr<UAnimMontage> ThrowMontageFemale;

	/** 投掷释放音效；当前迁移音频没有专用 throw 文件，由 GA_Grenade 蓝图配置可替换资产。 */
	UPROPERTY(EditDefaultsOnly, Category="Grenade|Audio")
	TObjectPtr<USoundBase> ThrowSound;

	UPROPERTY(EditDefaultsOnly, Category="Grenade|Audio")
	TObjectPtr<USoundAttenuation> ThrowSoundAttenuation;

private:
	/** 按当前 Avatar 的性别 Kit Tag（Abilities.Kit.Protagonist.Male/Female）选择蒙太奇。 */
	UAnimMontage* GetThrowMontageForAvatar() const;
};
