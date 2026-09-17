# 技能系统基础状态

日期：2026-09-05

状态：P0-P4 主链与技能输入资产调整已落地；机器人伙伴首版与战术超载修复均已通过用户 PIE 验收

## 1. 最终产品方向

- 玩家局内最多装备四个主动技能。
- 技能、等级、冷却与构筑属于 Match，不进入 SaveGame，离开 Experience 后完整清理。
- 技能通过 Definition/DataAsset 提供图标、名称、说明、等级、AbilitySet/GA 和 UI 展示数据。
- 当前世界技能来源用于打通随机获得技能的第一条可玩链；它与未来 Round 三选一、商店、神秘商人、融合和变异
  共用 PlayerState 上的权威技能事务，不建立“仅供调试”的第二套授予逻辑。
- 手雷不再是固定 T 键能力，后续改为四槽中的普通技能。
- 技能栏使用 CommonUI/UIExtension。用户重做的 `/Game/UI/Skills/W_SkillBar` 与 `/Game/UI/Skills/W_SkillSlot`
  直接复用 `WBP_QuickBar`、`WBP_WeaponSlot`、`W_GrenadeCooldown`、`W_ActionTouchButton` 的布局、材质和输入链。
- 机器人伙伴是第一项正式技能垂直切片，不能用简单跟随 Actor 代替 AIController、Blackboard、Behavior Tree、AnimBP 和服务器权威战斗链。

## 2. P0 已完成的实现

### 2.1 单一授予来源

- 新建 `/Game/GameFramework/Abilities/DA_AbilitySet_PlayerCore`。
- PlayerCore 当前包含相机切换、Jump、Interact、QuickBar Next、QuickBar Previous 和 DropWeapon。
- `DA_Experience_ExpeditionSandbox`、`DA_Experience_DungeonTest`、`DA_Experience_SplitScreenTest` 的 `UShootExperienceDefinition::CommonAbilitySet` 已引用 PlayerCore。
- HomeMap 使用独立的 `/Game/GameFramework/Abilities/DA_AbilitySet_HomeCore`，只包含相机切换、Jump 和 Interact；
  `/Game/GameFramework/Experiences/DA_Experience_Home` 引用该套件，`BP_ShootGameMode` 显式选择 Home Experience。
- HomeCore 不含 QuickBar Next/Previous、DropWeapon 或四槽配置；`DA_Experience_Lobby` 仍是远征大厅 UI Experience，不能与 Home Hub 混用。
- `BP_ShootCharacter` 原 Startup、Startup Passive、Core Interaction、Core Combat 数组已清空。
- 玩家 `AShootCharacter::LoadProgress` 不再从 Character 数组授予能力；AI 仍可使用 `AShootCharacterBase` 的 Pawn-owned Startup 路径。

### 2.2 可撤销生命周期

- `AShootPlayerState` 分别保存公共 AbilitySet 与性别 AbilitySet 的 GrantedHandles。
- Apply、Refresh Gender、Take 三个入口职责分离；性别切换只刷新性别来源。
- Experience 卸载按性别套件、公共套件的逆序撤销。
- AbilitySet 动态 AttributeSet 使用 `RemoveSpawnedAttribute` 从 ASC 注销。
- 四个主角被动使用 `OnSpawn` 自动激活。
- MedicalExpertise、SmartAssist、ArmorEnhancement 保存自身 ActiveGE 句柄，并在 AbilitySet 清除 Ability 时撤销。
- MarkHunter 保持激活监听击杀事件，直到 Ability 被清除时解绑。
- Round 重置只重置属性，不重授 Match AbilitySet。
- PostLogin 对迟到加入玩家幂等补授予。

### 2.3 有意未迁移的遗留

- `GA_ListenForEvent` 是父类为原生 `GameplayAbility` 的旧 Aura Event 到 GE 桥接蓝图。
- 当前 EventBasedEffectClass 与 EventTags 默认值为空，只有 `BP_ShootCharacter` 引用；本次清空引用但不删除资产。
- `UShootAbilitySystemComponent::GrantAbilitiesWithKit` 暂作为未确认蓝图引用的兼容入口保留，新主线禁止调用。确认全项目蓝图无引用后再删除。

## 3. P0 验收清单

- 单人进入可玩 Experience 后，Jump、Interact、QuickBar Next/Previous、DropWeapon 和相机切换各只有一份 AbilitySpec。
- 死亡重生十次后上述 AbilitySpec 数量不增长。
- 男女性别往返切换十次，公共 AbilitySpec 不变化，另一性别技能与被动 GE 不残留。
- 男主 MarkHunter 在授予后持续监听，切换女主或卸载 Experience 后不再响应。
- 被动 Buff 在对应性别套件存在时生效，套件撤销后 ActiveGE 消失。
- Experience 卸载后 PlayerCore、性别 Ability、长期 GE 与动态 AttributeSet 均被撤销。
- `InitializeRoundForAll` 不改变玩家 Match AbilitySpec 数量和技能冷却。
- Listen Server 的 Host、Remote Client 与分屏各自输入正确，没有重复授予。

## 4. Experience、主动技能与被动技能的实际关系

- 主动与被动仍全部属于 Experience 生命周期，不进入 SaveGame。
- Experience 载入时立即授予 `CommonAbilitySet` 与当前性别 AbilitySet。男女主 AbilitySet 目前只保留各自被动，
  因为被动是角色固有能力，进入对应可玩 Experience 后应立即生效。
- Experience 同时引用 `UShootExperienceDefinition::SkillLoadoutConfig`。该配置是本局可获得主动技能的目录，
  但不会在入场时把整个目录全部授予玩家。
- 玩家通过局内交互、Round 三选一或商店获得主动技能后，`UShootSkillLoadoutComponent` 才把该 Definition 的
  AbilitySet 授予对应槽位，并保存该槽自己的 `FShootAbilitySet_GrantedHandles`。
- Experience 卸载时，PlayerState 先清除四个主动槽位，再撤销性别被动与公共 AbilitySet。因此“获得时机不同”
  不等于“主动技能脱离 Experience”。如果入场时直接授予全部主动技能，随机获取、四槽上限、升级和替换规则都会失效。

### 4.1 当前桥接与 Lyra PawnData 对照

- Lyra 的 `ULyraPawnData::AbilitySets` 由 `ALyraPlayerState::SetPawnData` 遍历授予，核心原则是玩法数据声明来源、
  PlayerState/ASC 执行授予并保存生命周期，而不是 Character 永久直授。
- 本项目尚未引入完整 `UShootPawnData`，当前由每个 Experience 的 `CommonAbilitySet` 承载同一职责：战斗玩法选择
  PlayerCore，Home Hub 选择 HomeCore。两者互斥，并不是两条平行授予链。
- 未来若引入 PawnData，只迁移 AbilitySet 引用的归属；禁止复制一份继续双授予，也禁止为修 HomeMap 把 Jump/Interact
  重新放回 `BP_ShootCharacter`。

## 5. P1-P3 已落地资产与调用链

### 5.1 四槽权威数据

- `UShootSkillDefinition` 保存 SkillTag、名称、说明、图标、AbilitySet、最大等级、CooldownTag 与可选模式 Tag。
- `UShootSkillLoadoutConfig` 保存四个同序 `SlotInputTags`、`SlotInputActions` 与随机技能池。InputTag 决定 ASC
  授予到哪个槽，InputAction 只用于 CommonUI 当前改键提示和触摸按钮输入；具体键位仍由 IMC 决定。
- `UShootSkillLoadoutComponent` 位于 `AShootPlayerState`，固定四槽、服务器权威、仅向 Owner 复制。
- 有空槽时只会抽取尚未拥有的新技能并占第一个空槽；四槽填满后才从未满级技能中随机升级，最高三级；
  每槽独立持有完整 AbilitySet 撤销句柄。
- `/Game/GameFramework/Skills/DA_SkillLoadoutConfig_PVE` 被 ExpeditionSandbox、DungeonTest、SplitScreenTest
  三个可玩 Experience 共同引用，当前槽顺序严格为 Q/E/C/X，对应 IA_Skill1/2/3/4 与 InputTag.Q/E/C/X。
- 2026-09-04 用户验收发现旧配置错误写成 C/Q/E/X，并把 IA 排成 Skill3/1/2/4，导致 Q 消费第二槽；
  现已同时校正 InputTag 与 InputAction 顺序。IMC 继续维护实际按键，C 从蹲伏释放后由 IA_Skill3 使用。

### 5.2 当前机制调试池

- `DA_SkillLoadoutConfig_PVE::RandomSkillPool` 当前固定为手雷、医疗站、战术超载、战术突击，正好用于验证四槽填充、
  输入覆盖、升级和清理；这不代表四项都达到发布品质。
- 手雷是当前最成熟的基准技能，Definition 映射 `Cooldown.Weapon.Grenade`，当前冷却 10 秒，投掷和爆炸表现沿用正式手雷链。
- 战术突击使用 `Cooldown.Skill.TacticalAssault`，当前冷却 15 秒，与旧数值文档一致；释放时发送
  `GameplayCue.Skill.TacticalAssault.Activate`，Cue 蓝图使用 `P_ky_shotShockwave`。它仍只是第四槽机制占位，
  伤害、阵营/遮挡过滤、无敌帧、等级成长和动画不能宣称完成。
- 战术超载使用原生 `UShootEffect_TacticalOverloadState` 作为 8/10/12 秒权威持续 GE；GE 授予
  `Status.Overload`，`UShootAbilityCost_AmmoTagStack` 据此跳过弹药检查与消费，状态 HUD 也查询同一 ActiveGE。
- 2026-09-05 用户 PIE 证明环绕 Cue 能播放，但无限弹药和状态读条同时失效。根因是 GameplayEffect CDO 构造早于
  `FShootGameplayTags::InitializeNativeGameplayTags`，旧构造函数读取到无效的 `FShootGameplayTags::Get().Status_Overload`，
  导致持续 GE 没有真正授予 Tag。修复改为在构造阶段按 `DefaultGameplayTags.ini` 中的名称请求 Tag；2026-09-05 已完成冷构建，
  用户随后确认有限时间无限弹药与状态读条 PIE 验收通过。
- `/Game/UI/Skills/Status/W_TacticalOverloadStatus` 复用交互/换弹读条壳，通过
  `HUD.Slot.StatusEffects` 只注入三个战斗 Experience，直接读取同一个 ActiveGE 的 Remaining/Duration 显示名称、秒数和进度。
- `GameplayCue.Skill.TacticalOverload.Active` 使用 Looping GameplayCue 附着角色并播放
  `/Game/Effects/Niagara/Skills/TacticalOverload/NS_TacticalOverload_Aura`；GA 任意结束路径都会移除 Cue 和持续 GE。
  充能系统落地前仍使用 `Cooldown.Skill.TacticalOverload` 的 30 秒临时冷却；移速/射速/换弹倍率仍属于后续正式 Buff 数值配置。
- 医疗站已有服务器权威 Spawn、复制、阵营过滤和范围治疗。`GA_MedicalStation` 蓝图将生成
  `BP_SkillMedicalStation`，后者在继承自 `AShootSkillMedicalStation` 的 Sphere 根组件下配置
  `P_ky_healAura` 粒子组件，使整个 15 秒治疗期都有可见范围反馈；部署瞬间还会发送
  `GameplayCue.Skill.MedicalStation.Deploy`。GA 当前按产品要求直接取 Character 胶囊脚底再加 2cm 间隙，不做 Visibility Trace；
  原地面 Trace 作为注释参考保留在 C++，供以后投掷部署物贴合斜坡时复用。医疗站由服务器 `SetLifeSpan(15)` 精确销毁，Destroy 复制会连同客户端持续光环一起回收。
  充能系统落地前使用 `Cooldown.Skill.MedicalStation` 的 30 秒临时冷却。
- 三项新增冷却都由 GA 的 `CommitAbility` 应用 GameplayEffect，Definition 的 CooldownTag 与 GE 授予标签严格一致；
  `UShootSkillBarWidgetBase` 因此可以读取同一 ActiveGE 的剩余时间驱动卡片材质，不维护第二套 UI 计时器。
- 救援隐身、快速充能、震撼手雷、钢铁壁垒、战术扫描等旧 POC 不在当前池中；机器人、无人机进入 P4 正式开发。

### 5.3 随机获得交互

- `/Game/Blueprints/Interaction/BP_RandomSkillPickup` 继承 `AShootRandomSkillPickup`。
- `UShootGA_Interaction_AcquireSkill` 只依赖 `IShootSkillGrantSource`，不知道目标是世界拾取物、商人还是 Round 奖励；
  旧 `UShootGA_Interaction_GrantRandomSkill` 仅作为已有资产兼容类保留。
- `UShootSkillLoadoutComponent::AcquireSkill` 是指定技能的服务器权威事务入口，供未来商人购买和三选一使用；
  `AcquireRandomSkill` 只负责从当前 Experience 目录筛选候选，最终仍复用同一事务。
- 世界随机来源成功后写槽、授予 AbilitySet、复制结果并广播
  `Message.Skill.LoadoutChanged`。
- `TestMap_ListenServer` 与 `TestMap_SplitScreen` 各放置两个技能球，分别位于两个 PlayerStart 正前方；球体使用发光拾取材质。
- 交互射线仍负责聚焦和视线，技能来源在服务器按 500cm 做最终距离校验；旧的 120cm `IsOverlappingActor`
  限制已删除，避免玩家明明看见技能球却收不到交互选项。
- 未来商人实现 `IShootSkillGrantSource` 并调用 `AcquireSkill`；价格、库存和刷新属于商人事务，不能写入通用 GA，
  也不能让商人继承世界拾取物 Actor。

### 5.4 技能 HUD 与材质约束

- `W_DefaultHUD` 暴露 `HUD.Slot.Skills`，三个可玩 Experience 均向该插槽注册 `W_SkillBar`。
- `W_DefaultHUD` 还以原 Healthbar/Shieldbar 的锚点和尺寸暴露 `HUD.Slot.Health`、`HUD.Slot.Shield`；
  三个战斗 Experience 注入 `W_Healthbar` 与 `W_Shieldbar`，Home Experience 不注入战斗状态条。
- `DA_Experience_Home` 仍把 `W_DefaultHUD` 推入 `UI.Layer.Game`。该根布局继承 `ULyraActivatableWidget`，
  使用父类 `InputConfig=Game`；衣柜或副本页面通过 CommonUI 栈关闭后由栈恢复游戏输入，禁止额外调用旧式 `SetInputMode`。
- `BP_ShootHUD.EventGraph` 的无条件 Push 路径已由用户删除并保留为空；HUD 根布局只能由 Experience 创建一次。
- 用户重做的 `W_SkillBar` 保持纯蓝图布局壳，根结构沿用 QuickBar 的 Overlay、BackgroundBlur、Border、HBox、Spacer。
- 用户重做的 `W_SkillSlot` 保持纯蓝图视觉壳，内部继续使用 `W_ActionTouchButton` 与 `InputActionWidget`；其
  PreConstruct、Construct、Tick 和触摸输入图不得因 C++ 接线而删除。
- `UShootSkillBarWidgetBase` 是四槽唯一 C++ 数据桥，绑定目标 LocalPlayer 的 PlayerState，统一刷新四槽。
  旧的 `UShootSkillSlotWidgetBase` 已被替换，不再要求每个槽继承 C++ 类。
- 技能图标只允许写入 `W_ActionTouchButton.WeaponCard` 动态材质的 `IconTexture` 参数；禁止 `SetBrush`
  覆盖为裸 Texture，否则会丢失 `MI_UI_WeaponCard` 的底色、边框、遮罩和发光层。
- 冷却直接查询槽位 Definition 的 CooldownTag，并写 `WeaponCard`/`ItemGlow` 的 `Animate_Cooldown` 参数；
  `0` 表示刚进入冷却，`1` 表示重新可用。
- `W_ActionTouchButton::AssociatedAction` 与 `InputActionWidget::AssociatedInputAction` 由当前 Experience 的
  `SlotInputActions` 注入，保证触摸点击、键鼠改键与手柄仍走同一个 Enhanced Input Action。
- `ULyraActionWidget::SetAssociatedInputAction` 必须继续调用父类 `UCommonActionWidget::SetEnhancedInputAction`。
  UE 5.8 的 `SynchronizeProperties()` 在运行时不会调用 `UpdateActionWidget()`；只写自定义属性会得到截图中的白色空按键块。
- 空槽时隐藏 `WeaponCard`、`ItemGlow`、`ItemGlow_Boost` 和 `InputActionWidget`，显示用户迁入的 `EmptyText`；
  占用时反转。`ItemGlow_Boost` 当前始终隐藏，留给未来强化或选中态，不能在空槽显示青色发光块。
- 用户新增的 `W_SkillSlot.LevelSizer` 是等级容器，`W_SkillSlot.SkillLevel` 是等级文本；空槽隐藏 LevelSizer，
  获得或升级技能时显示复制槽数据中的 1-3。旧名称 `LevelText` 不再是有效 Widget 契约。
- 用户新增的 `SkillNameSizer/SkillName` 与 `CooldownSizer/RemainingCooldownTime` 已进入同一 C++ 数据桥生命周期：
  空槽清空并隐藏；占用时名称读取 Skill Definition 的 DisplayName；冷却中显示向上取整的剩余秒数，冷却结束立即清空并隐藏。
- C++ 只按名称寻找 `ActionTouchButton`、`WeaponCard`、`ItemGlow`、`ItemGlow_Boost`、`EmptyText`、
  `InputActionWidget`、等级、名称和冷却控件并驱动运行时状态，没有删除或替换 `W_SkillSlot` 的 PreConstruct、Construct、Tick 图表。

### 5.5 开发期调试入口

- `Shoot.Skill.AcquireRandom [PlayerIndex=0] [Count=1]`：通过正式 `AcquireRandomSkill` 获取或升级，限 Authority 世界。
- `Shoot.Skill.Clear [PlayerIndex=0]`：通过正式 `ClearMatchSkills` 清空四槽并撤销每槽 AbilitySet，限 Authority 世界。
- `Shoot.Skill.Dump [PlayerIndex=0]`：打印四槽 Definition、等级和输入 Tag。
- `PlayerIndex` 用于分屏隔离；Listen Server 必须在主机/服务器世界执行 Acquire/Clear。调试命令不维护第二套授予链。
- 这些命令只用于诊断和清理，不再是玩家验收“能否获得技能”的主路径；正常验收必须走地图中的 F 交互。

### 5.6 分屏血条初始化边界

- `W_Healthbar::SetHealthValue` 仅把有效快照建立后的 `Health>0 -> Health<=0` 视作真实死亡。
- 分屏第二个 LocalPlayer 的 PlayerState/ASC 可能先复制、默认属性 GE 后到达。旧 ViewModel 会先发布默认 100，
  再发布未初始化的 0/0，错误触发 `OnEliminated`；之后满血值到达也不能可靠撤销动画终态。
- `UAttributeViewModel` 现在等待 `Health>0 && MaxHealth>0` 的首个有效快照。有效快照前的 0 只表示属性尚未就绪；
  快照建立后后续的真实 0 仍正常广播，所以不会破坏死亡动画。

### 5.7 技能释放表现与 FXVarietyPack 边界

- `UShootGameplayCueNotify_Presentation` 新增可由 Cue 蓝图配置的 Cascade `UParticleSystem` 字段；项目原有 Niagara、声音和衰减配置保持不变。
- 同一 Cue 蓝图还可配置 LocationOffset、RotationOffset、EffectScale 与 CascadeLifetimeSeconds。循环 Cascade 不能依赖
  AutoDestroy；当前三个技能 Cue 的强制清理时间均为 1.5 秒，时间到后销毁本地粒子组件，不再把红色部署特效永久留在场景。
- 瞬时技能表现统一由服务器权威 GameplayCue 广播到各客户端，不在 GA C++ 中硬编码 `/Game/` 资产路径，也不让每端各自 Spawn 一份世界表现。
- `/Game/Effects/FXVarietyPack/Simple/AOE/BP_AOE_Base` 会自行 SphereOverlap、循环 Actor 并广播蓝图委托，且还保留调试输出；
  `BP_AOE_Health` 负责生成粒子和向地面 LineTrace。它们可作为表现和落地方式参考，但不能接管当前医疗站的阵营过滤、
  GAS 治疗或服务器权威逻辑，否则会形成第二套 AOE Gameplay。因此本轮直接复用粒子资产，不复用其伤害/治疗蓝图逻辑。

### 5.8 输入映射审计

- 完整映射与冲突矩阵见 `InputMappingAudit_技能与手柄弦操作.md`。
- 当前手柄四槽实际为 LB + Y/X/A/B；`IA_SkillModifier` 单独映射 LB，四条正式 IA 分别映射 Y/X/A/B，并各自使用
  `InputTriggerChordAction` 指向 `IA_SkillModifier`。
- 用户补充的黑神话悟空参考是 LT + Y/X/A/B。本项目 LT 已用于瞄准，RT 已用于攻击，不能在本轮直接替换。
- UE 5.8 `InjectChordBlockers` 只会阻断与“带 ChordAction 的映射”共享物理主键的低优先级映射。四个正式
  `IA_Skill1-4` 已直接映射面键，阻断器方向与 Y 交互、X 换弹和 A 跳跃的共享物理主键一致；实际优先级仍需 PIE 验证。
- `IA_Skill1-4`、`IA_SkillModifier` 已统一为 Boolean，旧 `IA_SkillY/X/A/B` 条件 IA 已删除。
- 设置页返回、应用、取消和恢复默认动作已从项目旧表迁移到 Lyra 主表 `/Game/UI/DT_UniversalActions`。
- 静态资产核验已完成；机器人 PIE 后再确认 LB 方案、四槽顺序、基础动作阻断和技能图标跟随输入设备。

### 5.9 本轮编译与资产核验

- 2026-09-04 使用项目 `Scripts/Build_Windows.ps1` 完成 `NewWorldOrderEditor Win64 Development` 项目编译，正式
  `UnrealEditor-NewWorldOrder.dll` 链接成功；编辑器随后以 `TestMap_SplitScreen` 启动并收到 VibeUE readiness signal。
- 三个 GA 的 CDO 已核对：Cooldown GE、ActivationBlockedTags 与 GameplayCue Tag 均指向各自的同名标签，没有共享或串槽。
- 三个 Cooldown GE 的 CDO 已核对为 `HasDuration`，时长分别为 15/30/30 秒，且各自只有一个命名为
  `TargetTags` 的 `UTargetTagsGameplayEffectComponent`；这避免旧实现中在 CDO 构造期动态 `NewObject` 导致的编辑器启动崩溃。
- 三个 GameplayCue 蓝图均继承 `UShootGameplayCueNotify_Presentation`，CueTag 与 Cascade 资产已经保存并重新读取核对。
- `BP_SkillMedicalStation` 已核对：`HealAuraFX` 是附着在继承 Sphere 上的 `ParticleSystemComponent`，Template 为
  `P_ky_healAura` 且 AutoActivate 为 True；医疗站 AbilitySet 已核对为授予 `GA_MedicalStation` 蓝图子类。
- 2026-09-05 已从编辑器回读技能输入资产：`IMC_Default` 共 44 条映射；`IA_SkillModifier` 为 LB，四个技能槽为
  Y/X/A/B 弦映射，LT/RT 仍分别为 ADS/攻击；Lyra 键鼠 ControllerData 为 90 条映射，设置页四个动作行均指向
  `/Game/UI/DT_UniversalActions`。
- 该次静态资产批的编辑器日志未发现 Blueprint、GameplayCue 或 AbilitySystem 编译错误；当时未启动 PIE，运行时验证见后续记录。
- 2026-09-05 使用 `Scripts/Build_Windows.ps1 -RestartEditor -WaitForReady` 完成一次冷构建，9 个 Action 全部成功，
  `UnrealEditor-NewWorldOrder.dll` 链接成功并收到 VibeUE readiness signal。随后从冷启动编辑器回读确认：
  `W_DefaultHUD.StatusEffectExtensionPoint` 使用 `HUD.Slot.StatusEffects`；三个战斗 Experience 各只注册一份
  `W_TacticalOverloadStatus`；Looping Cue 使用 `GameplayCue.Skill.TacticalOverload.Active` 并附着目标；战术超载持续时间为
  8/10/12 秒；机器人存续时间为 30/35/40 秒。资产层批处理脚本可幂等重跑，未启动额外 C++ 编译。
- 2026-09-05 修复 `Status.Overload` 初始化后再次使用同一脚本冷构建，4 个 Action 全部成功，DLL 重新链接并收到
  VibeUE readiness signal；用户随后完成战术超载 PIE 并确认状态读条与无限弹药通过。

### 5.10 当前四项 GA 的蓝图入口与 C++ 实现

| 技能 | AbilitySet 实际授予的蓝图 | 背后 C++ 类与源文件 |
|---|---|---|
| 手雷 | `/Game/Weapons/Grenade/GA_Grenade` | `UShootGA_ThrowGrenade`，`Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_ThrowGrenade.cpp` |
| 医疗站 | `/Game/Blueprints/Skills/Abilities/GA_MedicalStation` | `UShootGA_Female_MedicalStation`，`Source/NewWorldOrder/Private/AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_MedicalStation.cpp` |
| 战术突击 | `/Game/Blueprints/Skills/Abilities/GA_TacticalAssault` | `UShootGA_Male_TacticalAssault`，`Source/NewWorldOrder/Private/AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalAssault.cpp` |
| 战术超载 | `/Game/Blueprints/Skills/Abilities/GA_TacticalOverload` | `UShootGA_Male_TacticalOverload`，`Source/NewWorldOrder/Private/AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalOverload.cpp` |

- 三个非手雷 AbilitySet 已改为授予上述蓝图生成类，不再直接授予 C++ Class。
- 蓝图 Class Defaults 可以覆盖 C++ 暴露的 EditDefaultsOnly 参数。医疗站蓝图还能覆盖
  `ResolveMedicalStationSpawnTransform`，用于以后改成投掷部署或 NavMesh 吸附；不覆盖时使用当前脚下寻地实现。
- 网络权威、GAS Commit、伤害/治疗、生命周期等复杂规则仍留在 C++。蓝图壳负责资源引用、参数调优与必要的表现覆写，
  避免蓝图和 C++ 出现两套并行 Gameplay 逻辑。
- 2026-09-04 修正批使用 `Scripts/Build_Windows.ps1` 重新完成正式 DLL 编译和编辑器 readiness；重启后复读确认
  三个 GA 蓝图父类、AbilitySet 引用、医疗站 15 秒 Duration、地面参数和三个 Cue 的 1.5 秒清理值均已持久化。
- 2026-09-04 首次机器人 PIE 在任意槽位均无法激活。资产回读确认旧 `UShootAbilitySet::GiveToAbilitySystem` 只在条目
  已有固定 InputTag 时才应用槽位覆盖；机器人条目保持空标签后未进入 ASC 输入分发。第一次修复曾用 `InputTag.Q` 作为占位，
  用户指出它错误表达了技能固定属于 Q，因此该方案已撤销，不能作为最终架构。
- 用户复核后采用更简单的最终模型：`UShootSkillLoadoutComponent` 是唯一传入 `InputTagOverride` 的调用方，
  `UShootAbilitySet::GiveToAbilitySystem` 只要收到有效覆盖值就直接用于本次授予。所有 Match Skill AbilitySet 的标签保持为空，
  Q/E/C/X 的唯一配置来源是 `DA_SkillLoadoutConfig_PVE.SlotInputTags`。Experience、武器、男女主被动和机器人自身 AbilitySet
  调用时不传覆盖值，因此继续使用固定标签或保持无输入；无需增加额外布尔字段。

### 5.11 战术超载 PIE 失败与修复边界

- 失败不是 Niagara 或技能输入问题：同一次激活能看到 `GameplayCue.Skill.TacticalOverload.Active` 环绕效果，说明 GA 已进入激活链。
- 无限弹药和 `W_TacticalOverloadStatus` 唯一共同前置是 ASC 拥有 `Status.Overload`；两者同时失败据此收敛到持续 GE 的 TargetTags。
- `UShootEffect_TacticalOverloadState` 已改用 `FGameplayTag::RequestGameplayTag("Status.Overload")` 在 CDO 构造阶段取值，
  与项目 `UShootEffect_Marked` 等原生 GE 的既有初始化规则一致；不在弹药 Cost 或 Widget 中增加隐藏兼容分支。
- 2026-09-05 冷构建成功后，用户确认战术超载 PIE 验收通过：状态条显示名称与递减秒数，有限时间无限弹药恢复；
  这证明同一 `Status.Overload` 持续 GE 已重新驱动 HUD 与弹药 Cost。持续结束时 Tag、读条和环绕 Cue 的共同清理由原 GA EndAbility 链维持。

## 6. 用户统一 PIE 验收矩阵

- 已通过 1-4、8：HomeMap 交互与 HUD、Standalone 空槽、F 获取、Experience 退出清理。
- 待复验输入顺序：四槽从左到右显示 Q/E/C/X，按键分别激活同序第一至第四槽，不再由 Q 消费第二槽。
- 待复验输入图：获得技能后显示当前改键对应图标，不出现白色空块；键鼠/手柄切换后图标跟随 CommonUI 更新。
- 待复验等级：空槽不显示默认等级；新技能显示 1，继续获取升级后显示 2、3，且不超过三级。
- 待复验分屏血条：玩家 2 满血出生时不播放 `OnEliminated`，真实受到致死伤害时仍播放。
- 四槽冷却：手雷、战术突击、战术超载和医疗站释放后分别进入 10/15/30/30 秒冷却；冷却期间重复按键不能再次激活，
  卡片材质从不可用状态连续恢复，结束后允许再次释放。
- 释放反馈：战术突击出现冲击波、战术超载出现雷电、医疗站部署瞬间有反馈且 15 秒内持续显示治疗光环；
  Listen Server 的 Host 和 Remote Client 对同一次服务器技能只看到一份世界表现。
- 医疗站落点与清理：Actor 和光环贴近角色脚下地面，部署红色 Cue 在约 1.5 秒后消失；受伤友军进入光环后按 Tick
  恢复生命，敌对 Zombie 不被治疗；15 秒后医疗站 Actor 与绿色持续光环同时从 Host/Client 消失。
- 技能槽文本：空槽不显示 SkillName 或 RemainingCooldownTime；获得技能后显示 Definition 名称；释放后剩余秒数逐秒减少，
  冷却结束和 `Shoot.Skill.Clear` 后文本立即清空并隐藏。
- Standalone 获取参考：进入 `TestMap_ListenServer`，准星对准出生点正前方发光技能球并按 F；每次成功后球隐藏，
  5 秒重生，四次交互依次填入四个不同技能；卡片保留材质壳、图标和当前改键提示。
- Standalone 升级：四槽填满后继续与重生的技能球交互，只升级现有四项且不超过三级；必要时用
  `Shoot.Skill.Dump 0` 诊断，但不以控制台注入替代交互验收。
- Standalone 清理：执行 `Shoot.Skill.Clear 0` 后四槽恢复 Empty，按槽授予的 AbilitySpec 均被撤销。
- SplitScreen：两个出生点正前方各有一个技能球；两名本地玩家分别对准并按 F，确认两套 HUD 与槽位数据互不污染。
- Listen Server：Host 与 Remote Client 分别通过 F 交互获取，确认只有对应 Owner 收到槽位，并能用实际槽 IA
  激活服务器授予的 GA。
- 生命周期：带技能退出或切换 Experience 后重新进入 TestMap，四槽为空；SaveGame 不恢复 Definition、等级或 AbilitySpec。
- 机器人 P4 首版按 `RobotCompanion_机器人伙伴技能设计与验收.md` 第 12 节集中验收：固定 F 获取、召唤、正式
  AIController/Blackboard/BehaviorTree、AnimBP/Montage、Team Hostile 索敌、三模式、Lv1-Lv3、战斗击毁冷却、Owner 死亡自爆、
  Experience 清理、Listen Server 与 SplitScreen 隔离。
- 当前旧 POC 与无人机不列为 P4 首版通过项；机器人伙伴首版已于 2026-09-05 由用户确认 PIE 验收通过。
- 机器人复验先只执行第 1-2 步：固定 F 获取后，第一槽 Q 应能召唤且不会立即进入冷却。首次召唤无冷却是产品规则；
  只有第 6 步的战斗击毁才施加 20 秒冷却。第 1-2 步通过后再继续余下机器人验收，避免把输入故障与 AI 行为混在一起判断。
- 机器人首次攻击验收失败：能够运行 BehaviorTree 并跟随 Owner，但在尸群中不进入 Fire。修正后 Zombie 显式注册为 Sight
  Stimuli Source；机器人黑板直接采用 Sight 当前感知结果，不再叠加一套可能冲突的 `LineOfSightTo`；Fire GA 仍在服务器用
  Visibility Trace 裁决实际命中，前排 Hostile 挡住原目标时改为伤害前排目标。
- 攻击失败的直接行为树原因已经确认：攻击 Sequence 的 Blackboard Decorator 原为 `FlowAbortMode=None`，低优先级 MoveTo
  运行后无法被新出现的射程条件抢占；现改为 `LowerPriority`。Controller 继承 Owner TeamId 后还会调用
  `RequestStimuliListenerUpdate()`，对齐 Lyra 对 Perception Listener 队伍缓存的刷新方式。
- 首次召唤增加 500cm 高处自然下落；落地通过 `GameplayCue.Skill.RobotCompanion.SummonImpact` 同时播放
  `NS_ImpactConcrete` 与缩小的 `P_ky_explosion`，落点来自 `Landed` 的 ImpactPoint，不会再次出现悬空特效。
- 落地新增 45/350cm、Owner 死亡自爆新增 120/450cm 的服务器权威范围伤害；两者只作用于 Team Attitude 为 Hostile
  且持有 ASC 的目标，并通过现有 Damage GE 结算。四个数值定义在 `AShootRobotCompanionCharacter`，可由
  `BP_RobotCompanionCharacter` 的 Class Defaults 覆盖。
- `ABP_RobotCompanion` 补齐 Airborne 状态，父类 `UShootRobotAnimInstance` 提供 bIsFalling/bIsDead；落地规则显式为
  `!bIsFalling`。右/左 Buster 与 PowerPod 分别使用 SkeletalMesh 自带的 `Buster_RSocket`、`Buster_LSocket`、`PowerPod`，
  左右 Fire Montage 跟实际枪口对应。
- 2026-09-04 修正批已完成 `NewWorldOrderEditor Win64 Development` 冷构建，结果为 `Succeeded`（587.90 秒）。编辑器
  冷启动回读确认行为树、AnimBP、机器人蓝图默认值、两项 GA 和三个 GameplayCue 均已保存并处于最新编译状态；运行行为仍待用户按
  机器人文档第 12 节统一 PIE，不能以静态回读代替验收。
- 机器人 GA、Character、Controller、固定拾取和 GameplayCue 已按 ContentOrganization 迁移到
  GameFramework/AI/Gameplay/Effects 域。`Config/DefaultGame.ini` 的旧兼容段与 UE 5.8 DeveloperSettings 段均新增
  `/Game/Effects/GameplayCues` 扫描路径，确保新目录既符合内容规范，也能被 GameplayCueManager 在运行时发现。
- 从 Home 打开衣柜、关闭衣柜、进入副本后分别确认 CommonUI 自动在 `Menu` 与 `Game` 输入配置间恢复；出现鼠标游离时
  先检查页面是否调用 `DeactivateWidget()`，禁止用 PlayerController `SetInputMode` 掩盖栈错误。

## 7. 下一阶段顺序

### P4 机器人伙伴垂直切片

- 2026-09-04 已完成 `AShootRobotCompanionCharacter`、`AShootRobotCompanionController`、PlayerState 伙伴组件、
  Blackboard、Behavior Tree、RadicalMike AnimBP、Fire/Death Montage、两层 AbilitySet、技能 Definition、固定交互来源与 GameplayCue。
- AI 感知只收集候选；敌我结论走 Team Attitude，射击只对 Hostile 且持有 ASC 的存活目标生效。Character 不用 Tick 寻敌，
  BehaviorTree Task 只激活机器人自身服务器 GA。
- 被敌人击毁后施加 20 秒冷却，结束可重召；Owner 死亡时走独立自爆清理，默认不附加战斗击毁冷却；
  技能移除和 Experience 卸载清理 Actor 且不写 SaveGame。
- 首版采用已确认降级方案：再次按同一技能键按远程压制、近战强袭、均衡护卫循环，技能栏名称和图标同步变化；CommonUI 指令轮盘留到 P4.1。
- 2026-09-05 三模式从“活动范围差异”收束为真正的攻击策略：远程只射击，近战只在贴近后使用左爪/咬击，均衡按距离择招并以 Owner 周围威胁为优先。标签已改为 `Ability.Mode.Robot.Ranged/Melee/Balanced`，旧 Follow/Assault/Guard 不再写入技能资产。
- 平衡基线为远程单发 8/10/12、射击基础间隔 0.7/0.65/0.6 秒；远程模式射击间隔乘 0.6。左爪为 32/38/44、1 秒一次，近战模式每第三击使用 1.35 倍咬击。该数值定位是“远程稳定副输出、近战高风险高回报”，仍需与正式玩家武器做同场 DPS 验收。
- 机器人 Lv1/Lv2/Lv3 当前存续 30/35/40 秒，到期执行 120/450cm Team Hostile 自爆并开始 20 秒重召冷却；提前被击毁立即进入同一冷却。Owner 死亡默认自爆但不额外处罚，技能移除与 Experience 卸载静默回收。冷却不在召唤时开始，因为同一个技能 GA 在机器人存续期间还负责模式切换。
- 机器人伙伴首版的三模式、远近程攻击、召唤/自爆、范围伤害和冷却主链已通过用户 PIE 验收；存续时间从历史验证值 18 秒调整为 30/35/40 秒后只需补做定时回归。
- Lv1 右武器，Lv2 增强生命/伤害/射速，Lv3 显示左武器并交替枪口；升级更新唯一实例，不重复生成 AIController 或 Ability。
- GA 蓝图入口、资产路径、实现细节、已知表现调优点和完整 PIE 清单见
  `RobotCompanion_机器人伙伴技能设计与验收.md` 第 11-12 节。
- 修正后的代码与资产将在本批末只执行一次 `Scripts/Build_Windows.ps1 -RestartEditor`；冷启动后统一回读蓝图、BehaviorTree、
  AnimBP、Montage Notify、两层 AbilitySet、技能池和 Character CDO。本轮仍不启动 PIE。

### P4.1 验收后重构与表现增强

- 机器人通过 PIE 后，再补 RadicalMike HitReact/Reload、正式机器人图标和可选 CommonUI 指令轮盘。
- 下一项 Gameplay 主线是 Zombie 生产化重构。现状、保留边界、分批步骤和验收标准以
  `Docs/Tasks/ZombieAI/RefactorPlan_生产化重构计划.md` 为准：先做正式 AnimBP，再把即时近战迁为 Montage/Notify 驱动的 Zombie GA，
  最后处理尸群目标评分和可复用抽取。只在机器人与 Zombie 都验证后提取真实共用接口，避免提前制造宽泛父类。
- GameplayTag 清理采用“先证明无资产与代码依赖，再删除定义和调用点”的规则。当前 `Status.Overload`、
  `Cooldown.Skill.TacticalOverload` 与两项 TacticalOverload GameplayCue Tag 都有明确运行时消费者，必须保留；
  编辑器回读已确认 `DA_Skill_TacticalOverload.SkillTag` 正在使用 `Abilities.Male.Ultimate.TacticalOverload` 作为技能身份，
  因此本轮不改名。2026-09-07 已专项核对所有 Match Skill AbilitySet、Definition、Experience、角色 CDO 和
  `UShootSkillLoadoutComponent` 授予链：十套技能 AbilitySet 条目的 `InputTag` 均为空，只有 SkillLoadout 的
  `InputTagOverride` 传入当前槽位 Tag；未发现绕过路径。已从九个 Match Skill GA 的 C++ 构造函数移除固定
  `StartupInputTag`（包括旧 POC 和手雷技能），统一只允许槽位事务动态传入 InputTag；武器、换弹和机器人自身能力的固定输入保持不变。
  本阶段统一编译后还需回读原生 CDO，确认默认值在编辑器加载态同步为空。
- 对照玩家、Zombie、机器人后审计 `AShootCharacterBase` 与 `ICombatInterface` 的 Aura 历史残留；本批只恢复通用死亡委托，
  不借机器人功能重构无关父类代码。

## 8. 本阶段明确不做

- 不继续生化地图白盒。
- 不全量复制 Lyra GameFeature Experience。
- 不把技能、等级或局内金币写入 SaveGame。
- 不建立第二套 Team、Input、Inventory、UI Layer 或 GameplayTag 权威体系。
- 不在四槽闭环前提前制作机器人简单 POC。
