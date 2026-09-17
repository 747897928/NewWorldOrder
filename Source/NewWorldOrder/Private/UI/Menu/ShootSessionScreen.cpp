// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSessionScreen.h"

#include "NativeGameplayTags.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ShootSessionScreen"

namespace ShootSessionScreenTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_MENU, "UI.Layer.GameMenu");
}

void UShootSessionScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	Coordinator = GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>();
	if (Coordinator)
	{
		Coordinator->RefreshPlatformSessionState();
		Coordinator->OnCoordinatorChanged.AddUObject(this, &ThisClass::RefreshFromCoordinator);
	}
	RefreshFromCoordinator();

	// 玩家加入/离开和 Ping 没有统一的客户端事件；仅在本页面激活时每秒刷新一次展示数据，
	// 页面关闭立即清理计时器，不把旧项目的全局轮询带回新架构。
	GetWorld()->GetTimerManager().SetTimer(
		RuntimeInformationTimerHandle, this, &ThisClass::RefreshRuntimeInformation, 1.0f, true);
}

void UShootSessionScreen::NativeOnDeactivated()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RuntimeInformationTimerHandle);
	}
	if (Coordinator)
	{
		Coordinator->OnCoordinatorChanged.RemoveAll(this);
	}
	Coordinator = nullptr;
	Super::NativeOnDeactivated();
}
bool UShootSessionScreen::NativeOnHandleBackAction()
{
	CloseSessionScreen();
	return true;
}

APlayerController* UShootSessionScreen::GetOwningSessionPlayer() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

void UShootSessionScreen::RefreshFromCoordinator()
{
	if (!Coordinator)
	{
		BP_OnSessionStateDataChanged(EShootSessionLifecycleState::Error,
			LOCTEXT("CoordinatorMissing", "Session coordinator unavailable"), {});
		return;
	}

	TArray<UCommonSession_SearchResult*> SearchResults;
	for (UCommonSession_SearchResult* SearchResult : Coordinator->GetSearchResults())
	{
		SearchResults.Add(SearchResult);
	}
	BP_OnSessionStateDataChanged(Coordinator->GetState(), Coordinator->GetStatusText(), SearchResults);
	RefreshRuntimeInformation();
}

void UShootSessionScreen::RefreshRuntimeInformation()
{
	if (!Coordinator)
	{
		return;
	}

	BP_OnRuntimeSessionDataChanged(Coordinator->GetRuntimeSessionInfo(), Coordinator->GetRuntimePlayerInfo());
}

void UShootSessionScreen::HostOnlineSession()
{
	if (Coordinator)
	{
		Coordinator->HostSession(GetOwningSessionPlayer(), ECommonSessionOnlineMode::Online,
			HostMapId, MaxPlayers, AdvertisedMode);
	}
}

void UShootSessionScreen::HostLanSession()
{
	if (Coordinator)
	{
		Coordinator->HostSession(GetOwningSessionPlayer(), ECommonSessionOnlineMode::LAN,
			HostMapId, MaxPlayers, AdvertisedMode);
	}
}

void UShootSessionScreen::FindOnlineSessions()
{
	if (Coordinator)
	{
		Coordinator->FindSessions(GetOwningSessionPlayer(), ECommonSessionOnlineMode::Online);
	}
}

void UShootSessionScreen::FindLanSessions()
{
	if (Coordinator)
	{
		Coordinator->FindSessions(GetOwningSessionPlayer(), ECommonSessionOnlineMode::LAN);
	}
}

void UShootSessionScreen::OpenSessionBrowser()
{
	if (SessionBrowserClass.IsNull())
	{
		return;
	}

	// 浏览页属于当前 LocalPlayer 的 GameMenu 栈；关闭浏览页后自然返回完整会话页。
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(LocalPlayer))
		{
			RootLayout->PushWidgetToLayerStackAsync(
				ShootSessionScreenTags::TAG_UI_LAYER_GAME_MENU, true, SessionBrowserClass);
		}
	}
}

void UShootSessionScreen::ShowSessionInviteUI()
{
	if (Coordinator)
	{
		Coordinator->ShowInviteUI(GetOwningSessionPlayer());
	}
}

void UShootSessionScreen::ConfirmLeaveOrDestroySession()
{
	if (Coordinator)
	{
		Coordinator->LeaveOrDestroySession(GetOwningSessionPlayer());
	}
}

void UShootSessionScreen::CleanUpSessionState()
{
	if (Coordinator)
	{
		Coordinator->CleanUpResidualSession();
	}
}

void UShootSessionScreen::CloseSessionScreen()
{
	DeactivateWidget();
}

#undef LOCTEXT_NAMESPACE
