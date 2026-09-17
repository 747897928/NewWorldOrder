// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Frontend/ShootHomeHubStateComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogShootHomeHubState, Log, All);

void UShootHomeHubStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsConfiguredHomeHubWorld())
	{
		StartHomeHubFlow();
	}
}

bool UShootHomeHubStateComponent::IsConfiguredHomeHubWorld() const
{
	const UWorld* World = GetWorld();
	const FString ConfiguredPackage = HomeHubMap.ToSoftObjectPath().GetLongPackageName();
	// PIE 会把 /Game/Maps/HomeMap 临时改为 /Game/Maps/UEDPIE_0_HomeMap；去掉前缀后再比较，
	// 这样编辑器验收和打包运行使用同一份蓝图地图配置。
	const FString CurrentPackage = World ? UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) : FString();
	return World && !ConfiguredPackage.IsEmpty() && CurrentPackage == ConfiguredPackage;
}

void UShootHomeHubStateComponent::StartHomeHubFlow()
{
	// Lyra 的启动前端图与比赛图分离，所以进入前端图可以无条件清理。当前项目的 HomeMap 是家园大厅，暂时也
	// 是 Listen Server/Client 的联机目标图。ServerTravel 加载新 World 的早期阶段可能仍报告
	// NM_Standalone，因此还要检查 URL 会话选项，避免清理请求与 Host 请求交错后销毁新会话。
	UWorld* World = GetWorld();
	const bool bIsSessionTravel = World &&
		(World->URL.HasOption(TEXT("listen")) || World->URL.HasOption(TEXT("bIsLanMatch")));
	UE_LOG(LogShootHomeHubState, Display,
		TEXT("HomeHub loaded Map=%s NetMode=%d Listen=%d LanMatch=%d CleanupStandalone=%d"),
		World ? *UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) : TEXT("None"),
		World ? static_cast<int32>(World->GetNetMode()) : INDEX_NONE,
		World && World->URL.HasOption(TEXT("listen")),
		World && World->URL.HasOption(TEXT("bIsLanMatch")),
		World && World->GetNetMode() == NM_Standalone && !bIsSessionTravel);
	if (World && World->GetNetMode() == NM_Standalone && !bIsSessionTravel)
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UShootSessionCoordinatorSubsystem* Coordinator =
				GameInstance->GetSubsystem<UShootSessionCoordinatorSubsystem>())
			{
				Coordinator->CleanUpResidualSession();
			}
		}
	}

}
