// Copyright ZhaoYiJie

#include "UI/Foundation/LyraActionWidget.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

//当前这个SetAssociatedInputAction方法不是lyra的代码，为了适配技能槽显示设备对应的图标加的
void ULyraActionWidget::SetAssociatedInputAction(UInputAction* InInputAction)
{
	if (AssociatedInputAction != InInputAction || GetEnhancedInputAction() != InInputAction)
	{
		AssociatedInputAction = InInputAction;
		// UE 5.8 的 SynchronizeProperties 只会在设计时刷新 CommonActionWidget。
		// 运行时必须走父类入口，它会调用 UpdateActionWidget，才能立刻显示当前改键和输入设备对应的图标。
		SetEnhancedInputAction(InInputAction);
	}
}

FSlateBrush ULyraActionWidget::GetIcon() const
{
	// If there is an Enhanced Input action associated with this widget, then search for any
	// keys bound to that action and display those instead of the default data table settings.
	// This covers the case of when a player has rebound a key to something else
	if (AssociatedInputAction)
	{
		if (const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = GetEnhancedInputSubsystem())
		{
			TArray<FKey> BoundKeys = EnhancedInputSubsystem->QueryKeysMappedToAction(AssociatedInputAction);
			FSlateBrush SlateBrush;

			const UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();
			if (!BoundKeys.IsEmpty() && CommonInputSubsystem && UCommonInputPlatformSettings::Get()->TryGetInputBrush(SlateBrush, BoundKeys[0], CommonInputSubsystem->GetCurrentInputType(), CommonInputSubsystem->GetCurrentGamepadName()))
			{
				return SlateBrush;
			}
		}
	}
	
	return Super::GetIcon();
}

UEnhancedInputLocalPlayerSubsystem* ULyraActionWidget::GetEnhancedInputSubsystem() const
{
	const UWidget* BoundWidget = DisplayedBindingHandle.GetBoundWidget();
	if (const ULocalPlayer* BindingOwner = BoundWidget ? BoundWidget->GetOwningLocalPlayer() : GetOwningLocalPlayer())
	{
		return BindingOwner->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	}
	return nullptr;
}
