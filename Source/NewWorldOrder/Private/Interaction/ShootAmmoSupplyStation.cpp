// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootAmmoSupplyStation.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Abilities/ShootGA_Interaction_RefillAmmo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAmmoSupplyStation)

AShootAmmoSupplyStation::AShootAmmoSupplyStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->InitSphereRadius(180.0f);
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	InteractionCollision->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(InteractionCollision);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionPromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	InteractionPromptComponent->SetupAttachment(InteractionCollision);
	InteractionPromptComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPromptComponent->SetGenerateOverlapEvents(false);
	InteractionPromptComponent->SetHiddenInGame(true);
	InteractionPromptComponent->SetVisibility(false);

	InteractionText = NSLOCTEXT("AmmoSupplyStation", "InteractionText", "Restock Ammo");
	InteractionSubText = NSLOCTEXT("AmmoSupplyStation", "InteractionSubText", "Hold the interact button for 3 seconds");
	InteractionAbilityClass = UShootGA_Interaction_RefillAmmo::StaticClass();
}

void AShootAmmoSupplyStation::BeginPlay()
{
	Super::BeginPlay();

	InteractionCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	InteractionCollision->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnOverlapEnd);
	if (InteractionPromptWidgetClass)
	{
		InteractionPromptComponent->SetWidgetClass(InteractionPromptWidgetClass);
	}

	// 继承组件只保存蓝图里的 Widget、尺寸与相对位置，不能直接显示；
	// 真正提示由每个 LocalPlayer 的独立组件承载，避免分屏和 Listen Server 串线。
	InteractionPromptComponent->SetHiddenInGame(true);
	InteractionPromptComponent->SetVisibility(false);
}

void AShootAmmoSupplyStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LocalPromptComponents.Reset();
	Super::EndPlay(EndPlayReason);
}

void AShootAmmoSupplyStation::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	const APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!CanSupplyPawn(RequestingPawn))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.HoldDuration = HoldDuration;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_RefillAmmo::StaticClass());
	Option.InteractionWidgetClass = InteractionPromptWidgetClass;
	OptionBuilder.AddInteractionOption(Option);
}

void AShootAmmoSupplyStation::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	FGameplayEventData& InOutEventData)
{
	(void)InteractionEventTag;
	InOutEventData.Target = this;
}

bool AShootAmmoSupplyStation::CanSupplyPawn(const APawn* Pawn) const
{
	// InteractionCollision 是设计师在 BP_AmmoSupplyStation 中可视化调节的唯一交互边界。
	// 客户端扫描和服务器最终结算都调用这里，禁止再用另一套硬编码距离绕过碰撞体。
	return Pawn && Pawn->IsPlayerControlled() && InteractionCollision && InteractionCollision->IsOverlappingActor(Pawn);
}

void AShootAmmoSupplyStation::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;
	SetLocalPromptVisible(Cast<APawn>(OtherActor), true);
}

void AShootAmmoSupplyStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;
	SetLocalPromptVisible(Cast<APawn>(OtherActor), false);
}

void AShootAmmoSupplyStation::SetLocalPromptVisible(const APawn* Pawn, bool bVisible)
{
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	UWidgetComponent* LocalPrompt = FindOrCreateLocalPrompt(Pawn);
	if (!LocalPrompt)
	{
		return;
	}

	// 同一 Pawn 可能有多个 PrimitiveComponent；某个部件 EndOverlap 时若 Pawn 仍在球体内，提示不能提前消失。
	const bool bShouldShow = bVisible || CanSupplyPawn(Pawn);
	LocalPrompt->SetHiddenInGame(!bShouldShow);
	LocalPrompt->SetVisibility(bShouldShow);
}

UWidgetComponent* AShootAmmoSupplyStation::FindOrCreateLocalPrompt(const APawn* Pawn)
{
	if (!InteractionPromptComponent || !Pawn)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		return nullptr;
	}

	if (TObjectPtr<UWidgetComponent>* ExistingPrompt = LocalPromptComponents.Find(LocalPlayer))
	{
		return ExistingPrompt->Get();
	}

	UWidgetComponent* LocalPrompt = NewObject<UWidgetComponent>(this);
	LocalPrompt->SetupAttachment(InteractionCollision);
	LocalPrompt->SetRelativeTransform(InteractionPromptComponent->GetRelativeTransform());
	LocalPrompt->SetWidgetSpace(InteractionPromptComponent->GetWidgetSpace());
	LocalPrompt->SetDrawSize(InteractionPromptComponent->GetDrawSize());
	LocalPrompt->SetPivot(InteractionPromptComponent->GetPivot());
	LocalPrompt->SetWidgetClass(InteractionPromptComponent->GetWidgetClass());
	LocalPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LocalPrompt->SetGenerateOverlapEvents(false);
	LocalPrompt->SetOwnerPlayer(LocalPlayer);
	LocalPrompt->SetIsReplicated(false);
	LocalPrompt->SetHiddenInGame(true);
	LocalPrompt->SetVisibility(false);

	// 组件只在当前客户端动态创建，不进入网络复制；OwnerPlayer 决定它属于哪个分屏视口。
	AddInstanceComponent(LocalPrompt);
	LocalPrompt->RegisterComponent();
	LocalPromptComponents.Add(LocalPlayer, LocalPrompt);
	return LocalPrompt;
}
