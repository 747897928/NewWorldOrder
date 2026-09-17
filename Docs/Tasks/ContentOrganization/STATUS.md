---
task_id: ContentOrganization
status: in_progress
assigned_to: User
progress: 90%
started: 2026-09-04
last_updated: 2026-09-13
---

# 任务状态

当前状态：in_progress
负责人：User
进度：90%

## 当前阶段

目录规范方案已完成，CommonUI、ArchViz 以及 `/Game/Blueprints` 的功能域迁移批次已按用户授权落地。
`/Game/NiagaraExamples` 仍按用户要求暂缓；全项目仍需独立完成编译和关键入口回归，不能把本轮批次误报为全项目整理完成。

## 已完成

- 完成当前 `/Game` 顶层目录和几个重点目录的只读盘点。
- 确认 `/Game/Assets` 当前包含 `Animations`、`Furniture`、`Inventory`、`Materials`。
- 确认 `/Game/Blueprints` 是按资产形式聚集的重灾区，包含 AbilitySystem、Actor、Animations、Character、CommonUI、GameMode、GameplayCueNotifies、GameplayCues、GameState、HUD、Input、Interaction、Mutable、Player、Skills、System、Testing 等目录。
- 初始盘点确认 ArchViz 资源包包含 Maps、Materials、Meshes、Textures；用户随后确认全部资产（含地图）都要保留并使用，当前已迁入 HomeMap 生产目录。
- 确认 `/Game/NiagaraExamples` 当前包含 666 个 `.uasset` 和 2 个 `.umap`，包括 FX_Explosions、FX_Weapons、FX_Player、Materials、Textures 等示例分类。
- 形成 Lyra 借鉴版的项目目录标准和五类重点资产的建议归属。
- 机器人伙伴功能批次已按域归位：AI 蓝图进入 `/Game/AI/Robot/Blueprints`，GA 进入
  `/Game/GameFramework/Skills/Abilities/Robot`，交互来源进入 `/Game/Gameplay/Interactables/SkillAcquisition`，
  Cue 进入 `/Game/Effects/GameplayCues/Abilities/RobotCompanion`。
- 为支持生产 Cue 目录，`Config/DefaultGame.ini` 的旧兼容段与 UE 5.8 DeveloperSettings 段均增加
  `/Game/Effects/GameplayCues`。资产迁移后必须更新 Redirector References；无本地引用的 Redirector 由用户在
  Content Browser 报告中确认并删除。
- 机器人资产归位批次已完成 `NewWorldOrderEditor Win64 Development` 冷构建和编辑器冷启动回读；新 Cue 路径被正常扫描，
  本轮日志未再出现 `Invalid GameplayCue Path`。该结论仅覆盖机器人批次，不代表全项目目录整理已经完成。
- CommonUI → Lyra 增量迁移批次已完成：71 个键鼠微图标已迁入
  `/Game/UI/Foundation/Platform/Input/KeyboardMouse/Micro_Icons/PC_Controller/Light`，Lyra 键鼠 ControllerData 已补入项目键位映射，
  并继续使用 Lyra 原有手柄图标和 ControllerData。
- 项目动作提示行已合并到 Lyra 主表 `/Game/UI/DT_UniversalActions`，`W_LyraSettingScreen` 的返回、应用、取消和恢复默认动作已切换到该表。
- 原 `/Game/Blueprints/CommonUI` 的自定义 ControllerData、旧动作表、PMM 输入数据和未使用手柄图标已按逐项审计结果移除；
  不同尺寸、仍有引用用途的 `XboxSeriesX_Diagram` 已作为补充资产迁入
  `/Game/UI/Foundation/Platform/Input/GamepadXboxSeriesX/Diagrams`。
- 迁移后已完成等效的引用刷新与清理核验：`/Game/Blueprints/CommonUI` 在 Asset Registry 中无资产或重定向器，旧资产路径均不再存在；
  若 Content Browser 仍显示空的虚拟目录，可由用户手动刷新后删除该空目录。
- ArchViz HomeMap 生产迁移批次已完成：全部 255 个资产已通过 Unreal Editor/AssetTools 迁入
  `/Game/Environment/HomeMap`，实际分布为 `Maps` 3 个、`Materials` 96 个、`Textures` 72 个、
  `Architecture` 19 个、`Furniture` 19 个、`Props` 46 个；三张地图 `Interior`、`InteriorHomeMap`、`Overview` 全部保留在
  `/Game/Environment/HomeMap/Maps`。
- ArchViz 批次已完成重定向器处理：63 个受影响包的旧软引用已更新并保存，旧第三方源路径 Asset Registry 资产数为 0，
  项目内 `ObjectRedirector` 数为 0；三张目标地图依赖中不再出现旧第三方路径。目标 `Blueprints` 无资产，未创建空的生产目录。

## 2026-09-13 `/Game/Blueprints` 功能域迁移批次

- 通过 Unreal Editor AssetTools 迁移了 701 个资产，没有直接操作 `.uasset` 或 `.umap` 文件；四个 `GA_Skill1` 到 `GA_Skill4` 的旧资产删除是用户有意进行的清理，本批次没有恢复或迁移它们。
- GameMode、GameState、GameInstance、Player、HUD、Interaction、Input、GameplayCue、Potion、Skills、Testing 和 CommonUI 资产已分别进入对应的功能域目录。
- Mutable 的 594 个资产已分流：角色/服装/发型/鞋类进入 `/Game/Characters/Heroes/CC/Customization`，衣柜目录资产进入 `/Game/UI/CharacterCustomization/Wardrobe`。
- `/Game/Blueprints/Mutable` 和 `/Game/Blueprints` 均已无有效资产或 Redirector；用户已按计划执行过 Redirector References 更新和未引用 Redirector 清理。
- 原先与并行 AnimationMigration 交叉的 8 个资产现已按当前项目结构归位：姿势库进入 CC Shared Poses，玩家角色进入 Player Blueprints，共享动画接口进入 CC LinkedLayers，ShenWanYun Spine 层进入 MF LinkedLayers，物理材质进入 `/Game/Characters/Heroes`。
- `/Game/NiagaraExamples` 未执行任何迁移，当前仍为 668 个有效资产。
- 已同步更新 `Config/DefaultEngine.ini`、`Config/DefaultGame.ini`、`Config/DefaultGameplayTags.ini` 和 `SourceArt/HomeMap/dress_scene.py` 中受影响的运行时路径；`Config`、`Source` 和 `SourceArt` 中不再残留旧 `/Game/Blueprints` 运行时路径。

## 当前建议

- 本轮 ArchViz 全部内容已进入 `/Game/Environment/HomeMap`；未来其他第三方包仍按“源包隔离、使用内容提取”的规则处理。
- 生产 Blueprint 按功能域归位，不保留全局 `/Game/Blueprints` 作为长期生产入口。
- 目录移动必须使用 Unreal Editor 的 AssetTools 或 Content Browser，并在每一批后处理 Redirector、编译和引用验证。
- Lyra 增量迁移原则：先复用 Lyra 同类资产，再把项目确有用途且 Lyra 缺失的内容补入 Lyra 对应目录；迁移完成后以 Lyra 主资产作为运行时引用入口。
- 2026-09-05 用户确认降低 `NiagaraExamples` 迁移优先级，本阶段不移动源目录，也不批量提取依赖。战术超载当前生产 System
  `/Game/Effects/Niagara/Skills/TacticalOverload/NS_TacticalOverload_Aura` 仍直接依赖多个 `/Game/NiagaraExamples` 材质、纹理与 EffectType；
  这是一项已知依赖闭包技术债。任何迁移必须先把 System、Emitter、Module、Parameter Collection、Material、Material Instance、Texture
  和软引用闭包列清，再与用户讨论并获得明确确认，不能交给子任务按目录批量搬运。

## 本轮迁移边界

本轮已处理 `/Game/Blueprints/CommonUI` 及其直接引用的 CommonUI 输入资产、技能输入与设置页动作表，
完成 ArchViz 全部资产到 HomeMap 的迁移，并完成 `/Game/Blueprints` 的功能域迁移。
当前没有宣称 `/Game/Assets` 或 `/Game/NiagaraExamples` 已完成全量整理；机器人战斗运行时问题仍由 PVEExperience 任务继续处理。

## 并发工作区变更

本轮 ArchViz 资产迁移产生的资产变更已纳入当前批次；后续执行其他资产整理前，仍必须重新盘点实际路径、引用和暂存状态，不能依据旧路径假设继续操作。

## 下一步

- 由用户执行完成后的 Redirector References 更新和 Delete Unreferenced Redirectors 已完成；后续只需在 Content Browser 刷新虚拟目录。
- 对 `/Game/Assets` 和 `/Game/NiagaraExamples` 继续保持分批审计，不把示例源包与生产依赖混迁。
- 对本批次受影响 Blueprint、AnimBlueprint、DataAsset 和关键地图执行编译与 PIE 验收；本轮迁移期间未启动 PIE，以避免干扰并行工作。
- `NiagaraExamples` 暂不进入近期迁移批次；以后排期时先提醒用户讨论“保留只读源包”还是“提取已用依赖并隔离源包”。

## 遇到问题

- 无迁移失败；`/Game/NiagaraExamples` 未触碰，编译和关键地图 PIE 回归仍是后续验收项。

## 相关 Commit

- `70d9d063`：创建内容目录整理任务包（方案阶段）。
