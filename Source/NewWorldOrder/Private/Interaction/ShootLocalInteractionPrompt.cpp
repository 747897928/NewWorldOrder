#include "Interaction/ShootLocalInteractionPrompt.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootLocalInteractionPrompt)

UShootLocalInteractionPrompt::UShootLocalInteractionPrompt()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	// 不在 C++ 设置图标、尺寸、位置或颜色；继承 UWidgetComponent 的属性在蓝图中维护。
}

void UShootLocalInteractionPrompt::BeginPlay()
{
	Super::BeginPlay();
	// 原组件只作为模板，避免未靠近前的默认可见图标及分屏串屏。
	SetHiddenInGame(true);
	SetVisibility(false);
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshLocalPrompts, 0.2f, true);
	}
}

void UShootLocalInteractionPrompt::RefreshLocalPrompts()
{
	for (const auto& Pair : LocalPrompts)
	{
		if (IsValid(Pair.Value)) Pair.Value->SetVisibility(false);
	}
	if (!GetWidgetClass() || !GetOwner()) return;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		// 提示位置通常抬高到角色视线附近；显示范围按物件根部的水平距离计算，避免 Widget 的 Z 偏移让玩家贴脸才看到。
		if (!Player || !Pawn || FVector::DistSquared2D(Pawn->GetActorLocation(), GetOwner()->GetActorLocation()) > FMath::Square(DisplayDistance)) continue;
		UWidgetComponent* Prompt = LocalPrompts.FindRef(Player);
		if (!Prompt)
		{
			Prompt = NewObject<UWidgetComponent>(GetOwner());
			Prompt->SetupAttachment(GetOwner()->GetRootComponent());
			Prompt->SetWidgetSpace(GetWidgetSpace());
			Prompt->SetDrawSize(GetDrawSize());
			Prompt->SetPivot(GetPivot());
			Prompt->SetWidgetClass(GetWidgetClass());
			Prompt->SetOwnerPlayer(Player);
			Prompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Prompt->SetGenerateOverlapEvents(false);
			GetOwner()->AddInstanceComponent(Prompt);
			Prompt->RegisterComponent();
			LocalPrompts.Add(Player, Prompt);
		}
		// 使用模板的世界变换，兼容蓝图中的多级附着；OwningPlayer 供现有 ActionWidget 查询设备图标。
		Prompt->SetWorldTransform(GetComponentTransform());
		if (UUserWidget* PromptWidget = Prompt->GetUserWidgetObject()) PromptWidget->SetOwningLocalPlayer(Player);
		Prompt->SetHiddenInGame(false);
		Prompt->SetVisibility(true);
	}
}

void UShootLocalInteractionPrompt::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
	for (const auto& Pair : LocalPrompts)
	{
		if (IsValid(Pair.Value)) Pair.Value->DestroyComponent();
	}
	LocalPrompts.Empty();
	Super::EndPlay(Reason);
}
