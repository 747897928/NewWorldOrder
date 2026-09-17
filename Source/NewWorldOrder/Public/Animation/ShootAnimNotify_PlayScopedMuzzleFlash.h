#pragma once

#include "Animation/AnimNotifies/AnimNotify_PlayParticleEffect.h"
#include "ShootAnimNotify_PlayScopedMuzzleFlash.generated.h"

class UParticleSystemComponent;

/**
 * 播放武器枪口 Cascade，并在武器拥有者处于 ADS 时仅对该拥有者视图隐藏粒子。
 *
 * 狙击镜是全屏 UI，但第三人称枪口仍位于相机视锥边缘。直接跳过 Notify 会让同一进程里的
 * 其他分屏玩家也看不到枪口火焰；Owner No See 由每个 SceneView 按武器 Owner 过滤，既清理
 * 开镜玩家自己的烟雾穿帮，又保留其他本地玩家、远端玩家和模拟代理看到的枪口表现。
 */
UCLASS(const, CollapseCategories, meta=(DisplayName="Play Scoped Muzzle Flash"))
class NEWWORLDORDER_API UShootAnimNotify_PlayScopedMuzzleFlash : public UAnimNotify_PlayParticleEffect
{
	GENERATED_BODY()

protected:
	virtual UParticleSystemComponent* SpawnParticleSystem(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) override;
};
