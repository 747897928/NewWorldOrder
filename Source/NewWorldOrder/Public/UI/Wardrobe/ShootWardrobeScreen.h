// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"
#include "ShootWardrobeScreen.generated.h"

class APlayerController;
class AShootWardrobePreviewActor;
class UCommonButtonBase;
class UInputAction;
class ULyraTabListWidgetBase;
class UShootObjectEntryButtonBase;
class UPanelWidget;
class UShootWardrobeCatalogDataAsset;
class UShootWardrobePreviewController;
class UShootWardrobeViewModel;
class UShootWardrobeItemViewModel;
class UUniformGridPanel;
class UVerticalBox;
class UViewport;
class UWidget;
class UWidgetSwitcher;

/**
 * 衣柜主页面的运行时协调层。
 *
 * W_Cloth 蓝图只维护控件树、视觉、动画、固定按钮事件和可调配置；本类统一处理 MVVM 生命周期、
 * 五套独立 Panel、条目分发和预览生命周期。装备权威逻辑仍在 UShootWardrobeViewModel 与 PlayerState，
 * 本类不会直接访问库存、Mutable、SaveGame 或服务器 RPC。
 *
 * 这些 BindWidget 名称是 W_Cloth 的稳定结构契约。迁移资产前必须先迁移并编译本类及其依赖，
 * 再迁移 Widget 蓝图；不要通过 FindWidget 或兼容分支掩盖控件树不匹配。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootWardrobeScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Actions")
	void SaveOutfit();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Actions")
	void SwitchCharacter();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void RotatePreviewLeft();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void RotatePreviewRight();

	/**
	 * 切换"预览操控模式"：模式开启时 TopSettingsTabs 让出 LB/RB（SetListeningForInput(false)），
	 * LB/RB 落到旋转按钮的 TriggeringEnhancedInputAction（IA_WardrobePreviewRotateLeft/Right）；
	 * 模式关闭时恢复 TabList 监听。由隐藏按钮 PreviewModeToggleButton（L3）触发，蓝图可调用。
	 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void TogglePreviewControlMode();

	/** 当前是否处于预览操控模式（蓝图可读，用于操作提示等视觉反馈）。 */
	UFUNCTION(BlueprintPure, Category="Wardrobe|Preview")
	bool IsPreviewControlMode() const { return bPreviewControlMode; }

	/** 按增量偏航角旋转预览（度），供蓝图每帧根据右摇杆 X 轴调用；正值右转、负值左转。 */
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void RotatePreviewByDelta(float DeltaYawDegrees);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void ZoomPreviewIn();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void ZoomPreviewOut();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void ResetPreview();

	UFUNCTION(BlueprintPure, Category="Wardrobe")
	UShootWardrobeViewModel* GetWardrobeViewModel() const { return RuntimeViewModel; }

	/**
	 * 手柄/键盘焦点目标的数据查询：当前页第一个物品格子；当前分类没有物品时返回 nullptr。
	 * 注意：焦点目标的选择策略不在 C++，而在 W_Cloth 蓝图的事件 BP_GetDesiredFocusTarget 中实现
	 * （先取本函数结果，为空时取 GetFirstTopTabButton 作为兜底）。修改衣柜组件结构时只改蓝图即可。
	 */
	UFUNCTION(BlueprintPure, Category="Wardrobe|Focus")
	UWidget* GetFirstItemEntryForFocus() const;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 蓝图实现：条目被悬停（鼠标移入 / 手柄焦点）。Item 是只读快照，EntryWidget 是悬停格子，供蓝图计算左右与跟随位置。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Wardrobe|Tooltip")
	void BP_OnWardrobeItemHovered(UShootWardrobeItemViewModel* Item, UWidget* EntryWidget);

	/** 蓝图实现：条目结束悬停。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Wardrobe|Tooltip")
	void BP_OnWardrobeItemUnhovered(UShootWardrobeItemViewModel* Item, UWidget* EntryWidget);

	/** 正式目录由 W_Cloth 蓝图配置，C++ 不加载 /Game 路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Data")
	TObjectPtr<UShootWardrobeCatalogDataAsset> WardrobeCatalog;

	/** 预览 Actor 蓝图类由 W_Cloth 配置，灯光、背景和相机基准仍在该 Actor 蓝图调整。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Preview")
	TSubclassOf<AShootWardrobePreviewActor> PreviewActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Preview", meta=(ClampMin="0.0"))
	float PreviewRotationStepDegrees = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Preview", meta=(ClampMin="0.0"))
	float PreviewZoomStep = 25.0f;

	/** 两种条目蓝图类均由 W_Cloth 配置；C++ 不加载 /Game 路径，也不决定条目视觉。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Entries")
	TSubclassOf<UShootObjectEntryButtonBase> SubCategoryEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Entries")
	TSubclassOf<UShootObjectEntryButtonBase> ItemEntryClass;

	/** GridPanel 的列数、数量上限和间距是页面可调参数；运行时代码只消费这些值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Layout", meta=(ClampMin="1"))
	int32 ItemGridColumnCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Layout", meta=(ClampMin="1"))
	int32 MaxVisibleItemCount = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Layout")
	FMargin ItemGridSlotPadding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Layout")
	FMargin SubCategorySlotPadding;

	/**
	 * 右组按钮的稳定结构契约（与 Implementation_实现指南 的 W_Cloth 绑定约定一致）：
	 * 用显式 BindWidgetOptional 而不是运行时父链查找，因为 ConfigureFocusNavigation 的
	 * 规则直接写在这些控件上，接手者不需要推导父链结构。蓝图删除或改名某个按钮
	 * 只会让对应按钮退出规则，不会导致编译失败。新增按钮时同步加入右组数组。
	 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> PreviewZoomInButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> PreviewZoomOutButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> PreviewResetViewButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> SwitchCharacterButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> SaveOutfitButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> BackButton;

private:
	bool ResolveRuntimeViewModel();
	void BindStableWidgetDelegates();
	void UnbindStableWidgetDelegates();
	void BindViewModelDelegates();
	void ShutdownRuntime();
	void ClearPagePanels();
	void RefreshVisiblePage();
	void RefreshPreview();
	UVerticalBox* GetSubCategoryPanelForPage(int32 PageIndex) const;
	UUniformGridPanel* GetItemPanelForPage(int32 PageIndex) const;
	bool PanelHasEntryObjects(const UPanelWidget* Panel, const TArray<UObject*>& ExpectedItems) const;
	UShootObjectEntryButtonBase* CreateEntry(TSubclassOf<UShootObjectEntryButtonBase> EntryClass,
	                                          UObject* EntryObject);

	UFUNCTION()
	void HandlePreviewModeToggleClicked();

	/**
	 * 运行时焦点桥接。W_Cloth 焦点分组（一级分类 Tab 不参与焦点，只靠 LB/RB 切换）：
	 * 左组 = MainBorder 下各页动态二级分类按钮 + 动态物品格子；
	 * 右组 = PreviewRightOverlay 下五个预览按钮 + 切换角色/保存/返回。
	 * 要求左右两组所有可聚焦控件之间，方向键/左摇杆左右都能游走。
	 *
	 * Slate 默认几何导航只把"与当前焦点控件垂直区间相交"的控件作为候选，
	 * 左右两栏的空隙、分类按钮与格子的数量/高度不同都会让扫描落空，所以这里用
	 * SetNavigationRuleExplicit 建立确定性跳转。左组控件是 C++ 动态创建，蓝图
	 * Explicit 导航无法跨 UserWidget 解析目标，因此这一段必须留在 C++。
	 * 右组控件是文档化稳定契约，用 BindWidgetOptional 显式引用，规则直接可读。
	 *
	 * 焦点导航模型（方向键/左摇杆，入口出口固定，中间格子闭合）：
	 *
	 *   二级分类首项 <--左-- [入口] [   ] [   ] [   ]
	 *                       [   ] [   ] [   ] [   ]
	 *                       [   ] [   ] [   ] [出口] --右--> PreviewRotateLeftButton
	 *
	 * - 入口 = 第一个格子（左上）；出口 = 最后一个格子（右下）。
	 * - 矩阵内部：左右按行移动（行尾 -> 下一行行首，行首 -> 上一行行尾）；
	 *   上下按列移动并在首末行回绕；中间格子不跨出矩阵。
	 * - 二级分类按钮 右方向 -> 矩阵入口；本页无格子时 -> 右组。
	 * - 右组所有子控件 左方向 -> 左组入口（优先二级分类首项，其次格子首项）。
	 *
	 * W_Cloth 蓝图侧待办（改完在 EventGraph 放注释节点，避免以后忘记）：
	 * - PreviewHintText 接 L3 进入/退出提示（PreviewModeToggleButton.OnClicked
	 *   -> IsPreviewControlMode -> 更新文案与可见性）。
	 * - Hair/Face/BodyShape/Preset 的 SubTabsVBox Visibility 统一为 Visible。
	 */
	void ConfigureFocusNavigation();

	UFUNCTION()
	void HandleTopLevelTabSelected(FName TabId);

	UFUNCTION()
	void HandleViewStateChanged();

	UFUNCTION()
	void HandlePreviewModeChanged();

	void HandleEntryClicked(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject);
	void HandleEntryHovered(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject);
	void HandleEntryUnhovered(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject);

	UPROPERTY(Transient)
	TObjectPtr<UShootWardrobeViewModel> RuntimeViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UShootWardrobePreviewController> PreviewController;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<ULyraTabListWidgetBase> TopSettingsTabs;

	/**
	 * 预览操控模式切换按钮（L3）。W_Cloth 蓝图中此按钮视觉透明（RenderOpacity=0、
	 * Visibility=SelfHitTestInvisible，不参与焦点导航），仅通过
	 * TriggeringEnhancedInputAction=IA_WardrobePreviewModeToggle 接收 L3 按下。
	 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonButtonBase> PreviewModeToggleButton;

	/**
	 * 步进旋转按钮（LB/RB 手柄触发）。进入预览模式时 C++ 给这两个按钮设置
	 * TriggeringEnhancedInputAction=CachedRotateLeft/RightAction（此时 TabList 已
	 * 让出 LB/RB）；退出时清空，把 LB/RB 还给 TabList 切分类。
	 * 注意：运行时 SetTriggeringEnhancedInputAction 会覆盖蓝图默认值，所以进入时必须
	 * 重新设置。IA 在首次进入时从按钮的蓝图配置（GetEnhancedInputAction）缓存，避免
	 * 新增需要蓝图配置的属性（MCP 不可用时也能工作）。
	 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonButtonBase> PreviewRotateLeftButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonButtonBase> PreviewRotateRightButton;

	/** 首次进入预览模式时从按钮蓝图配置缓存的旋转输入动作（运行时清空后用于恢复）。 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CachedRotateLeftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CachedRotateRightAction;

	/** 预览操控模式状态（C++ 管理输入所有权，蓝图用 IsPreviewControlMode() 读取）。 */
	bool bPreviewControlMode = false;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UViewport> CharacterPreviewViewport;

	/**
	 * W_Cloth 蓝图中的五页固定容器。页面索引由 Catalog 顺序决定；每个页面保留自己的布局，
	 * C++ 只同步 Switcher 并把当前快照填入该页的 VerticalBox 与 UniformGridPanel。
	 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WardrobeTopLevelContentSwitcher;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ClothingSubTabsVBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> HairSubTabsVBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> FaceSubTabsVBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> BodyShapeSubTabsVBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> PresetSubTabsVBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> ClothingUniformGrid;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> HairUniformGrid;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> FaceUniformGrid;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> BodyShapeUniformGrid;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> PresetUniformGrid;
};
