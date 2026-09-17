# 当前框架审计

# 审计结论

当前项目不是纯 Aura，也不是纯 Lyra，而是三类实现共存：

- Aura 风格：Character 启动技能数组、从 SaveGame 恢复 AbilitySpec、等级和技能点模型。
- Lyra 风格：PlayerState ASC、AbilitySet、Equipment 授予、GameplayMessage、CommonUI、UIExtension、团队接口和交互扫描生命周期。
- 项目业务：双主角快照、库存、QuickBar、衣柜、Mutable 外观和角色切换。

因此正确方向是替换框架胶水，不是推倒全部业务代码。

# 保留并继续演进

- `AShootPlayerState` 上的 ASC、InventoryManager 和 ResourceInventory 归属。
- `UShootAbilitySet` 及 `FShootAbilitySet_GrantedHandles`。
- EquipmentDefinition 到 EquipmentManager 再到 AbilitySet 的装备能力链。
- InventoryManager、ResourceInventory、QuickBar、Equipment、Persistent/RuntimeOnly 分层。
- SaveGame 中共享库存、男女主独立 QuickBar 和外观快照的业务含义。
- `ILyraTeamAgentInterface` 阵营体系。
- CommonUI、PrimaryGameLayout、UIExtension、MVVM、GameplayMessage。
- `UShootGA_Interact` 的 OnSpawn 扫描主能力和附近目标动态授予执行能力的 Lyra 生命周期。
- 衣柜 GameplayTag 分类与 DataAsset 目录。
- `AShootPlayerController::RequestSwitchCharacter` 到 `AShootPlayerState::SwitchToCharacter` 的服务器权威后端。

# 需要迁移

- `AShootCharacterBase::StartupAbilities` 和 `StartupPassiveAbilities`。
  - 当前蓝图只保留 Jump 与 `GA_ListenForEvent`，但数据仍挂在 Character 蓝图启动数组。
  - 目标是迁入 PawnData 引用的基础 AbilitySet。
- `AShootCharacter::CoreInteractionAbilities`。
  - 当前 C++ 构造函数硬编码 `UShootGA_Interact`。
  - 目标是玩家 PawnData 的基础 AbilitySet；AI PawnData 不配置该能力。
- `AShootPlayerState` 的 Male/Female 主动和被动技能数组。
  - 当前 C++ 构造函数直接引用十二个具体 GA 类。
  - 目标是男女主 PawnData 或角色 AbilitySet 数据资产。
- `AShootPlayerState::ApplyGenderAbilityKit`。
  - 目标是切换 PawnData AbilitySet 句柄，不再靠 KitTag 扫描和手工清理作为主生命周期。
- `AShootCharacter::LoadProgress` 与 `UShootAbilitySystemComponent::AddCharacterAbilitiesFromSaveData`。
  - 当前会从 `UShootSaveGame::SavedAbilities` 重建运行时 AbilitySpec。
  - 目标是“配置授予能力，存档只恢复解锁、等级和槽位选择”。
- GameMode、Pawn 类、输入和玩法规则的分散配置。
  - 目标是 Experience 与 PawnData 的显式组合。

# 保留为参考或扩展资产

- `/Game/Blueprints/AbilitySystem/Abilities/GA_Interact`
  - 人工照 Lyra 蓝图迁移的参考资产。
  - 当前运行时主线授予 C++ `UShootGA_Interact`。
- `GA_Skill1` 至 `GA_Skill4`
  - 后续技能扩展占位资产。
  - 当前不在启动授予数组中，但不是垃圾资产。

# 候删但必须先取证

- `UShootAbilitySystemLibrary` 中仅服务 Aura CharacterClassInfo 初始化的入口。
- `UCharacterClassInfo` 及其 DataAsset 引用。
- `UShootSaveGame::SavedAbilities` 和关联序列化结构。
- AbilityStatus、AbilityType、SpellPoints 等仅服务旧技能树的字段或标签。
- `AddCharacterAbilities`、`AddCharacterPassiveAbilities`、`AddCharacterAbilitiesFromSaveData` 等重复授予入口。
- 旧输入标签 `InputTag_1` 至 `InputTag_4` 和旧技能类型模板标签。
- `DefaultGame.ini` 中当前没有实际 GameFeatureData 目录时遗留的 GameFeatureData 扫描配置。

这些内容只有在以下条件全部满足后才能删除：

- C++ 和配置引用为空。
- 蓝图、DataAsset、地图和存档迁移引用已核对。
- 新链路覆盖首次建档、读档、重生、切换角色和旅行。
- 编译、资产加载和多人验收通过。

# 当前不存在或尚未形成主线

- 项目源码中没有正式 PawnData、ExperienceDefinition、ExperienceManager 或 GameplayTagRelationshipMapping 实现。
- `/Game` 中没有已确认承担上述职责的正式数据资产。
- `.uproject` 已使用 ModularGameplayActors，Build.cs 已依赖 CommonGame、CommonUser、ModularGameplay 和 GameplayMessageRuntime，但未形成完整 Lyra Experience 生命周期。
- 项目配置存在 GameFeatureData PrimaryAsset 扫描项，但 `.uproject` 未显式启用 GameFeatures，当前也没有正式 GameFeature 内容主线。

# Session + CommonUI 边界

联机、旧菜单审计、MainMenu、衣柜入口和完整 Session 生命周期已经迁入独立任务包：

- `Docs/Tasks/SessionUI/`

GameFrameworkMigration 只保留 Experience、PawnData、AbilitySet 和框架初始化职责，不再承载 Session UI 的实现计划。

# 文档冲突

- `Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md` 规定本地分屏共享进度和解锁。
- `Docs/SystemDesign/GameFlow/Overview_总览.md` 仍描述第二 LocalPlayer 选择独立存档，并写有本地分屏不能进入 Hub。
- 本任务以最新需求中的“共享进度、角色独立 Loadout”为目标；正式编码前必须同步修订 GameFlow SSOT，不能让两种持久化模型同时存在。
