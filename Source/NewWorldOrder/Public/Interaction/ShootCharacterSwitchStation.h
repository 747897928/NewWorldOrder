// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "Interaction/ShootCharacterSwitchEntryActorBase.h"
#include "ShootCharacterSwitchStation.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 宿舍角色切换站：当前开发期占位入口，提供角色切换交互选项。
 * 说明：站点本身只负责暴露世界入口，不再要求交互主能力内置长按。
 * 真正切换仍统一走 PlayerController / PlayerState 后端。
 * 该类现在继承 AShootCharacterSwitchEntryActorBase，只保留站点自己的组件与目标性别解析。
 */
UCLASS()
class NEWWORLDORDER_API AShootCharacterSwitchStation : public AShootCharacterSwitchEntryActorBase
{
	GENERATED_BODY()

public:
	AShootCharacterSwitchStation();

	virtual bool ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SwitchStation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SwitchStation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualComponent;
};
