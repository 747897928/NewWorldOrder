// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Zombie/ShootAnimNotify_ZombieMeleeHit.h"

#include "AI/EnemyBotCharacter.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_ZombieMeleeHit)

void UShootAnimNotify_ZombieMeleeHit::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (AEnemyBotCharacter* Zombie = MeshComp
		? Cast<AEnemyBotCharacter>(MeshComp->GetOwner())
		: nullptr)
	{
		// 客户端也会播放同一 Montage，但只有服务器广播委托，避免客户端重复创建伤害 GE。
		Zombie->NotifyMeleeAnimationEvent();
	}
}
