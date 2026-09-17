// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "ShootGameplayCueNotify_WeaponFire.generated.h"

class USoundBase;

/**
 * 项目武器开火 GameplayCue 适配器。
 *
 * Lyra 的 GCN_Weapon_*_Fire 通过 Ability SourceObject 找到 EquipmentInstance 的 B_Weapon，
 * 再调用 B_Weapon.Fire。项目保留同一表现分层，但 SourceObject 是 UShootWeaponInstance，
 * 因此由该类把项目 WeaponInstance 适配到迁入的 B_Weapon 蓝图事件。
 *
 * 声音链按武器数据配置，而不是按 WeaponId 写死：Rifle 才使用 B_Weapon.TriggerFireAudio
 * 复用 AudioComponent；Pistol/Shotgun/Shotgun_A 由 BurstEffects 每发播放一次 MetaSound。
 * Sniper、Rocket、Grenade Launcher 当前保留各自武器 AnimSequence 的专用 Fire Sound Notify，
 * 所以其 GCN 不再叠加通用枪声。把半自动枪送入 Rifle 链会只正常启动第一发，后续 Fire Trigger
 * 被一次性 MetaSound 忽略；同时保留 GCN 与动画两份枪声又会造成双重播放。
 * 新增武器时先选择上述唯一入口，详见 WeaponFireAudio_开火音频接入规范.md。
 */
UCLASS(Blueprintable, Category="GameplayCueNotify")
class NEWWORLDORDER_API UShootGameplayCueNotify_WeaponFire : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

	/** 仅持久触发链使用；当前只有 Rifle 配置。普通单发枪的声音放在父类 BurstEffects。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Fire|Audio")
	TObjectPtr<USoundBase> FireSound;

	/** 只对需要向同一个 AudioComponent 重复发送 Trigger 的自动武器开启；普通单发/逐发枪必须关闭。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Fire|Audio")
	bool bUsePersistentFireAudio = false;

private:
	const class UShootWeaponInstance* ResolveWeaponInstance(AActor* Target, const FGameplayCueParameters& Parameters) const;
	AActor* ResolveWeaponPresentationActor(const UShootWeaponInstance* WeaponInstance) const;
	bool ExecuteLyraWeaponFire(AActor* WeaponActor, const FGameplayCueParameters& Parameters) const;
	bool ExecuteLyraFireAudio(AActor* WeaponActor, AActor* Target) const;
};
