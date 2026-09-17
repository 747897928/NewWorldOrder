// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Foundation/ShootObjectEntryButtonBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootObjectEntryButtonBase)

void UShootObjectEntryButtonBase::SetEntryObject(UObject* InEntryObject)
{
	EntryObject = InEntryObject;
	// 动态 Panel 由页面主动注入快照；条目蓝图在此事件中只刷新视觉，不保存衣柜业务状态。
	OnListItemObjectSet(InEntryObject);
}

void UShootObjectEntryButtonBase::NativeOnClicked()
{
	Super::NativeOnClicked();

	if (UObject* Object = EntryObject.Get())
	{
		// 点击只上报对象身份，由页面协调层分发分类切换或装备请求。
		EntryClicked.Broadcast(this, Object);
	}
}

void UShootObjectEntryButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();

	if (UObject* Object = EntryObject.Get())
	{
		// CommonUI 的 Hovered 语义是设备无关的：鼠标移入、手柄/键盘焦点导航、触屏按压都会走到这里。
		// 这里不做任何输入设备分支，条目只上报对象身份，由页面协调层决定预览选中。
		EntryHovered.Broadcast(this, Object);
	}
}

void UShootObjectEntryButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();

	if (UObject* Object = EntryObject.Get())
	{
		EntryUnhovered.Broadcast(this, Object);
	}
}
