#include "Interaction/ShootEnvironmentControl.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootLocalInteractionPrompt.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlayer.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEnvironmentControl)

AShootEnvironmentControl::AShootEnvironmentControl()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->InitSphereRadius(160.0f);
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(InteractionCollision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPrompt = CreateDefaultSubobject<UShootLocalInteractionPrompt>(TEXT("InteractionPrompt"));
	InteractionPrompt->SetupAttachment(InteractionCollision);
	// 模型、序列引用和提示样式由蓝图/关卡实例配置；C++ 只提供可复用的交互桥接。
}

void AShootEnvironmentControl::BeginPlay()
{
	Super::BeginPlay();
	CurrentTimeOfDay = InitialTimeOfDay;
	// 进入地图先落在配置的端点；按钮交互再用 PlayTo 平滑移动到下一个端点。
	ApplyTimeOfDay(CurrentTimeOfDay, false);
	// LevelSequenceActor 可能在本 Actor 之后才完成运行时播放器初始化，再补一次初始定位。
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyInitialTimeOfDay);
	}
}

void AShootEnvironmentControl::ApplyInitialTimeOfDay()
{
	ApplyTimeOfDay(CurrentTimeOfDay, false);
}

void AShootEnvironmentControl::GatherInteractionOptions(const FInteractionQuery& Query, FInteractionOptionBuilder& Builder)
{
	const APawn* Pawn = Cast<APawn>(Query.RequestingAvatar.Get());
	if (!Pawn || !Pawn->IsPlayerControlled()) return;
	if (InteractionCollision &&
		FVector::DistSquared2D(Pawn->GetActorLocation(), GetActorLocation()) >
		FMath::Square(InteractionCollision->GetScaledSphereRadius()))
	{
		return;
	}
	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.InteractionAbilityToGrant = UShootGA_Interaction_Environment::StaticClass();
	Builder.AddInteractionOption(Option);
}

void AShootEnvironmentControl::AdvanceTimeOfDay()
{
	switch (CurrentTimeOfDay)
	{
	case EShootEnvironmentTimeOfDay::Day:
		CurrentTimeOfDay = EShootEnvironmentTimeOfDay::Dusk;
		break;
	case EShootEnvironmentTimeOfDay::Dusk:
		CurrentTimeOfDay = EShootEnvironmentTimeOfDay::Night;
		break;
	case EShootEnvironmentTimeOfDay::Night:
	default:
		CurrentTimeOfDay = EShootEnvironmentTimeOfDay::Day;
		break;
	}

	ApplyTimeOfDay(CurrentTimeOfDay, true);
}

int32 AShootEnvironmentControl::GetFrameForTimeOfDay(EShootEnvironmentTimeOfDay TimeOfDay) const
{
	switch (TimeOfDay)
	{
	case EShootEnvironmentTimeOfDay::Day:
		return DayFrame;
	case EShootEnvironmentTimeOfDay::Night:
		return NightFrame;
	case EShootEnvironmentTimeOfDay::Dusk:
	default:
		return DuskFrame;
	}
}

void AShootEnvironmentControl::ApplyTimeOfDay(EShootEnvironmentTimeOfDay TimeOfDay, bool bPlayToFrame)
{
	if (!DayNightSequenceActor)
	{
		return;
	}

	ULevelSequencePlayer* SequencePlayer = DayNightSequenceActor->GetSequencePlayer();
	if (!SequencePlayer)
	{
		return;
	}

	const FMovieSceneSequencePlaybackParams PlaybackParams(
		FFrameTime(FFrameNumber(GetFrameForTimeOfDay(TimeOfDay))),
		bPlayToFrame ? EUpdatePositionMethod::Play : EUpdatePositionMethod::Jump);
	if (bPlayToFrame)
	{
		SequencePlayer->PlayTo(PlaybackParams, FMovieSceneSequencePlayToParams());
	}
	else
	{
		SequencePlayer->SetPlaybackPosition(PlaybackParams);
	}
}

UShootGA_Interaction_Environment::UShootGA_Interaction_Environment()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UShootGA_Interaction_Environment::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	APawn* Pawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	AShootEnvironmentControl* Control = TriggerEventData
		? const_cast<AShootEnvironmentControl*>(Cast<AShootEnvironmentControl>(TriggerEventData->Target.Get())) : nullptr;
	const bool bAllowed = Pawn && Pawn->IsLocallyControlled() && Control && CommitAbility(Handle, ActorInfo, ActivationInfo);
	if (bAllowed)
	{
		Control->AdvanceTimeOfDay();
		Control->OnEnvironmentInteraction.Broadcast(Pawn);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, false, !bAllowed);
}
