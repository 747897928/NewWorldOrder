// Copyright ZhaoYiJie

#include "Animation/ShootAnimNotify_RocketLauncherInsert.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAnimNotify_RocketLauncherInsert)

bool UShootAnimNotify_RocketLauncherInsert::ConfigureMontageNotify(
	UAnimMontage* CharacterMontage,
	const float TriggerTime)
{
	if (!CharacterMontage)
	{
		return false;
	}

	CharacterMontage->Modify();

	FAnimNotifyEvent* ExistingNotify = nullptr;
	for (int32 NotifyIndex = CharacterMontage->Notifies.Num() - 1; NotifyIndex >= 0; --NotifyIndex)
	{
		FAnimNotifyEvent& NotifyEvent = CharacterMontage->Notifies[NotifyIndex];
		const UAnimNotify* NotifyObject = NotifyEvent.Notify;
		const bool bIsRocketInsertNotify = NotifyObject &&
			(NotifyObject->IsA<UShootAnimNotify_RocketLauncherInsert>() ||
			 NotifyObject->GetClass()->GetName().Contains(TEXT("RocketLauncherInsert")));
		if (!bIsRocketInsertNotify)
		{
			continue;
		}

		if (!ExistingNotify)
		{
			ExistingNotify = &NotifyEvent;
			continue;
		}

		CharacterMontage->Notifies.RemoveAt(NotifyIndex);
	}

	if (!ExistingNotify)
	{
		FAnimNotifyEvent& NewNotify = CharacterMontage->Notifies.AddDefaulted_GetRef();
		NewNotify.Notify = NewObject<UShootAnimNotify_RocketLauncherInsert>(CharacterMontage);
		NewNotify.TrackIndex = 0;
		ExistingNotify = &NewNotify;
	}

	if (!Cast<UShootAnimNotify_RocketLauncherInsert>(ExistingNotify->Notify))
	{
		ExistingNotify->Notify = NewObject<UShootAnimNotify_RocketLauncherInsert>(CharacterMontage);
	}

	const float ClampedTime = FMath::Clamp(TriggerTime, 0.0f, CharacterMontage->GetPlayLength());
	ExistingNotify->Link(CharacterMontage, ClampedTime);
	ExistingNotify->SetTime(ClampedTime);
	ExistingNotify->TriggerTimeOffset = 0.0f;
	ExistingNotify->NotifyName = NAME_None;
	CharacterMontage->SortNotifies();
	CharacterMontage->RefreshCacheData();
	CharacterMontage->MarkPackageDirty();
	return true;
}

void UShootAnimNotify_RocketLauncherInsert::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* CharacterOwner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const FGameplayTag RocketInsertTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("GameplayEvent.Reload.RocketInsert")), false);
	if (!CharacterOwner || !RocketInsertTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = RocketInsertTag;
	Payload.Instigator = CharacterOwner;
	Payload.Target = CharacterOwner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		CharacterOwner,
		RocketInsertTag,
		Payload);
}

FString UShootAnimNotify_RocketLauncherInsert::GetNotifyName_Implementation() const
{
	return TEXT("Rocket Launcher Insert");
}
