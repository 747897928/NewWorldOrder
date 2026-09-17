// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "UI/Foundation/LyraButtonBase.h"
#include "ShootObjectEntryButtonBase.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FShootObjectEntryClicked, class UShootObjectEntryButtonBase*, UObject*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FShootObjectEntryHovered, class UShootObjectEntryButtonBase*, UObject*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FShootObjectEntryUnhovered, class UShootObjectEntryButtonBase*, UObject*);

/**
 * 可显示 UObject 快照的项目 CommonUI 按钮基类。
 *
 * 页面协调层创建条目后调用 SetEntryObject；条目蓝图通过 OnListItemObjectSet 刷新自身表现，
 * 页面只监听对象点击并执行具体业务。本类不依赖 ListView，不设置文本、颜色或布局。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEWWORLDORDER_API UShootObjectEntryButtonBase : public ULyraButtonBase
{
	GENERATED_BODY()

public:
	/** 普通 Panel 动态创建条目后调用，向条目蓝图传递当前对象快照。 */
	void SetEntryObject(UObject* InEntryObject);

	UObject* GetEntryObject() const { return EntryObject.Get(); }
	FShootObjectEntryClicked& OnEntryClicked() { return EntryClicked; }
	FShootObjectEntryHovered& OnEntryHovered() { return EntryHovered; }
	FShootObjectEntryUnhovered& OnEntryUnhovered() { return EntryUnhovered; }

protected:
	// UCommonButtonBase interface
	virtual void NativeOnClicked() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;

	/**
	 * 兼容现有衣柜条目蓝图的事件签名；名称沿用旧事件，语义是普通动态条目的对象注入，
	 * 与 ListView 已无运行时关系。待条目蓝图图表下一次整体整理时再统一更名，避免为命名
	 * 收尾引入第二套临时数据刷新链路。
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Entry Object Set")
	void OnListItemObjectSet(UObject* ListItemObject);

private:
	UPROPERTY(Transient)
	TObjectPtr<UObject> EntryObject;

	FShootObjectEntryClicked EntryClicked;
	FShootObjectEntryHovered EntryHovered;
	FShootObjectEntryUnhovered EntryUnhovered;
};
