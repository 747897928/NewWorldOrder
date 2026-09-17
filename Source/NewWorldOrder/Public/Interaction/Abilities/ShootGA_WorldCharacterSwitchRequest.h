// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_WorldCharacterSwitchRequest.generated.h"

/**
 * 世界内角色切换入口原型能力
 * - 当前只负责把世界入口拿到的目标角色转发给共享切换请求入口
 * - 不承载菜单入口，也不应继续扩展成角色切换业务本体
 * - 当前世界入口是 PressToInteract 占位方案；如果以后恢复世界内长按，也应放在入口层而不是交互主能力里
 * - 世界入口对象只要实现 IShootCharacterSwitchEntry，就能复用这条桥接能力
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_WorldCharacterSwitchRequest : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_WorldCharacterSwitchRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
};
