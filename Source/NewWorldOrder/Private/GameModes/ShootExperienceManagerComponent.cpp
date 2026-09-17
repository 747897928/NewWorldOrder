// Copyright NewWorldOrder Game. All Rights Reserved.

#include "GameModes/ShootExperienceManagerComponent.h"

#include "CommonUIExtensions.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"
#include "UI/ShootHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootExperienceManagerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogShootExperience, Log, All);

UShootExperienceManagerComponent::UShootExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UShootExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateExperience();
	Super::EndPlay(EndPlayReason);
}

void UShootExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CurrentExperience);
}

void UShootExperienceManagerComponent::SetCurrentExperience(
	TSoftObjectPtr<UShootExperienceDefinition> Experience)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CurrentExperience == Experience)
	{
		return;
	}

	CurrentExperience = Experience;
	LoadAndActivateExperience();
}

void UShootExperienceManagerComponent::OnRep_CurrentExperience()
{
	LoadAndActivateExperience();
}

void UShootExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(
	FShootExperienceLoadedDelegate::FDelegate&& Delegate)
{
	if (LoadedExperience)
	{
		Delegate.ExecuteIfBound(LoadedExperience);
		return;
	}

	ExperienceLoadedDelegate.Add(MoveTemp(Delegate));
}

void UShootExperienceManagerComponent::LoadAndActivateExperience()
{
	// Standalone/分屏 PIE 中服务器赋值后仍可能收到一次本地 OnRep。
	// 同一 Experience 已经建立 HUD 生命周期时不重复撤销和注入，避免留下失活的重复 HUD 实例。
	if (LoadedExperience && LoadedExperience == CurrentExperience.Get() && HUDRequestHandle.IsValid())
	{
		return;
	}

	DeactivateExperience();

	if (CurrentExperience.IsNull())
	{
		return;
	}

	LoadedExperience = CurrentExperience.LoadSynchronous();
	if (!LoadedExperience)
	{
		UE_LOG(LogShootExperience, Error, TEXT("Failed to load Experience %s"),
			*CurrentExperience.ToSoftObjectPath().ToString());
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGameFrameworkComponentManager* ComponentManager =
		GameInstance ? UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance) : nullptr;
	if (!ComponentManager)
	{
		UE_LOG(LogShootExperience, Error, TEXT("Experience %s cannot register HUD extensions: no component manager."),
			*GetNameSafe(LoadedExperience));
		return;
	}

	// 与 Lyra UGameFeatureAction_AddWidgets::AddToWorld 一致：监听 HUD receiver，
	// 这样先加载 Experience 或先生成 HUD 都会走同一条可撤销生命周期。
	const TSoftClassPtr<AActor> HUDActorClass(AShootHUD::StaticClass());
	HUDRequestHandle = ComponentManager->AddExtensionHandler(
		HUDActorClass,
		UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
			this, &ThisClass::HandleHUDExtension));

	// Pawn 相机等本地系统必须等到资产真正加载后读取配置；
	// 服务器与每个客户端各自广播，不通过全局玩家索引查找视图。
	ExperienceLoadedDelegate.Broadcast(LoadedExperience);

	// 技能套件由 Experience 数据授予(照 Lyra: 副本内获得, 出副本取回)。
	// 玩家可能在 Experience 加载前已生成(PlayerState BeginPlay 早于加载), 这里补齐授予; 服务器权威。
	GrantPlayerExperienceAbilities();
}

void UShootExperienceManagerComponent::GrantPlayerExperienceAbilities()
{
	if (GetWorld() && GetWorld()->GetAuthGameMode())
	{
		if (const AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			for (APlayerState* PS : GameState->PlayerArray)
			{
				if (AShootPlayerState* ShootPS = Cast<AShootPlayerState>(PS))
				{
					ShootPS->ApplyExperienceAbilitySets();
				}
			}
		}
	}
}

void UShootExperienceManagerComponent::TakePlayerExperienceAbilities()
{
	if (GetWorld() && GetWorld()->GetAuthGameMode())
	{
		if (const AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			for (APlayerState* PS : GameState->PlayerArray)
			{
				if (AShootPlayerState* ShootPS = Cast<AShootPlayerState>(PS))
				{
					ShootPS->TakeExperienceAbilitySets();
				}
			}
		}
	}
}

void UShootExperienceManagerComponent::DeactivateExperience()
{
	// 出副本: 取回 Experience 授予的技能套件(副本数据, 不持久)。
	TakePlayerExperienceAbilities();

	HUDRequestHandle.Reset();

	TArray<TWeakObjectPtr<AShootHUD>> HUDs;
	ActiveHUDData.GenerateKeyArray(HUDs);
	for (const TWeakObjectPtr<AShootHUD>& HUD : HUDs)
	{
		if (HUD.IsValid())
		{
			RemoveWidgetsForHUD(HUD.Get());
		}
	}
	ActiveHUDData.Reset();
	LoadedExperience = nullptr;
}

void UShootExperienceManagerComponent::HandleHUDExtension(AActor* Actor, FName EventName)
{
	AShootHUD* HUD = Cast<AShootHUD>(Actor);
	if (!HUD)
	{
		return;
	}

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved ||
		EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveWidgetsForHUD(HUD);
	}
	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded ||
		EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		AddWidgetsForHUD(HUD);
	}
}

void UShootExperienceManagerComponent::AddWidgetsForHUD(AShootHUD* HUD)
{
	if (!LoadedExperience || !HUD || ActiveHUDData.Contains(HUD))
	{
		return;
	}

	APlayerController* PlayerController = HUD->GetOwningPlayerController();
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		return;
	}

	FPerHUDData& HUDData = ActiveHUDData.Add(HUD);
	for (const FShootHUDLayoutRequest& Entry : LoadedExperience->HUDLayouts)
	{
		if (TSubclassOf<UCommonActivatableWidget> LayoutClass = Entry.LayoutClass.LoadSynchronous();
			LayoutClass && Entry.LayerID.IsValid())
		{
			HUDData.LayoutsAdded.Add(
				UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, Entry.LayerID, LayoutClass));
		}
	}

	if (UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>())
	{
		for (const FShootHUDElementEntry& Entry : LoadedExperience->HUDWidgets)
		{
			if (TSubclassOf<UUserWidget> WidgetClass = Entry.WidgetClass.LoadSynchronous();
				WidgetClass && Entry.SlotID.IsValid())
			{
				// Lyra 使用 LocalPlayer 作为 ContextObject；这是分屏下隔离 HUD 片段的关键。
				HUDData.ExtensionHandles.Add(ExtensionSubsystem->RegisterExtensionAsWidgetForContext(
					Entry.SlotID, LocalPlayer, WidgetClass, -1));
			}
		}
	}
}

void UShootExperienceManagerComponent::RemoveWidgetsForHUD(AShootHUD* HUD)
{
	FPerHUDData* HUDData = ActiveHUDData.Find(HUD);
	if (!HUDData)
	{
		return;
	}

	for (TWeakObjectPtr<UCommonActivatableWidget>& Layout : HUDData->LayoutsAdded)
	{
		if (Layout.IsValid())
		{
			Layout->DeactivateWidget();
		}
	}
	for (FUIExtensionHandle& Handle : HUDData->ExtensionHandles)
	{
		Handle.Unregister();
	}
	ActiveHUDData.Remove(HUD);
}
