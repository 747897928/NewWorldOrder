# 技术约束与架构规范（代码开发必读）

## 仓库结构与构建

- 项目入口：NewWorldOrder.uproject
- 源码：Source/NewWorldOrder
- 插件：Plugins
- 资源：Content
- 配置：Config
- 忽略：Binaries、Intermediate、Saved、DerivedDataCache
- 运行示例：UnrealEditor.exe NewWorldOrder.uproject -game

---

## 关键实现不变量

服务器权威：
- 弹药、散布、后坐力、射击/装填结果仅服务器修改
- 客户端通过复制与校正显示

GAS授予：
- 授予GA时必须设置Spec.SourceObject = WeaponInstance
- 避免能力找不到来源

投射物：
- 仅服务器生成
- 正确设置所有权

弹药不是能力消耗：
- 避免GA再次扣除弹药

---

## GAS（Gameplay Ability System）架构

### ASC位置与复制模式

玩家：
- ASC位于PlayerState
- ReplicationMode = Mixed
- InitAbilityActorInfo(PlayerState, Character)
- 在OnRep_PlayerState、OnRep_Pawn时重绑

AI：
- ASC位于Character
- ReplicationMode = Minimal
- 服务器侧调用InitAbilityActorInfo(this, this)

GAS详细流程参考：
- Docs/GASDocumentation_Chinese/README.md（UE4.26社区教程）
- 注意对照本项目的UE5变更与自定义实现

### GameplayTag约定

注册方式按用途分源：需要 C++ 稳定引用的 Native Tag 优先使用宏：
```cpp
// ShootGameplayTags.h
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TagName)

// ShootGameplayTags.cpp
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TagName, "Path", "Comment")
```

宏在模块加载阶段完成注册，构造函数内可直接使用。

只供配置、蓝图或 GameplayAbility CDO 使用的 Tag 放在
`Config/DefaultGameplayTags.ini`：
```ini
+GameplayTagList=(Tag="Ability.Skill.Zombie.Melee",DevComment="Zombie-owned melee ability")
```

构造阶段读取配置 Tag：
```cpp
const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(
    FName(TEXT("Ability.Skill.Zombie.Melee")), false);
```

同一个 Tag 只能选择一种注册源，不能既作为 Native Tag 添加到 `FShootGameplayTags`，又在
`DefaultGameplayTags.ini` 中重复注册。

兼容层：
- 保留FShootGameplayTags::InitializeNativeGameplayTags()作为兼容层
- 注意它在UShootAssetManager::StartInitialLoading()期间执行
- 早于该时刻获取到的值可能为空

使用指引：
- 构造函数、静态成员等早期阶段禁止访问FShootGameplayTags::Get()
- 若必须取值，调用FGameplayTag::RequestGameplayTag("Path", false)作为兜底
- Native Tag 的游戏运行时共享访问统一通过FShootGameplayTags::Get()或宏定义的静态变量；
  Config Tag 在构造期和运行时均通过RequestGameplayTag读取
- 避免散布字符串常量

新增标签流程：
1. 先判断 Tag 是 C++ Native Tag，还是配置/蓝图/GameplayAbility CDO 使用的 Config Tag
2. Native Tag 在 ShootGameplayTags.h/.cpp 使用宏注册；Config Tag 只写入 Config/DefaultGameplayTags.ini
3. 构造阶段读取 Config Tag 时使用 FGameplayTag::RequestGameplayTag(FName(TEXT("Path")), false)，
   不访问尚未完成 AssetManager 初始化的 FShootGameplayTags 字段
4. 文档中同步记录用途、唯一注册源及是否支持构造阶段调用

Zombie 当前配置 Tag：`Ability.Skill.Zombie.Melee`、`Cooldown.AI.ZombieMelee`。两者只存在于
`Config/DefaultGameplayTags.ini`，不在 `FShootGameplayTags` 中维护字段。

清理遗留：
- 继续迁移旧的InputTag_1/2/3/4、Ability_Type_Action_Skill1~4等模板标签到新规范（现用 InputTag.Q/E/C/X + Ability.Type.Skill.*）。
- 未使用的标签应标记"待移除"或删除，避免与键位设计混淆（Q/E/C/X）。

---

## 武器设计哲学

### 核心原则

ASC永远挂在角色侧：
- 玩家ASC位于PlayerState
- AI ASC位于Character
- 武器实例仅保存状态
- 禁止在武器上新增ASC

装备流程统一：
- UCombatComponent调度
- 武器实例完成附着、能力授予、消息广播
- 扩展武器时不要绕开这条链路

插槽命名：
- 来自UWeaponDefinition
- 右手：weapon_socket_hand_r
- 左手：weapon_socket_hand_l
- 背后右：weapon_socket_spine_back_r
- 背后左：weapon_socket_spine_back_l
- 右大腿：weapon_socket_thigh_r
- 近战腰间：weapon_socket_pelvis_melee
- 需要调整请修改数据资产字段并复用，禁止硬编码

装备/卸载：
- 同时处理能力套装
- 动画层链接（LinkAnimClassLayers / UnlinkAnimClassLayers）
- 服务器权威数据
- 保持"武器驱动行为、角色提供ASC"的拆分

拔枪/收枪动画：
- 通过能力或组件触发蒙太奇
- 在Anim Notify或Gameplay Event中调用现有attach/detach逻辑
- 确保与网络时序一致

数据表与公式参考：
- Docs/SystemDesign/GameDesign/NumericalDesign/Weapons/（WeaponData、BalanceValidation、CSVStructure）

Mutable（角色定制）：
- 若对Customizable Object流程不熟，可查阅Docs/Mutable-Documentation（官方Wiki镜像与中文笔记）
- 涵盖CO/COI、状态、UI元数据、代码/蓝图接入、Descriptor复制、性能优化等
- 实现前先确认所需节点与状态配置

### 关键类型与接口

ARangedWeaponInstance：
- 复制字段：CurrentAmmo、CurrentReserve
- 服务器接口：HasEnoughAmmo、ConsumeAmmo、ReloadAmmo、IsMagazineFull、IsMagazineEmpty、GetAmmoPerShot、GetMuzzleLocation、GetBaseDamage、GetOwnerAbilitySystemComponent
- OnRep回调需通过UGameplayMessageSubsystem广播UI

AHitscanWeaponInstance：
- 复制字段：LastFiredTime、CurrentSpreadAngleMultiplier
- 服务器接口：AddSpread、UpdateFiringTime、GetCalculatedSpreadAngle、GetCalculatedSpreadAngleMultiplier、GetSpreadExponent、GetBulletTraceSweepRadius、GetBulletsPerCartridge

开火/装填GA：
- UShootGameplayAbility_Weapon_Fire
- UShootGA_Weapon_Fire_Rifle
- UShootGA_Weapon_Fire_Projectile
- UShootGameplayAbility_ReloadMagazine
- UShootAbilityCost_Ammo：仅读取Spec.SourceObject

UCombatComponent：
- 装备/卸下调用武器实例的OnEquipped / OnUnequipped
- 广播消息：Msg_Quickbar_SlotsChanged、Msg_Quickbar_ActiveIndexChanged

---

## 网络复制准则

### Listen Server架构

- Host同时承担服务器与本地客户端
- 其他玩家通过JoinSession加入
- 兼容Steam OSS，后续将扩展至EOS/Null

### 复制准则

所有状态写入：
- 必须在HasAuthority() / ROLE_Authority侧执行
- 客户端发起改动需调用Server RPC

OnRep_回调：
- 仅在非权威端触发
- 用于UI或本地表现同步
- 禁止在其中反写服务器数据

复制字段变更：
- 继续通过UGameplayMessageSubsystem推送UI / ViewModel更新

### 会话流程要求

Host与Client：
- 均需支持创建、加入、离开、销毁会话
- Host结束副本时调用ReturnToMainMenu() → HandleDisconnect()并Travel至默认主菜单地图
- 确保FindSessions不被阻塞

客户端：
- 可以单独离开会话而不影响其他玩家

### 提示建议

给Codex / Claude Code派发任务时：
- 务必重申上述网络准则
- 重申"修改前先输出计划"的硬性要求
- 若缺乏数据或权威模块约束，需先向需求方确认，禁止臆测

---

## 技能系统键位设计

键位定义（参考无畏契约）：
- Q：主动技能1（战术标记/战术扫描）
- E：主动技能2（震撼手雷/救援掩护）
- C：主动技能3（战术闪避/紧急闪避）
- X：终极技能（大招，需充能，Lv5解锁）
- 可选技能：Lv10+解锁，替换Q/E槽位，非独立槽位
- 被动技能：4个槽位（Lv5/10/15/20解锁）

InputTag命名（现用）：
- InputTag.Q / InputTag.E / InputTag.C / InputTag.X（技能槽，对应 Q/E/C/X）
- InputTag.Passive.1 / InputTag.Passive.2（被动槽）

重要约束：
- 大招槽位是X，不是R（R是换弹键）
- 可选技能替换现有Q/E槽，不是额外的独立槽位
- 技能设计遵循"武器80%伤害+技能20%伤害"原则
- 详细数值与Build参考：Docs/SystemDesign/GameDesign/NumericalDesign/Skills/

---

## 主角切换与外观

单一ASC常驻PlayerState：
- 切换主角时仅替换Avatar、能力套装
- 刷新属性快照

外观组件UMutableAppearanceComponent：
- SwitchGender
- LoadWardrobeForGenderFromSave
- OnCustomizableSkeletalUpdated

能力套装工具：
- RemoveAbilitiesByKit
- GrantAbilitiesWithKit
- 属性快照由UShootAbilitySystemLibrary负责刷新/卸载

---

## 队伍与感知

使用ILyraTeamAgentInterface：
- 人类玩家TeamId = 1
- 丧尸AI TeamId = 2
- 伤害上下文通常提供 Avatar Pawn，但玩家队伍接口定义在 PlayerState、AI 队伍接口定义在 Controller。敌我判断必须像 Lyra 的 `FindTeamFromObject` 一样沿 `Pawn -> PlayerState / Controller` 解析，禁止只对 Pawn 本体 Cast 接口后把解析失败当成敌对。
- `AShootGameModeBase::GetFriendlyFireScalarForActors` 是当前伤害结算的统一友伤倍率入口；同队使用难度配置的 FriendlyFireScalar，双方队伍不同或目标无队伍信息时维持正常伤害。
- `AShootGameModeBase::PostLogin` 通过蓝图可配置的 `DefaultPlayerTeamId` 为玩家分队；`AShootPlayerState` 只保存和复制结果，不在自身生命周期里猜测默认队伍。
- `AShootPlayerController` 不保存第二份 TeamId，只按 Lyra 模式代理 PlayerState 并转发队伍变更委托，`ULyraLocalPlayer` 再观察 Controller。禁止分别在这三个对象硬编码互不联动的队伍状态。
- Actor、Pawn、Controller、PlayerState 与 Instigator 的敌我关系统一由 `UShootAbilitySystemLibrary::GetTeamAttitudeForActors` 解析；GameMode、伤害和技能筛选不得再维护各自的队伍解析器。

AEnemyBotController：
- 缓存TeamId
- 实现GetTeamAttitudeTowards

---

## 消息与标签入口

消息结构：
- Source/NewWorldOrder/Public/Character/QuickbarMessageTypes.h

Gameplay Tags配置：
- 参照FShootGameplayTags

UGameplayMessageSubsystem：
- 提供GameInstance级消息总线
- 使用UGameplayMessageSubsystem::Get(WorldContext)广播/监听结构化消息
- 复制回调或服务器状态变更（如ARangedWeaponInstance::OnRep_CurrentAmmo、UCombatComponent::OnRep_ActiveSlotIndex）必须通过该总线向UI / ViewModel或其他松耦合系统同步
- 同一对象内部的即时交互仍可使用直接调用

---

## UI绑定与ViewModel

### ULyraLocalPlayer

Source/NewWorldOrder/Private/Player/LyraLocalPlayer.cpp：
- 实现IAttributeViewModelInterface
- 按需创建持久化UAttributeViewModel

### UAttributeViewModel

Source/NewWorldOrder/Public/UI/ViewModel/AttributeViewModel.h：
- 继承自UMVVMViewModelBase
- 依赖FieldNotify属性和UE_MVVM_SET_PROPERTY_VALUE宏向UMG广播
- 聚合玩家属性、经验、可升级点数等数据
- 在InitializeWithPlayerState / BindCallbacksToDependencies中监听ASC、AttributeSet与PlayerState
- 禁止在ViewModel中写入服务器权威逻辑或直接修改游戏状态

### UMVVMViewModelBase特点

适合本地UI同步：
- 字段多为标量或轻量结构
- 复杂容器（TArray、TMap）只支持整体刷新，不提供元素级通知
- 若需展示列表，请维护快照并调用UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED，或拆分为多个简单字段

### MVVM与消息总线的协作

- 当数据仅面向本地UI且需要蓝图自动更新时使用ViewModel
- 跨对象广播或复制回调使用消息总线
- 推荐流程：服务器/ASC更新 → OnRep/委托 → 广播消息 → 本地ViewModel监听并写入FieldNotify字段

### 蓝图与C++的分工

- 无法在此环境直接编辑蓝图
- 核心流程（能力、武器逻辑、UI数据流等）需优先落在C++
- 使用虚/纯虚函数让子类重写
- 必要时再提供BlueprintNativeEvent供额外覆盖
- 蓝图仅负责表现层或调参钩子，并保持注释明确

---

## 注释与可读性

跨类消息流、OnRep回调链、ASC委托：
- 新增或修改运行路径时，务必在关键语句旁添加简洁中文行注释
- 说明消息来源、目的与执行顺序
- 若逻辑依赖UGameplayMessageSubsystem、ViewModel或GAS事件，需要在注释中指明对应的消息标签、ViewModel字段或委托名称，便于追踪

简单赋值/初始化：
- 可不加注释
- 但跨类、复制或异步流程必须补中文说明
- 缺少注释视为未满足要求

---

## 代码职责与复用

实现新功能前：
- 先检索已有函数
- 若逻辑仅此一处使用且非常短小，可本地实现
- 但需在注释或提交中说明不抽取原因

类保持单一职责：
- 如需跨模块协作通过现有接口或新增专用组件
- 避免把临时逻辑塞入无关类

编写新功能前：
- 评估后续扩展潜力
- 在提交描述中说明逻辑归属
- 如扩展潜力大，需拆分为独立组件

若发现类内混入其他系统逻辑：
- 应在下一轮迭代中重构拆分
- 保持结构清晰

---

## 权威模块标记

继承Lyra原实现，默认不可直接改动，请使用子类或扩展：
- Source/NewWorldOrder/Private/Interaction/及相关ASC交互框架
- 任何标记为// Canonical的文件或段落（请保留原注释）

草稿/待完善模块（可重构）：
- WeaponDefinition
- RangedWeaponInstance
- 新武器或未注明的业务代码

若确实需要修改权威模块：
- 务必在计划和最终总结中写明原因
- 先尝试通过继承、组合或局部override解决

模板遗留标签：
- 如InputTag_3/4、Ability_Type_Action_Skill1~4等目前未使用
- 后续可清理或标注"待移除"
- 避免误导

---

## Sonar约束规则

函数复杂度要求：
- 行数≤80行（包含空行和注释）
- 嵌套层级≤3层
- 认知复杂度≤15
- 单一职责（只做一件事）

降低复杂度技巧：
- 提前返回（Early Return）：if (!Condition) return;
- 提前continue（循环中）：if (!Valid) continue;
- 提炼函数（Extract Method）：拆分大函数
- 表驱动法：用Map替代长串if-else

---

## 开发笔记与知识积累

新增知识点时：
- 立即记录到Docs/DevelopmentNotes/对应分类
- 使用笔记模板（见DevelopmentNotes/README.md）
- 标注状态：[可用] / [待验证] / [已废弃]
- 更新DevelopmentNotes/INDEX.md索引

踩坑时：
- 记录到Docs/DevelopmentNotes/Pitfalls/
- 说明：错误现象、原因、为什么会犯错、如何避免
- 链接到对应的Solutions/解决方案
- 更新INDEX.md

找到解决方案时：
- 记录到Docs/DevelopmentNotes/Solutions/
- 说明：正确做法、代码示例、注意事项、适用场景
- 如果替代了旧方案，在旧笔记顶部标记"[已废弃]"并链接新方案

发现笔记错误时：
- 不要删除旧笔记，在顶部添加"[已废弃]"标记
- 说明为什么错了、正确的理解是什么
- 链接到正确的新笔记

实现系统时：
- 在对应的SystemDesign文档中添加"实现笔记"章节
- 记录：架构决策、关键类设计、数据表结构、网络同步方案
- 代码中添加详细中文注释

---

## 参考文档

详细系统设计：
- Docs/SystemDesign/GameDesign/NumericalDesign/Attributes/（属性系统）
- Docs/SystemDesign/GameDesign/NumericalDesign/Skills/（技能系统）
- Docs/SystemDesign/GameDesign/NumericalDesign/Weapons/（武器系统）

快速参考：
- Docs/QuickReference/Attributes.md
- Docs/QuickReference/Skills.md
- Docs/QuickReference/Weapons.md

任务包：
- Docs/Tasks/CharacterSwitching/（角色切换完整实现指南）

开发笔记：
- Docs/DevelopmentNotes/INDEX.md（笔记索引）
- Docs/Engineering/Notes/（关键技术笔记）

---

最后更新：2025-11-08
