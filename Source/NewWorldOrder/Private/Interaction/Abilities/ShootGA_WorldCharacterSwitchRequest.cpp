// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.h"

#include "Interaction/IShootCharacterSwitchEntry.h"
#include "Player/ShootPlayerController.h"

UShootGA_WorldCharacterSwitchRequest::UShootGA_WorldCharacterSwitchRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UShootGA_WorldCharacterSwitchRequest::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                           const FGameplayAbilityActorInfo* ActorInfo,
                                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 先提交能力消耗/冷却，失败则立即结束。
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 从能力上下文或交互事件中拿到发起交互的 Pawn。
	const APawn* InstigatorPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!InstigatorPawn && TriggerEventData)
	{
		InstigatorPawn = Cast<APawn>(TriggerEventData->Instigator.Get());
	}

	// 定位玩家控制器，后续由控制器统一承接“请求切换 -> 服务器执行 -> 结果回包”这条后端链路。
	AShootPlayerController* ShootPC = nullptr;
	if (InstigatorPawn)
	{
		ShootPC = Cast<AShootPlayerController>(InstigatorPawn->GetController());
	}
	if (!ShootPC && ActorInfo)
	{
		ShootPC = Cast<AShootPlayerController>(ActorInfo->PlayerController.Get());
	}

	AActor* SwitchEntryActor = nullptr;
	if (TriggerEventData)
	{
		SwitchEntryActor = const_cast<AActor*>(ToRawPtr(TriggerEventData->Target));
	}

	ECharacterGender TargetGender = ECharacterGender::UNKNOWN;
	if (SwitchEntryActor && InstigatorPawn)
	{
		// 这里故意只依赖“世界入口接口”，不直接把能力写死到某个具体站点类上。
		// 后续换成“另一位主角 NPC”时，只要入口对象实现同一个接口，就能复用这条桥接链路。
		if (const IShootCharacterSwitchEntry* SwitchEntry = Cast<IShootCharacterSwitchEntry>(SwitchEntryActor))
		{
			// 服务端重新按 PlayerState 计算目标性别，不直接信任客户端透传值。
			SwitchEntry->ResolveSwitchTargetGender(InstigatorPawn, TargetGender);
		}
	}
	else if (TriggerEventData)
	{
		// 兜底路径：没有站点对象时，读取 EventMagnitude 编码的目标性别。
		const int32 EncodedGender = FMath::RoundToInt(TriggerEventData->EventMagnitude);
		TargetGender = (EncodedGender == 0) ? ECharacterGender::MALE : (EncodedGender == 1 ? ECharacterGender::FEMALE : ECharacterGender::UNKNOWN);
	}

	if (ShootPC && TargetGender != ECharacterGender::UNKNOWN)
	{
		// 世界入口只负责提出“切换到谁”的请求；
		// 具体是本地转 Server RPC 还是服务器直接执行，由控制器统一决定。
		ShootPC->RequestSwitchCharacter(TargetGender, SwitchEntryActor);
	}

	// 交互能力职责到此结束，结果提示由 PlayerController 回包给本地 UI。
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
