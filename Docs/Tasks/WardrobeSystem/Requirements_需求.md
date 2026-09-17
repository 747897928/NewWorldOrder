# 衣柜与换装需求

# 目标

- 玩家通过衣柜为当前主角装备已经拥有的服装、鞋子和发型。
- 衣柜读取 Persistent InventoryManager，不绕过库存、PlayerState、Mutable 与 SaveGame 主链。
- 普通内容迭代只修改 ItemDefinition 蓝图、目录 DataAsset 和 Widget 蓝图，不修改 C++。
- 分类使用 GameplayTag，支持一件物品进入多个二级分类，并避免字符串 ID 拼写错误。
- 文档只记录当前架构和下一步目标，不保留实现过程流水账。

# 当前主链

1. `DA_WardrobeCatalog.WardrobeItems` 定义衣柜展示哪些 `UShootInventoryItemDefinition`。
2. ItemDefinition 中的 `UShootInventoryFragment_WardrobeItem` 保存性别、部位、描述、稀有度、排序和 Mutable 外观标签。
3. `UShootWardrobeViewModel` 将目录与玩家 Persistent InventoryManager 合并成 UI 快照，`UShootWardrobeScreen` 负责页面协调与条目分发。
4. 条目的点击、悬停与焦点统一走 `UShootObjectEntryButtonBase` 语义委托；悬停/焦点只更新详情卡数据源 `SelectedItem`（鼠标、手柄、键盘与触屏由 CommonUI 统一驱动，无设备分支）。
5. 玩家点击格子时，只向 `AShootPlayerState` 提交 ItemInstanceId，装备/卸下由服务器校验。
6. PlayerState 在服务器重新校验所有权、Lifetime、性别和 Fragment，然后更新当前主角 AppearanceTags。
7. `UMutableAppearanceComponent` 消费 AppearanceTags 刷新角色。
8. 保存搭配时，PlayerState 与 SaveGameSubsystem 捕获库存和男女主独立外观快照。

# 标签注册表

Mutable 外观与衣柜分类使用两份职责独立的 Gameplay Tag DataTable：

```text
/Game/Blueprints/Mutable/DataTable/DT_MutableAppearanceGameplayTags
/Game/Blueprints/Mutable/DataTable/DT_WardrobeCategoryGameplayTags
```

要求：

- Row Struct 使用 UE 的 Gameplay Tag Table Row。
- 资产加入 Project Settings 的 Gameplay Tag Table List。
- `DT_MutableAppearanceGameplayTags` 只注册 `Female.*`、`Male.*` Mutable 外观标签。
- `DT_WardrobeCategoryGameplayTags` 只注册 `Wardrobe.Category.*` 衣柜分类标签。
- 两张表都加入 Gameplay Tag Table List。
- 不保留旧混合表或重复注册同一标签。

# 标签命名空间

Mutable 外观标签表达装备后的具体参数选项：

```text
Female.Body.*
Female.Head.*
Male.Body.*
Male.Head.*
```

示例：

```text
Female.Body.Shirts.Camisole
Female.Head.HeadAccessories.LongHair
Male.Body.Shoes.Sneakers
```

衣柜分类标签只表达 UI 归类，不表达具体服装：

```text
Wardrobe.Category.Clothing
Wardrobe.Category.Clothing.UpperBody
Wardrobe.Category.Clothing.LowerBody
Wardrobe.Category.Clothing.Dress
Wardrobe.Category.Clothing.Footwear
Wardrobe.Category.Clothing.Accessory

Wardrobe.Category.Hair
Wardrobe.Category.Hair.Full
Wardrobe.Category.Hair.Front
Wardrobe.Category.Hair.Back
Wardrobe.Category.Hair.Color

Wardrobe.Category.Face
Wardrobe.Category.Face.Brow
Wardrobe.Category.Face.EyeMakeup
Wardrobe.Category.Face.Iris
Wardrobe.Category.Face.Lipstick
Wardrobe.Category.Face.Blush
Wardrobe.Category.Face.Marking

Wardrobe.Category.BodyShape
Wardrobe.Category.BodyShape.Height
Wardrobe.Category.BodyShape.Bust
Wardrobe.Category.BodyShape.Waist
Wardrobe.Category.BodyShape.Hips
Wardrobe.Category.BodyShape.Legs
Wardrobe.Category.BodyShape.Muscle

Wardrobe.Category.Preset
Wardrobe.Category.Preset.Mine
Wardrobe.Category.Preset.Default
Wardrobe.Category.Preset.Recent
Wardrobe.Category.Preset.Favorite
```

# 数据职责

- `WardrobeItems`
  - 定义哪些 ItemDefinition 属于正式衣柜目录。
  - 不复制到每个二级分类定义中。

- `AppearanceTags`
  - 类型为 `FGameplayTagContainer`。
  - 定义装备后写入 Mutable 的具体外观选项。
  - 这是 Mutable 外观的唯一数据源。

- `SubCategoryTags`
  - 类型为 `FGameplayTagContainer`。
  - 定义物品属于哪些 UI 分类。
  - 同一物品可以填写多个标签。

- 一级页与二级分类定义
  - 使用 `FGameplayTag` 作为稳定身份。
  - DataAsset 保存显示文本、默认分类、预览镜头语义和分类顺序。
  - 不在 Widget C++ 中维护分类字符串、中文文案或标签前缀判断。

# 分类匹配规则

- 具体二级分类使用精确匹配：

```cpp
Item.SubCategoryTags.HasTagExact(SubCategory.CategoryTag)
```

- “全部服装”“全部发型”等父分类使用层级匹配：

```cpp
Item.SubCategoryTags.HasTag(ParentCategoryTag)
```

- 物品只需填写最具体的标签。例如填写 `Wardrobe.Category.Clothing.UpperBody` 后，应自动进入 `Wardrobe.Category.Clothing` 对应的全部页。
- 不要求物品同时重复填写父标签和子标签。

# 新增一件衣服

标准流程：

1. 确认 Mutable 已存在这件衣服对应的具体外观选项。
2. 如果需要新的外观标签，在 `DT_MutableAppearanceGameplayTags` 注册；普通衣服直接复用现有 `Wardrobe.Category.*` 分类，不新增分类标签。
3. 在 `/Game/Blueprints/Mutable/Wardrobe/Items` 复制同部位、同性别的 ItemDefinition 蓝图。
4. 设置 `DisplayName`、`Icon` 和 StackRules。
5. 在 WardrobeItem Fragment 设置：
   - `SuitableGender`
   - `Category`
   - `Rarity`
   - `SortPriority`
   - `Description`
   - `AppearanceTags`
   - `SubCategoryTags`
6. 将 ItemDefinition 加入 `DA_WardrobeCatalog.WardrobeItems`。
7. 通过正式库存或调试发放入口授予该 Persistent 物品；加入 Catalog 只代表允许展示，不代表玩家已经拥有。
8. 编译蓝图并在衣柜中验证分类、拥有、装备、卸下与保存恢复。

新增普通物品和新增二级分类不得要求修改 C++。

# C++、DataAsset 与蓝图分工

- C++
  - Persistent InventoryManager、服务器装备校验、PlayerState、Mutable、SaveGame、输入状态和必要的 UI 数据快照。
  - 不硬编码具体分类和具体物品资产路径。

- `DA_WardrobeCatalog`
  - 正式物品目录、一级页定义和二级分类显示配置。

- ItemDefinition 蓝图
  - 单件物品的图标、说明、适用性别、外观标签和分类标签。
  - 玩家可见的 `DisplayName` 只写商品名称，不带“测试”“女主”“男主”等开发期前缀；适用性别由 Tooltip 单独显示为“女性”“男性”或“通用”。

- `W_Cloth`
  - 页面布局、固定按钮事件、动画、样式和各一级页内容容器。

# POC 资产

- `Struct_Cloth_Attribute` 与 `ClothDataTable` 是早期 POC，用户已删除。
- 正式衣柜不得重新引用这两个资产或恢复旧 DataTable 读取图表。
- 删除提交前只需确认没有残留软路径、配置引用或 Cook 依赖。

# 已确定的代码边界

- `EShootWardrobeCategory` 只保留物品类型与详情显示职责，不参与分类筛选。
- `EShootWardrobeAppearanceSection` 只索引 W_Cloth 的五套固定内容容器。
- Widget C++ 不保留分类字符串、分类枚举过滤或 FName/GameplayTag 双轨状态。

# 验收标准

- 两张标签表在编辑器重启和 Cook 后仍能分别注册 Mutable 与衣柜分类标签。
- 18 个现有 ItemDefinition 的 `AppearanceTags` 不丢失。
- 现有发型进入“全部发型”和“整发”。
- 一件物品填写两个 `SubCategoryTags` 后会同时出现在两个分类。
- 新增物品只修改蓝图和 DA，不修改 C++。
- C++ 编译通过，相关蓝图编译通过，W_Cloth 控件树验证通过。
- 旧标签表、旧 FName 分类字段和 POC DataTable 不再有引用。

# 非目标

- 本轮不实现商店、制作、成就或解锁条件。
- 本轮不新增完整的体型编辑和预设保存系统。
- 本轮不把服务器装备、库存或存档权威逻辑迁入 Widget 蓝图。
