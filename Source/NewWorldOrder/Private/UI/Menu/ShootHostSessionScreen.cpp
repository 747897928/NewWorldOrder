// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootHostSessionScreen.h"

#include "Engine/AssetManager.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "NativeGameplayTags.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootHostSessionScreen)

#define LOCTEXT_NAMESPACE "ShootHostSessionScreen"

namespace ShootHostSessionScreenTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_MENU, "UI.Layer.GameMenu");
}

void UShootHostSessionScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	Coordinator = GetGameInstance() ? GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>() : nullptr;
	if (Coordinator)
	{
		Coordinator->RefreshPlatformSessionState();
		Coordinator->OnCoordinatorChanged.AddUObject(this, &ThisClass::RefreshCoordinatorState);
	}

	RefreshExperienceCatalog();
	RefreshCoordinatorState();
}

void UShootHostSessionScreen::NativeOnDeactivated()
{
	if (Coordinator)
	{
		Coordinator->OnCoordinatorChanged.RemoveAll(this);
	}
	Coordinator = nullptr;
	Super::NativeOnDeactivated();
}

bool UShootHostSessionScreen::NativeOnHandleBackAction()
{
	CloseScreen();
	return true;
}

void UShootHostSessionScreen::RefreshExperienceCatalog()
{
	ExperienceCatalog.Reset();
	SelectedDefinition = nullptr;

	TArray<FPrimaryAssetId> AssetIds;
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("LyraUserFacingExperienceDefinition")), AssetIds);

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		ULyraUserFacingExperienceDefinition* Definition =
			Cast<ULyraUserFacingExperienceDefinition>(AssetPath.TryLoad());
		if (!Definition || !Definition->bShowInFrontEnd)
		{
			continue;
		}

		ExperienceCatalog.Add(Definition);
		if (!SelectedDefinition || Definition->bIsDefaultExperience)
		{
			SelectedDefinition = Definition;
		}
	}

	TArray<ULyraUserFacingExperienceDefinition*> BlueprintCatalog;
	BlueprintCatalog.Reserve(ExperienceCatalog.Num());
	for (ULyraUserFacingExperienceDefinition* Definition : ExperienceCatalog)
	{
		BlueprintCatalog.Add(Definition);
	}
	BP_OnExperienceCatalogChanged(BlueprintCatalog, SelectedDefinition);
	NotifyOptionsChanged();
}

void UShootHostSessionScreen::SelectExperience(ULyraUserFacingExperienceDefinition* Experience)
{
	if (Experience && ExperienceCatalog.Contains(Experience))
	{
		SelectedDefinition = Experience;
		if (!Experience->bSupportsOnline && IsOnlineMode())
		{
			// 分屏专用 Experience 被选择后回退到 Local，避免 UI 显示 Online 但请求被拒绝。
			SelectedOnlineMode = ECommonSessionOnlineMode::Offline;
		}
		RequestedMaxPlayers = FMath::Clamp(RequestedMaxPlayers, 1, Experience->MaxPlayerCount);
		NotifyOptionsChanged();
	}
}

void UShootHostSessionScreen::SetOnlineMode(bool bOnline)
{
	const bool bCanUseOnline = !bOnline || DoesSelectedExperienceSupportOnline();
	SelectedOnlineMode = bOnline && bCanUseOnline
		? ECommonSessionOnlineMode::Online
		: ECommonSessionOnlineMode::Offline;
	if (IsOnlineMode())
	{
		// 当前在线边界为一台机器一个 LocalPlayer；分屏只在 Standalone 创建，避免平台身份混用。
		LocalPlayerCount = 1;
	}
	NotifyOptionsChanged();
}

bool UShootHostSessionScreen::DoesSelectedExperienceSupportOnline() const
{
	return !SelectedDefinition || SelectedDefinition->bSupportsOnline;
}

void UShootHostSessionScreen::SetRequestedMaxPlayers(int32 NewMaxPlayers)
{
	const int32 ExperienceLimit = SelectedDefinition ? SelectedDefinition->MaxPlayerCount : 16;
	RequestedMaxPlayers = FMath::Clamp(NewMaxPlayers, 1, FMath::Max(1, ExperienceLimit));
	NotifyOptionsChanged();
}

void UShootHostSessionScreen::SetLocalPlayerCount(int32 NewLocalPlayerCount)
{
	LocalPlayerCount = IsOnlineMode() ? 1 : FMath::Clamp(NewLocalPlayerCount, 1, 2);
	NotifyOptionsChanged();
}

void UShootHostSessionScreen::AddLocalPlayer()
{
	SetLocalPlayerCount(LocalPlayerCount + 1);
}

void UShootHostSessionScreen::RemoveLocalPlayer()
{
	SetLocalPlayerCount(LocalPlayerCount - 1);
}

void UShootHostSessionScreen::SetAllowJoinInProgress(bool bAllowed)
{
	bAllowJoinInProgress = bAllowed;
	NotifyOptionsChanged();
}

void UShootHostSessionScreen::SetFillEmptySlotsWithBots(bool bEnabled)
{
	bFillEmptySlotsWithBots = bEnableBotFillOption && bEnabled;
	NotifyOptionsChanged();
}

bool UShootHostSessionScreen::HostSelectedExperience()
{
	if (!SelectedDefinition || !Coordinator ||
		(IsOnlineMode() && !SelectedDefinition->bSupportsOnline))
	{
		return false;
	}

	UCommonSession_HostSessionRequest* Request = SelectedDefinition->CreateHostingRequestWithOptions(
		this, SelectedOnlineMode, RequestedMaxPlayers, LocalPlayerCount, bAllowJoinInProgress,
		bFillEmptySlotsWithBots);
	return Coordinator->HostSessionRequest(GetOwningSessionPlayer(), Request);
}

void UShootHostSessionScreen::FindOnlineSessions()
{
	if (Coordinator)
	{
		Coordinator->FindSessions(GetOwningSessionPlayer(), ECommonSessionOnlineMode::Online);
	}
}

void UShootHostSessionScreen::OpenSessionBrowser()
{
	if (SessionBrowserClass.IsNull())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(LocalPlayer))
		{
			RootLayout->PushWidgetToLayerStackAsync(
				ShootHostSessionScreenTags::TAG_UI_LAYER_GAME_MENU, true, SessionBrowserClass);
		}
	}
}

void UShootHostSessionScreen::CloseScreen()
{
	DeactivateWidget();
}

void UShootHostSessionScreen::RefreshCoordinatorState()
{
	if (Coordinator)
	{
		BP_OnExpeditionSessionStateChanged(Coordinator->GetState(), Coordinator->GetStatusText());
	}
	else
	{
		BP_OnExpeditionSessionStateChanged(
			EShootSessionLifecycleState::Error, LOCTEXT("CoordinatorMissing", "Session coordinator unavailable"));
	}
}

void UShootHostSessionScreen::NotifyOptionsChanged()
{
	BP_OnExpeditionOptionsChanged();
}

APlayerController* UShootHostSessionScreen::GetOwningSessionPlayer() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

#undef LOCTEXT_NAMESPACE
