#include "UI/Weapons/ShootHUDReticleComponent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ShootGameplayTags.h"
#include "UI/Weapons/ShootReticleHostWidget.h"
#include "UIExtensionSystem.h"

UShootHUDReticleComponent::UShootHUDReticleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;
	bWantsInitializeComponent = true;
}

void UShootHUDReticleComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedPlayerController = Cast<APlayerController>(GetOwner());
	if (!CachedPlayerController.IsValid())
	{
		// 只在玩家控制器上生效，其它持有者直接返回
		return;
	}

	if (!CachedPlayerController->IsLocalController())
	{
		// Listen Server 的远端 Controller 不注册消息、不 Tick，也不创建任何玩家私有 UI。
		return;
	}

	// 第二个本地 Controller 的 BeginPlay 可能早于 LocalPlayer 绑定。若此刻还不能注册，
	// 只开启短期低频重试；Host 注册成功后立即关闭组件 Tick。
	if (!TryRegisterReticleHost())
	{
		SetComponentTickEnabled(true);
	}
}

void UShootHUDReticleComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TryRegisterReticleHost())
	{
		SetComponentTickEnabled(false);
	}
}

bool UShootHUDReticleComponent::TryRegisterReticleHost()
{
	if (ReticleExtensionHandle.IsValid())
	{
		return true;
	}

	if (!CachedPlayerController.IsValid() || !CachedPlayerController->IsLocalController())
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = CachedPlayerController->GetLocalPlayer();
	UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
	if (LocalPlayer && ExtensionSubsystem)
	{
		// UIExtension 的稳定条目只是一枚 Host；切枪仅替换 Host 内部子 Widget，旧准星不会继续
		// 留在扩展点。LocalPlayer Context 同时保证分屏隔离和 Listen Server 远端 UI 边界。
		ReticleExtensionHandle = ExtensionSubsystem->RegisterExtensionAsWidgetForContext(
			FShootGameplayTags::Get().HUD_Slot_Reticle,
			LocalPlayer,
			UShootReticleHostWidget::StaticClass(),
			10);
	}

	return ReticleExtensionHandle.IsValid();
}

void UShootHUDReticleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ReticleExtensionHandle.IsValid())
	{
		ReticleExtensionHandle.Unregister();
	}

	CachedPlayerController.Reset();
	Super::EndPlay(EndPlayReason);
}
