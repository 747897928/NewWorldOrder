// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Messages/LyraVerbMessage.h"

FString FLyraVerbMessage::ToString() const
{
	FString HumanReadableMessage;
	FLyraVerbMessage::StaticStruct()->ExportText(
		/*out*/ HumanReadableMessage,
		this,
		/*Defaults=*/nullptr,
		/*OwnerObject=*/nullptr,
		PPF_None,
		/*ExportRootScope=*/nullptr);
	return HumanReadableMessage;
}
