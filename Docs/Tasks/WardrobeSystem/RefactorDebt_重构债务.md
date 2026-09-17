# 衣柜系统重构债务

# 已完成的分类重构

- Mutable 外观表、衣柜分类表、Catalog 和 18 个 ItemDefinition 已迁移到 GameplayTag。
- 已删除 FName 分类字段、分类枚举过滤、分类字符串比较、重复的当前分类状态和相机模式推断。
- `EShootWardrobeCategory` 仍用于物品类型与详情显示，不参与导航或分类筛选。
- `EShootWardrobeAppearanceSection` 仍用于选择 W_Cloth 中五套固定 BindWidget 容器；它是 UI 布局索引，不是物品分类数据。
- `ResolveTopLevelContentWidget`、`ResolveActiveSubTabsPanel`、`ResolveActiveUniformGrid` 保留为固定蓝图容器映射。若未来改为蓝图提供容器对象，再在同一改动中删除这三个函数，不先增加第二套映射。

# UI 可维护性与迁移债务

## 已完成的动态条目去 ListView 化

- `W_Cloth_Item` 与 `W_WardrobeSubCategoryButton` 已改为继承项目类 `UShootObjectEntryButtonBase`。
- 旧 `ULyraObjectListEntryButtonBase` 已删除；衣柜不再保留 `IUserObjectListEntry`、`UListView` 或列表选择同步兼容路径。
- `UShootObjectEntryButtonBase` 只保留普通动态 Panel 所需的对象快照注入和对象点击回传。条目蓝图继续通过 `On Entry Object Set` 刷新表现，页面协调层负责分类切换和装备请求。

当前衣柜的数据和服务器权威链可以保留，但 Widget 层不应作为最终架构继续扩展。

已核对的代码事实：

- `UShootWardrobeWidgetBase.h` 约 375 行，包含 33 个 `BindWidgetOptional`。
- `UShootWardrobeWidgetBase.cpp` 约 1659 行，同时处理数据查询、分类、动态控件、焦点、鼠标命中、Tooltip 定位、隐藏状态和预览相机。
- `UShootWardrobeItemWidgetBase` 与 `UShootWardrobeItemTooltipBase` 各自还包含 8 个命名控件绑定。
- `W_Cloth`、`W_Cloth_Item`、`W_ClothItemTooltip` 均直接继承衣柜 C++ Widget 类，没有 MVVM ViewModel 或 MVVM Binding。
- C++ 仍直接设置颜色、固定文本、对勾字符、Slot Padding、对齐和预览相机兜底坐标。
- Item 与 Tooltip 还通过 `WidgetTree->FindWidget` 按名称查找控件，形成第二套隐式绑定。
- `NativeOnInitialized` 连续调用两次 `ApplyWardrobeHiddenUIState()`，说明当前大类已经开始出现人工维护遗漏。

质量结论：

- 目录、GameplayTag 分类、InventoryManager、PlayerState 校验、Mutable 和 SaveGame 分层是可维护的。
- 二级分类由 DataAsset 驱动，新增普通衣服和二级分类不需要改 C++，这部分扩展性合格。
- 一级页仍由五套 C++ 指针、枚举和 switch 固定映射；新增一级分类仍需要改 C++、蓝图控件名和绑定，扩展性不合格。
- 当前 Widget 层功能可用，但不够优雅，不符合 AGENTS 中“固定布局和视觉由控件蓝图维护、C++ 只处理数据与状态”的最终要求。
- 当前资产不适合单独迁移到没有这些 C++ 父类的项目。

关于 `BindWidget` 的准确结论：

- 蓝图迁移后无法加载的首要原因是 `NativeParentClass` 不存在；即使父类没有 `BindWidget`，缺少 C++ 父类仍可能导致资产无法打开。
- `BindWidget` 增加的是第二层结构耦合：控件名称和类型必须与 C++ 一致，UI 设计师无法自由重命名或重排。
- 因此不能只把 `BindWidget` 换成 `FindWidget`。按名称查找仍是结构耦合，而且失败更隐蔽。

目标方向：

- W_Cloth 优先继承项目稳定的 `ULyraActivatableWidget`，不再以大型衣柜 C++ Widget 作为 NativeParentClass。
- C++ 保留衣柜查询、服务器命令、分类快照和预览 Actor 等业务服务，通过 BlueprintCallable 接口、GameplayMessage 或 ViewModel 提供给蓝图。
- 使用 ViewModel 提供当前分类、条目快照、选择状态、拥有状态和空列表状态；数组允许整体刷新，不需要为每个元素建立复杂通知。
- 固定按钮、五个页面容器、隐藏 UI、Tooltip 展示、布局、动画和镜头按钮响应由 W_Cloth 蓝图维护。
- 动态条目优先评估 TileView/ListView 与 Entry Widget，避免主 Widget C++ 直接创建并摆放全部格子。
- 如果仍需 Native Widget 基类，只保留极少数稳定语义端口和生命周期接口，不绑定整棵控件树。
- 重构时先建立新的单一链路并验收，再删除旧 C++ Widget 基类；不得长期保留 MVVM、BindWidget 和 FindWidget 三套并行更新路径。

# 2026-08 已解决

- Tooltip 悬停回归已修复：条目基类 `UShootObjectEntryButtonBase` 新增与点击对称的 `EntryHovered` 语义委托，`UShootWardrobeScreen::HandleEntryHovered` 汇入 `SelectedItem`；未引入 FindWidget、MVVM 之外的第二套更新路径。
- `UShootCharacterSwitchWidgetBase` 已删除；菜单切换由 `UShootWardrobeViewModel` 直连 PlayerController。
- 待 PIE 验收：`ItemTooltipPanel` 的 MVVM 绑定随本次 C++ 修复生效情况。

# PressToInteract 状态

- HomeMap 的衣柜全部获取和全部清除调试 Actor 已由用户确认单按 E 后生效。
- PressToInteract 不再是衣柜遗留问题。

## 运行验收

- 五个一级页的 VBox、滚动区和网格保持对齐。
- 键鼠与手柄焦点切换分类后不落到已销毁的动态格子。
- 预览角色头身动画保持同步。

已由用户确认：

- 隐藏 UI 后，按钮与输入均能恢复 UI。
- 男女主装备状态、预览外观和保存恢复保持独立。
- 当前未再观察到 Tooltip 相关 Slate/UMG 崩溃。

# 已确认无需重构

- `WardrobeItems` 继续作为唯一正式目录，不复制到分类定义。
- `AppearanceTags` 继续作为 Mutable 外观唯一数据源。
- `AShootWardrobePreviewActor` 保持单一预览职责，不为形式上的分层继续拆类；具体视觉和镜头点位继续由其蓝图子类配置。
- 服务器装备、库存和存档逻辑不迁入 Widget 蓝图。

# 删除规则

- 删除前检查 C++、蓝图、配置、软路径、Asset Referencers 和 Cook 依赖。
- 替代链路完成后同一提交删除旧字段，不保留双路径。
- 废弃控件与旧函数图从资产中真正删除，不用 Collapsed 或空函数保留。
