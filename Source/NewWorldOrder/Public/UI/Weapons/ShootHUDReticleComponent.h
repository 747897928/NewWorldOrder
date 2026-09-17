#pragma once

#include "Components/ActorComponent.h"
#include "UIExtensionSystem.h"
#include "ShootHUDReticleComponent.generated.h"

class APlayerController;

/**
 * HUD 端准星入口：只为本地玩家向 HUD.Slot.Reticle 注册一枚稳定 ReticleHost。
 * 武器切换和子准星生命周期由 Host 管理，不再把每把武器的准星类反复注册到 UIExtension。
 */
UCLASS(ClassGroup="UI", meta=(BlueprintSpawnableComponent))
class UShootHUDReticleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootHUDReticleComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	//~End of UActorComponent interface

protected:
	bool TryRegisterReticleHost();

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	// 句柄使用 LocalPlayer Context；分屏两位玩家与 Listen Server 本地玩家互相隔离。
	FUIExtensionHandle ReticleExtensionHandle;
};
