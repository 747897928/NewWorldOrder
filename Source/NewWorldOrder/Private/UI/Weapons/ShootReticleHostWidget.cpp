#include "UI/Weapons/ShootReticleHostWidget.h"

#include "Equipment/ShootQuickBarComponent.h"
#include "Inventory/Fragments/ShootInventoryFragment_WeaponBasicConfig.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Player/ShootPlayerController.h"
#include "UI/Weapons/ShootReticleWidgetBase.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include "Widgets/SOverlay.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootReticleHostWidget)

UShootReticleHostWidget::UShootReticleHostWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> UShootReticleHostWidget::RebuildWidget()
{
	ReticleOverlay = SNew(SOverlay);
	return ReticleOverlay.ToSharedRef();
}

void UShootReticleHostWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	ReticleOverlay.Reset();
	ActiveReticleWidget = nullptr;
	CurrentWeaponInstance = nullptr;
}

void UShootReticleHostWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromOwningQuickBar();
}

void UShootReticleHostWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 与 LyraWeaponUserInterface 相同，Tick 是复制乱序后的最终对账入口。QuickBar 消息可能先于
	// Equipment FastArray 到达客户端；这里不使用 Timer，也不依赖服务器为 UI 发送额外 RPC。
	RefreshFromOwningQuickBar();
}

void UShootReticleHostWidget::RefreshFromOwningQuickBar()
{
	UShootRangedWeaponInstance* NewWeaponInstance = nullptr;
	if (const AShootPlayerController* PlayerController = Cast<AShootPlayerController>(GetOwningPlayer()))
	{
		if (const UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
		{
			NewWeaponInstance = Cast<UShootRangedWeaponInstance>(QuickBar->FindActiveWeaponInstance());
		}
	}

	if (NewWeaponInstance == CurrentWeaponInstance)
	{
		return;
	}

	CurrentWeaponInstance = NewWeaponInstance;
	ActiveReticleWidget = nullptr;
	if (ReticleOverlay.IsValid())
	{
		ReticleOverlay->ClearChildren();
	}

	const TSubclassOf<UShootReticleWidgetBase> ReticleClass = ResolveReticleClass(NewWeaponInstance);
	if (!NewWeaponInstance || !ReticleClass || !ReticleOverlay.IsValid())
	{
		return;
	}

	// 先把切枪消息对应的准确实例交给子准星，再构建 Slate。子准星不再自行重读 QuickBar，
	// 从根源消除“注册时是 Rifle、Widget 构造时已切成 Shotgun”的快速切枪竞态。
	ActiveReticleWidget = CreateWidget<UShootReticleWidgetBase>(GetOwningPlayer(), ReticleClass);
	if (ActiveReticleWidget)
	{
		ActiveReticleWidget->InitializeFromWeapon(NewWeaponInstance);
		ReticleOverlay->AddSlot()
		[
			ActiveReticleWidget->TakeWidget()
		];
	}
}

TSubclassOf<UShootReticleWidgetBase> UShootReticleHostWidget::ResolveReticleClass(
	const UShootRangedWeaponInstance* WeaponInstance) const
{
	if (!WeaponInstance)
	{
		return nullptr;
	}

	const UShootInventoryItemInstance* ItemInstance = WeaponInstance->GetItemInstance();
	const TSubclassOf<UShootInventoryItemDefinition> ItemDefinitionClass =
		ItemInstance ? ItemInstance->GetItemDef() : nullptr;
	const UShootInventoryItemDefinition* ItemDefinition =
		ItemDefinitionClass ? ItemDefinitionClass->GetDefaultObject<UShootInventoryItemDefinition>() : nullptr;
	const UShootInventoryFragment_WeaponBasicConfig* BasicConfig = ItemDefinition
		? Cast<UShootInventoryFragment_WeaponBasicConfig>(
			ItemDefinition->FindFragmentByClass(UShootInventoryFragment_WeaponBasicConfig::StaticClass()))
		: nullptr;

	return BasicConfig ? BasicConfig->ReticleWidgetClass : nullptr;
}
