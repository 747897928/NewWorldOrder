// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "UIExtensionSystem.h"
#include "ShootHUDInteractionComponent.generated.h"

class APlayerController;
class UShootInteractionProgressWidget;

/** 为每个 LocalPlayer 向 HUD.Slot.Interaction 注册一枚稳定交互读条。 */
UCLASS(ClassGroup="UI", meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootHUDInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootHUDInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool TryRegisterWidget();

	/**
	 * 由 BP_ShootPlayerController 的继承组件配置为 Interactive_Progress_Bar。
	 * C++ 只负责每 LocalPlayer 生命周期，不硬编码具体 /Game 资产路径。
	 */
	UPROPERTY(EditDefaultsOnly, Category="UI|Interaction")
	TSubclassOf<UShootInteractionProgressWidget> InteractionProgressWidgetClass;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	FUIExtensionHandle ExtensionHandle;
};
