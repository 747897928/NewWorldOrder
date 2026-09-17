// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Inventory/ShootResourceToastWidgetBase.h"

void UShootResourceToastWidgetBase::PushResourceToast(const FResourceChangedMessage& Message)
{
	LastToastMessage = Message;
	HandleResourceToast(Message);
}
