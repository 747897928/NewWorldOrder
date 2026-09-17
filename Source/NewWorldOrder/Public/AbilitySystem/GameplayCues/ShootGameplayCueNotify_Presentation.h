// 一次性 GameplayCue 表现适配器：把项目资产中的 Niagara 与音频统一放在 Cue 上播放。
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "ShootGameplayCueNotify_Presentation.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class USoundAttenuation;
class USoundBase;

/**
 * 项目化一次性表现 Cue。
 *
 * Lyra 的爆炸、治疗等一次性反馈由 GameplayCueNotify 负责在本地执行；
 * 项目只保留 Niagara/音频资产配置在 Cue 蓝图中，避免在 C++ 构造函数写死 /Game/ 路径。
 */
UCLASS(Blueprintable, Category="GameplayCueNotify")
class NEWWORLDORDER_API UShootGameplayCueNotify_Presentation : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	/** FXVarietyPack 等仍使用 Cascade；由 Cue 蓝图配置，避免 GA 硬编码资源路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UParticleSystem> CascadeSystem = nullptr;

	/** 相对 Cue 朝向的世界表现偏移；具体资产的落地点由 Cue 蓝图调节。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation|Transform")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation|Transform")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation|Transform")
	FVector EffectScale = FVector::OneVector;

	/** 大于 0 时强制销毁 Cascade 组件，防止无限循环粒子永久留在场景中；0 表示沿用资产自身生命周期。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation|Lifetime", meta=(ClampMin="0.0", Units="s"))
	float CascadeLifetimeSeconds = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundAttenuation> Attenuation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.0"))
	float PitchMultiplier = 1.0f;
};
