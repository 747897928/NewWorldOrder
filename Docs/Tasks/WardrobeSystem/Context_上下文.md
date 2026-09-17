# 衣柜与换装上下文

# 一句话结论

衣柜是 Persistent InventoryManager、PlayerState、Mutable、SaveGame 与 CommonUI 的组合系统。物品所有权和装备结果由服务器负责；ItemDefinition、DataAsset 和 Widget 蓝图负责内容与表现。

# 系统边界

- `UResourceInventoryComponent`
  - 管理材料、货币、徽章和设计图等数量型资源。
  - 不保存服装所有权。

- `UShootInventoryManagerComponent`
  - 管理有身份的 Persistent 服装 ItemInstance。
  - 衣柜、装备和 SaveGame 只引用这里的物品实例。

- `AShootPlayerState`
  - 保存男女主独立 AppearanceTags。
  - 服务器校验装备、卸下和保存请求。

- `UMutableAppearanceComponent`
  - 消费 PlayerState AppearanceTags 更新 Mutable 表现。
  - 不判断库存所有权。

- `UShootWardrobeScreen` + `UShootWardrobeViewModel`
  - Screen 是页面协调层；ViewModel 将目录、库存和当前外观转换成 UI 快照。
  - 条目的点击、悬停与焦点经 `UShootObjectEntryButtonBase` 语义委托汇入页面协调层。
  - 不拥有物品，不直接修改服务器状态。

- `W_Cloth`
  - 维护布局、固定按钮事件、样式和动画。
  - 不保存库存、Mutable 或 SaveGame 业务规则。

# 当前正式资产

- 目录：`/Game/Blueprints/Mutable/Wardrobe/DA_WardrobeCatalog`
- 物品：`/Game/Blueprints/Mutable/Wardrobe/Items/BP_Item_Wardrobe_*`
- 主页面：`/Game/UI/Mutable/W_Cloth`
- 格子：`/Game/UI/Mutable/W_Cloth_Item`
- 详情卡：`/Game/UI/Mutable/W_ClothItemTooltip`
- 预览 Actor：`/Game/UI/Mutable/BP_ShootWardrobePreviewActor`

# 数据关系

- `WardrobeItems`：哪些 ItemDefinition 进入衣柜。
- `AppearanceTags`：装备后选择哪些 Mutable 外观选项。
- `SubCategoryTags`：物品属于哪些 UI 分类。
- Catalog 分类定义：分类身份、文本、顺序、默认项和预览语义。

`SubCategoryTags` 已是正式分类数据；同一物品可以持有多个分类标签。

`SelectedItem`：Tooltip 详情卡的唯一数据源；点击装备、鼠标悬停与手柄/键盘焦点导航都汇入它，由 MVVM 绑定驱动 `ItemTooltipPanel`。

# GameplayTag 注册表

职责独立的标签注册表：

```text
/Game/Blueprints/Mutable/DataTable/DT_MutableAppearanceGameplayTags
/Game/Blueprints/Mutable/DataTable/DT_WardrobeCategoryGameplayTags
```

命名空间：

- `Female.*`、`Male.*`：Mutable 外观选项。
- `Wardrobe.Category.*`：衣柜 UI 分类。

`DT_MutableAppearanceGameplayTags` 保存 22 个 `Female.*`、`Male.*` 外观标签；`DT_WardrobeCategoryGameplayTags` 保存 30 个 `Wardrobe.Category.*` 分类标签。配置只注册这两张职责明确的表。

# 必须保持的架构

- 服装 Lifetime 为 Persistent。
- RuntimeOnly 不表示服装所有权。
- 男女主可以共享物品所有权，但装备快照彼此独立。
- UI 选中态不等于已装备状态。
- 新增普通物品和二级分类不修改 C++。
- 不恢复 `Struct_Cloth_Attribute`、`ClothDataTable` 或旧 W_Cloth POC 图表。
- 不在 C++ 中硬编码具体资产路径、分类文本或物理键。

# 当前下一步

在 PIE 中验收分类切换、父分类、多分类、装备与存档玩家路径；PressToInteract 调试领取问题仍按低优先级单独处理。
