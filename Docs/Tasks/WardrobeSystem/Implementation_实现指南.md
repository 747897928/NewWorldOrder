# 衣柜与换装实现指南

# 当前实现

衣柜页面 `/Game/UI/Mutable/W_Cloth` 继承 `UShootWardrobeScreen`，通过 CommonUI 层级打开。页面协调层（`UShootWardrobeScreen`）负责 MVVM 生命周期、五套 Panel 与条目分发；数据快照（`UShootWardrobeViewModel`）负责目录、库存、分类与选中状态；控件蓝图负责布局、固定按钮事件、样式与动画。

当前正式目录：

```text
/Game/Blueprints/Mutable/Wardrobe/DA_WardrobeCatalog
```

当前正式物品：

```text
/Game/Blueprints/Mutable/Wardrobe/Items/BP_Item_Wardrobe_*
```

# 核心运行链

1. `W_Cloth` 激活时 `UShootWardrobeScreen` 创建并初始化 `UShootWardrobeViewModel`。
2. ViewModel 从 `WardrobeCatalogAsset` 读取目录，并从 PlayerState 的 Persistent InventoryManager 合并拥有状态。
3. ItemDefinition 的 WardrobeItem Fragment 转换成 `FShootWardrobeItemViewData` / `UShootWardrobeItemViewModel`。
4. ViewModel 按当前分类和性别过滤，Screen 的 `RefreshVisiblePage` 动态创建 `W_Cloth_Item`。
5. 条目交互统一走 `UShootObjectEntryButtonBase` 语义委托：
   - 点击 `EntryClicked` -> `HandleEntryClicked` -> `RequestToggleEquip`（装备/卸下）。
   - 悬停/焦点 `EntryHovered` -> `HandleEntryHovered` -> `SelectItem`（只刷新详情卡，不装备）。
   - 鼠标、手柄/键盘焦点与触屏由 CommonUI 统一驱动，C++ 不写设备分支。
6. `SelectedItem`（FieldNotify）驱动 `ItemTooltipPanel` 的 MVVM 绑定显示详情卡。
7. `AShootPlayerState` 在服务器重新校验物品并更新当前性别 AppearanceTags。
8. Mutable 外观组件刷新正式角色和衣柜预览角色。
9. 保存搭配时写入角色快照与 SaveGame。

# 当前类职责

## `UShootWardrobeCatalogDataAsset`

- `WardrobeItems` 是正式物品目录。
- `PageDefinitions` 保存现有一级页配置。
- `SubCategoryDefinitions` 保存现有二级分类显示与筛选配置。
- `AppendWardrobeItemsTo` 同时供衣柜、调试发放 Actor 和调试生成器使用。

- `PageDefinitions.CategoryTag` 与 `DefaultSubCategoryTag` 使用 `FGameplayTag`。
- `SubCategoryDefinitions` 使用 `ParentCategoryTag`、`CategoryTag` 和 `bIncludeChildCategories` 描述层级筛选。
- 分类显示文本、顺序和预览镜头来自 DA；Widget C++ 不保存分类字符串或枚举筛选表。

## `UShootInventoryFragment_WardrobeItem`

- `SuitableGender`：适用性别。
- `Rarity`、`SortPriority`、`Description`：UI 数据。
- `AppearanceTags`：装备后写入 Mutable 的正式标签。
- `SubCategoryTags` 是 `FGameplayTagContainer`，允许同一物品属于多个二级分类。
- `BuildTagsAfterEquip` 按 Mutable 槽位替换旧选项。
- `BuildTagsAfterUnequip` 只移除该物品写入的标签。

## `UShootWardrobeScreen`（页面协调层）

- W_Cloth 的 NativeParentClass；维护 MVVM 生命周期与五套固定 Panel。
- 动态创建二级分类按钮和物品格子，并绑定条目的点击/悬停语义委托。
- 只把业务转发给 `UShootWardrobeViewModel`，不直接访问库存、Mutable、SaveGame 或服务器 RPC。
- 固定按钮（保存、切换角色、隐藏 UI、预览旋转缩放）由 W_Cloth EventGraph 调用本类的 BlueprintCallable 入口。

## `UShootWardrobeViewModel`（数据快照）

- 读取目录与玩家库存，生成页面、分类与条目快照。
- `SelectedItem` 是 Tooltip 详情卡的唯一数据源：点击装备、鼠标悬停与手柄/键盘焦点导航都汇入它。
- 通过 FieldNotify/委托通知页面刷新 Tab、GridPanel 和 Tooltip。
- 装备、卸下、保存与角色切换请求最终交回 `AShootPlayerState` / `AShootPlayerController` 服务器校验。
- 不保存具体物品资产路径，不维护分类中文文本。

## `AShootWardrobePreviewActor`

- 只存在于 UMG Viewport 的预览世界。
- 使用 `UMutableAppearanceComponent` 同步当前性别与外观标签。
- BodyMesh 通过 LeaderPose 跟随 HeadMesh。
- CameraAnchor 与 CameraFocusTarget 由 `BP_ShootWardrobePreviewActor` 调整。
- 灯光、背景、材质和构图参数由蓝图子类维护。
- Item Fragment 的 `PreviewEquipAnimation` 只供预览 Actor 使用。装备请求前暂存动画，Mutable 异步完成网格更新后在 `PresentationAnimationSlot` 播放，避免表演落在旧网格；正式角色不播放该动画。

## 角色切换入口

- 旧菜单模板类 `UShootCharacterSwitchWidgetBase` 已于 2026-08 删除（全仓零引用，git 可恢复）。
- 衣柜切换改由 `UShootWardrobeViewModel::SwitchCharacter` 直接调用 `AShootPlayerController::RequestSwitchCharacter`，并监听 `OnCharacterSwitchResult` 刷新当前性别。
- `IA_SwitchCharacter` 与 `IMC_CharacterSwitchMenu` 资产保留，供后续独立菜单长按入口复用；键位分配继续走 Enhanced Input 的 IA/IMC，不在 C++ 硬编码键盘或手柄分支。

## `W_Cloth_Item`（物品条目）

- 继承 `UShootObjectEntryButtonBase`（CommonUI 按钮），在 `On Entry Object Set` 中从 `UShootWardrobeItemViewModel` 刷新图标、锁定、拥有与已装备视觉。
- 悬停、焦点与点击只通过基类语义委托上报对象身份，不持有页面引用，不直接调用服务器 RPC。

## `W_ClothItemTooltip`（详情卡）

- 纯蓝图控件（父类 `UCommonUserWidget`），内嵌在 W_Cloth 的 `ItemTooltipPanel`。
- 内容与显隐通过 MVVM 绑定跟随 `SelectedItem`；悬停、焦点与点击共用同一个常驻详情卡。
- 生命周期跟随 W_Cloth，不在运行时 AddToViewport，也不自己推算屏幕位置。

## `AShootPlayerState`

- 服务器校验 ItemInstance 属于当前玩家 Persistent InventoryManager。
- 校验 WardrobeItem Fragment 和适用性别。
- 更新男女主各自 AppearanceTags。
- 保存当前搭配和角色快照。

## `AShootInventoryGrantActor`

- 开发期通过正式交互链发放 Persistent 服装、武器或资源。
- Persistent 服装进入 InventoryManager；数量型资源进入 ResourceInventory。
- 授予 Persistent 内容后触发 SaveGame 捕获，RuntimeOnly 不写账号存档。
- `ItemGrants` 为空时可以读取 `WardrobeCatalogAsset`，用于领取完整衣柜目录。
- `DebugOperation = GrantConfiguredContents` 时按 `ItemGrants` 或 `WardrobeCatalogAsset` 授予内容。
- `DebugOperation = ClearWardrobeOwnership` 时只移除 `WardrobeCatalogAsset.WardrobeItems` 对应的 Persistent 服装，并立即覆盖存档；不会清空武器、资源或 RuntimeOnly 物品。

## `AShootInventoryGrantSpawner`

- 定时生成并配置一个 GrantActor，不直接写玩家库存。
- 支持 PressToInteract 与 AutoOverlap 两种调试入口。
- 下一轮生成前清理未领取的旧 Actor，避免地图堆积。

## `USaveGameSubsystem`

- 保存 Persistent ItemInstance 和男女主各自 AppearanceTags。
- PIE 使用独立槽名，但已有槽位存在时不得因内存对象为空而覆盖存档。
- 衣柜 Widget 不直接序列化 SaveGame。

# `W_Cloth` 绑定约定

一级内容：

- `WardrobeTopLevelContentSwitcher`
- `ClothingTabContent`
- `HairTabContent`
- `FaceTabContent`
- `BodyShapeTabContent`
- `PresetTabContent`

二级分类容器：

- `ClothingSubTabsVBox`
- `HairSubTabsVBox`
- `FaceSubTabsVBox`
- `BodyShapeSubTabsVBox`
- `PresetSubTabsVBox`

物品网格：

- `ClothingUniformGrid`
- `HairUniformGrid`
- `FaceUniformGrid`
- `BodyShapeUniformGrid`
- `PresetUniformGrid`

预览与操作：

- `CharacterPreviewViewport`
- `CharacterPreviewPanel`
- `MainUIRoot`
- `PreviewControlsHBox`
- `HideUIButton`
- `BackButton`
- `SaveOutfitButton`
- `SwitchCharacterButton`
- `ItemTooltipPanel`
- `EmptyStateText`

固定按钮的点击事件保留在 W_Cloth EventGraph，调用父类 BlueprintCallable 入口。固定文案、图标、尺寸、位置和动画不写入 C++。

# 输入与隐藏 UI

- 鼠标左键或右键拖拽旋转预览角色。
- 鼠标滚轮缩放，左键双击重置。
- 五个预览按钮提供可聚焦的旋转、缩放和重置入口。
- `HideUIButton` 位于 `MainUIRoot` 外部，隐藏内容后仍可恢复。
- C++ 只切换 `MainUIRoot` 与 `PreviewControlsHBox`，不按旧控件名单逐个隐藏。
- Widget 通过 OwningPlayer 工作，不使用 FirstPlayerController。
- L3 预览操控的进入/退出提示由 W_Cloth EventGraph 维护：用 PreviewModeToggleButton 的 OnClicked 与 IsPreviewControlMode 更新 PreviewHintText；C++ 不写提示文案。

# 焦点分组与导航（2026-08）

- 一级分类 Tab（TopSettingsTabs）不参与焦点，只靠 LB/RB 切换。
- 左组 = MainBorder 下各页动态创建的二级分类按钮和物品格子；右组 = PreviewRightOverlay 下五个预览按钮 + 切换角色/保存/返回。
- 导航模型（由 C++ `UShootWardrobeScreen::ConfigureFocusNavigation()` 实现，页面刷新时重建规则）：
  - 格子是闭合矩阵：左上 = 入口，右下 = 出口。
  - 入口左方向 -> 当前页二级分类首项；出口右方向 -> 右组 PreviewRotateLeftButton。
  - 矩阵内：左右按行移动（行尾 -> 下一行行首，行首 -> 上一行行尾）；上下按列移动，首末行回绕；中间格子不跨出矩阵。
  - 二级分类按钮右方向 -> 矩阵入口；右组所有按钮左方向 -> 二级分类首项（无分类时回格子首项）。
- 为什么焦点桥接留在 C++：左组控件是动态创建，没有 W_Cloth 命名 WidgetTree 条目，蓝图 Explicit 导航无法跨 UserWidget 解析目标；右组按钮是文档化稳定契约，用 `BlueprintReadOnly + BindWidgetOptional` 显式引用，规则直白可读。除此之外的布局、可见性、样式仍在 W_Cloth 蓝图配置。
- W_Cloth 蓝图侧（2026-08 用户完成并 PIE 验收通过，资产改动尚未提交）：
  - EventGraph：PreviewModeToggleButton.OnClicked -> IsPreviewControlMode -> 更新 PreviewHintText（L3 进入/退出提示）。
  - Hair/Face/BodyShape/Preset 四个 SubTabsVBox 的 Visibility 统一为 Visible（与 ClothingSubTabsVBox 一致）。

# 新增一件衣服

## 1. 确认 Mutable 外观选项

先确认 Mutable Customizable Object 中已经存在这件衣服对应的参数选项。`AppearanceTags` 只负责选择已有选项，不会创建模型、材质或 Mutable 参数。

如果具体外观标签尚不存在，在下面的 Gameplay Tag DataTable 新增一行：

```text
/Game/Blueprints/Mutable/DataTable/DT_MutableAppearanceGameplayTags
```

标签表达具体外观选项，例如：

```text
Female.Body.Shirts.Camisole
Male.Body.Pants.JeansMale
```

不要把具体物品标签写进 `DT_WardrobeCategoryGameplayTags`。新增 GameplayTag 后应重启编辑器，确认标签可在属性面板中选择。

## 2. 创建 ItemDefinition 蓝图

在下面的目录复制一件同部位、同性别的现有物品：

```text
/Game/Blueprints/Mutable/Wardrobe/Items
```

例如新增女式上衣时，可以复制 `BP_Item_Wardrobe_Female_Camisole` 或 `BP_Item_Wardrobe_Female_TShirts`。按 `BP_Item_Wardrobe_性别_物品名` 命名，避免从鞋子或发型蓝图复制后漏改部位数据。

在 ItemDefinition 默认值中设置：

- `DisplayName`：衣柜格子和详情卡显示的商品名称，例如“T恤”“牛仔裤”“运动鞋”。不要添加“测试”“女主”“男主”等前缀，适用性别由 Tooltip 根据 `SuitableGender` 单独显示。
- `Icon`：格子图标。
- `Fragments` 中保留一个 `UShootInventoryFragment_WardrobeItem` 和现有 StackRules Fragment。
- StackRules 保持 Persistent 物品所需的非堆叠设置，不把衣服配置成数量型资源。

## 3. 设置 WardrobeItem Fragment

在 `UShootInventoryFragment_WardrobeItem` 中设置：

- `SuitableGender`：Female、Male 或 UNKNOWN。
- `Category`：详情卡显示的物品类型，例如 UpperBody、LowerBody、Dress、Footwear、Hair；它不参与 GameplayTag 分类筛选。
- `Rarity`：详情显示的稀有度。
- `SortPriority`：同一分类内的排序权重。
- `Description`：面向玩家的正式说明，不使用调试文案。
- `AppearanceTags`：装备后写入 Mutable 的具体外观标签。
- `SubCategoryTags`：物品在衣柜中出现的 UI 分类。

女式吊带上衣示例：

```text
SuitableGender = Female
Category = UpperBody
AppearanceTags = Female.Body.Shirts.Camisole
SubCategoryTags = Wardrobe.Category.Clothing.UpperBody
```

分类只填写最具体的叶子标签。填写 `Wardrobe.Category.Clothing.UpperBody` 后，物品会自动进入“全部服装”，不需要重复填写 `Wardrobe.Category.Clothing`。

如果同一物品确实属于多个二级分类，可以在 `SubCategoryTags` 中添加多个叶子标签。不要为了同一物品创建多份 ItemDefinition，也不要把 `WardrobeItems` 复制到分类定义中。

## 4. 加入正式目录

打开：

```text
/Game/Blueprints/Mutable/Wardrobe/DA_WardrobeCatalog
```

将新的 ItemDefinition 加入 `WardrobeItems`。这里决定物品是否属于正式衣柜目录；`PageDefinitions` 和 `SubCategoryDefinitions` 不需要为普通新衣服修改。

只有需要新增一种二级分类时，才需要：

1. 在 `DT_WardrobeCategoryGameplayTags` 注册新的 `Wardrobe.Category.*` 标签。
2. 在 `DA_WardrobeCatalog.SubCategoryDefinitions` 添加显示文本、父分类、镜头模式和匹配规则。

## 5. 授予物品

加入 `WardrobeItems` 不等于玩家已经拥有该物品。衣柜会将目录与玩家的 Persistent InventoryManager 合并：

- Catalog 中存在但库存中没有：显示为未拥有或锁定。
- Catalog 和 Persistent InventoryManager 中都存在：可以装备。

开发期可以使用读取 `WardrobeCatalogAsset` 的 `AShootInventoryGrantActor`，或在其 `ItemGrants` 中单独加入新物品进行授予。正式游戏必须通过商店、奖励或其他服务器权威入口写入 Persistent InventoryManager。

## 6. 编译与验收

1. 编译并保存新的 ItemDefinition 蓝图。
2. 如果新增了 GameplayTag，重启编辑器确认标签仍然有效。
3. 用调试发放入口领取物品，确认它进入 Persistent InventoryManager。
4. 打开衣柜，确认物品出现在目标二级分类和对应“全部”分类。
5. 验证图标、名称、描述、稀有度、拥有状态和排序。
6. 验证装备后 Mutable 外观正确，卸下后只移除该物品写入的标签。
7. 切换男女主，确认性别过滤和各自搭配互不干扰。
8. 保存并重新进入，确认物品所有权与搭配恢复。

普通新增衣服只修改 ItemDefinition 蓝图和 `DA_WardrobeCatalog`，不修改 C++。

# GameplayTag 分类实现

- Mutable 外观标签表：`/Game/Blueprints/Mutable/DataTable/DT_MutableAppearanceGameplayTags`。
- 衣柜分类标签表：`/Game/Blueprints/Mutable/DataTable/DT_WardrobeCategoryGameplayTags`。
- `Female.*`、`Male.*` 继续表示 Mutable 外观；`Wardrobe.Category.*` 表示 UI 分类。
- 具体分类调用 `HasTagExact`；“全部”分类由 `bIncludeChildCategories` 选择 `HasTag`。
- TopSettingsTabs 仅在 CommonUI 接口边界把 `CategoryTag.GetTagName()` 转成 FName，业务状态始终保存 GameplayTag。
- 旧混合标签表、FName 分类字段和分类枚举过滤路径已删除，不提供双轨兼容。

# POC 与调试资产

- `Struct_Cloth_Attribute` 与 `ClothDataTable` 已由用户删除，不再属于正式衣柜。
- 旧正式地图 `/Game/Maps/HomeMap` 已退出运行入口；其 `Debug/Wardrobe` 调试 Actor 已迁移到下面两个开发测试地图：
  - `/Game/Maps/TestMap_ListenServer`
  - `/Game/Maps/TestMap_SplitScreen`
- 两张测试地图各保留四个调试 Actor：
  - `Debug_WardrobeOwnership_GrantAll`：获取 `DA_WardrobeCatalog` 中的全部服装。
  - `Debug_WardrobeOwnership_ClearAll`：移除存档中属于 `DA_WardrobeCatalog` 的全部服装。
  - `Debug_WardrobeSpawner_PressToInteract`：按交互键生成领取 Actor。
  - `Debug_WardrobeSpawner_AutoOverlap`：进入范围自动生成领取 Actor。
- 两个所有权入口继续走正式 Interaction、InventoryManager、PlayerState 与 SaveGame 链；四个 Actor 均只用于开发测试，正式家园地图 `HomeMap_Courtyard` 不放置这些调试入口。
- 调试 Actor 的操作提示属于开发者文本边界，不能作为正式产品 UI 文案；正式页面和交互提示继续使用英文源文案并通过 Game 本地化资源翻译。
- Shipping 前应从正式地图移除调试 Actor，或关闭其 Debug 开关。

# 已完成的实现事实

- 旧 W_Cloth POC 列表生成、旧 DataTable 读取和旧函数图已删除。
- 正式目录、18 个蓝图 ItemDefinition、Persistent 库存与服务器装备链已经接通。
- Item Fragment 已统一使用正式 AppearanceTags，旧名称兜底已删除。
- 五个一级页拥有各自的 VBox、滚动区和网格。
- 固定按钮事件已经迁入 W_Cloth EventGraph。
- 内嵌 Tooltip、角色预览、镜头切换、焦点恢复和隐藏 UI 状态代码已经落地。
- Tooltip 悬停链路已修复（2026-08）：条目悬停/焦点经 `EntryHovered` 语义委托汇入 `SelectedItem`，与点击共用一条数据路径。
- C++ 与相关蓝图已经通过最近一次编译和资产级验证。

以上表示实现和静态验证完成，不代替玩家路径验收。

# 验证要求

- Windows Editor Target 编译成功。
- `W_Cloth`、`W_Cloth_Item`、`W_ClothItemTooltip`、预览 Actor 和全部 Item BP 编译保存成功。
- `W_Cloth` 控件树验证成功。
- PIE 验证鼠标悬停与手柄/键盘焦点导航都能弹出并刷新详情卡。
- PIE 验证领取、装备、卸下、切换角色、隐藏/恢复 UI 和保存重进。
- GameplayTag 迁移后必须重启编辑器验证，资产级编译不能替代标签注册与 PIE 验收。

# 相关文档

- `Requirements_需求.md`：最终需求和 GameplayTag 数据模型。
- `STATUS.md`：当前完成状态与唯一下一步。
- `RefactorDebt_重构债务.md`：迁移后必须复查的剩余代码边界。
