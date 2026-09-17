// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootMainMenuScreen.h"

#include "CommonButtonBase.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "GameFramework/PlayerController.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "NativeGameplayTags.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "PrimaryGameLayout.h"
#include "System/ShootGameInstance.h"
#include "Engine/World.h"

namespace ShootMainMenuTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_MENU, "UI.Layer.GameMenu");
}

void UShootMainMenuScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	UpdateMenuContext();
}

bool UShootMainMenuScreen::NativeOnHandleBackAction()
{
	CloseScreen();
	return true;
}

void UShootMainMenuScreen::OpenSessionScreen()
{
	PushChildScreen(SessionScreenClass);
}

void UShootMainMenuScreen::OpenWardrobeScreen()
{
	PushChildScreen(WardrobeScreenClass);
}

void UShootMainMenuScreen::OpenSettingsScreen()
{
	PushChildScreen(SettingsScreenClass);
}

bool UShootMainMenuScreen::IsInExpedition() const
{
	const UShootGameInstance* GameInstance = Cast<UShootGameInstance>(GetGameInstance());
	return GameInstance && !GameInstance->IsSessionReturnMap(GetWorld());
}

void UShootMainMenuScreen::RequestExitExpedition()
{
	if (!IsInExpedition())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = LocalPlayer
		? LocalPlayer->GetSubsystem<UCommonMessagingSubsystem>()
		: nullptr;
	if (!Messaging)
	{
		return;
	}

	FText Message = NSLOCTEXT("ShootMainMenu", "ExitLocalExpeditionMessage",
		"Leave the current expedition and return to the Home Base? Temporary items and unsettled progress from this expedition will be cleared.");
	if (const UShootSessionCoordinatorSubsystem* Coordinator =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>() : nullptr)
	{
		if (Coordinator->GetRole() == EShootSessionRole::Host)
		{
			Message = NSLOCTEXT("ShootMainMenu", "ExitHostExpeditionMessage",
				"You are the host. Ending the expedition will destroy the session and return all players to the Home Base. Continue?");
		}
		else if (Coordinator->GetRole() == EShootSessionRole::Client)
		{
			Message = NSLOCTEXT("ShootMainMenu", "ExitClientExpeditionMessage",
				"Leave the current expedition and return to the Home Base? The host and other players can continue playing.");
		}
	}

	Messaging->ShowConfirmation(
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(
			NSLOCTEXT("ShootMainMenu", "ExitExpeditionTitle", "Exit Expedition"), Message),
		FCommonMessagingResultDelegate::CreateUObject(
			this, &ThisClass::HandleExitExpeditionConfirmation));
}

void UShootMainMenuScreen::HandleExitExpeditionConfirmation(ECommonMessagingResult Result)
{
	if (Result == ECommonMessagingResult::Confirmed)
	{
		ExitExpeditionAfterConfirmation();
	}
}

void UShootMainMenuScreen::ExitExpeditionAfterConfirmation()
{
	if (!IsInExpedition())
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	UWorld* World = GetWorld();
	UShootGameInstance* GameInstance = Cast<UShootGameInstance>(GetGameInstance());
	if (!PlayerController || !World || !GameInstance)
	{
		return;
	}

	ClearRuntimeSessionForExit();
	DeactivateWidget();
	if (World->GetNetMode() == NM_Standalone)
	{
		// Local 与本地分屏共享同一个 World；一次确认会结束整次本地副本并回到家园地图。
		GameInstance->ReturnToMainMenu();
		return;
	}

	if (UShootSessionCoordinatorSubsystem* Coordinator =
		GameInstance->GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		// Coordinator 按角色分流：Host 销毁房间，Client 只离开自己。
		Coordinator->LeaveOrDestroySession(PlayerController);
	}
}

void UShootMainMenuScreen::UpdateMenuContext()
{
	const bool bExpedition = IsInExpedition();
	if (ExitExpeditionButton)
	{
		ExitExpeditionButton->SetVisibility(bExpedition ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	for (UCommonButtonBase* HomeOnlyButton : {OnlineButton.Get(), WardrobeButton.Get(), ReturnMainMenuButton.Get()})
	{
		if (HomeOnlyButton)
		{
			HomeOnlyButton->SetVisibility(bExpedition ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}
}

void UShootMainMenuScreen::ClearRuntimeSessionForExit() const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	// RuntimeOnly 副本物品只属于本次副本。Host/Standalone 在 Travel 前清理全部玩家；
	// 远端 Client 离开后其服务端 PlayerState 会随连接销毁，不从客户端伪造权威清理。
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			if (UShootQuickBarComponent* QuickBar = PlayerController->FindComponentByClass<UShootQuickBarComponent>())
			{
				QuickBar->ClearRuntimeSession();
			}
		}
	}
}

void UShootMainMenuScreen::CloseScreen()
{
	DeactivateWidget();
}

void UShootMainMenuScreen::PushChildScreen(const TSoftClassPtr<UCommonActivatableWidget>& ScreenClass)
{
	if (ScreenClass.IsNull())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(LocalPlayer))
		{
			RootLayout->PushWidgetToLayerStackAsync(
				ShootMainMenuTags::TAG_UI_LAYER_GAME_MENU, true, ScreenClass);
		}
	}
}
