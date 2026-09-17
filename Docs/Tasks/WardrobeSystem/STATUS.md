---
task_id: WardrobeSystem
status: gameplay_tag_classification_implemented
assigned_to: Codex
progress: 98%
---

# 衣柜与换装任务状态

# 当前可用

- 正式目录由 `/Game/Blueprints/Mutable/Wardrobe/DA_WardrobeCatalog` 驱动。
- 18 个 ItemDefinition 位于 `/Game/Blueprints/Mutable/Wardrobe/Items`。
- Item Fragment 使用正式 `AppearanceTags` 驱动 Mutable，不再有 AppearanceTagNames 兜底。
- 衣柜从 Persistent InventoryManager 读取拥有状态，通过 PlayerState 服务器接口装备、卸下和保存。
- 男女主拥有独立 AppearanceTags 与 SaveGame 快照。
- W_Cloth 使用五个独立 TabContent、纵向二级分类、独立滚动区和网格。
- 固定按钮事件位于 W_Cloth EventGraph；C++ 负责业务入口。
- 角色预览、旋转缩放、内嵌 Tooltip、隐藏与恢复 UI 已接入。
- 旧 POC Widget 图表、C++ 样例物品、硬编码分类文本和兼容详情面板均已删除。
- 分类已使用 `FGameplayTag` / `FGameplayTagContainer`；旧 FName 分类路径已删除。
- Mutable 外观表包含原有 22 个标签，衣柜分类表包含 30 个 `Wardrobe.Category.*` 标签。

# 2026-09 地图迁移

- 正式家园运行地图为 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`；旧 `/Game/Maps/HomeMap` 已退出运行入口，资产暂保留用于迁移回溯。
- `Debug_WardrobeOwnership_ClearAll`、`Debug_WardrobeOwnership_GrantAll`、`Debug_WardrobeSpawner_AutoOverlap`、`Debug_WardrobeSpawner_PressToInteract` 已从旧 HomeMap 移除，并分别迁入 `/Game/Maps/TestMap_ListenServer` 与 `/Game/Maps/TestMap_SplitScreen` 的 `Debug/Wardrobe` 文件夹。
- 新家园地图不放置开发调试入口；衣柜运行链仍由正式家具交互和 CommonUI 页面提供。

# 2026-08 更新（悬停修复与旧类删除）

- `UShootCharacterSwitchWidgetBase` 已删除：全仓蓝图与 C++ 零引用，git 可恢复。
- 衣柜角色切换已由 `UShootWardrobeViewModel::SwitchCharacter` 直连 `AShootPlayerController::RequestSwitchCharacter`。
- Tooltip 悬停回归已修复：`UShootObjectEntryButtonBase` 新增与点击对称的 `EntryHovered` 语义委托，`UShootWardrobeScreen::HandleEntryHovered` 汇入 `SelectedItem`；鼠标、手柄/键盘焦点与触屏统一由 CommonUI 驱动，无设备分支。
- `IMC_CharacterSwitchMenu` 按用户要求保留，未删除。

# 已完成并验证

- `NewWorldOrderEditor Win64 Development` 最近一次完整编译成功。
- 18 个 Item BP 的 AppearanceTags 均非空，并已编译保存。
- W_Cloth、W_ClothItemTooltip 与预览 Actor 可编译保存。
- W_Cloth 控件树验证通过。
- W_Cloth 的旧 `RegisterTopLevelTab`、`GetIClothItemByIndex`、旧焦点覆盖和废弃变量已删除。
- 五个一级页的 VBox、滚动区和网格资产结构已经落地。
- AutoOverlap 调试领取入口曾由用户确认可用。
- HideUIButton 与配置输入均能在隐藏 UI 后恢复界面，用户已验收通过。
- 男女主分别装备并保存后，可以恢复各自搭配，用户已验收通过。
- 当前快速切换分类、角色和 Tooltip 未再观察到 Slate/UMG 崩溃。
- 18 个 Item BP 均已写入 `SubCategoryTags` 并通过资产编译。
- 旧标签表与两个 POC DataTable 的 Asset Referencers 均为空。
- TestMap_ListenServer 与 TestMap_SplitScreen 的“获取全部衣服”和“清除全部衣服”两个 PressToInteract 调试入口，已保留为开发期验证入口；正式家园地图不承载它们。
- 两个调试入口当前配置为可重复使用，交互后不消失；这是为了反复切换拥有权状态，不代表交互失败。
- 方向键/左摇杆焦点导航（左右组互通 + 格子矩阵入口/出口模型）PIE 验收通过（2026-08）。
- L3 预览操控进入/退出提示与 Hair/Face/BodyShape/Preset 四个 SubTabsVBox 可见性统一：用户蓝图侧完成并随本轮验收通过；对应 W_Cloth.uasset 改动尚未提交，收尾时确认是否纳入提交。
- HomeMap 的 `BP_Dressing_Table_Set` 已改用 `UShootWardrobeInteractionComponent` 暴露统一交互选项，动态交互 GA 只为发起交互的 LocalPlayer 把现有 `W_Cloth` 推入 `UI.Layer.GameMenu`。家具蓝图原先直接 Push UI、发送 GameplayEvent 和移除 Aura 遗留 Tag 的旧图表已删除。
- 2026-09-01 玩家在梳妆台交互范围内按 `IA_Interact` 默认键 F，确认能打开与 M 菜单完全相同的衣柜页面：通过。

# 待玩家验收

- 使用两个调试入口分别切到全部已获取和全部未获取后，完整检查上装、下装、连衣裙、鞋、发型、“全部”父分类和多分类显示。

# 已确认删除

- `Struct_Cloth_Attribute`
- `ClothDataTable`

这两个资产属于用户早期 POC，不再是正式衣柜依赖。提交删除前仍需完成引用与配置检查。

# 下一步唯一主线

由玩家在 PIE 中使用 TestMap_ListenServer 或 TestMap_SplitScreen 的两个调试入口验收全部已获取/未获取、上装、下装、连衣裙、鞋、发型、“全部”父分类和多分类显示；同时验收鼠标悬停与手柄/键盘焦点导航都能弹出并刷新详情卡。

# 已决定的数据边界

- `WardrobeItems`：哪些物品进入衣柜。
- `AppearanceTags`：装备后 Mutable 选择什么。
- `SubCategoryTags`：物品显示在哪些 UI 分类。
- Catalog 分类定义：分类文本、顺序、默认项和预览语义。
- W_Cloth：布局、固定按钮、视觉与动画。
- C++：库存、网络权威、Mutable、SaveGame、输入状态和 UI 数据快照。

# 关键文档

- `Requirements_需求.md`：最终 GameplayTag 分类需求。
- `Implementation_实现指南.md`：当前实现和迁移顺序。
- `RefactorDebt_重构债务.md`：迁移后的代码复查边界。
