// Copyright ZhaoYiJie


#include "UI/ShootHUD.h"

#include "Components/GameFrameworkComponentManager.h"

AShootHUD::AShootHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AShootHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 完整复制 LyraHUD 的 receiver 生命周期：Experience 可在 HUD 生成前后注册扩展处理器。
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AShootHUD::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AShootHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}
