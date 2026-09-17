// Copyright ZhaoYiJie

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ShootAnimNotify_RocketLauncherInsert.generated.h"

class UAnimMontage;

/**
 * 火箭筒换弹的“弹头进入发射器”表现通知。
 *
 * 该通知只发送 Rocket 专用 GameplayEvent；弹药库存仍由既有 AN_Reload
 * -> GameplayEvent.ReloadDone 链路在服务器上结算，避免污染通用换弹 GA。
 */
UCLASS(meta=(DisplayName="Shoot Rocket Launcher Insert"))
class NEWWORLDORDER_API UShootAnimNotify_RocketLauncherInsert : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 编辑器/UE Python 配置入口，重复执行会更新已有同类通知。 */
	UFUNCTION(BlueprintCallable, Category="Rocket Launcher|Reload")
	static bool ConfigureMontageNotify(
		UAnimMontage* CharacterMontage,
		float TriggerTime = 1.323233f);

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
