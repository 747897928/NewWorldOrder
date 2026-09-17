# Lyra ShooterCore 适配状态

## 当前检查点

- 基线分支：`main`
- 本轮全量重定向基线提交前的代码检查点：`0428406`，已推送 `origin/main`；本轮新的基线提交哈希以 Git 历史和交接汇报为准，避免文档为记录自身哈希而产生循环提交。
- 状态日期：2026-09-10
- 总目标：完成 NewWorldOrder 对 Lyra ShooterCore 的项目化适配，并形成玩家可以实际操作验收的单机、本地分屏与 Listen Server 闭环。任何新实现或疑难修复都必须先通过 11000 端口读取 Lyra 的 C++、蓝图、动画或配置事实，再决定项目层的最小改动；不得仅凭记忆重写 Lyra 逻辑。
- 准星稳定宿主、散布显示、Ammo UI 复制顺序、地图重命名、QuickBar 满槽替换当前 RuntimeOnly 武器、补给站网络结算和 Lyra Niagara 伤害数字已有运行证据，不再从头调查。
- 当前优先级：准星、男女 GameplayTag 属性映射、Hair/Shoe 最终层顺序、补给站和 Fire/Reload 武器脱手已经用户验收，从活动代办移除。男女三枪 Equip 与 Generic Unequip 共用的 8 条 Sequence 已完成 `weapon_r` 数据修复，等待一次真实切枪/丢最后一把枪的视觉复验；女性 Pistol 静止 Reload 的右臂橡皮手经过 CC 组合后处理后目前暂时看上去已解决，但只属于阶段性结论，不关闭扩展回归门槛。动画范围收口后转入 Lyra GE 伤害与距离衰减适配。

## 2026-09-10 CC PostProcess 与橡皮手阶段性回归

- 本轮的直接结论不是“后处理有魔法”，而是 CC 骨架需要在重定向主链之后补回自身的 twist/corrective 分配。Pose Driver 根据输入姿势读取 Pose Asset，CC Control Rig 再把修正写入 `forearmtwist`、`upperarmtwist`、`thightwist`、`calftwist` 等真实替代骨；组合 ABP 同时保留原有皱纹 AnimBP。
- `MF_Pistol_Idle_ADS_AO_CD`、`MM_Pistol_Idle_ADS_AO_CD` 与默认姿势已做组合后处理启用/禁用 A/B，并检查正面、侧面和背面。代表性姿势中的手腕扭曲及肘、膝整条肢体级别错误目前没有再复现。
- 因此任务中的橡皮手问题标记为：`目前暂时看上去已解决（阶段性），待扩展回归`。这比写成“已彻底解决”更准确，因为三把武器全部动作、移动/跳跃/蹲伏叠加、真实 PIE、不同 LOD 和所有网络模式仍需补齐。
- `weapon_r` 数据修复和 PostProcess 变形修复是两个层次：前者处理武器脱手/挂点轨道，后者处理 CC 身体蒙皮的扭转与 corrective 分布，不能互相替代。
- 详细原理、资产、错误共享骨映射和验证证据见 [CC PostProcess 适配记录](../AnimationMigration/CC_PostProcess_适配记录.md)；四个源资产到 CC 的完整差异和以后复现时的排坑步骤见 [CC PostProcess 适配差异与排坑](../AnimationMigration/CC_PostProcess_适配差异与排坑.md)；调查历史与后续回归门槛见 [橡皮手与武器动画叠加链调查](Investigation_橡皮手与武器动画叠加链.md)。

## 2026-08-20 Action `weapon_r` 修复与 RTG 边界

- 用户提交 `2459cfe` 后，重新重定向的角色 Montage 已恢复 Lyra 的声音与 Notify，但正式 CC Action Sequence 的动态 `weapon_r` 再次出现大幅局部位移/旋转误差。男女三枪 Fire/Reload 共 16 条目标 Sequence 已只写回 Manny 同名源的 `weapon_r`；保存复读均为最大位移误差 `0`、最大缩放误差 `0`、旋转四元数点积约 `1`。用户分屏 PIE 已确认三枪 Fire/Reload 的武器脱手消失。
- 男女 `MM_Pistol_Equip`、`MM_Pistol_Equip_Additive`、`MM_Rifle_Equip`、`MM_Rifle_Equip_Additive` 共 8 条目标 Sequence 已用同一安全流程修复。Pistol 两条同时被 `AM_Generic_Unequip` 使用，Rifle 两条同时被 Shotgun Equip 使用，因此覆盖男女三枪 Equip 与最后一枪丢弃链；运行视觉终验待用户下次打开 PIE 完成。
- `AM_Generic_Unequip` 不是无用资产：三个 `ID_*` 都引用它，`UShootGA_DropWeapon -> QuickBar -> EquipmentManager -> UShootWeaponInstance::OnUnequipped` 会在最后一把武器丢弃时播放。它复用 Pistol Equip Sequence 的 `0.1-0.6s` 片段，本轮无需另造 Unequip Sequence。
- 新建的两个 Manny -> CC RTG 仍未包含可精确保留动态 `weapon_r/weapon_l` Local Transform 的内建步骤。隔离 `Pin Bones` 实验未达到误差门槛并已撤销；当前不把失败配置写进用户 RTG。后续 RTG 工具任务应实现“隔离重定向 + 指定辅助骨 Local Track 写回 + 验证”，不能启用 UE 5.8 会 Force Replace/Delete 的 `overwrite_existing_files=True`。
- 当前动画已知遗留为女性 Pistol 静止 Reload 右臂橡皮手，以及移动/跳跃/蹲伏叠加时可能出现的 CC forearm twist/TwoBoneIK 适配问题。用户决定本阶段不再投入无边界调查；只有重新成为阻断项时才按 `Investigation_橡皮手与武器动画叠加链.md` 的精确断点继续。

## 2026-08-17 用户验收与全量重定向前基线

- 用户已完成补给站快速验收：临时 `InputActionWidget`、`InteractionProgressWidget` 和临时 EventGraph 节点均已清除；提示 LocalPlayer 归属、`HUD.Slot.Interaction` 进度条、松手重置、补弹和 `InteractionCollision` 范围均正常。本项从活动代办移除。
- 用户确认准星、MF/MM GameplayTag Property Map 修复和男女 Hair/Shoe 末端动画层已经验收通过。这三项不再重新调查；每次 PIE 前编译男女 Base AnimBP 只是回归门禁，不代表任务仍在进行。
- 丢弃最后一把武器进入空手时会实际播放 Generic Unequip 链。用户从两个分屏视角观察到男性在 Montage 持续期间整个上半身消失，Montage 结束后恢复，因此 `AM_Generic_Unequip` 不是可直接判定无用的零引用资产，而是“正式运行入口存在、当前动画或曲线消费损坏”的阻断项。
- 用户另确认男性 `AM_Pistol_Equip` 在资产编辑器预览和运行时均损坏。两个 Montage 不再继续叠加补丁：先读取引用、Skeleton、Segment、Slot、Section、曲线和 Notify，再基于重新重定向后的健康 AnimSequence 在原路径重建。
- 姿势库底层播放能力不得按性别限制兼容骨架。将来 Male/Female/通用只属于玩家目录的内容准入策略；开发/调试播放入口必须能绕过内容策略。当前 `CompatibleGender` 和服务器拒绝属于尚未最终定型的目录策略，不得误写成 AnimBP 或骨架能力限制。
- 用户将从 `/Game/Characters/Heroes/Mannequin/Animations` 按原目录批量重新重定向到 MM/MF 正式目录。重定向完成前不继续修改正式动画；完成后先从 Git 生成实际资产清单，再检查 Skeleton、Additive Base Pose、Root Motion、曲线、Notify、`weapon_r` 与 IK Retarget 设置，最后重建 Montage 和 AimOffset 容器。
- 详细调查边界与接续步骤见 `Docs/Tasks/LyraShooterCoreAdaptation/Investigation_动画重定向重建前基线.md`。

## 2026-08-17 统一续作断点与全量待办

本节整合旧会话全部可用分页记录、用户后续截图纠错和当前仓库事实。上下文压缩后从本节继续；历史章节用于解释过程，不得覆盖这里的当前状态。

### 已完成且不得重复调查

1. 稳定准星 Host、三枪散布与 LocalPlayer/Listen Server 隔离，检查点 `e1c901e`。
2. 补给站三秒长按、松手取消、服务器权威补弹、`InteractionCollision` 同源范围、每 LocalPlayer 提示与 `HUD.Slot.Interaction` 居中偏下 `100x20` 进度条。旧的 `550cm` 隐藏距离、共享进度 WidgetComponent 和全屏拉伸结论均已纠正。
3. 数据驱动左手握把沿用 Lyra Item Anim Layer 的 `LeftHandPose_OverrideState -> HandIKRetargeting -> CopyBone -> TwoBoneIK`；Shotgun 使用同侧 CC 覆盖序列，不新增 C++ 偏移或性别分支。
4. Lyra Niagara 数字的逐 TargetData、服务器实际掉血、伤害来源本地 Controller、每 Controller 单组件生命周期和分屏逐视口隔离已有运行 A/B；不再把资产存在或数组消费当成可见性证据。
5. MF Rifle Hipfire/Unarmed 两个 AimOffset 的 30 个正式样本与 Preview Base，以及 MM 31 个 Additive Base Pose 已脱离 Manny 运行姿势；18 个正式 CC AnimBP 目标 Skeleton 正确。
6. 男女三枪 Fire/Reload 的角色到武器 Montage 同步已统一到原生 `UShootAnimNotify_PlayWeaponMontage`；Reload 继续由 `AN_Reload -> GameplayEvent.ReloadDone` 在装匣帧结算。
7. 男女主 AnimBP 的 Hair/Shoe 正式末端顺序为 `FootPlant/Control Rig -> ShoeAnimationLayer -> HairAnimationLayer -> Output`。表演 Pose 使用 `DefaultGroup.DefaultSlot` 的全身入口，不再错误塞进 `UpperBodyAdditive`。
8. MF/MM `ABP_Mannequin_Base` 的五条 GameplayTag 映射已用各自变量当前 GUID 重建。男女蓝图不互斥；每次 PIE 前仍须分别编译并检查，防止资产修改后旧 GUID 再次持久化为 `Property [None]`。
9. 男女六个 Equip Montage 已补齐 Lyra 精确曲线插值/切线和 `sfx_WeaponSwap_nl_meta_Preset` PlaySound Notify；第二条 Timing 标记来自声音 Notify。男性 Rifle/Pistol 四条底层 Equip Sequence 已去除重定向阶段额外手臂 IK 过弯。

### 当时进行中（已由 2026-08-20 断点取代）

1. 男性 Shotgun Fire：旧轨道 Additive 元数据与 `weapon_r` 已修过，但用户最新画面仍有开火时枪体上抬、短暂脱手。必须在当前正式资产上重新以男性/女性同帧、角色 Montage/武器 Montage、`weapon_r`/hand_r 和 Actor 附着 Transform 做 A/B；禁止用 Actor Rotation 锁死补丁。
2. Equip/Unequip 橡皮手与武器飞头：旧结论“Generic Unequip 已排除、只剩写实 CC 美术造型”已被用户的丢枪截图推翻。当前必须同时调查损坏 AnimSequence/Montage、`weapon_r` 原始轨道、`ScaleDownWeaponR` 消费、`DisableLHandIK`、角色/武器 Montage 同步和 Weapon Actor 相对 Transform；禁止继续用音效 Notify、Timing 数量或 C++ 锁 Transform 代替根因验证。

### 后续高优先级

1. 主菜单姿势库：M 键打开后部分 MMA Pose 出现角色整体放大或扭曲。复核 26 个条目的 Skeleton、Additive/Root Motion、播放 Slot 和全身覆盖方式；不能给所有动画一刀切添加禁用 IK/FK 曲线。
2. 鞋履与头发层运行回归：男女平底鞋、高跟鞋、发型物理、武器动作和表演 Pose 组合测试，确认 Shoe 在 FootPlant 之后仍能抬高脚部，Hair 最后消费最终身体姿势，并补齐蓝图注释。
3. 正式 CC 玩家可见动画复验：男女八方向、开火、换弹、快速切枪、最后一枪 Unequip、表演 Pose 的正面/侧面/背面 A/B；只把真正复现的异常重新打开。

### 后续系统任务

1. 武器伤害 GE 审计：先从 Lyra 11000 读取 `GE_Damage_Pistol` 及 Rifle/Shotgun、Fire GA、Physical Material/WeakSpot 调用链，再审计项目 `ShootInventoryFragment_RangedWeaponConfig`、伤害执行和遗留 Fragment。目标是每枪以配置的 GE 为权威，不保留硬编码伤害；`PhysMat_Player` 与 `PhysMat_Player_WeakSpot` 的爆头/弱点标签继续进入 GAS。未来 RPG 属性加成只预留扩展点，本阶段不提前实现。
2. RuntimeOnly 武器、QuickBar、Equipment、SaveGame 与返回 Hub 的完整生命周期回归，防止战斗临时武器进入 Persistent 仓库。
3. 玩家手感终验：三个准星视觉尺寸、鼠标/手柄后坐力、枪口与手部贴合、Listen Server 客户端真实按键拾取/丢枪，以及 `TestMap_ListenServer` 返回入口 ServerTravel。

### 手工清理清单

- `/Game/Weapons` 下 8 个零引用 Manny 角色 Montage。
- MF Poses 下零引用的 `SplashPose_Quinn_1/10/11`。
- 上述均涉及多个资产，按项目删除红线只记录精确清单，由用户在 Content Browser 手工处理；本任务不批量删除。

## 2026-08-14 冷启动 GameplayTag 依赖收尾

- 冷启动加载 MF `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base` 后，`Event.Movement.ADS/WeaponFire/Reload/Dash/Melee` 五条 `[None]` 错误保持为零；当前日志也没有 `LogAnimBlueprint/LogAnimation/LogBlueprint Error`。
- 正式 CC 移动动画的 `UAnimNotify_LyraContextEffects` 原引用 `AnimEffect.Footstep.Walk/Land`，但项目漏迁 Lyra `DefaultGameplayTags.ini` 中的两张标签表。现已迁入 `/Game/ContextEffects/DT_AnimEffectTags` 与 `DT_SurfaceTypes` 并注册为 GameplayTagTable，四个脚步 Tag 与四个 SurfaceType Tag 均从对应数据表加载；移动动画复读不再刷 Invalid GameplayTag。
- 同轮发现并清理两个历史数据残留：`DA_ShootInputConfig` 的四个技能 IA 从废弃 `InputTag.1/2/3/4` 改为权威 `InputTag.Q/E/C/X`；`GA_ListenForEvent.EventTags` 和 `GE_EventBasedEffect` 的旧 `Intelligence/Resilience/Vigor` 引用改为现行 `Vitality/Agility/Perception`，GE Modifier 同时绑定到真实 `UShootAttributeSet` 属性。
- 编辑器重启后复读三项资产均持久化，当前启动日志 `Invalid GameplayTag=0`。下一断点转入剩余玩家可见动画问题，不再重复调查上述 Tag 映射和数据表。

## 2026-08-14 ContextEffects 运行资源闭环

- 两张 GameplayTag 表只能让动画 Notify 正确解析 Tag，不能自行产生声音。对照 Lyra 11000 后确认 `/Game/Characters/Heroes/B_Hero_Default` 的蓝图组件 `LyraContextEffect` 配置了 `/Game/ContextEffects/CFX_DefaultSkin`，而项目 `/Game/Blueprints/Character/BP_ShootCharacter` 继承自 `AShootCharacter`，使用父类创建的 `ContextEffectComponent`，其 `DefaultContextEffectsLibraries` 原为空。
- 已迁入 `CFX_DefaultSkin` 及其脚步、落地 MetaSound、SoundWave、衰减和并发资源依赖；在 `BP_ShootCharacter` 的继承组件 `ContextEffectComponent` 上配置该 Library。C++ 仍只负责 Notify、组件、Subsystem 的运行时调用链，具体声音资产继续由蓝图和 Library 数据维护。
- 已在 `Config/DefaultGame.ini` 的 `/Script/NewWorldOrder.LyraContextEffectsSettings` 配置 Lyra 同构的物理表面映射：Default、Character、Concrete、Glass 转换为相应 `SurfaceType.*` Context Tag。项目已有 `DefaultEngine.ini` 的 SurfaceType 1/2/3 名称，无需新增 C++ 分支。
- 运行时直接调用 Library 的 `LoadEffects/GetEffects` 复读通过：Walk 与 Land 在 Default、Concrete、Glass 六种组合下均返回一个对应 MetaSound，且没有误返回 Niagara。项目 ContextEffects 的 Component、Subsystem、Library 与 AnimNotify 逐文件对照 Lyra 后，差异是项目命名、空指针防护和资源生命周期收敛，不再发现缺失调用阶段。
- HomeMap 单玩家 PIE 又直接从真实 `BP_ShootCharacter` 的继承组件调用一次 `AnimMotionEffect`：调用前世界内 AudioComponent 为 0，调用后角色生成并播放一个 `sfx_Character_FS_Concrete_nl_meta`。这证明 `ContextEffectComponent -> LyraContextEffectsSubsystem -> CFX_DefaultSkin -> SpawnSoundAttached` 的项目运行链已经闭合；测试结束后 PIE 恢复为 1 Player。

## 2026-08-13 冷启动与 Mannequin 依赖分类收尾

- 关闭且只关闭 NewWorldOrder 编辑器后执行 `Scripts/Build_Windows.ps1`，`NewWorldOrderEditor Win64 Development` 在带 `-NoHotReloadFromIDE` 的冷编译中返回 `Result: Succeeded`。Lyra 11000 编辑器保持运行，未生成临时 Hot Reload DLL。
- 全新 NewWorldOrder 编辑器重新连接 8000 MCP 后位于 `/Game/Maps/HomeMap`；18 个正式 CC AnimBP/动画接口均为 `BS_UP_TO_DATE`，MM 九项继续使用 `ChenHaoYu_Skeleton`，MF 九项继续使用 `ShenWanYun_Skeleton`。
- 对男女正式 CC 目录重新统计：409 个资产存在直接 Mannequin 包依赖、共 1097 条边。其中 1065 条来自 AnimSequence 的 268 条曲线压缩设置，以及 797 条 AnimationModifier/Transition Notify 引用，不是运行姿势源；AnimBP 余下边为 Lyra 枚举/结构、`TransitionToLocomotion`、动画层接口/类型、`CR_Mannequin_FootPlant`、`RTG_Mannequin` 及编辑器 Skeleton 契约。
- 六个带直接 Mannequin 依赖的正式 AnimBP 已复读父类：男女主 Base 均继承项目原生 `ShootMannequinAnimInstance`，Item Base 与 Retarget 均继承引擎 `AnimInstance`；不存在继续继承源 Mannequin 蓝图父类的运行链。男女正式 `ALI_ItemAnimLayers` 各有 14 个 Lyra 动画层函数，已补齐移动、跳跃、骨骼控制和左手姿势职责，旧文档“只有 FullBody_Aiming”属于历史状态。
- 唯一仍直接绑定 Manny/Quinn Skeleton 的动画数据是 MF Poses 目录中的 `SplashPose_Quinn_1/10/11`。AssetRegistry 对三者的直接引用者均为零，它们不进入当前运行时；因涉及多个 `.uasset` 且删除红线禁止批量删除，本任务只记录精确清单，留给用户在 Content Browser 手工处理。
- 当前结论：`MANNY_REF_POSE=0`、`MANNY_AIM_REFS=0` 后，正式玩家姿势链已项目化闭环；不能为了清零包路径复制 Lyra 枚举、压缩设置、Notify、ControlRig 或 Retargeter，从而制造两套类型与工具资产。

## 2026-08-13 用户复验后的补给站修正

- 用户指出旧表现存在三个实际问题：隐藏的 `ServerValidationDistance=550` 允许在蓝图碰撞范围外按 F；共享 Screen WidgetComponent 会让分屏提示归属不清；`HUD.Slot.Interaction` 的 Fill 布局把小进度条拉伸到接近整屏。此前“视觉闭环完成”的说法不再作为终态结论，以本节为准。
- `AShootAmmoSupplyStation::CanSupplyPawn` 已删除独立距离变量，客户端扫描与服务器结算统一使用 `BP_AmmoSupplyStation` 继承组件 `InteractionCollision` 的实际 Overlap。当前蓝图半径为 `50cm`，设计师在蓝图中调球体即可同时改变扫描和服务器有效范围。
- `BP_AmmoSupplyStation` 的 `InteractionPrompt` 是唯一视觉配置模板：`W_Icon_Interact`、Screen、`60x60`、相对位置 `(0,0,70)`、初始隐藏。父类 C++ 构造函数不再写 WidgetSpace、DrawSize 或 RelativeLocation；Visual Mesh、比例和碰撞半径均由蓝图维护。
- 一个共享 Screen WidgetComponent 无法同时绑定两个 LocalPlayer。C++ 仅承担该网络/分屏生命周期：按本地 Pawn 的 Begin/End Overlap 为对应 LocalPlayer 创建非复制提示实例，复制蓝图模板的 Widget、尺寸和 Transform，并调用 `SetOwnerPlayer(LocalPlayer)`；模拟代理不会创建提示。
- 分屏运行复验：P0 进入球体时只有 OwnerPlayer=P0 的 `WidgetComponent_0` 可见；P1 随后进入并让 P0 离开后，P0 实例隐藏，OwnerPlayer=P1 的 `WidgetComponent_1` 独立显示。模板始终隐藏，两个运行实例均为 Screen、`60x60`、`(0,0,70)`。
- `W_DefaultHUD` 的 `InteractionExtensionPoint` 已在 Widget Blueprint 中改为 `(0.5,0.82)` 居中偏下锚点、中心对齐、AutoSize；`Interactive_Progress_Bar` 在 Widget Blueprint 中固定为 `100x20`，文案为“弹药补充中”，默认 Percent 为 `0`。C++只更新所属 Controller 的可见性和百分比，不设置尺寸、颜色或屏幕位置。
- Station、Controller、HUD 与 Progress WBP 均重新编译为 `BS_UP_TO_DATE`；C++纯函数体修正通过 Live Coding。PIE 已停止并回到 HomeMap。

## 2026-08-12 补给站视觉闭环完成

- `BP_ShootPlayerController` 继承组件 `InteractionHUDComponent` 已把 `InteractionProgressWidgetClass` 显式配置为 `/Game/UI/Foundation/Widgets/Interactive_Progress_Bar.Interactive_Progress_Bar_C`。原生组件不再默认注册一个缺少 UMG 控件树的 C++ 基类，`NativeConstruct` 和刷新函数也对两个可选控件做空指针保护。
- `Interactive_Progress_Bar` 已保留语义明确的蓝图调用链：Widget Blueprint 的 `Event Tick` 调用 `Refresh Interaction Progress`。UE 会裁剪未连接的 Widget Tick；本实现对齐 Lyra `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost` 的“蓝图 Tick 调用语义函数”模式，而不是依赖无法保证执行的原生 `NativeTick`。
- `BP_AmmoSupplyStation` 使用父类已有的 `InteractionPrompt`：`W_Icon_Interact`、Screen、`60x60`、相对位置 `(0,80,190)`、初始隐藏。临时 `InputActionWidget`、`InteractionProgressWidget` SCS 组件以及七个临时 EventGraph 节点均已删除，用户选择的 AmmoBox Visual Mesh 保持不变。
- Lyra 11000 对照确认：官方交互能力声明了 `Ability.Interaction.Duration.Message`，但 `/ShooterExplorer/Input/Abilities/GA_Interact` 仍明确留有 `TODO Duration?`。项目层用 `UShootGA_Interact` 补齐按 Pawn 广播的完整开始、取消、完成生命周期，再由每个 PlayerController 的 `UShootHUDInteractionComponent` 按 `Message.Instigator == GetPawn()` 过滤，并通过各 LocalPlayer 的 `HUD.Slot.Interaction` 注册私有 Widget。
- 本地分屏视觉实测：P0 持续交互时实际 `InteractionProgressBar` 为可见且进度约 `0.307`，P1 保持 `Collapsed/0`；P0 提前释放后立即恢复 `Collapsed/0`，P1 全程不变。
- Listen Server 视觉实测：远端客户端进度约 `0.367` 时，Host 保持 `Collapsed/0`；远端提前释放后客户端立即隐藏并清零，Host 全程不串线。当前地图该轮没有活动武器实例，因此不重复伪造装备；此前真实功能验收已经确认完成后服务器与客户端均为 `12/48`。
- C++ 防崩与刷新入口已通过 `NewWorldOrderEditor Win64 Development` 冷编译和最终 Live Coding；Station 已恢复正式 `HoldDuration=3.0`，PIE 已结束并恢复 `Standalone + 1 Player`。
- 精确下一断点：读取 Lyra 11000 的武器动画、`weapon_r`、`VB IK_Hand_L_weaponSpace`、TwoBoneIK 和武器数据来源，再审查项目 `ItemDefinition -> WeaponInstance -> AnimBP` 调用链，设计逐武器可配置且不按性别硬编码的左手握把数据。

## 2026-08-12 数据驱动左手握把审计完成

- 11000 端 Lyra 的正式机制不是通用 `LeftHandGripOffset`。`ABP_ItemAnimLayersBase.LeftHandPose_OverrideState` 用可配置 AnimSequence 做 Layered Blend，再进入 `HandIKRetargeting -> CopyBone(VB IK_Hand_L_weaponSpace 到 ik_hand_l) -> TwoBoneIK(hand_l)`。
- Lyra Rifle 与 Pistol 的 `EnableLeftHandPoseOverride=false`；Shotgun 为 `true`，`LeftHandPose_Override=/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Shotgun/MM_Shotgun_Idle_Hipfire`。这证明 Shotgun 的特殊护木握姿由武器动画层数据承担，不由 WeaponInstance C++ 猜偏移。
- 项目男女正式 CC 已完整保留该机制：MM/MF Base 的 `FullBody_SkeletalControls` 均为 `14` 节点、`12` 连接，`LeftHandPose_OverrideState` 均为 `6` 节点、`5` 连接，`SetLeftHandPoseOverrideWeight` 均为 `8` 节点、`7` 连接，编译状态全部 `UpToDate`。
- 项目 Rifle/Pistol 与 Lyra 一样关闭额外覆盖；男女 Shotgun 均启用覆盖，并分别引用同侧正式 `MM_Shotgun_Idle_Hipfire`、`MF_Shotgun_Idle_Hipfire`。六个子层继续使用同侧 CC 父类链，没有跨性别资产。
- 既有真实输入验收已经覆盖男女约 `300cm/s` 移动 Shotgun Fire，左手保持护木跟随；当前资产配置与该证据一致。因此本项不新增第二套 C++ Transform、Socket 或按性别分支，避免与 `weapon_r` 和动画层形成双重姿势源。
- 精确下一断点：先读取 Lyra 11000 的 Niagara 伤害数字资产、生成入口、聚合规则、LocalPlayer/网络归属和生命周期，再在项目层确认可复用消息与伤害执行链，实施最小项目化适配。

## 2026-08-12 Lyra Niagara 伤害数字闭环完成

- 11000 端对照确认：Lyra 的 `GCNL_Character_DamageTaken` 只把命中反馈送到伤害来源 Pawn 的本地 Controller；`ULyraNumberPopComponent_NiagaraText` 为每个本地 Controller 懒创建一个长期 NiagaraComponent，把 `XYZ=命中位置、W=伤害值` 追加到 `DamageInfo`，负 W 表示暴击。`GA_Weapon_Fire` 对每个 TargetData 分别应用伤害，不做霰弹枪按目标聚合。
- 项目没有复制 Lyra 的 GameplayCue Notify 链，而是在服务器权威 `UShootAttributeSet` 完成最终 Health 扣减后，以实际掉血量构造 `FShootNumberPopRequest`，通过伤害来源 `AShootPlayerController` 的 Unreliable Client RPC 送到其本地 `UShootNumberPopComponent_NiagaraText`。友伤、免疫或未实际掉血时不生成数字；`UPhysicalMaterialWithTags` 的 `Gameplay.Zone.WeakSpot` 继续编码为负 W 强调样式。
- `/Game/Effects/Particles/Impacts/NS_DamageNumbers` 及其材质、纹理和 Niagara Module 通过 Unreal AssetTools 从 Lyra 迁移；`DA_DamagePopStyle_Niagara` 配置 `DamageInfo` 与该系统，`BP_ShootPlayerController` 的继承组件 `DamageNumberComponent` 引用此 Style。组件只在第一次有效请求时创建，换 Pawn 仍归属持久 Controller，不为每发创建 Actor 或 UObject。
- 本地分屏额外补齐 Lyra 默认未覆盖的多 LocalPlayer 边界：Niagara Primitive 使用 `OnlyOwnerSee`，并通过 `UPrimitiveComponentUtilities` 在每次请求时把当前 Controller 的 ViewTarget/Pawn 刷新为 VisibilityOwner。原因是第三人称 SceneView 的 ViewActor 是 Pawn，而不是 Controller；仅设置 `OnlyOwnerSee` 会把所属玩家自己也隐藏。
- 本地分屏真实 `IA_Attack`：P0 Rifle `30/30 -> 29/29 -> 28/28`，敌人 `276 -> 266`；P0 只有一个活动 NiagaraComponent，P1 为零。画面已捕获实际伤害数字；后续请求复用同一组件，组件数组在 Niagara Tick 后被系统消费。
- Listen Server 远端客户端真实 `IA_Attack`：客户端与服务器远端 Rifle 同步 `30/30 -> 29/29`，目标 Health 两端同步 `276 -> 266`；只有客户端本地 Controller 创建一个 NiagaraComponent，服务器远端 Controller 与 Host 本地 Controller 均为零。
- C++ 反射改动已通过无编辑器占用的 `NewWorldOrderEditor Win64 Development` 冷编译；最后的分屏 VisibilityOwner 补丁通过 Live Coding。PIE 已结束，Play 设置恢复 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`，编辑器回到 HomeMap。
- 精确下一断点：先读取 Lyra 11000 的 Equip/Unequip C++、Equipment 蓝图、Item Anim Layer、Montage 与源动画，再同时审查项目正式 CC 重定向资产；优先判断用户所见橡皮手来自调用顺序、Montage 叠加还是离线重定向，而不是再次凭旧截图结案。

## 2026-08-14 玩家复验后的纠错

- 女性 `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base` 的 `GameplayTagBlueprintPropertyMap` 曾保留五个 Tag，却把目标属性和 GUID 丢成 `None/0`。现已按同构 MM 资产恢复 `ADS -> GameplayTag_IsADS`、`WeaponFire -> GameplayTag_IsFiring`、`Reload -> GameplayTag_IsReloading`、`Dash -> GameplayTag_IsDashing`、`Melee -> GameplayTag_IsMelee`，保存后重新编译为 `BS_UP_TO_DATE`；最新日志不再产生五条 `[None]` 错误。
- 男性 `MM_Shotgun_Fire` 的 Additive 元数据此前虽已恢复，但它自己的 `weapon_r` 轨道仍与 Lyra/女性目标不同：0 秒 Z 已偏到约 `24.5cm`，0.1 至 0.2 秒峰值约 `33.5cm`。现已逐帧从 Lyra 同名源迁回全部 21 帧 `weapon_r`；`0/0.1/0.2/0.3/0.4/0.5/0.6333s` 复读差值均为零，角色 Montage Blend 也恢复为 Lyra 的 `0/0.3s`。
- Equip/Unequip 不再使用 `EquipMontageStartPosition=0.6s` 掩盖开头。项目此前漏迁了 Lyra Equip 的 `ScaleDownWeaponR` 与 `DisableLHandIK` 曲线，导致 CC 最终 IK/FK 过渡缺少必要权重。男女 Rifle/Shotgun Equip 已复制 Rifle 源曲线，Pistol Equip 已复制 Pistol 源曲线，Generic Unequip 按其 `0.1-0.6s` 源片段写入平移后的曲线；三把 WeaponInstance 起播位置均恢复为 `0.0s`。
- 伤害数字的旧 `VisibilityOwner`、Pawn Owner 与 `HiddenPrimitiveComponents` 方案在 UE 5.8 Niagara 状态流的实际 A/B 中均不可靠：前两者会把所属玩家自己的数字隐藏，后者一次注入 20 条仍完全不可见。资产、数组和材质链在关闭可见性过滤时可正常显示，因此缺口确定在分屏逐视口过滤，不在 Niagara 内容。
- 最终方案为每个本地 PlayerController 懒创建一个专属临时 `ANiagaraActor`，仍长期复用其 NiagaraComponent 和 `DamageInfo` 数组；所属 Controller 从 `HiddenActors` 移除该 Actor，其他本地 Controller 加入隐藏列表，组件 EndPlay 时清理列表并销毁临时 Actor。两个摄像机同时看同一世界位置的 A/B 中，P0 只显示蓝色 `600-607`，P1 只显示橙色暴击 `700-707`，无串屏；反射改动已通过 `NewWorldOrderEditor Win64 Development -NoHotReloadFromIDE` 冷编译。真实伤害链已在同一代码基线上确认敌人 `276 -> 186` 且弹药消耗，下一轮冷启动只需补真实命中的最终画面证据。

## 2026-08-12 Equip/Unequip 橡皮手终验完成

- 11000 端重新读取了 `ULyraEquipmentInstance`、`ULyraEquipmentManagerComponent`、`ULyraWeaponInstance` 与 `/ShooterCore/Weapons/B_WeaponInstance_Base`。Lyra 蓝图顺序为：Equip 计算 CosmeticTags，选择并链接 Equipped Item Anim Layer，再播放成对 Equip Montage；Unequip 先排除死亡角色，选择并链接 Unarmed Layer，再播放 Generic Unequip。项目 `UShootWeaponInstance` 在正式 CC Mesh 上保持同一核心顺序，Listen Server 的 Instigator 复制补播只补 Montage，不重复进入完整装备生命周期。
- 三份项目 Equipment 与 Lyra 三份 `WID_*` 逐字段一致：Actor 分别为 Pistol、Rifle、Shotgun，挂点均为 `weapon_r`，相对旋转均为 Z `-90`，位移为零。三把 WeaponInstance 仍按男性默认层、`Cosmetic.AnimationStyle.Feminine` 女性规则和同侧 Unarmed 层选择，不存在玩家编号或性别硬编码分支。
- 男女 Pistol/Rifle/Shotgun Equip 与 Generic Unequip 的 Skeleton、双 Slot、片段区间、RateScale 和 Blend 参数已复读。Equip 基础序列为非 Additive，配对序列为 Local Space Additive；Generic Unequip 保持 Lyra 的 Pistol Equip `0.1-0.6s`、`RateScale=0.9`、`0.20/0.30 Cubic`。项目男女 Equip 中 `weapon_r` 在 `0.0/0.3/0.6/1.0s` 的局部姿势与 11000 源数据逐项一致。
- 分屏真实输入加慢速逐帧冻结确认：源 Rifle Equip 的 `0.3s` 本身就是双肘外翻的过渡姿势，项目不是因骨长、挂点或丢轨额外制造该姿势；男女 Rifle、Shotgun、Pistol 从配置的 `0.6001s` 起进入自然持枪。女性 Generic Unequip `0.25s` 手臂比例正常，另一名 LocalPlayer 的 Montage 和装备状态不受影响。
- Listen Server 远端客户端真实拾取 Rifle 后，服务器模拟代理与客户端本地 Pawn 同时播放 MF `AM_Rifle_Equip`，均可稳定冻结在 `0.6001s`；Host 无活动 Montage。客户端真实丢弃后，两端同时进入 MF `AM_Generic_Unequip` 的 `0.25s`，Host 仍无误播，服务器与客户端均生成同一把掉落 Rifle。
- 当前资产复读纠正了旧文档的“男女六个 Equip Montage 全部零 Notify”：MM `AM_Shotgun_Equip` 目前在 `0.0001s` 有一枚 `AN_ShootPlayWeaponMontage`，其余本轮读取的正式 Equip Montage 为零。该资产属于用户动画改动保护范围，本阶段未修改；它单独纳入下一项 Shotgun 开火旋转调查，不能继续用旧零 Notify 结论掩盖。
- 历史结论已由 2026-08-14 纠错取代：该轮只在 `0.6001s` 之后观察自然持枪，漏掉了 Montage 曲线迁移缺口，不能再作为“无需修复”的证据。当前正式方案见上方 2026-08-14 章节。
- 精确下一断点：读取 Lyra 11000 的 Shotgun Fire 角色 Montage、武器 Montage、`B_Shotgun` EventGraph/AnimBP 和武器 Actor 相对旋转；再对照项目 MM/MF `AM_Shotgun_Fire`、`AM_Weap_Shotgun_Fire`、`AN_ShootPlayWeaponMontage` 与 `B_Shotgun`，复现并隔离开火时武器旋转。

## 2026-08-12 Shotgun 开火旋转审计完成

- Lyra 11000 的正式调用链为 `GCN_Weapon_Shotgun_Fire -> B_Weapon.Fire` 生成枪口、弹壳、Tracer、Impact 和 Decal；`B_Weapon.Fire` 没有 Actor Rotation 或 Montage 节点。角色 `AM_MM_Shotgun_Fire` 在 `0.0001s` 通过 `AN_PlayWeaponMontage` 播放 `AM_Weap_Shotgun_Fire`，再以角色 Montage 为 Leader 执行 `MontageSync_Follow`。
- 项目男女真实 `IA_Attack` 均保持 B_Shotgun 根组件相对旋转为 `Yaw=-90`，附着 Socket 始终为 `weapon_r`。男性 Fire `0.000 -> 0.07645s` 的 Actor 四元数只发生约 `7` 度的连续后坐变化；女性同区间几乎不变。此前从 Rotator 读到约 `30` 度的 Yaw 数值变化来自欧拉角表示，不能据此判断 Actor 被代码旋转。
- Lyra 源 `weapon_r` 在同区间约变化 `4-5` 度；项目没有 socket 突跳、相对 Transform 改写或第二把武器 Actor。禁止通过锁死 Actor Rotation、删除 `weapon_r` 轨道或改回 `weapon_socket_hand_r` 消除官方后坐动作。
- 当时独立发现的适配缺口：男女 Rifle/Pistol/Shotgun 六个 Fire Montage 使用空配置 BP Notify，Reload 也没有武器同步入口。该缺口已于 2026-08-13 统一改为原生 `UShootAnimNotify_PlayWeaponMontage` 并完成网络回归，详见当前检查点与验收文档。
- MM `AM_Shotgun_Equip` 上额外的空 Notify 仍属于用户动画资产保护范围。本轮未修改任何正式动画或 `AN_*` 资产；武器 Montage 同步须在用户允许修改这些资产后，按 Lyra 逐 Montage 配置并重新跑男女、分屏和 Listen Server 回归。

## 2026-08-12 三环境终态冒烟回归完成

- HomeMap 干净启动为单 World、单 LocalPlayer、空 RuntimeOnly QuickBar 和无副本 Equipment，证明本轮副本状态没有带回 Persistent 环境。
- `TestMap_SplitScreen` 的男女主互斥、分离出生点、两人三枪 RuntimeOnly 拾取、射击、整弹匣换弹、切枪、Crouch、逐把丢弃到空手均通过。两人每枪弹药变化独立，最终均为三个空槽、`ActiveIndex=-1`、Weapon Equipment 数为 0。
- `TestMap_ListenServer` 的 Host Rifle 与远端客户端 Pistol 首次拾取、真实射击、真实换弹和真实丢弃均在服务器/客户端同步。客户端丢弃后出现短暂 Slot 快照延迟，但最终三个槽位和 Equipment 均正确清空，不是稳定残留。
- 服务器远端 Controller 不创建 QuickBar、ReticleHost 或 InteractionProgress 玩家私有 Widget；只有 Host 本地和客户端本地 Controller 拥有各自 UI。阶段提交没有破坏 LocalPlayer/网络边界。
- 本轮新的持续 Move API 注入没有产生速度，因此不计为移动验收；移动、Jump、连续反向、友伤、敌人重生、补给站完成结算与 Niagara 数字沿用同一代码基线上已经归档的真实输入证据。详细数值见 `Verification_验收.md`。
- PIE 已停止，编辑器回到 HomeMap，Play 设置恢复 `PIE_Standalone + 1 Player + RunUnderOneProcess`。

## 2026-08-11 精确续作断点

- 已完整分页读取前一任务 `019fdcd6-ff86-7b83-bf49-5c1eb25f30f6` 的全部可用聊天记录，并结合任务包与仓库状态恢复上下文。后续上下文压缩后从本节继续，不从准星、Lean、脚部控制或补给站网络逻辑重新调查。
- 阶段快照 `783fe4a` 已把当时项目内全部产品改动提交并推送。未跟踪的 `.codex_mcp_request.json`、`.reasonix/`、`Build/`、`reasonix.toml` 和项目父目录 `VibeUE-master.zip` 是工具产物，未纳入提交，也未因删除红线擅自清理。
- 补给站功能与网络逻辑已经通过：`FInteractionOption::HoldDuration`、`UShootGA_Interact` 长按/松手取消/Duration Message、服务器权威补弹、每 Controller 进度字段与 `Message.Instigator == GetPawn()` 过滤均已接通。
- 分屏功能验收已通过：P0/P1 初始为 `1/12`；提前松开不补弹且进度重置；P0 完成长按后为 `12/48`，P1 不变。Listen Server 已确认客户端完成后服务器与客户端均为 `12/48`，Host 进度不串线。
- 当前缺口只在可见资产接线：`Interactive_Progress_Bar` 必须注册到每个 LocalPlayer 的 `HUD.Slot.Interaction`；世界中的 `InteractionPrompt` 只负责本地 Pawn 靠近提示。禁止把进度条留在所有玩家共享的世界 `WidgetComponent`。
- `Interactive_Progress_Bar` 已改为继承 `UShootInteractionProgressWidget`，控件树含 `InteractionProgressBar` 和 `MessageCommonText`。此前 Controller 组件默认注册原生基类，导致 `BindWidget` 为空并在 `NativeConstruct` 崩溃；当前 C++ 已移除原生默认类并对两个控件增加空指针保护，后续冷编译与 Live Coding 均已通过。
- 下一资产操作：在 `/Game/Blueprints/Player/BP_ShootPlayerController` 的继承组件 `InteractionHUDComponent` 上把 `InteractionProgressWidgetClass` 配置为 `/Game/UI/Foundation/Widgets/Interactive_Progress_Bar.Interactive_Progress_Bar_C`；把 `/Game/Blueprints/Interaction/BP_AmmoSupplyStation` 的父类组件 `InteractionPrompt` 配置为 `W_Icon_Interact`、Screen、`60x60`、相对位置 `(0,80,190)`、初始隐藏，并由父类按本地 Pawn overlap 控制显隐。
- 配置复读成功后，删除补给站临时 SCS 组件 `InputActionWidget`、`InteractionProgressWidget` 和临时 BeginPlay/Tick/Overlap/SetVisibility 图表节点；只删除蓝图节点与组件，不删除资产文件，不修改用户选择的 Visual Mesh。
- 当前实现成功标准：提示只在对应玩家靠近时显示；正确 LocalPlayer 的 HUD 显示真实进度条；三秒内百分比增长；提前松开立即消失并重置；完成后补满当前弹匣与备弹；另一本地玩家或 Host 不串线；客户端与服务器弹药复制一致；结束后恢复 Standalone、1 Player 的 PIE 设置。

## 全部剩余任务

1. 已完成并推送：补给站玩家复验修正。范围已统一到蓝图球体、每 LocalPlayer 提示和 HUD 居中偏下 `100x20` 读条。
2. 已完成：对照 Lyra 11000 确认并复读逐武器动画层的左手姿势覆盖、`weapon_r` 与 TwoBoneIK 链。项目已使用同侧 CC AnimBP/AnimSequence 数据，不新增性别硬编码偏移。
3. 已完成：对齐 Lyra Niagara 伤害数字的生成、逐 TargetData 规则、来源玩家网络归属和单组件生命周期，并完成本地分屏与 Listen Server 验收。
4. 已复验：Equip/Unequip 的基础与 Additive 元数据、双轨 Montage、`weapon_r/-90` 和 `0.6s` 起播路径均与 Lyra 一致；本轮冻结 Equip/Unequip 中间帧未复现骨长拉伸。
5. 已修复：男性 `MM_Shotgun_Fire` 丢失 Mesh Space Additive 与自身第 28 帧 Base Pose，导致绝对姿势叠加到 `FullBodyAdditivePreAim`。恢复后真实 `IA_Attack` 画面不再飞枪。
6. 已完成：最终三环境冒烟回归。HomeMap 单人生命周期、分屏两人三枪核心链和 Listen Server 主机/客户端权威链均未被阶段提交破坏；未在本轮重复执行的高成本用例继续引用同版本既有实测证据。
7. 已完成：MF 两份 AimOffset 接入 30 个同侧 ShenWanYun 样本与正式 Preview Base；MM 31 个 Additive Base Pose 已改为同侧 ChenHaoYu，剩余 Manny Base 为 0。
8. 已完成：男女三枪 6 个 Fire 与 6 个 Reload 角色 Montage 已配置配套 `AM_Weap_*`，正式链仅保留原生 `UShootAnimNotify_PlayWeaponMontage`。
9. 已完成：18 个正式 CC AnimBP 编译零失败；男性 Shotgun Fire 分屏真实输入与女性远端客户端 Listen Server 同步均通过。
10. 已完成本阶段维护：Equipment、Anim Layer、GameplayEvent/GameplayMessage 与 UIExtension 调用边界已沉淀到任务包和上下文文档；Mannequin 运行姿势/类型工具依赖已分类，冷编译和冷启动复读通过。工具垃圾和用户无关改动未混入。

## 已完成且禁止重复调查

- Lean、Foot Controls、Reload 主链、Ammo UI 复制乱序、出生点、队伍/友伤、敌人死亡重生已经有运行证据。
- 稳定 `UShootReticleHostWidget`、散布 Heat/UI 映射、本地分屏和 Listen Server 准星隔离已完成并由提交 `e1c901e` 推送。
- 补给站服务器权威逻辑、长按取消、弹药结算、分屏/Listen Server 数据隔离和玩家可见 UI 均已通过；不再从视觉接线重新开始。
- 旧的 `TestMap`、`TestMap2` 已重命名为 `TestMap_SplitScreen`、`TestMap_ListenServer`。后续文档和测试统一使用新名称。

以下日期章节保留为历史证据；若与上方 2026-08-11 断点冲突，以上方断点为准。

## 2026-08-10 当前执行断点

- 已排除准星半径公式、Heat 曲线和性别特判：两名本地玩家的 WeaponInstance 散布与屏幕半径
  都会随真实开火更新。旧异常由“每把枪直接注册 UIExtension + 子 Widget 构造时再次读取
  QuickBar”的双重生命周期竞态，以及 QuickBar 重复 Equip 把 Heat 重置到区间中点共同造成。
- `UShootHUDReticleComponent` 现在只为每个 LocalPlayer 注册一枚稳定
  `UShootReticleHostWidget`。Host 内只保留当前武器的一枚子准星，并把精确 WeaponInstance
  在 Slate 构造前交给子 Widget。远端 Listen Server Controller 不创建 UI。
- 本地双人已覆盖快速切枪、Shotgun 连续开火、Reload、Jump、Crouch 和移动；Listen Server
  已确认服务器本地、服务器远端、客户端本地三种 Controller 的 Host 边界正确。详细数值见
  `Verification_验收.md`。
- 旧文档中“每次 Equip 从 Heat 区间中点开始是正确 Lyra 行为”的结论已作废。Lyra 的该值依赖
  真正装备实例生命周期；本项目 QuickBar 会对同一实例反复 Equip，因此项目层从曲线最小 Heat
  开始，避免切枪凭空产生高散布。服务器 Trace 与 UI 继续共享同一散布值。
- 当前待办顺序固定为：准星/地图检查点 -> 3 秒长按补给站 -> 可配置左手握把 -> Niagara
  伤害数字 -> Equip/Unequip 橡皮手与 Shotgun 开火旋转。上下文压缩后从本节继续，不重新调查
  已排除的准星根因。

## 2026-08-09 用户复验后的当前断点

- 待原位修复的正式 Montage：MF `AM_Shotgun_Fire`、`AM_Shotgun_Equip`、`AM_Pistol_Fire`；MM `AM_Shotgun_Fire`、`AM_Shotgun_Equip`、`AM_Rifle_Fire`、`AM_Pistol_Fire`。
- Fire 的直接修复对象不只包含 Montage 容器。源 Manny Fire 是 Mesh Space Additive；CC 目标若在重定向时保留 Additive 标记，绝对关键帧会被错误解释。正确流程与 Jog Lean 同源：`retain_additive_flags=False` 先重建绝对姿势，再在目标序列恢复 Mesh Space Additive、同名自引用 Base Pose 和 Rifle/Pistol/Shotgun 的参考帧 `22/24/28`，最后重新迁移逐帧 `weapon_r`。
- Shotgun Equip 必须恢复 Lyra/Rifle Equip 的双轨结构：`UpperBody` 基础序列加 `UpperBodyAdditive` Additive 序列。此前为了诊断移除 `UpperBody` 的结果不是正式修复，必须撤销。
- MM `MM_Shotgun_Fire` 已按上述流程重建到“待真实开火验收”；第一轮自动测试只产生移动，弹药仍为 `8` 且没有活动 Montage，因此该轮作废，不能算通过。
- Lyra 11000 已确认左手链为 `HandIKRetargeting -> CopyBone(VB IK_Hand_L_weaponSpace 到 ik_hand_l) -> TwoBoneIK(hand_l)`，`DisableLHandIK` 曲线只控制左手 IK 权重。Lyra 默认武器数据没有常规的每枪 `LeftHandGripOffset`；握把目标主要由动画中的 `weapon_r` 与 `hand_l` 相对姿势决定。
- 下一步固定顺序：修 7 个正式资产 -> 男女 Equip/快速切枪/移动 Fire -> 准星快速切换卡最大或不扩散 -> 其余分屏与网络矩阵。长按补弹站和 Niagara 伤害数字保持低优先级。

## 九项执行进度

1. 已完成：建立任务包、需求边界、Git 基线与验收矩阵，提交 `c573a9b`。
2. 已完成：正式 CC `MM/MF` 运行时切换、旧引用审计，删除旧男女 Animations 与 `AnimPoseProbe`，提交 `1cd6507`、`1082d08`、`ae144bc`。
3. 已完成核心闭环：弹药 UI 消息链已实现；换弹已恢复 Lyra GameplayEvent 主线，男女六个正式 Reload Montage 均接入现有 `AN_Reload`。客户端首次拾枪时 Inventory Item 晚于 QuickBar Guid、Equipment Instigator 晚于 WeaponInstance 的复制顺序已修正；首把且唯一一把 Shotgun 的真实客户端 Slate HUD 已通过 `8/16 -> 0/16 -> 8/8`，本地分屏两人的换弹和快速切枪也保持独立。
4. 已完成地基资产迁移：男女 `weapon_r`、三个虚拟骨骼、Rifle/Pistol/Shotgun 与 AimOffset 的 weapon-space 轨道已补齐；男女 Item Base 的主 AnimBP 类型和外部成员引用已迁移；8 个武器/空手子层已改为继承同侧正式 CC 父层；18 个正式 AnimBP/动画接口资产编译错误为零。
5. 已完成本地动画地基：一次性 `StanceTransition` 与 `IdleBreak` 已取消循环；男女 Rifle Jog Lean 已按正确 Additive 流程重建；Foot Controls 单变量 A/B 已完成且全部恢复；Fire/Reload 的动态 `weapon_r`、Fire Montage Slot 和 Equip/Unequip 时序均已修复并通过真实输入验收。
6. 本地分屏核心玩法已完成：真实交互拾取、Fire/Reload、三枪 Equip、快速切换、丢枪自动装备和最后一枪 Unequip/空手闭环均通过；TestMap2 Listen Server 的远端 Equip、快速切枪、丢枪自动装备和最后一枪 Unequip 也已通过服务器/客户端双端逐帧回归。
7. 已完成：姓名牌由用户在原 W_Nameplate/MVVM 链完成；两名玩家均为 Team 1、敌人为 Team 2，真实 Rifle 射击验证友军不掉血、敌人正常受伤；TestMap GameMode 已接通敌人尸体清理与原出生点重生。
8. 已完成当前全矩阵汇总：Equip/Unequip、散布倍率与 Ammo UI 复制顺序修复均由 `NewWorldOrderEditor Win64 Development` 验证成功；TestMap 出生点遮挡已修正。单人、本地双人与 Listen Server 的移动、三枪、弹药、切换、丢弃、队伍伤害、敌人重生和动画证据已归档到验收文档。
9. 已完成阶段检查点：功能与验收提交 `7997cda`、状态提交 `532a6ba`、用户汇总提交 `4caef3b` 和出生点验收提交 `ae30884` 均已推送远端 `main`；本轮只提交其后的橡皮手修复和终态证据。

## 2026-08-02 调用链纠错

- `BP_ShootCharacter` 已经通过父类 `Startup Abilities` 属性授予 `/Game/Blueprints/AbilitySystem/GameplayAbilitys/GA_Hero_Jump`。
- `IA_Jump -> InputTag.space -> GA_Hero_Jump` 是既有正确主线；不能仅检查 `CoreCombatAbilities` 就判断 Jump 未授予。
- 本轮误加的原生 Jump 实现和 `CoreCombatAbilities` 条目已经全部撤销，源码和角色蓝图均恢复到远端基线。
- 后续修改既有 GAS/动画链前必须同时检查 Lyra 对照实现、蓝图子类、引用者和所有授予入口，不能只检查一个 C++ 配置数组。

## 十项任务状态

1. Git、需求与 Lyra 基线审查：已完成基础审查；本任务包用于持续记录后续差异。
2. Persistent 仓库、RuntimeOnly QuickBar、Pawn 装备表现桥：代码主链、切图、丢枪、分屏和联机回归均已完成，RuntimeOnly 物品未进入存档。
3. HomeMap 到 TestMap、副本入口与男女主互斥：已有实现并已提交；旧的“选择男性后分屏出现两个男性”已修复，TestMap 最终分屏中 P0/P1 为男女互斥且出生点分离。
4. 姓名牌：`BP_ShootCharacter.NameplateWidget`、原 `W_Nameplate` 和 `UAttributeViewModel` FieldNotify 链保留；用户已手动完成该项，不再新增 C++ Widget 基类或 Character 刷新桥。
5. 队伍、友军伤害、敌人死亡与重生：Controller 按 Lyra 代理 PlayerState；GameMode 负责分队并按地图决定敌人重生；敌我解析统一走 AbilitySystemLibrary。真实 Rifle 射击中 P1 Health 保持 `276`，相同射线下敌人 `276 -> 266`；致死后旧 Actor 清理，新实例以原蓝图类、初始 Transform、满血和新 AIController 重建。
6. Rifle、Pistol、Shotgun 射击与整弹匣换弹：三把枪均已在本地分屏通过真实输入；P0/P1 弹药相互独立。Listen Server 主机 Rifle 与远端客户端 Pistol 已完成拾取、开火、换弹和丢枪回归。
7. CommonUI 准星、散布、本地后坐力和弹药显示：已完成 Lyra 资产、曲线、UI 半径映射和运行时姿态倍率对照。拾枪时从 Heat 区间中点开始是 Lyra 原行为，不是从最大散布恢复。Ammo UI 已覆盖客户端首把武器复制乱序、空仓换弹、快速切枪以及本地分屏隔离。
8. 角色和武器动画同步：角色动画、武器挂点、Jog Lean、Foot/Hand Controls、Reload 结算、网络装备生命周期，以及角色 Fire/Reload 到武器 SkeletalMesh Montage 的唯一原生同步链均已闭合。
9. 正式 CC 资产切换与旧目录清理：运行时切换已完成。主 AnimClass、默认空手层、三把武器层、展示 AnimBP 和三个 ItemDefinition 已切到正式 CC 路径。旧男女 Animations、`/Game/Developers/Codex/AnimPoseProbe` 和两个 `NoAddCandidate` 已由用户从编辑器清理，本任务不恢复。
10. 武器插槽、Transform、Equip/Unequip 与最终验收：已完成当前范围。武器挂点和 Transform 已有配置入口；本地男女、Listen Server 远端 Equip/Unequip、TestMap 双出生点和正式资产冷扫描均通过。剩余内容只涉及用户手工清理临时资产与未来的 Mannequin 类型依赖项目化。

## 已证明的运行时事实

- 真实 Enhanced Input 射击能够使 Rifle 从 30/60 变为 29/60、Pistol 从 12/48 变为 11/48、Shotgun 从 8/16 变为 7/16；每次只扣一发，权威值与预测值一致。
- 男性 Shotgun 实际播放的正式开火蒙太奇为 `/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_Shotgun_Fire`。
- Rifle 真实输入换弹已从 `29/60` 结算为 `30/59`，Shotgun 从 `7/16` 结算为 `8/15`，女性 Pistol 从 `11/48` 结算为 `12/47`；HUD 均与权威值、预测值同步。
- 三槽真实拾取后分别保存 Shotgun `8/15`、Pistol `11/48`、Rifle `30/59`；切枪不串弹药。依次丢弃三把枪后 `ActiveIndex=-1`、三个 RuntimeOnly 槽位均为空，角色回到正式空手层。
- P1 女性实际链接层为 `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Pistol/ABP_PistolAnimLayers_Feminine`，开火和换弹均由 P1 自己的 Enhanced Input 子系统驱动，没有串到 P0。
- 男女六个正式 Fire Montage 已恢复 Lyra 源槽 `FullBodyAdditivePreAim`；其对应 Fire Action 以及男女 Reload/Reload Additive、Rifle/Pistol Equip/Equip Additive 已逐帧补回动态 `weapon_r`，目标与源的局部位移最大差为 `0`。
- 女性三枪 Fire 与 Reload 的运行时上臂/前臂长度稳定为约 `27.95/25.50 cm`。TestMap 中一度出现在女性镜头左侧的灰色“巨臂”来自距角色仅 `68.25 cm` 的 EnemyBot 进入相机近裁剪，移开敌人后的干净截图不再出现；它与骨骼拉伸是两个问题。
- `UShootWeaponInstance::OnEquipped/OnUnequipped` 已消费 ItemDefinition 的男女 `FShootWeaponCharacterMontageSet`，按 Lyra 顺序先链接同侧 Item Anim Layer，再在角色主 AnimInstance 播放 Equip/Unequip；Rifle、Pistol、Shotgun 三份 ItemDefinition 已配置同侧正式 Montage。
- 真实 `IA_WeaponNext` 覆盖男女三枪 Equip、快速 A/B 连续切换和丢枪后自动装备。最后一枪丢弃时，男性键盘 `G` 与女性手柄 `Gamepad_DPad_Down` 均在 ActiveIndex 变为 `-1` 的同帧进入同侧 `AM_Generic_Unequip`。
- 男女 Generic Unequip 原来约在 `0.06` 秒开始淡出，根因是从 Pistol Equip 复制后错误保留 `RateScale=1.1`、Blend In `0.25/Hermite Cubic`、Blend Out `0.40/Hermite Cubic`。恢复 Lyra 的 `0.9`、`0.20/Cubic`、`0.30/Cubic` 后，帧级日志推进到约 `0.23` 秒才进入 Blend Out；截图为 `Saved/Screenshots/WindowsEditor/ScreenShot00001.png` 和 `ScreenShot00002.png`。
- Listen Server 首次远端 Equip 曾出现服务器播放、客户端漏播：`EquipmentList` 的 FastArray `PostReplicatedAdd -> OnEquipped` 可以先于装备子对象的 `Instigator` 属性到达，而 Montage 配置必须经 `Instigator -> ItemDefinition -> RangedWeaponConfig` 解析。当前由 `UShootEquipmentInstance::OnInstigatorReplicated` 提供复制完成扩展点，`UShootWeaponInstance` 只在首次 `OnEquipped` 因缺少 Instigator 而未能解析 Montage 时记录一次性待补状态；Instigator 到达后仅补播 Equip Montage，不重复链接 Anim Layer、显隐武器或调用蓝图装备事件。
- 修复后远端 Shotgun 权威拾取在服务器第 `6` 帧播放 MF `AM_Shotgun_Equip`，客户端于第 `8` 帧完成 QuickBar/Instigator 复制并播放同一 Montage。客户端真实 Slate 输入以约 `20` 帧间隔执行 `Shotgun -> Rifle -> Shotgun -> Rifle`，服务器模拟代理与客户端分别在第 `17/37/57` 帧同步进入相应 Equip Montage；最后一枪真实 `G` 丢弃后两端在第 `88` 帧同时进入 MF `AM_Generic_Unequip`，ActiveIndex 均为 `-1`。
- Mutable 会令女性主 `CharacterMesh0` 使用 `/Engine/Transient.Skeleton_*`，身体组件仍使用正式 `ShenWanYun_Skeleton`。原蒙太奇选择只检查主 Mesh，导致女性换弹报 `no character reload montage`；当前改为遍历 Pawn 的全部 SkeletalMeshComponent，仍只接受 ItemDefinition 显式配置的精确 Skeleton，Live Coding 编译并通过 `11/48 -> 12/47` 回归。
- Lyra 11000 端口已确认 `/Game/Weapons/GA_Weapon_ReloadMagazine` 使用 `PlayMontageAndWait + WaitGameplayEvent(GameplayEvent.ReloadDone)`：Montage 完成只 `EndAbilityLocally`，服务器只在 AnimNotify 事件到达时结算。项目原 C++ 翻译主链正确，真正缺口是正式 CC Reload Montage 没带 ReloadDone Notify。
- 三枪准星 Widget 的尺寸与映射精确匹配 Lyra 11000：Rifle 外圈使用 `Radius*2` 与 `48/96`，Pistol 使用 `40/80`，Shotgun 使用 `SpreadRadiusScaler=0.3125`、宽度 `1.25`、高度 `2` 及对应尺寸上限；C++ 屏幕空间散布角计算也与 Lyra 一致，并保留分屏视口尺寸修正。
- 三枪拾取后的初始 Heat 为 Lyra 区间中点：Rifle `7.276888` 并约 `1.7s` 回到 `2.5`，Pistol `7.25` 并约 `0.5s` 回到 `2.5`，Shotgun `12.125` 并约 `1.5s` 回到 `6`。Rifle 开火后的运行时倍率实测为移动 `1.0`、停止 `0.8`、蹲伏约 `0.4801`、腾空约 `1.4544`、瞄准约 `0.5216`，证明 Fragment Heat 曲线武器现在会消费站姿倍率。
- Listen Server 远端客户端首次且只拾取 Shotgun 时，QuickBar Guid 可先于 Inventory Item UObject 到达，WeaponInstance 又可先于 Equipment Instigator 到达。`CombatComponent` 现在只在该客户端复制窗口重试解析未到达的占用槽位，并在 Instigator 尚未到达时从同槽 Inventory Item 填充弹药；`ShootInventoryItemInstance::OnRep_ItemDef` 复用既有 StatTag 广播触发刷新。修复后的真实客户端 Slate HUD 为 `8/16 -> 0/16 -> 8/8`，再拾 Rifle、切换后显示 `30/60`，五次快速切换回 Shotgun 仍为 `8/8`。
- 本地分屏回归中 P0 Shotgun 从 `0/16` 换弹为 `8/8`，P1 Rifle 保持 `27/60`；两人各自再拾 Pistol 后独立切换，最终 P0 Shotgun `8/8`、P1 Rifle `27/60`，快捷栏选中态和弹药没有跨玩家串线。
- TestMap 原 `PlayerStart_0=(0,0,92)` 与 EnemyBot `(0,0,88)` 重合，导致 P0/P1 都选择 `PlayerStart_1`。当前仅将 `PlayerStart_0` 对称移到 `(0,-330,92)`；新 PIE 中 P0/P1 分别生成在 `(0,330,89.65)` 与 `(0,-330,89.65)`，EnemyBot 保持 `(0,0,89.65)`，男女互斥且两个相机均无贴脸遮挡。
- 用户不接受把 Equip 双肘外翻归类为“官方姿势”。重新读取 Lyra 11000 后发现项目 Equipment 的 `weapon_socket_hand_r + Identity` 与 Lyra 三枪 `weapon_r/-90` 不一致，现已正式对齐。六个 Equip Montage 及其基础/Additive Sequence 均为零 Notify。`EquipMontageStartPosition=0.4s` 对男性可用但女性 Shotgun 仍偏早；女性 `0.6/0.8/1.0s` 采样后，三把 WeaponInstance 的正式值已调整为 `0.6s`。
- 男性 Shotgun 真实持续 `IA_Move` 后速度从约 `240` 稳定到 `300cm/s`，同时触发 `IA_Attack`，Fire Montage 捕获约 `0.067s`；连续三帧双手稳定且武器保持附着 `weapon_r`。只注入一帧 Move 得到速度 `0` 的第一次测试已明确作废，不计入验收。
- 正式 `0.6s` 配置下，女性真实拾取 Rifle 的首个捕获帧约为 `0.669s`；随后拾齐三枪并连续十次触发 `IA_WeaponNext`，运行顺序稳定为 Pistol、Shotgun、Rifle 循环，各次 Equip 捕获位置约 `0.69-0.71s`，唯一附着武器始终位于 `weapon_r`。女性再以 `300cm/s` 持续移动触发 Shotgun Fire，开火帧约 `0.028s`，截图 `HighresScreenshot00075-00076.png` 未见双肘翻折或手臂拉伸。
- 历史缺口已修复：男女正式 `ALI_ItemAnimLayers` 现在各提供 14 个 Lyra 动画层函数，覆盖移动阶段、跳跃阶段、骨骼控制、左手姿势和 `FullBodyAdditives`，不再只有 `FullBody_Aiming`。

## 当前阻断问题

- 已排除的地基阻断：男女 CC 均已有父级为 `hand_r` 的 `weapon_r`，并同步拥有 `VB IK_Hand_R_chestSpace`、`VB IK_Hand_L_chestSpace`、`VB IK_Hand_L_weaponSpace`；相关正式动画轨道已迁移并逐帧核对。
- 已排除的类型阻断：Item Layer 原本虽然复制到 CC 目录，`GetMainAnimBPThreadSafe` 的返回签名、DynamicCast、变量/函数 MemberReference 和 Property Access 仍指向源 Mannequin 类，导致运行时 Cast 失败。当前男女均已迁移到各自 CC 主 AnimBP。
- 已排除的继承链阻断：正式 CC Rifle/Pistol/Unarmed 子层原本仍以 Mannequin Item Base 为父类，Shotgun 又以 Mannequin Rifle 为父类。此时即使修复正式 CC Base，运行时子层也会绕开它。当前八个子层均已重定父类到同侧 CC Base/Rifle，并保留原 CC 动画覆盖。
- 已排除的自动过渡阻断：`StanceTransition -> Idle` 与 `IdleBreak -> Idle` 的源 Sequence Player 已取消 Loop，编译不再报告自动过渡使用循环源动画。
- `Foot Placement` experimental 只表示引擎节点实验状态，不等于本项目配置正确；仍需以真实输入 A/B 判断。
- 已排除的 AimOffset 阻断：正式男性 `AO_MM_Pistol_Idle_ADS` 原有 15 个坐标但动画引用全为空，冷启动会报告 invalid sample。当前按 Mannequin 源资产的同名/同坐标关系接入 15 个正式 MM 重定向序列；全量扫描的 8 个正式 CC BlendSpace/AimOffset 已无空样本。

- 已排除的 Lean 资产阻断：正式 Left/Right 原先在保留 Additive 标记时直接重定向，目标关键帧被错误解释成 Additive 差值。四个正式男女 Rifle Lean 资产已按 `retain_additive_flags=False` 重建绝对姿势，再恢复同侧 LocalSpace Additive 与 CC Center Base Pose；真实 `IA_Move` 连续反转已通过，证据为 `Saved/Diagnostics/LeanFix_Phase_07.png`、`LeanFix_Phase_15.png`、`LeanFix_Phase_23.png`。
- 已排除的 Foot Controls 运行阻断：男女均完成 Idle、前后左右、连续反转、Jump/落地和 Crouch 的真实输入回归。四向移动稳定为 `300cm/s`，男/女运行时两脚间距分别约 `34-70cm`、`49-73cm`；Jump 垂直速度约 `446/433cm/s`，落地脚部高度回到约 `7-9cm`，Crouch 胶囊半高从 `88` 正确降到 `40`。截图 `HighresScreenshot00077-00090.png` 未见双脚绑定、膝盖强制反折或脚尖持续悬空。
- 已排除的 Hand Controls 阻断：用户最新验收图中的 Shotgun 开火橡皮手来自 Fire Action 丢失动态 `weapon_r` 且六个 CC Fire Montage 使用了错误 Slot。修复后男女 Shotgun Fire、女性 Rifle/Pistol Fire 和三枪 Reload 均通过真实输入；HandIK Retargeting A/B、骨长采样和 EnemyBot 近裁剪对照也已完成。
- 已排除的本地 Equip/Unequip 阻断：项目现已接通 Lyra Item Anim Layer -> Montage 顺序；男女三枪切换、快速切枪、丢枪自动装备和最后一枪空手均通过真实输入。Generic Unequip 的早退由短 Montage Rate/Blend 参数精确修复，不使用延时或重复播放补丁。
- 已排除的远端 Equip/Unequip 阻断：FastArray 与 Instigator 的无序复制由一次性待补状态处理；TestMap2 Listen Server 的远端拾取、快速切枪、丢枪自动装备和最后一枪空手均已通过双端逐帧回归，不使用 Timer、Multicast 或延迟重试。
- 已排除的 Reload 阻断：男女六个正式 CC Reload Montage 已按 Lyra 原 Montage 的装匣帧接入现有 `/Game/Characters/Heroes/Abilities/AN_Reload`；Pistol `1.631531`、Rifle `1.746580`、Shotgun `2.243124`，且 Dedicated Server 可触发。Rifle/Pistol/Shotgun、本地分屏和 Listen Server 的装匣结算均已通过。
- 已排除的动画同步阻断：正式 Fire 的空配置 BP Notify 已移除，男女三枪 Fire/Reload 均使用唯一原生 `UShootAnimNotify_PlayWeaponMontage`；分屏与 Listen Server 实测角色、武器 Montage 时间轴一致。
- 2026-08-12 使用 AssetRegistry 复核男女正式 18 个 CC AnimBP/动画接口后确认：它们直接硬依赖 9 个 Mannequin 包，类别为枚举、结构、接口、父 AnimBP、Transition Notify、Skeleton、ControlRig 与 IK Retargeter；沿这些包递归展开为 37 个 Mannequin 包。这里既有可解释的类型/工具链依赖，也有父类默认动画形成的传递依赖，不能直接据此删除 `/Game/Characters/Heroes/Mannequin`。
- 18 个正式资产的 `target_skeleton` 已逐项复读：MM 九项全部为 `ChenHaoYu_Skeleton`，MF 九项全部为 `ShenWanYun_Skeleton`。因此上面的 `SK_Mannequin` 硬依赖不是正式 AnimBP Target Skeleton 回退，但仍需查清它来自节点、父类还是工具链字段。
- 同轮扩大到男女正式 CC 目录后发现的两份 MF AimOffset 与 31 个 MM `ref_pose_seq` 运行姿势依赖已于 2026-08-13 修复；当前复读 `MANNY_REF_POSE=0`、`MANNY_AIM_REFS=0`。余下直接包依赖主要来自 AnimationModifier、压缩设置、Transition Notify、父 AnimBP 类型、ControlRig 与 IK Retargeter，必须按类型契约继续分类，不能批量替换。
- 2026-08-13 冷启动最终分类把上述“待分类”关闭：正式目录的 1097 条直接 Mannequin 边中，1065 条是 AnimSequence 到曲线压缩设置、AnimationModifier 或 Transition Notify 的引用；正式 AnimBP 的剩余边是 Lyra 枚举/结构、Transition Notify、动画层接口类型、FootPlant ControlRig、IK Retargeter 和编辑器 Skeleton 契约。六个相关 AnimBP 的真实父类均为项目原生类或 `AnimInstance`，不是源 Mannequin Blueprint。三份零引用 `SplashPose_Quinn_1/10/11` 是唯一残留 Manny/Quinn 姿势资产，不进入运行时并交由用户手工清理。
- Lyra 11000 对照确认上述 Rifle、Unarmed、Pistol AimOffset 都采用相同 15 点网格；源方向 Sequence 为 Mesh Space Rotation Offset Additive，Base Pose 类型为 Anim Frame，指向同组中心样本第 0 帧。项目修复应只替换为同侧 CC Sequence 并保持这些字段与采样坐标，不能把 AimOffset 改成普通 BlendSpace 或删除 Additive 语义。
- 用户提交 `4caef3b` 已汇总 TestMap、三个 ItemDefinition、C++ 和正式动画资产，也一并纳入两个 `NoAddCandidate`；本轮不能回退该提交，只把候选资产列入手工清理清单，且不得把工具垃圾继续带入后续提交。
- 已排除的出生点阻断：`PlayerStart_0` 已从 EnemyBot 中心移到对称的 `Y=-330`；默认 GameMode 现在为两名本地玩家选择不同 PlayerStart，分屏相机近景遮挡不再出现。
- TestMap 的 `BP_TestGameMode` 已启用可配置敌人重生：尸体 1 秒清理，5 秒后按敌人初始 Transform 生成相同蓝图类。其他 GameMode 默认关闭，不影响 HomeMap。
- `ShootInventoryGrantActor` 不再在 C++ 构造函数加载失效 UI 路径或调试网格；正式配置位于 `/Game/Blueprints/Interaction/BP_ShootInventoryGrantActor`，HomeMap 直接实例和 Spawner 均已迁移。
- `BP_TestListenGameMode` 原误配为 `PRIMARY_ONLY`，会把 `?MaxPlayers=1` 注入网络地图并令第二客户端停留在 FrontEndMap。当前已改为 `KEEP_CURRENT`；Listen Server 权威世界稳定拥有两名 PlayerController，客户端进入同一 TestMap2。

## 下一执行顺序

1. 已完成并推送三环境终态回归文档；临时 MCP 请求和工具垃圾未纳入提交。
2. 已完成维护阈值与 Mannequin 依赖复核；维护规则未触发归档，依赖审计发现两份 MF AimOffset 仍直接播放 Mannequin 样本。
3. 用户已授权的正式动画与 `AN_*` 修复已完成并推送：MF AimOffset、MM Additive Base Pose、12 个角色到武器 Montage 同步和男性 Shotgun Fire Additive 元数据均已通过复读与真实输入验收；不得再用 Actor Rotation 补丁替代。
4. `/Game/Weapons` 下 8 个零引用 Manny 角色 Montage，以及 MF Poses 下零引用的 `SplashPose_Quinn_1/10/11` 当前仍存在。它们均为多个文件，按删除红线只保留精确清单，不由本任务批量删除；由用户在 Content Browser 手工处理。

## 维护检查

- 2026-08-12 按 `Docs/Engineering/Maintenance_Rules.md` 完成六项检查。仓库没有项目级 `CHANGELOG.md`、`CHANGELOG_Recent.md` 或 `SettingRegistry.md`；章节版本文件为 0；`Engineering/ADR/Superseded` 为 0；DevelopmentNotes 为 23 个文件，规则未给出该项自动归档阈值。
- 用户没有发出 Phase 切换指令，也没有出现规则要求的设定删除或 CHANGELOG 数量触发条件；`Docs/PROJECT_PHASE.md` 保持 Phase 1，不擅自切换。
- 六项规则均未达到归档阈值，本轮不移动、不删除维护文件。

## 工作区保护

以下内容不属于本任务提交，除非用户另行确认：

- `Content/Characters/Heroes/Mannequin/Meshes/SKM_Manny.uasset`
- `.reasonix/`
- `Build/`
- `reasonix.toml`
- 工作区根目录的 `VibeUE-master.zip`
- 用户已手工删除 `Content/Developers/Codex/AnimPoseProbe/ABP_ShenWanYun_Main_BypassShoe.uasset` 和两个 `NoAddCandidate`；本任务保留这些删除，不恢复候选资产。

## 2026-08-17 男性 Shotgun Fire 瞬时抬枪修复

- 运行时确认实际 GAS 使用的角色资产仍是正式 MM `AM_Shotgun_Fire`；`AM_Generic_Unequip` 在 Fire 播放期间没有进入活动 Montage，因此不是开火抬枪的直接原因。
- Manny 源、正式 MM 与正式 MF 的 Shotgun Fire Action 都是 `0.666667s`、Mesh Space Additive、自身 Base Pose。逐帧骨骼采样中三者的 `weapon_r -> hand_r` 距离一致，运行时武器 Actor 也始终附着 `weapon_r/-90`，排除了 Actor Transform、Socket 突跳和 `weapon_r` 脱手。
- 真正的男女差异在 Montage 混入：MM 为 `BlendIn=0.0s`，MF 为 `BlendIn=0.25s/HermiteCubic`。男性因此会在首帧瞬间吃满 Mesh Space Additive 后坐，视觉上像枪体突然顶向头部；女性的同一动作会平滑混入。
- 已将正式 MM `AM_Shotgun_Fire` 的 Blend In 调整为 `0.25s/HermiteCubic`，Blend Out 保持原 `0.30s/HermiteCubic`。同一分屏、同一 `0.15s` 冻结帧 A/B 中，枪体和双手不再瞬间顶到脸部，角色与武器 Montage 仍同步。
- 正式 Fire Montage 的三枚 Notify 已复读：首帧唯一类 Notify 是原生 `UShootAnimNotify_PlayWeaponMontage`；结尾 `ResetCombo`、`SaveAttack` 是 Skeleton Notify。换枪音效 `sfx_WeaponSwap_nl_meta_Preset` 属于 Equip Montage，不属于 Fire，也不会修复开火姿势。
- 修复后 MM、MF 两份正式 `ABP_Mannequin_Base` 均重新编译为 `BS_UP_TO_DATE`；2026-08-17 当前日志没有新的 `property [None]` / `Event.Movement.*` 错误。两份 AnimBP 不是互斥关系，历史报错仍是各资产内部 GameplayTagPropertyMap 的 PropertyGuid 失效问题。
