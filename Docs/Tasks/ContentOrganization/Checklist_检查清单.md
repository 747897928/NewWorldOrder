# 内容目录整理检查清单

## 本轮批次核验：CommonUI 与输入资产

- [x] 资产通过 Unreal Editor/AssetTools 迁移，没有直接操作 `.uasset` 和 `.umap` 文件。
- [x] Lyra 键鼠 ControllerData 已补入项目缺失键位，Lyra 原有手柄 ControllerData 与图标继续作为主资源。
- [x] 71 个键鼠图标已迁入 Lyra 键鼠输入目录，旧 CommonUI 输入表与自定义 ControllerData 已逐项清理。
- [x] `DT_UniversalActions` 已合并项目缺失动作行，设置页已切换到 Lyra 主表。
- [x] `/Game/Blueprints/CommonUI` 已通过 Asset Registry 核验无资产和重定向器，旧资产路径不再存在。
- [ ] 关键地图 PIE 已完成输入设备切换、组合键阻断和设置页显示回归；本轮只完成静态资产核验。

## 方案确认

- [ ] 用户确认顶层功能域名称和保留目录。
- [x] 用户确认 `ArchVizInteriorVol3` 全部资产（含 `Interior`、`InteriorHomeMap`、`Overview` 三张地图）保留并迁入 HomeMap 生产目录。
- [x] 用户确认当前阶段不迁移 `NiagaraExamples`，降低优先级并要求未来迁移前再次讨论。
- [ ] 用户确认 `NiagaraExamples` 最终是长期保留只读源包，还是在完整提取生产依赖后隔离/清理源包。
- [ ] 用户确认 AmmoBox、Dressing_Table_Set、FirstKitAid 的目标归属。
- [ ] 用户确认 MF 专属动画与 CC Shared 动画的划分。
- [ ] 用户确认 `/Game/Blueprints` 是否分批迁移，而不是一次性全量重排。

## 迁移前审计

- [ ] 已记录当前 Git 分支和工作区状态。
- [ ] 已建立 `/Game/Assets` 全部资产清单。
- [x] 已建立 ArchViz 全部资产清单，并核对迁移后的 255 个 HomeMap 目标资产。
- [ ] 已建立 `/Game/Blueprints` 各子目录的运行时归属清单。
- [ ] 已建立 `/Game/NiagaraExamples` 的引用和依赖清单。
- [x] 已确认战术超载生产 Niagara System 仍直接依赖 `/Game/NiagaraExamples` 中的 EffectType、材质和纹理，当前不是自包含依赖闭包。
- [ ] 已区分生产、第三方、测试、开发者和未使用资产。
- [ ] 已确认没有资产只依赖旧路径字符串或蓝图软引用而未被普通扫描发现。

## 迁移执行

- [x] ArchViz 目标目录已按设计规格创建。
- [x] ArchViz 资产通过 Unreal Editor 或 AssetTools 移动，没有直接操作 `.uasset` 和 `.umap` 文件。
- [x] ArchViz 全部 255 个资产已按生产用途进入 HomeMap：Maps 3、Materials 96、Textures 72、Architecture 19、Furniture 19、Props 46。
- [x] ArchViz 三张地图已保留在 `/Game/Environment/HomeMap/Maps`，没有因其资源包演示命名而删除。
- [ ] AmmoBox 已进入 Gameplay Ammo Props 归属。
- [ ] Dressing_Table_Set 已进入 Environment Furniture/Wardrobe 归属。
- [ ] FirstKitAid 已进入 Gameplay Health Props 归属。
- [ ] MF/MM 共用动画没有被错误迁入单一性别目录。
- [ ] PoseIcons 已与动画本体分离到 UI 目录。
- [ ] Blueprint 已按运行时所有者归位，而不是全部归入一个新类型目录。
- [ ] Niagara System 的完整依赖闭包已随迁移资产处理。
- [ ] 战术超载 Niagara 的完整依赖闭包已与用户讨论并形成单独迁移批次；当前明确暂缓。

## 验证

- [ ] 受影响 Blueprint 编译成功。
- [ ] 受影响 AnimBlueprint 编译成功。
- [ ] 受影响 Widget、DataAsset、GameplayCue 和 Niagara 资产验证通过。
- [ ] Reference Viewer 或 AssetRegistry 未发现断引用。
- [ ] 关键地图加载成功。
- [ ] Listen Server 关键入口回归成功。
- [ ] 本地分屏关键入口回归成功。
- [ ] 编辑器重启后目标资产路径和引用仍然正确。
- [x] ArchViz 与 CommonUI 批次的 Redirector 已处理并复核；ArchViz 旧源路径资产数为 0，项目内 `ObjectRedirector` 数为 0。
- [x] ArchViz 63 个受影响包的旧软引用已更新并保存；三张目标地图依赖中不再出现旧第三方路径。

## 删除旧内容

- [x] `/Game/Assets` 已无生产资产和有效引用。
- [ ] `/Game/Blueprints` 已无需要长期保留的生产资产。
- [ ] 未使用 Niagara 示例已完成引用审计。
- [ ] 删除范围已逐项列出，没有使用通配符或模糊范围。
- [ ] 删除前已获得用户确认。
- [ ] 删除后已完成编译、地图加载和 PIE 回归。
- [ ] 删除结果已提交并推送 Git。
