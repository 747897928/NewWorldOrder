// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Frontend/ShootFrontEndScreen.h"

#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootFrontEndScreen)

namespace ShootFrontEndScreenTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
}

void UShootFrontEndScreen::OpenSettingsScreen()
{
	if (SettingsScreenClass.IsNull())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(LocalPlayer))
		{
			RootLayout->PushWidgetToLayerStackAsync(
				ShootFrontEndScreenTags::TAG_UI_LAYER_MENU, true, SettingsScreenClass);
		}
	}
}
