---
task_id: GameFrameworkMigration
status: in_progress
assigned_to: Codex
progress: 50%
---

# 任务状态

当前状态：P0 AbilitySet 生命周期、Experience HUD 单一入口与 P1-P3 四槽 Match Skill 基础闭环已落地；用户已通过验收项 1-4、8，输入/等级/分屏血条修复等待复验；完整 PawnData 与正式技能内容仍未完成。

# 2026-09-04 四槽输入、等级与分屏血条修复

- 用户验收确认 HomeMap、HUD 单一入口、空槽、F 获取和 Experience 清理通过。
- `DA_SkillLoadoutConfig_PVE` 旧顺序 C/Q/E/X 已修正为 Q/E/C/X，InputTag 和 IA_Skill1/2/3/4 保持严格同序；
  具体物理键继续由 IMC 与玩家改键决定。
- `ULyraActionWidget` 运行时改用 UE 5.8 原生 `SetEnhancedInputAction` 刷新按键图，解决只设置
  `AssociatedInputAction` 但白色空块不更新的问题。
- `UShootSkillBarWidgetBase` 接入用户蓝图中的 `LevelSizer` 与 `SkillLevel`，按 Owner-only 槽数据维护空槽显隐和 1-3 级文本。
- `UAttributeViewModel` 不再把第二个 LocalPlayer 默认属性 GE 到达前的 0/0 当成死亡；首个有效 Health/MaxHealth
  快照建立后，真实的 Health=0 仍正常驱动 `W_Healthbar.OnEliminated`。

# 2026-09-04 Experience HUD 与技能获取回归收口

- 用户已删除 `BP_ShootHUD` 的 BeginPlay 无条件 HUD Push；该蓝图图表保持为空，不恢复旧入口。
- Home Experience 现在也向每个 LocalPlayer 的 `UI.Layer.Game` 推入 `W_DefaultHUD`。该 Widget 继承
  `ULyraActivatableWidget`，使用父类上的 `InputConfig=Game`，菜单关闭后的输入恢复继续交给 CommonUI 栈。
- `W_DefaultHUD` 原本固定嵌入的 `W_Healthbar`、`W_Shieldbar` 已替换为 `HUD.Slot.Health`、`HUD.Slot.Shield`；
  三个战斗 Experience 注入状态条，Home 不注入，因此安全区无战斗状态条但仍有游戏输入基线。
- 随机技能球不再要求 Pawn 进入 120cm 球体重叠；交互射线负责聚焦，服务器统一按 500cm 校验提交距离。
- 新增 `IShootSkillGrantSource` 与通用 `UShootGA_Interaction_AcquireSkill`；世界拾取物、未来商人和 Round 奖励
  共用 `UShootSkillLoadoutComponent::AcquireSkill` 权威事务，旧随机 GA 名称只作资产兼容。
- 两张测试地图的技能球均位于两个出生点正前方；正常验收走 F 交互，控制台命令只保留为诊断和清理工具。

# 2026-09-03 Home Hub 与四槽技能基础

- HomeMap 由 `BP_ShootGameMode` 显式选择 `DA_Experience_Home`，通过 `DA_AbilitySet_HomeCore` 只授予相机切换、Jump、Interact。
- 战斗 Experience 继续选择 PlayerCore；HomeCore/PlayerCore 是当前无 PawnData 阶段的互斥桥接，不恢复 Character 直授。
- PlayerState 上的 `UShootSkillLoadoutComponent` 已提供服务器权威、Owner-only 复制的四槽 Match 数据，每槽独立持有 AbilitySet 撤销句柄。
- Skill Definition/Config、通用技能来源交互、HUD Extension、C/Q/E/X InputAction 覆盖、先填空槽后升级和 Experience 清理已接通。
- 技能栏保留用户蓝图布局与 `MI_UI_WeaponCard` 材质壳；空槽显示 `EmptyText` 并隐藏卡片、Glow 和输入提示。
- 新增 `Shoot.Skill.AcquireRandom`、`Shoot.Skill.Clear`、`Shoot.Skill.Dump`，统一调用正式组件入口并支持分屏 PlayerIndex。
- 当前四项机制调试池为手雷、医疗站、战术超载、战术突击；基础设施可验收不等于旧 POC 内容达到发布品质。

# 2026-09-02 P0 Experience/GAS 生命周期收敛

- 玩家基础能力已从 `BP_ShootCharacter` 的四组直授数组迁入 `/Game/GameFramework/Abilities/DA_AbilitySet_PlayerCore`。
- 可玩 Experience 通过 `UShootExperienceDefinition::CommonAbilitySet` 授予 PlayerCore；Lobby 不授予战斗玩家能力。
- 公共与性别 AbilitySet 使用独立撤销句柄，性别切换不会再连带重置相机、交互和 QuickBar 能力。
- Experience 卸载会撤销 Ability、AbilitySet GE、动态 AttributeSet，以及主角被动自身创建的长期 GE/委托。
- 四个主角被动已改为 OnSpawn 生命周期；MarkHunter 不再激活后立即解绑。
- Round 初始化不再重授 Match 技能；迟到加入由 PostLogin 幂等补授予。
- Aura 遗留 `GA_ListenForEvent` 配置为空，未迁入 PlayerCore，资产等待后续旧内容清理。
- 本阶段有意不全量复制 Lyra PawnData/GameFeature；先用现有 Experience CommonAbilitySet 消除真实运行时的平行授予链。未来引入 PawnData 时迁移该资产归属，不建立第二套能力数据。

# 2026-09-01 Experience/HUD/技能授予审计

- 已核对 `BP_ShootHUD`、`W_DefaultHUD`、`BP_ShootCharacter`、PlayerState 属性生命周期和 Experience AbilitySet 回收路径。
- 结论是不全量复制 Lyra GameFeature Experience；保留项目 Experience，并分阶段补齐 HUD 单一入口、PawnData、可撤销句柄、模式属性事务和加载状态。
- HomeMap 应有显式 Hub Experience，不显示战斗血量、护盾、重生和战斗能力；禁止用地图名硬分支隐藏。
- 详细证据、风险和阶段门见 `ExperienceAudit_体验架构审计.md`。高风险迁移等待用户确认后执行。

# 2026-08-22 终结死亡与玩家重生纵切

- `AShootCharacterBase` 已从本地 `bDead` 收敛为复制的 `NotDead -> DeathStarted -> DeathFinished` 状态转换；客户端按状态重放停止移动、关闭碰撞和取消能力。
- `AShootGameModeBase` 新增可由具体蓝图配置的玩家重生延迟与尸体生命周期；`BP_TestGameMode`、`BP_TestListenGameMode` 启用 5 秒重生，HomeMap 默认规则保持关闭。
- 玩家 ASC 与属性继续位于 PlayerState；重生只恢复战斗即时 Health、Shield 与 IncomingDamage，不读写 SaveGame 成长数据。
- RuntimeOnly QuickBar 经过 PlayerController 的 UnPossess/Possess 会话缓存跨新 Pawn 保留；分屏实测 Rifle、激活槽和 30/60 弹药保持。
- 默认角色已移除 `Health <= 0` 自动开放救援的旧链。“倒地救起”只属于后续 PVE 丧尸挑战模式，必须由模式 Experience/AbilitySet/GA 在终结死亡前接管。
- 分屏真实 GAS 致死与重生闭环已通过。Listen Server 已通过服务端权威致死/解绑，仍保留客户端延迟后新 Pawn 占有的最终观察项。
- 实现、边界与复验数据见 `DeathRespawn_死亡与重生纵切.md`。

# 2026-08-25 Lyra 风格重生消息接入

- 已迁移项目需要的 `FLyraVerbMessage` 数据结构；未直接复制依赖 Lyra `ULyraHealthComponent` 的 `GA_AutoRespawn`，当前最终死亡仍由项目 GameMode 的单一延迟计时器控制。
- 已通过 `AShootPlayerController` 的拥有者 Client RPC，把重生时长与完成消息桥接到对应客户端的本地 GameplayMessageSubsystem。
- 已将 `W_RespawnTimer` 放入 `/Game/UI/Hud/Respawn`，并接入 Dungeon Experience 与 Default HUD Extension Point；死亡动画、溶解表现和 PVE 救援技能仍未完成。
- 源码目标编译成功；编辑器 DLL 被运行中的编辑器锁定，编辑器重启后的 PIE 消息复验待执行。

# 2026-07-27 首个 Experience 纵切

- 新增项目层 `UShootExperienceDefinition` 与 GameState 上的 `UShootExperienceManagerComponent`，没有修改 Lyra、引擎或第三方插件。
- `/Game/GameFramework/Experiences/DA_Experience_DungeonTest` 由 `/Game/Blueprints/GameMode/BP_TestGameMode` 选择，向每个 LocalPlayer 的 `UI.Layer.Game` 推入 `/Game/UI/Hud/W_DefaultHUD`，并在 `HUD.Slot.Quickbar` 注入 `/Game/UI/Weapon/WBP_QuickBar`。
- HUD 使用 Lyra `GameFrameworkComponentManager` receiver 生命周期；QuickBar 不再由 PlayerController 上的临时 UI Component 直接创建。
- 分屏 PIE 已确认两个 LocalPlayer 分别拥有自己的 HUD Layout 与 QuickBar，消息读取始终从 Widget 的 Owning PlayerController 开始。
- 本纵切只证明 Experience → HUD Layout → UIExtension 可用；尚未迁入 PawnData、ExperienceActionSet、GameFeatureAction 或完整 Lyra Experience 加载状态机。

# 2026-07-27 本地双人主角与存档边界

- 单人模式继续以账号存档的 `LastActiveGender` 展示最后切换的主角；现有角色切换流程仍负责更新该字段。
- 本地分屏产品边界固定为最多两个 LocalPlayer。Player01 与 Player02 操作的主角性别必须互斥，各自加载男主或女主对应的完整存档数据。
- TestMap 当前通过 `AShootGameMode::SplitPlayer01DefaultGender` 配置 Player01 的默认主角，Player02 自动取相反性别；资产默认值为男01/女02，不在 PlayerState 硬编码。后续双人主角选择页完成后，只需让 `ResolveInitialCharacterGender` 读取两位玩家的互斥选择结果。
- `AShootGameModeBase` 的默认实现不改变 SaveGame 性别；`AShootGameMode` 仅在 `SplitProtagonists` 策略下解析本机 LocalPlayer。Listen Server 远端 Controller 没有 `ULocalPlayer`，继续保留远端账号自己的选择。
- 性别规则仍由 `USaveGameSubsystem::RestorePlayerInventoryState` 的同一恢复事务解析并一次性恢复性别、男女外观标签与能力套件。首次 `PlayerState::BeginPlay` 时第二个本地 Controller 可能尚未完成 `ULocalPlayer` 关联，因此 `AShootGameMode::HandleStartingNewPlayer_Implementation` 会在 Pawn 生成前对已可识别的本地玩家幂等重放该恢复事务；禁止在 Pawn 生成后或下一帧直接切性别，否则会和 Mutable 异步生成 COI 的生命周期竞争。
- 分屏共享账号存档时，Player01/Player02 的互斥选择属于双人配置，二者都不得覆盖单人模式使用的 `LastActiveGender`。该权限在 PlayerState 恢复时缓存，避免 EndPlay 阶段 Controller 已解绑后误判归属；男女主各自的外观、技能与配置数据仍保存在各自主角数据中。
- 2026-08-01 双本地玩家 PIE 已确认 Player01 为男主、Player02 为女主；从 HomeMap 保存男主后进入 TestMap 不再生成两个男主。当前运行截图同时暴露头发、上衣、裤子、鞋子层未显示的表现回归，仍是动画/Mutable 联合验收阻塞项，不能把角色身份互斥通过误写为完整外观通过；修复不得删除或替换 Mutable 的头发层、鞋子层和 Pose 插槽。

# 已完成的设计决策

- 采用 Lyra 风格框架主干，保留可用的项目业务系统。
- 不做一次性 Aura 全删，不复制整个 Lyra。
- 复用现有 `UShootAbilitySet`。
- 引入 Experience、PawnData 和 GameplayTagRelationshipMapping。
- GameFeatures 延后到基础生命周期稳定之后评估。
- 存档从运行时 AbilitySpec 权威迁为“长期解锁与玩家配置”。
- `GA_Skill1` 至 `GA_Skill4` 和旧蓝图 `GA_Interact` 保留。

# 当前相关任务完成度

## 衣柜系统

状态：核心功能完成，剩余综合 PIE 验收。

- GameplayTag 分类、18 个 ItemDefinition、男女主独立外观存档、隐藏 UI 恢复、角色预览和调试拥有权工具已落地。
- 用户已确认 `Debug_WardrobeOwnership_ClearAll` 与 `Debug_WardrobeOwnership_GrantAll` 按 E 后能生效。
- 这也确认当前 PressToInteract 单按链在这两个调试 Actor 上可用。
- 仍需一次完整 PIE 验收：全部已获取、全部未获取、各一级和二级分类、多分类物品显示。
- 两个调试 Actor 当前不会消失，因为配置为可重复使用；该行为不是交互失败。

## 交互系统

状态：Lyra 生命周期主线已恢复并通过实际调试 Actor 验证。

- `UShootGA_Interact` 为 OnSpawn 常驻扫描主能力。
- 具体执行能力由附近目标动态授予和回收。
- WaitInputPress 和输入 Pressed/Released 分发已收敛。
- 交互主能力当前已进入可撤销 AbilitySet 桥接：战斗 Experience 使用 PlayerCore，Home 使用 HomeCore。
- 未来引入 PawnData 时只迁移该 AbilitySet 的数据归属，不再创建第二条授予链。

## 角色切换

状态：后端约 95%，最终入口和 UI 未完成。

- 服务器权威切换、男女主快照、QuickBar、外观、存档和统一请求入口已完成。
- 菜单长按 C++ 骨架已完成。
- 未完成 CommonUI/ESC 菜单蓝图接线、长按进度表现和完整验收。
- 未完成“另一位主角 NPC”替代 SwitchStation 的最终入口。
- 世界入口桥接层仍可在新框架下继续收窄。

## 库存与武器

状态：主架构可用，TestMap RuntimeOnly QuickBar 玩家闭环已通过，资产规则和安全区遗留未清零。

- InventoryManager、ResourceInventory、QuickBar、Equipment、Persistent/RuntimeOnly 和 SaveGame 主线已存在。
- TestMap Experience 默认显示 QuickBar；真实 F 拾取、滚轮切枪、G 丢弃、重新拾取和最后一把丢弃后空手已在双本地玩家 PIE 的玩家 0 视口通过。
- 旧 `UShootHUDQuickbarComponent` 未接入 Experience、CommonUI 或分屏生命周期，已删除；`UShootQuickbarWidgetBase` 已改为实际的 HUD Extension 消息驱动基类。
- 未完成 QuickBarSlotRules 资产配置。
- 未完成 StackRules 上限与唯一性资产配置。
- 安全区判断仍有 MapName 兜底需要替换。

## 主角技能

状态：四槽 Match Skill 系统基础已落地并等待统一 PIE；机器人、无人机和旧技能内容品质仍未完成。

- RescueCloak 感知/UI、队友范围隐身、完整救援流程和进度 UI 仍未完成。
- MedicalStation 高级规则、SmartAssist 无人机 AI、MarkHunter 表现、统一死亡管线等仍未完成。
- 这些技能应在 PawnData/AbilitySet 迁移时保持功能，不应借框架重构顺便重写全部技能内容。

# 下一步唯一主线

死亡/重生已形成最小纵切；逐武器左手握把也已经按 Lyra 的 Linked Anim Layer 数据入口完成并验收。Rifle/Pistol 保持默认姿势，男女 Shotgun 分别使用同侧 CC 覆盖序列；不得再新增与 `weapon_r -> ik_hand_l -> TwoBoneIK` 并行的 C++ Transform 或按性别硬编码分支。

框架迁移下一阶段不再重复迁移 Jump/Interact：它们已进入可撤销的 PlayerCore/HomeCore AbilitySet。当前唯一门槛是用户统一验收四槽、技能球 F 获取、HomeCore 与 CommonUI 输入恢复；通过后进入 P4 正式机器人伙伴纵切，必须包含 Character、AIController、Blackboard、Behavior Tree、AnimBP、GAS 战斗与网络生命周期。完整 PawnData 在真实需求出现时接管 AbilitySet 归属，不为形式对齐 Lyra 提前复制。未来 PVE 丧尸挑战的“倒地救起”由 PVE Experience 配置的模式 AbilitySet/GA 在终结 `Die()` 前接管；普通模式继续使用当前 GameMode 定时重生，不在角色基类混入模式分支。

Session + CommonUI 生命周期不再属于本任务包，独立状态与实现见 `Docs/Tasks/SessionUI/STATUS.md`。

禁止从阶段 7 的“删除 Aura”开始，也禁止先创建大量 Lyra 类再补调用链。

# 接手者恢复上下文检查

新会话只要完整阅读本目录七份正文和本状态页，应能回答：

- 为什么迁移。
- 哪些现有系统保留。
- 哪些 Aura 路径待替换。
- Ability、Experience、PawnData、存档和 UI 如何分工。
- 第一阶段修改什么、验收什么、什么时候能删除旧代码。
