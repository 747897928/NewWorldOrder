# 衣柜 GameplayTag 迁移检查清单

# 实施前

- [x] 确认 UE 编辑器稳定，读取 `DT_MutableClothGamePlayTags` 的 Row Struct 和全部标签。
- [x] 检查 Gameplay Tag Table List、配置文件和旧表 Asset Referencers。
- [x] 检查 `Struct_Cloth_Attribute`、`ClothDataTable` 删除后的引用和软路径。
- [x] 保存并复核 18 个 ItemDefinition 的 AppearanceTags。
- [x] 输出实施计划和修改文件列表。

# 标签表

- [x] 创建 `DT_MutableAppearanceGameplayTags` 与 `DT_WardrobeCategoryGameplayTags`，并删除旧混合表。
- [x] 保留全部有效 `Female.*`、`Male.*` 标签。
- [x] 添加 Requirements 中的 `Wardrobe.Category.*` 标签。
- [x] 更新 Gameplay Tag Table List 和配置文件路径。
- [x] 确认旧表不再存在且 Asset Referencers 为空。

# 代码与资产迁移

- [x] Item Fragment 使用 `FGameplayTagContainer SubCategoryTags`。
- [x] Catalog 一级页和二级分类使用 `FGameplayTag` 身份。
- [x] 迁移 `DA_WardrobeCatalog`。
- [x] 迁移 18 个 ItemDefinition。
- [x] 删除 FName 分类字段和兼容分支。
- [x] 搜索并删除旧字段、分类字符串和枚举筛选遗留。
- [x] 更新只描述最终状态的任务文档。

# 编译与验证

- [x] 重启编辑器，确认新标签表注册成功。
- [x] 编译 `NewWorldOrderEditor Win64 Development`。
- [x] 编译保存全部 Item BP。
- [x] 编译保存 W_Cloth、格子、Tooltip 与预览 Actor；控件树沿用已验证的五套容器结构。
- [ ] 验证上装、下装、裙装、鞋和发型分类。
- [ ] 验证父分类自动包含子分类。
- [ ] 验证一个物品同时进入两个二级分类。
- [ ] 验证男女主装备、卸下、切换和保存恢复。
- [ ] 验证隐藏 UI 后按钮和输入都能恢复 UI。

# 收尾

- [x] 复查 `EShootWardrobeCategory`、`EShootWardrobeAppearanceSection` 和固定一级页映射。
- [x] 确认无旧标签表、旧 POC DataTable 和 FName 分类引用。
- [x] 更新 STATUS 为当前状态。
- [ ] 提交并推送远程分支。
