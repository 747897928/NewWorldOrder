# 内容目录整理实施指南

## 当前边界

本文件描述后续执行方案和已完成批次的验收口径。建立任务包时不默认执行资产操作；本轮在用户明确授权后，已通过 Unreal Editor 资产操作完成 ArchViz 整包迁移和重定向器清理。其他目录仍按批次推进，不能据此宣称全量整理完成。

## 阶段一：冻结和盘点

1. 记录当前分支、工作区和编辑器状态。
2. 对 `/Game/Assets`、`/Game/ArchVizInteriorVol3`、`/Game/Blueprints`、`/Game/NiagaraExamples` 建立资产清单。
3. 为每个待迁移资产记录资产类型、直接引用者、依赖者、所属地图、是否第三方、是否测试内容。
4. 对 `/Game/Blueprints` 先按运行时所有者标注，而不是按文件扩展名移动。
5. 对 `ArchVizInteriorVol3` 和 `NiagaraExamples` 先确认哪些内容是源包、演示地图、示例效果和项目实际引用。

## 阶段二：建立目标目录

优先创建空的目标功能域目录，不立即删除旧目录：

```text
/Game/ThirdParty/Epic
/Game/Environment/HomeMap
/Game/Environment/Props/Gameplay/Ammo
/Game/Environment/Props/Gameplay/Health
/Game/Environment/Props/Furniture/Wardrobe
/Game/Gameplay/Interactables
/Game/Effects/Niagara
/Game/Characters/Heroes/CC/Shared/Animations
/Game/Developer
```

如果目标目录仅用于一次迁移且没有长期语义，不要提前创建。

## 阶段三：按完整资产包迁移

所有移动和重命名使用 Unreal Editor 的 Content Browser、AssetTools 或项目可用的 UE MCP 资产工具完成，不使用 Windows 资源管理器直接移动 `.uasset` 和 `.umap`。

每一批只处理一个功能闭包：

```text
资源包源目录
→ 目标目录
→ 修复引用
→ 编译相关 Blueprint
→ 保存
→ 复读路径和引用
→ Git 检查点
```

推荐批次顺序：

1. ArchViz 资源包全部迁入 `/Game/Environment/HomeMap`，含 `Maps` 和完整依赖闭包。
2. `/Game/Assets` 中三个特殊模型及其局部依赖。
3. 角色动画和 PoseIcons 分流。
4. `/Game/Blueprints` 中最明确的功能域资产。
5. HomeMap 其他实际使用的环境资产。
6. Niagara 正式使用效果。
7. 清理重定向器和确认可删除的旧目录。

## 阶段四：引用和编译验证

每批迁移后必须：

- 使用 Reference Viewer 或 AssetRegistry 检查迁移前后的引用闭包。
- 检查地图、Blueprint、DataAsset、AnimBlueprint、GameplayCue 和材质引用。
- 编译受影响的 Blueprint、AnimBlueprint、Widget Blueprint 和 Niagara 相关资产。
- 保存资产并重新读取目标路径，确认不是只在当前编辑器会话内生效。
- 关键地图执行最小 PIE 验证；目录整理完成前不能只根据资产路径检查宣布完成。
- 记录是否产生 ObjectRedirector，待全部外部引用修复后再统一处理。

## 阶段五：删除旧目录

只有在以下条件全部满足后，才能删除 `/Game/Assets`、旧 `/Game/Blueprints` 子目录或未使用的 Niagara 示例内容：

- 全局引用扫描确认无生产资产引用。
- 地图引用确认无遗漏。
- 蓝图、AnimBlueprint、Widget、DataAsset 和 GameplayCue 编译通过。
- 关键单人、Listen Server 和本地分屏入口完成回归。
- 重定向器已明确处理，且删除目标只包含已确认的旧内容。
- 用户确认第三方源资产的保留或删除策略。

资产删除必须在 Unreal Editor 内执行，并遵守项目关于单文件删除和多文件目录删除的安全规则；本任务包默认不授权批量删除，
只有用户明确授权且引用审计确认范围后，才能执行对应批次的旧资产清理。

## MCP 和编辑器注意事项

- 涉及蓝图、Widget、DataAsset 或资产迁移时，先阅读 `Docs/DevelopmentNotes/MCP_踩坑记录.md`。
- Lyra 11000 只作为参考编辑器实例，NewWorldOrder 使用自己的项目编辑器和 MCP 端口。
- 同时运行 Lyra 时进行 NewWorldOrder 冷编译，必须避免被识别成 Hot Reload，并核对生成的是基础项目 DLL。
- MCP 读取蓝图或 Widget 可能触发重存；操作前后应检查资产修改时间和 Git 状态，避免把读取噪音误当成目录整理结果。
- MCP 写入脚本必须幂等，按阶段打印 `CREATED`、`MODIFIED`、`DELETED` 路径，并逐批编译、保存和复读。
