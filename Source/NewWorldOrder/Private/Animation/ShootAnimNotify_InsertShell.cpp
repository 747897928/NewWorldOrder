// Copyright ZhaoYiJie

#include "Animation/ShootAnimNotify_InsertShell.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_InsertShell)

void UShootAnimNotify_InsertShell::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* CharacterOwner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const FGameplayTag InsertShellTag = FShootGameplayTags::Get().GameplayEvent_Reload_InsertShell;
	if (!CharacterOwner || !InsertShellTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = InsertShellTag;
	Payload.Instigator = CharacterOwner;
	Payload.Target = CharacterOwner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CharacterOwner, InsertShellTag, Payload);
}

FString UShootAnimNotify_InsertShell::GetNotifyName_Implementation() const
{
	return TEXT("Insert Shell");
}
