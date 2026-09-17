#include "Animation/ShootAnimNotify_PlayScopedMuzzleFlash.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Particles/ParticleSystemComponent.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_PlayScopedMuzzleFlash)

UParticleSystemComponent* UShootAnimNotify_PlayScopedMuzzleFlash::SpawnParticleSystem(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	UParticleSystemComponent* ParticleComponent = Super::SpawnParticleSystem(MeshComp, Animation);
	if (!ParticleComponent || !MeshComp)
	{
		return ParticleComponent;
	}

	// 武器 Montage 在 Equipment Actor 的 Mesh 上播放；该 Actor 的 Owner 才是 Pawn。
	// 同时兼容通知未来直接放到角色 Mesh 的情况，避免依赖具体武器蓝图类型。
	AActor* VisualOwner = MeshComp->GetOwner();
	APawn* OwningPawn = Cast<APawn>(VisualOwner);
	if (!OwningPawn && VisualOwner)
	{
		OwningPawn = Cast<APawn>(VisualOwner->GetOwner());
	}

	if (OwningPawn)
	{
		if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn))
		{
			if (ASC->HasMatchingGameplayTag(FShootGameplayTags::Get().Event_Movement_ADS))
			{
				// 该标志按 ViewActor 判断 Owner 链；分屏中的另一名本地玩家仍能看到粒子。
				ParticleComponent->SetOwnerNoSee(true);
			}
		}
	}

	return ParticleComponent;
}
