// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Wardrobe/ShootWardrobeScreen.h"

#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Viewport.h"
#include "Components/WidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "Input/NavigationReply.h"
#include "MVVMSubsystem.h"
#include "ShootLogChannels.h"
#include "UI/Common/LyraTabListWidgetBase.h"
#include "UI/Foundation/ShootObjectEntryButtonBase.h"
#include "UI/ViewModel/WardrobeViewModel.h"
#include "UI/Wardrobe/ShootWardrobeCatalogDataAsset.h"
#include "UI/Wardrobe/ShootWardrobePreviewController.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWardrobeScreen)

void UShootWardrobeScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// CommonUI 可能复用已经 Destruct 过的页面对象，而 NativeOnInitialized 对同一对象只执行一次。
	// 因此顶部 Tab 委托必须按每次激活重新绑定，否则第二次打开衣柜时页面切换会失去响应。
	BindStableWidgetDelegates();

	// 隐藏的模式切换按钮（L3）不参与手柄/键盘焦点导航，避免焦点落到这个透明按钮上
	// 导致后续 LB/RB 输入路由异常（用户实测第二次进入时焦点丢失）。
	if (PreviewModeToggleButton)
	{
		PreviewModeToggleButton->SetIsFocusable(false);
	}

	// L3 预览操控模式的进入/退出提示属于 W_Cloth 蓝图维护（PreviewHintText 配合
	// IsPreviewControlMode 在 EventGraph 更新），C++ 不接管静态提示文案。

	if (!ResolveRuntimeViewModel())
	{
		UE_LOG(LogShoot, Error, TEXT("%s: W_Cloth 缺少名为 WardrobeVM 的 UShootWardrobeViewModel MVVM 上下文。"),
		       *GetNameSafe(this));
		return;
	}

	BindViewModelDelegates();

	PreviewController = NewObject<UShootWardrobePreviewController>(this);
	PreviewController->Initialize(CharacterPreviewViewport, GetOwningPlayer(), PreviewActorClass);

	// ViewModel 初始化会选择 Catalog 中第一个有效页面并广播刷新；预览必须先创建，避免首次广播丢失角色。
	RuntimeViewModel->Initialize(GetOwningPlayer(), WardrobeCatalog, true);
	if (RuntimeViewModel->PageDefinitions.IsValidIndex(RuntimeViewModel->ActivePageIndex))
	{
		const FName InitialTabId = RuntimeViewModel->PageDefinitions[RuntimeViewModel->ActivePageIndex]
			.CategoryTag.GetTagName();
		TopSettingsTabs->SelectTabByID(InitialTabId, true);
	}

	// ViewModel 首次刷新后再次应用矩阵导航规则，确保动态格子已创建。
	ConfigureFocusNavigation();
}

void UShootWardrobeScreen::NativeOnDeactivated()
{
	// 退出衣柜时复位预览操控模式：清空旋转按钮的手柄触发动作，恢复 TabList 的 LB/RB 监听。
	bPreviewControlMode = false;
	if (PreviewRotateLeftButton)
	{
		PreviewRotateLeftButton->SetTriggeringEnhancedInputAction(nullptr);
	}
	if (PreviewRotateRightButton)
	{
		PreviewRotateRightButton->SetTriggeringEnhancedInputAction(nullptr);
	}
	if (TopSettingsTabs)
	{
		TopSettingsTabs->SetListeningForInput(true);
	}
	UnbindStableWidgetDelegates();
	ShutdownRuntime();
	Super::NativeOnDeactivated();
}

void UShootWardrobeScreen::NativeDestruct()
{
	UnbindStableWidgetDelegates();
	ShutdownRuntime();
	Super::NativeDestruct();
}

void UShootWardrobeScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PreviewController)
	{
		PreviewController->Tick(InDeltaTime);
	}
}

// 手柄/键盘焦点目标：策略在 W_Cloth 蓝图 BP_GetDesiredFocusTarget（先当前页第一个格子，
// 空分类时用顶部 Tab 按钮兜底），C++ 只提供下面两个纯数据查询，不参与焦点决策。
UWidget* UShootWardrobeScreen::GetFirstItemEntryForFocus() const
{
	UUniformGridPanel* ActiveItemPanel = RuntimeViewModel
		? GetItemPanelForPage(RuntimeViewModel->ActivePageIndex)
		: ClothingUniformGrid.Get();
	if (ActiveItemPanel && ActiveItemPanel->GetChildrenCount() > 0)
	{
		return ActiveItemPanel->GetChildAt(0);
	}
	return nullptr;
}

void UShootWardrobeScreen::SaveOutfit()
{
	if (RuntimeViewModel)
	{
		RuntimeViewModel->SaveOutfit();
	}
}

void UShootWardrobeScreen::SwitchCharacter()
{
	if (RuntimeViewModel)
	{
		RuntimeViewModel->SwitchCharacter();
	}
}

void UShootWardrobeScreen::RotatePreviewLeft()
{
	if (PreviewController)
	{
		PreviewController->Rotate(-PreviewRotationStepDegrees);
	}
}

void UShootWardrobeScreen::RotatePreviewRight()
{
	if (PreviewController)
	{
		PreviewController->Rotate(PreviewRotationStepDegrees);
	}
}

void UShootWardrobeScreen::RotatePreviewByDelta(float DeltaYawDegrees)
{
	// 调试日志：右摇杆旋转输入链路验证。若此日志不出现，说明 W_Cloth 蓝图里的
	// IA_WardrobePreviewOrbit 增强输入事件未触发（排查输入事件绑定问题用，验证后可移除）。
	UE_LOG(LogShoot, Log, TEXT("RotatePreviewByDelta called: %.2f deg"), DeltaYawDegrees);
	if (PreviewController)
	{
		PreviewController->Rotate(DeltaYawDegrees);
	}
}

// 注：曾尝试"右摇杆 Axis 旋转"（GetPreviewOrbitAxisX 查询 + Tick 驱动），
// 因 CommonUI 的 UI-Only 模式挡真实轴值、All 模式又引入鼠标捕获副作用，已放弃该方案。
// 手柄旋转改走"步进旋转"（RotatePreviewLeft/Right，布尔输入，见 W_Cloth 蓝图）。

void UShootWardrobeScreen::HandlePreviewModeToggleClicked()
{
	TogglePreviewControlMode();
}

// 预览操控模式切换：输入所有权在 C++ 管理（TabList 让权 <-> 旋转按钮接管）。
// 模式状态由蓝图通过 IsPreviewControlMode() 读取，L3 提示文案与视觉反馈留在 W_Cloth。
//
// 输入绑定为什么不需要"把焦点移到旋转按钮"：
// CommonUI 的 TriggeringEnhancedInputAction 绑定判定只看控件是否可见、可用、
// 在激活树内可到达（UIActionRouterTypes.cpp 的 CanWidgetReceiveInput），
// 不要求按钮持有焦点。所以进入模式后 LB/RB 直接落到旋转按钮，
// 视觉焦点高亮留在原地是正常现象，不是 bug。
void UShootWardrobeScreen::TogglePreviewControlMode()
{
	bPreviewControlMode = !bPreviewControlMode;

	if (bPreviewControlMode)
	{
		// 进入预览模式：TabList 让出 LB/RB，旋转按钮接管（必须显式设置触发动作——
		// 运行时 SetTriggeringEnhancedInputAction 会覆盖蓝图默认值，且上次退出时已清空）。
		if (TopSettingsTabs)
		{
			TopSettingsTabs->SetListeningForInput(false);
		}
		if (PreviewRotateLeftButton)
		{
			// 首次进入时从按钮的蓝图配置缓存 IA（W_Cloth 已配 TriggeringEnhancedInputAction），
			// 之后退出清空、再进入时用缓存恢复，避免依赖需要蓝图配置的新属性。
			if (!CachedRotateLeftAction)
			{
				CachedRotateLeftAction = PreviewRotateLeftButton->GetEnhancedInputAction();
			}
			PreviewRotateLeftButton->SetTriggeringEnhancedInputAction(CachedRotateLeftAction);
		}
		if (PreviewRotateRightButton)
		{
			if (!CachedRotateRightAction)
			{
				CachedRotateRightAction = PreviewRotateRightButton->GetEnhancedInputAction();
			}
			PreviewRotateRightButton->SetTriggeringEnhancedInputAction(CachedRotateRightAction);
		}
	}
	else
	{
		// 退出预览模式：清空旋转按钮的触发动作释放 LB/RB，TabList 恢复切分类。
		// 两者不能同时持有同一物理键，否则 TabList 抢不到输入。
		if (PreviewRotateLeftButton)
		{
			PreviewRotateLeftButton->SetTriggeringEnhancedInputAction(nullptr);
		}
		if (PreviewRotateRightButton)
		{
			PreviewRotateRightButton->SetTriggeringEnhancedInputAction(nullptr);
		}
		if (TopSettingsTabs)
		{
			TopSettingsTabs->SetListeningForInput(true);
		}
	}
}

// W_Cloth 焦点导航模型（方向键/左摇杆；一级分类 Tab 不参与焦点，只靠 LB/RB 切换）：
//
//   二级分类首项 <--左-- [入口] [   ] [   ] [   ]
//                        [   ] [   ] [   ] [   ]
//                        [   ] [   ] [   ] [出口] --右--> PreviewRotateLeftButton
//
// 左组 = MainBorder 下动态创建的二级分类按钮 + 物品格子；
// 右组 = PreviewRightOverlay 下静态按钮（五个预览按钮 + 切换角色/保存/返回）。
// 入口/出口固定，中间格子闭合：左右按行、上下按列并在首末行回绕。
//
// 为什么必须留在 C++：左组控件动态创建、没有 W_Cloth 命名 WidgetTree 条目，
// 蓝图 Explicit 导航无法跨 UserWidget 解析目标；这是运行时桥接，不是布局配置。
//
// W_Cloth 蓝图侧待办：PreviewHintText 接 L3 提示；Hair/Face/BodyShape/Preset
// 的 SubTabsVBox Visibility 统一为 Visible。
void UShootWardrobeScreen::ConfigureFocusNavigation()
{
	UVerticalBox* SubCategoryPanel = nullptr;
	UUniformGridPanel* ItemPanel = nullptr;
	if (RuntimeViewModel)
	{
		SubCategoryPanel = GetSubCategoryPanelForPage(RuntimeViewModel->ActivePageIndex);
		ItemPanel = GetItemPanelForPage(RuntimeViewModel->ActivePageIndex);
	}

	UWidget* FirstSubCategoryButton = nullptr;
	if (SubCategoryPanel && SubCategoryPanel->GetChildrenCount() > 0)
	{
		FirstSubCategoryButton = SubCategoryPanel->GetChildAt(0);
	}
	UWidget* FirstItemButton = nullptr;
	if (ItemPanel && ItemPanel->GetChildrenCount() > 0)
	{
		FirstItemButton = ItemPanel->GetChildAt(0);
	}

	// 右组 -> 左组的返回目标：优先当前页第一个二级分类按钮，其次当前页第一个格子。
	// 一级分类 Tab 不参与焦点，因此不做 Tab 兜底。
	UWidget* LeftColumnTarget = FirstSubCategoryButton ? FirstSubCategoryButton : FirstItemButton;

	// 右组 -> 左组：右组按钮是文档化稳定契约，用显式 BindWidgetOptional 引用，
	// 规则直白可读，也避免"父链几层到右组"这类隐性结构假设。
	// 蓝图删掉某个按钮时对应指针为空，该按钮自动退出规则，不影响编译。
	const TArray<UWidget*> RightColumnButtons = {
		PreviewRotateLeftButton.Get(), PreviewRotateRightButton.Get(),
		PreviewZoomInButton.Get(), PreviewZoomOutButton.Get(), PreviewResetViewButton.Get(),
		SwitchCharacterButton.Get(), SaveOutfitButton.Get(), BackButton.Get()
	};
	for (UWidget* RightButton : RightColumnButtons)
	{
		if (RightButton && LeftColumnTarget)
		{
			RightButton->SetNavigationRuleExplicit(EUINavigation::Left, LeftColumnTarget);
		}
	}

	// 左组内部 + 左组到右组：
	// - 二级分类按钮 右方向 -> 矩阵入口（当前页第一个格子）。
	// - 格子按 ItemGridColumnCount 组成闭合矩阵：左上入口、右下出口。
	//   矩阵内部左右按行移动（行尾 -> 下一行行首，行首 -> 上一行行尾），
	//   上下按列移动并在首末行回绕；只有入口左方向、出口右方向允许跨出矩阵。
	//   中间格子不被"左->分类 / 右->右组"劫持，保证矩阵内自由选择。
	if (SubCategoryPanel)
	{
		for (int32 Index = 0; Index < SubCategoryPanel->GetChildrenCount(); ++Index)
		{
			if (UWidget* SubCategoryButton = SubCategoryPanel->GetChildAt(Index))
			{
				if (UWidget* RightTarget = FirstItemButton ? FirstItemButton : PreviewRotateLeftButton.Get())
				{
					SubCategoryButton->SetNavigationRuleExplicit(EUINavigation::Right, RightTarget);
				}
			}
		}
	}
	if (ItemPanel)
	{
		TArray<UWidget*> ItemButtons;
		ItemButtons.Reserve(ItemPanel->GetChildrenCount());
		for (int32 Index = 0; Index < ItemPanel->GetChildrenCount(); ++Index)
		{
			ItemButtons.Add(ItemPanel->GetChildAt(Index));
		}

		const int32 ColumnCount = FMath::Max(1, ItemGridColumnCount);
		for (int32 Index = 0; Index < ItemButtons.Num(); ++Index)
		{
			UWidget* ItemButton = ItemButtons[Index];
			if (!ItemButton)
			{
				continue;
			}

			const int32 Column = Index % ColumnCount;
			const int32 Row = Index / ColumnCount;

			// 左方向：行首按行回绕；左上入口（0 号格子）才允许出矩阵回二级分类。
			UWidget* LeftTarget = nullptr;
			if (Column == 0)
			{
				LeftTarget = (Row == 0) ? FirstSubCategoryButton : ItemButtons[Index - 1];
			}
			else
			{
				LeftTarget = ItemButtons[Index - 1];
			}
			if (LeftTarget)
			{
				ItemButton->SetNavigationRuleExplicit(EUINavigation::Left, LeftTarget);
			}

			// 右方向：下一个索引存在就是矩阵内移动（行尾自然进下一行行首）；
			// 不存在时说明这是矩阵最后一个格子，即出口，允许进右组。
			UWidget* RightTarget = nullptr;
			if (ItemButtons.IsValidIndex(Index + 1))
			{
				RightTarget = ItemButtons[Index + 1];
			}
			else
			{
				RightTarget = PreviewRotateLeftButton.Get();
			}
			if (RightTarget)
			{
				ItemButton->SetNavigationRuleExplicit(EUINavigation::Right, RightTarget);
			}

			// 上/下方向也关在矩阵内：同列上一行/下一行，首末行回绕，
			// 避免焦点从矩阵顶部/底部漏到右组或页面其他区域。
			const int32 UpIndex = Index - ColumnCount;
			if (ItemButtons.IsValidIndex(UpIndex))
			{
				ItemButton->SetNavigationRuleExplicit(EUINavigation::Up, ItemButtons[UpIndex]);
			}
			else
			{
				const int32 LastRowStart = (ItemButtons.Num() - 1) / ColumnCount * ColumnCount;
				int32 WrappedUpIndex = LastRowStart + Column;
				if (!ItemButtons.IsValidIndex(WrappedUpIndex))
				{
					WrappedUpIndex = Column; // 末行没有该列时回到首行同列
				}
				if (ItemButtons.IsValidIndex(WrappedUpIndex) && WrappedUpIndex != Index)
				{
					ItemButton->SetNavigationRuleExplicit(EUINavigation::Up, ItemButtons[WrappedUpIndex]);
				}
			}

			const int32 DownIndex = Index + ColumnCount;
			if (ItemButtons.IsValidIndex(DownIndex))
			{
				ItemButton->SetNavigationRuleExplicit(EUINavigation::Down, ItemButtons[DownIndex]);
			}
			else if (ItemButtons.IsValidIndex(Column) && Column != Index)
			{
				ItemButton->SetNavigationRuleExplicit(EUINavigation::Down, ItemButtons[Column]);
			}
		}
	}

	// 一级分类 Tab 不参与焦点导航（只靠 LB/RB 切换），因此这里不再给 Tab 按钮设置方向规则。
}

void UShootWardrobeScreen::ZoomPreviewIn()
{
	if (PreviewController)
	{
		PreviewController->Zoom(-PreviewZoomStep);
	}
}

void UShootWardrobeScreen::ZoomPreviewOut()
{
	if (PreviewController)
	{
		PreviewController->Zoom(PreviewZoomStep);
	}
}

void UShootWardrobeScreen::ResetPreview()
{
	if (PreviewController)
	{
		PreviewController->ResetView();
	}
}

bool UShootWardrobeScreen::ResolveRuntimeViewModel()
{
	if (RuntimeViewModel)
	{
		return true;
	}

	// 优先复用 W_Cloth 的 MVVM 上下文（历史路径）。CreateInstance 类型的上下文只有在被绑定引用时才会被实例化；
	// 绑定移除后 GetViewModel 可能返回空，这里回退为本地创建，避免衣柜页面因拿不到 VM 而整页空置（预览区空白、格子不生成）。
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	if (View)
	{
		const TScriptInterface<INotifyFieldValueChanged> ViewModelInterface = View->GetViewModel(TEXT("WardrobeVM"));
		RuntimeViewModel = Cast<UShootWardrobeViewModel>(ViewModelInterface.GetObject());
	}

	if (!RuntimeViewModel)
	{
		RuntimeViewModel = NewObject<UShootWardrobeViewModel>(this);
	}

	return RuntimeViewModel != nullptr;
}

void UShootWardrobeScreen::BindStableWidgetDelegates()
{
	TopSettingsTabs->OnTabSelected.RemoveDynamic(this, &ThisClass::HandleTopLevelTabSelected);
	TopSettingsTabs->OnTabSelected.AddDynamic(this, &ThisClass::HandleTopLevelTabSelected);

	if (PreviewModeToggleButton)
	{
		// OnClicked() 是 UCommonButtonBase 的 DECLARE_EVENT 原生事件（非动态委托），用 AddUObject。
		PreviewModeToggleButton->OnClicked().RemoveAll(this);
		PreviewModeToggleButton->OnClicked().AddUObject(this, &ThisClass::HandlePreviewModeToggleClicked);
	}
}

void UShootWardrobeScreen::UnbindStableWidgetDelegates()
{
	if (TopSettingsTabs)
	{
		TopSettingsTabs->OnTabSelected.RemoveDynamic(this, &ThisClass::HandleTopLevelTabSelected);
	}
	if (PreviewModeToggleButton)
	{
		PreviewModeToggleButton->OnClicked().RemoveAll(this);
	}
}

void UShootWardrobeScreen::BindViewModelDelegates()
{
	RuntimeViewModel->OnViewStateChanged.RemoveDynamic(this, &ThisClass::HandleViewStateChanged);
	RuntimeViewModel->OnViewStateChanged.AddDynamic(this, &ThisClass::HandleViewStateChanged);
	RuntimeViewModel->OnPreviewModeChanged.RemoveDynamic(this, &ThisClass::HandlePreviewModeChanged);
	RuntimeViewModel->OnPreviewModeChanged.AddDynamic(this, &ThisClass::HandlePreviewModeChanged);
}

void UShootWardrobeScreen::ShutdownRuntime()
{
	ClearPagePanels();

	if (RuntimeViewModel)
	{
		RuntimeViewModel->OnViewStateChanged.RemoveDynamic(this, &ThisClass::HandleViewStateChanged);
		RuntimeViewModel->OnPreviewModeChanged.RemoveDynamic(this, &ThisClass::HandlePreviewModeChanged);
		RuntimeViewModel->Shutdown();
	}

	if (PreviewController)
	{
		PreviewController->Shutdown();
		PreviewController = nullptr;
	}
}

void UShootWardrobeScreen::ClearPagePanels()
{
	// 页面关闭时释放动态创建的条目和 ViewModel 引用，CommonUI 下次激活会从最新快照重新生成。
	for (int32 PageIndex = 0; PageIndex < 5; ++PageIndex)
	{
		if (UVerticalBox* SubCategoryPanel = GetSubCategoryPanelForPage(PageIndex))
		{
			SubCategoryPanel->ClearChildren();
		}
		if (UUniformGridPanel* ItemPanel = GetItemPanelForPage(PageIndex))
		{
			ItemPanel->ClearChildren();
		}
	}
}

void UShootWardrobeScreen::RefreshVisiblePage()
{
	if (!RuntimeViewModel)
	{
		return;
	}

	const int32 ActivePageIndex = RuntimeViewModel->ActivePageIndex;
	const TArray<UObject*> SubCategoryItems = RuntimeViewModel->GetCurrentSubCategoryListItems();
	const TArray<UObject*> VisibleItems = RuntimeViewModel->GetVisibleListItems();

	UVerticalBox* SubCategoryPanel = GetSubCategoryPanelForPage(ActivePageIndex);
	UUniformGridPanel* ItemPanel = GetItemPanelForPage(ActivePageIndex);
	if (!SubCategoryPanel || !ItemPanel || !SubCategoryEntryClass || !ItemEntryClass)
	{
		return;
	}

	// 五个 TabContent 是彼此独立的视图。这里只切换目标页并填充该页，不要求各页使用相同布局。
	WardrobeTopLevelContentSwitcher->SetActiveWidgetIndex(ActivePageIndex);

	// 一级页未变化时，二级分类对象身份保持稳定，不销毁正在 hover/focus 的按钮。
	if (!PanelHasEntryObjects(SubCategoryPanel, SubCategoryItems))
	{
		SubCategoryPanel->ClearChildren();
		for (UObject* Item : SubCategoryItems)
		{
			if (UShootObjectEntryButtonBase* Entry = CreateEntry(SubCategoryEntryClass, Item))
			{
				if (UVerticalBoxSlot* EntrySlot = SubCategoryPanel->AddChildToVerticalBox(Entry))
				{
					EntrySlot->SetPadding(SubCategorySlotPadding);
				}
			}
		}
	}

	ItemPanel->ClearChildren();
	ItemPanel->SetSlotPadding(ItemGridSlotPadding);
	const int32 EffectiveItemLimit = FMath::Max(1, MaxVisibleItemCount);
	const int32 VisibleItemCount = FMath::Min(VisibleItems.Num(), EffectiveItemLimit);
	for (int32 Index = 0; Index < VisibleItemCount; ++Index)
	{
		if (UShootObjectEntryButtonBase* Entry = CreateEntry(ItemEntryClass, VisibleItems[Index]))
		{
			const int32 Row = Index / FMath::Max(1, ItemGridColumnCount);
			const int32 Column = Index % FMath::Max(1, ItemGridColumnCount);
			if (UUniformGridSlot* EntrySlot = ItemPanel->AddChildToUniformGrid(Entry, Row, Column))
			{
				EntrySlot->SetHorizontalAlignment(HAlign_Left);
				EntrySlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}
	if (VisibleItems.Num() > EffectiveItemLimit)
	{
		UE_LOG(LogShoot, Warning, TEXT("%s: 当前分类有 %d 个条目，超过 W_Cloth 配置的上限 %d。"),
		       *GetNameSafe(this), VisibleItems.Num(), EffectiveItemLimit);
	}

	if (PreviewController)
	{
		PreviewController->RefreshAppearance();
	}

	// 物品面板是 ClearChildren + 重建的，切换一级页/二级分类后旧格子被销毁，
	// 手柄/键盘焦点若停留在旧格子上会丢失。先重建入口/出口矩阵导航规则，
	// 再请求 CommonUI 重新评估焦点，使蓝图 BP_GetDesiredFocusTarget 再次执行。
	ConfigureFocusNavigation();
	RequestRefreshFocus();
}

void UShootWardrobeScreen::RefreshPreview()
{
	if (PreviewController && RuntimeViewModel)
	{
		PreviewController->SetCameraMode(RuntimeViewModel->PreviewCameraMode);
	}
}

UVerticalBox* UShootWardrobeScreen::GetSubCategoryPanelForPage(int32 PageIndex) const
{
	UVerticalBox* const Panels[] = {
		ClothingSubTabsVBox.Get(), HairSubTabsVBox.Get(), FaceSubTabsVBox.Get(),
		BodyShapeSubTabsVBox.Get(), PresetSubTabsVBox.Get()
	};
	return PageIndex >= 0 && PageIndex < UE_ARRAY_COUNT(Panels) ? Panels[PageIndex] : nullptr;
}

UUniformGridPanel* UShootWardrobeScreen::GetItemPanelForPage(int32 PageIndex) const
{
	UUniformGridPanel* const Panels[] = {
		ClothingUniformGrid.Get(), HairUniformGrid.Get(), FaceUniformGrid.Get(),
		BodyShapeUniformGrid.Get(), PresetUniformGrid.Get()
	};
	return PageIndex >= 0 && PageIndex < UE_ARRAY_COUNT(Panels) ? Panels[PageIndex] : nullptr;
}

bool UShootWardrobeScreen::PanelHasEntryObjects(const UPanelWidget* Panel,
	const TArray<UObject*>& ExpectedItems) const
{
	if (!Panel || Panel->GetChildrenCount() != ExpectedItems.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < ExpectedItems.Num(); ++Index)
	{
		const UShootObjectEntryButtonBase* Entry =
			Cast<UShootObjectEntryButtonBase>(Panel->GetChildAt(Index));
		if (!Entry || Entry->GetEntryObject() != ExpectedItems[Index])
		{
			return false;
		}
	}
	return true;
}

UShootObjectEntryButtonBase* UShootWardrobeScreen::CreateEntry(
	TSubclassOf<UShootObjectEntryButtonBase> EntryClass, UObject* EntryObject)
{
	if (!EntryClass || !EntryObject)
	{
		return nullptr;
	}

	UShootObjectEntryButtonBase* Entry = CreateWidget<UShootObjectEntryButtonBase>(
		GetOwningPlayer(), EntryClass);
	if (Entry)
	{
		Entry->SetEntryObject(EntryObject);
		Entry->OnEntryClicked().AddUObject(this, &ThisClass::HandleEntryClicked);
		Entry->OnEntryHovered().AddUObject(this, &ThisClass::HandleEntryHovered);
		Entry->OnEntryUnhovered().AddUObject(this, &ThisClass::HandleEntryUnhovered);
	}
	return Entry;
}

void UShootWardrobeScreen::HandleTopLevelTabSelected(FName TabId)
{
	if (RuntimeViewModel)
	{
		RuntimeViewModel->SelectTopLevelTab(TabId);
	}
}

void UShootWardrobeScreen::HandleViewStateChanged()
{
	RefreshVisiblePage();
}

void UShootWardrobeScreen::HandlePreviewModeChanged()
{
	RefreshPreview();
}

void UShootWardrobeScreen::HandleEntryClicked(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject)
{
	(void)EntryButton;
	if (!EntryObject || !RuntimeViewModel)
	{
		return;
	}

	if (UShootWardrobeItemViewModel* Item = Cast<UShootWardrobeItemViewModel>(EntryObject))
	{
		const bool bIsEquipRequest = !Item->bEquipped;
		if (PreviewController)
		{
			// 连续快速点击时先丢弃上一件物品尚未消费的表演，卸下操作也不能误播旧动画。
			PreviewController->ClearQueuedPresentationAnimation();
		}
		if (bIsEquipRequest && PreviewController)
		{
			// 先排队，再提交服务器请求；Listen Server 可能在 RPC 返回前同步触发外观更新。
			PreviewController->QueuePresentationAnimation(Item->PreviewEquipAnimation);
		}

		if (!RuntimeViewModel->RequestToggleEquip(Item) && PreviewController)
		{
			PreviewController->ClearQueuedPresentationAnimation();
		}
		return;
	}

	if (UShootWardrobeSubCategoryViewModel* SubCategory =
		Cast<UShootWardrobeSubCategoryViewModel>(EntryObject))
	{
		RuntimeViewModel->SelectSubCategory(SubCategory->CategoryTag);
	}
}

void UShootWardrobeScreen::HandleEntryHovered(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject)
{
	// 悬停（鼠标移入）与手柄/键盘焦点导航都经 CommonUI 的 NativeOnHovered 汇聚到这里。
	// 详情卡的内容、左右定位、跟随与动画都交给 W_Cloth 蓝图；C++ 只透传悬停条目快照与格子引用，
	// 不在这里写显隐或坐标，也不做任何输入设备分支。二级分类按钮的悬停会被 Cast 过滤掉。
	if (UShootWardrobeItemViewModel* Item = Cast<UShootWardrobeItemViewModel>(EntryObject))
	{
		BP_OnWardrobeItemHovered(Item, EntryButton);
	}
}

void UShootWardrobeScreen::HandleEntryUnhovered(UShootObjectEntryButtonBase* EntryButton, UObject* EntryObject)
{
	if (UShootWardrobeItemViewModel* Item = Cast<UShootWardrobeItemViewModel>(EntryObject))
	{
		BP_OnWardrobeItemUnhovered(Item, EntryButton);
	}
}
