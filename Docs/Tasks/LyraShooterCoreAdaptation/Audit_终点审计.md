# Lyra ShooterCore 终点审计

## 当前有效性说明

本文主体记录 2026-08-02 至 2026-08-06 的阶段审计，保留用于解释当时的回撤与风险，不再代表当前断点。分屏男女互斥、三枪弹药、Foot/Hand Controls、Equip/Unequip、伤害数字、补给站、角色到武器 Montage 同步与男性 Shotgun Fire 已在后续阶段完成；当前权威状态见 `Status_状态.md`，数值与运行证据见 `Verification_验收.md`。

## 审计范围

- 起点：`0262c45a`
- 远端终点：`db72be5`
- 当前纠错工作区：以远端终点为基础，包含本文件记录的回撤和修正
- 审计方法：先比较起点与终点全部文本差异，再检查关键蓝图、动画资产和当前未提交内容

本审计关注“终点相对起点最终留下了什么”，不按单个提交孤立判断。二进制资产只做路径、父类、组件、引用和运行入口核对，不把无法文本比较的内容猜成正确实现。

## 文本变更结论

### 保留

- `AShootPlayerController` 按 Lyra 模式实现 `ILyraTeamAgentInterface`，只代理 `AShootPlayerState`，不保存第二份 TeamId。
- `GA_Hero_Jump` 继续由 `BP_ShootCharacter.StartupAbilities` 授予；删除 C++ 父类里重复添加的 Activate/WaitInputRelease 流程。
- `AShootGameMode::HandleStartingNewPlayer_Implementation` 在 Pawn 生成前按可靠 LocalPlayer 索引重做幂等存档恢复，用于分屏男女主互斥；最终分屏回归尚未完成。
- `UMutableAppearanceComponent` 的冷启动就绪检查与 Mutable 重建后重新链接武器动画层；最终外观和动画压力测试尚未完成。
- `UShootInventoryItemInstance` 的 StatTags 变更通知与 `UCombatComponent` 的既有弹药消息转发；Rifle 已有单人证据，Pistol、Shotgun、分屏和联机仍待验收。
- `UAttributeViewModel` 复用现有 Health、MaxHealth，新增 PlayerName 和 HealthPercent FieldNotify；世界名牌仍可直接从 Pawn 的 PlayerState 读取名字，不为它新建第二套 ViewModel。
- 项目维护的 `AnimationAssetFixer` 继续作为编辑器辅助插件，用于读取 Linked Anim Layer 图、A/B 验证 SkeletalControl、修复节点序列化 fallback、审计虚拟骨骼和管理正式 Montage Notify，不进入运行时玩法链，也不修改 VibeUE 或引擎源码。

### 2026-08-02 动画层内部引用与骨架契约审计

- 男女正式 `ABP_ItemAnimLayersBase` 的图数量和节点数量分别与 Lyra 原资产一致，排除“只复制了少量图”的误判。
- 两个 CC Base 的 `FullBody_Aiming` 各有两个 AimOffset 节点遗留 `AO_MM_Unarmed_Idle_Ready`，`LeftHandPose_OverrideState` 各有一个 SequenceEvaluator fallback 遗留 `MM_Rifle_Idle_Hipfire`。现已替换为同侧正式 CC 资产，并在编辑器重启后验证旧引用为零、变量 Pin 连接仍在。
- Lyra `weapon_r` 在 Rifle Idle/Jog 中是随时间变化的独立轨道，不是静态参考骨。CC Skeleton 和重定向动画均缺失它，导致 `VB IK_Hand_L_weaponSpace`、CopyBone 和 ModifyBone 链不完整。
- 详细原理、错误方案和修复顺序见 `AnimationFoundation_动画地基与IK.md`。

### 2026-08-06 CC 骨架与 Linked Layer 类型闭环

- 男女 CC SkeletalMesh/Skeleton 已同步补齐 Lyra `weapon_r`，父级为 `hand_r`；三个虚拟骨骼映射与 Lyra 一致。
- 正式 CC Rifle/Pistol/Shotgun、空手相关动画和 AimOffset 已迁移 `weapon_r` 轨道；迁移脚本区分普通全姿势和 additive 全姿势，不用静态帧替代动态 weapon space。
- 男女 `ABP_ItemAnimLayersBase.GetMainAnimBPThreadSafe` 原先仍返回源 Mannequin 主 AnimBP。除返回 Pin 和 DynamicCast 外，图内旧主类变量、函数 MemberReference 和 Property Access Pin 也必须迁移；只改一个节点会得到“同名对象引用不兼容”的编译错误。
- 当前 MM/MF 的 Base、CopyPose、Retarget、Item Base、Rifle、Pistol、Shotgun、Unarmed 共 16 个 AnimBP 已全部重新编译为 `BS_UP_TO_DATE`。
- 仍未证明运行时姿势正确；`StanceTransition -> Idle`、`IdleBreak -> Idle` 的 looping 自动过渡警告、Warping/FootPlacement 输入和 Shotgun 固定枪口偏差继续列为玩家验收阻断。

### 纠正

- 删除 `AShootGameModeBase` 内新增的第二套 `ResolveTeamAgent`；Actor、Pawn、Controller、PlayerState 与 Instigator 的队伍解析统一进入 `UShootAbilitySystemLibrary::GetTeamAttitudeForActors`。
- 删除 `AShootPlayerState::BeginPlay` 的默认 Team 1 埋点。`AShootGameModeBase::PostLogin` 通过蓝图可配置的 `DefaultPlayerTeamId` 分队，PlayerState 只保存和复制结果。
- 整弹匣换弹恢复 Lyra 事件链：当前武器选择正式男女/枪种 Montage，`AN_Reload` 发送 `GameplayEvent.ReloadDone`，服务器在事件时点修改库存弹药，客户端不伪造 Reload 结果。
- 六个正式 CC Reload Montage 各保存一个 `/Game/Characters/Heroes/Abilities/AN_Reload`：Pistol `1.631531`、Rifle `1.746580`、Shotgun `2.243124`，男女一致，Dedicated Server 可触发。

### 回撤或删除

- 删除 `UShootNameplateWidgetBase` 及 Character 内扫描 WidgetComponent 的刷新桥。`W_Nameplate` 恢复为 `UserWidget`，`BP_ShootCharacter.NameplateWidget` 保留。
- 删除独立的姓名牌 MVVM 文档；姓名、Health、MaxHealth、HealthPercent 继续复用 `UAttributeViewModel`，不维护第二套类。
- 删除临时 `ShootAcceptanceInputCommands.cpp` 和配套文档。玩家验收必须走真实 Enhanced Input；若以后需要自动化，放入明确的测试/编辑器工具模块并随任务维护。
- 删除旧 `UShootAnimNotify_ReloadDone` C++ 类。正式资产统一使用项目已有 `/Game/Characters/Heroes/Abilities/AN_Reload`。

## 二进制变更概览

起点到当前终点的 Content 差异为：

- 删除 592 个资产：其中 590 个位于旧 `/Game/Assets/Characters/CC`，2 个位于 `/Game/Developers/Codex/AnimPoseProbe`。
- 新增 31 个资产：正式 CC 男女武器动作和 Montage，位于 `/Game/Characters/Heroes/CC/MM|MF`。
- 修改 13 个资产：正式角色/展示入口、三把武器的 Instance/ItemDefinition/Pickup，以及工作区中的 Manny 和 TestMap。

以下工作区内容不属于本纠错提交：

- `Content/Characters/Heroes/Mannequin/Meshes/SKM_Manny.uasset`
- `Content/Maps/TestMap.umap`
- `.reasonix/`
- `Build/`
- `reasonix.toml`
- 工作区根目录的 `VibeUE-master.zip`

三把 `BP_WeaponPickup_*` 的可视化网格来自用户提交 `db72be5`，视为远端基线，不在本轮回撤。

## 已验证

- `NewWorldOrderEditor Win64 Development` 冷编译成功，UBT 结果为 `Succeeded`。
- `W_Nameplate` 父类为 `UserWidget`，状态 `BS_UP_TO_DATE`。
- `BP_ShootCharacter` 父类为 `ShootCharacter`，存在 `NameplateWidget`，并通过父类属性授予现有 `GA_Hero_Jump`。
- `/Game/Characters/Heroes/Abilities/AN_Reload` 父类为 `AnimNotify`，状态 `BS_UP_TO_DATE`。
- 六个正式 Reload Montage 重启编辑器后均能读取到唯一 `AN_Reload`。
- 项目日志和文本扫描均无 `ShootNameplateWidgetBase`、`ShootAnimNotify_ReloadDone` 或 `ShootAcceptanceInputCommands` 残留引用。

## 当时尚未证明

- HomeMap 到 TestMap 的分屏男女主互斥最终回归。
- 友军伤害在分屏和 Listen Server 下的真实伤害结果。
- Pistol、Shotgun 的射击、换弹、切枪和弹药 UI。
- 方向快速反转、蹲伏、跳跃、FootPlant/LegIK、HandIK 和左手握持的玩家验收。
- Equip、Unequip、丢枪空手、敌人死亡/复活、单人/分屏/Listen Server 完整闭环。

这些项目不得因为代码、资产或编译存在就标记完成。

以上清单已由 2026-08-07 至 2026-08-13 的真实 Enhanced Input、分屏和 Listen Server 验收逐项覆盖。当前仍需继续的是 Mannequin 依赖分类与项目化收尾，而不是重复执行本节的历史待办。
