// 一次性 GameplayCue 表现适配器：在服务器广播的 Cue 到达各客户端后本地播放表现。
#include "AbilitySystem/GameplayCues/ShootGameplayCueNotify_Presentation.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGameplayCueNotify_Presentation)

bool UShootGameplayCueNotify_Presentation::OnExecute_Implementation(
	AActor* Target, const FGameplayCueParameters& Parameters) const
{
	Super::OnExecute_Implementation(Target, Parameters);

	// 爆炸/治疗 Cue 都由调用方提供世界坐标；没有坐标时才回退到目标 Actor，
	// 这样远端客户端不依赖本地 Pawn 的位置猜测，也不会把表现播放到错误玩家身上。
	const FVector BaseLocation = (Parameters.Location.IsNearlyZero() && Target)
		? Target->GetActorLocation()
		: FVector(Parameters.Location);
	const FRotator BaseRotation = Parameters.Normal.IsNearlyZero()
		? FRotator::ZeroRotator
		: Parameters.Normal.Rotation();
	const FRotator Rotation = (BaseRotation.Quaternion() * RotationOffset.Quaternion()).Rotator();
	const FVector Location = BaseLocation + BaseRotation.RotateVector(LocationOffset);

	if (NiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Target, NiagaraSystem, Location, Rotation);
	}
	if (CascadeSystem)
	{
		UParticleSystemComponent* SpawnedCascade = UGameplayStatics::SpawnEmitterAtLocation(
			Target, CascadeSystem, Location, Rotation, EffectScale, true);
		if (SpawnedCascade && CascadeLifetimeSeconds > 0.f)
		{
			// AutoDestroy 只对会自然完成的粒子有效。循环 Cascade 必须有显式截止时间，否则会永久残留。
			TWeakObjectPtr<UParticleSystemComponent> WeakCascade(SpawnedCascade);
			FTimerHandle CleanupTimer;
			SpawnedCascade->GetWorld()->GetTimerManager().SetTimer(
				CleanupTimer,
				FTimerDelegate::CreateLambda([WeakCascade]()
				{
					if (UParticleSystemComponent* ParticleComponent = WeakCascade.Get())
					{
						ParticleComponent->DestroyComponent();
					}
				}),
				CascadeLifetimeSeconds,
				false);
		}
	}

	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			Target, Sound, Location, Rotation, VolumeMultiplier, PitchMultiplier, 0.0f, Attenuation);
	}

	// 一次性表现不保留持续 Cue 实例；循环 Cascade 可由蓝图配置强制清理时长。
	return false;
}
