# 橡皮手与武器动画叠加链调查

调查起始日期：2026-08-20
当前更新：2026-09-10
状态：橡皮手目前暂时看上去已解决，等待扩展回归
当前分支：`main`  
当前基线提交：`2459cfe`

## 目标

查清并修复以下两个必须分开验证的问题：

1. Rifle、Shotgun 与 Pistol 的 Fire/Reload 在静止时部分或全部正常，但叠加移动、跳跃、蹲伏后出现双臂和肩部扭曲。
2. 女性 Pistol Reload 被手工重新重定向后，即使静止也出现武器脱离手部的问题。

不以“看起来可能是动画”或“可能是 AnimBP”作为结论。每个结论必须有项目 8000、Lyra 11000 的资产语义对比或 PIE A/B 证据。

## 已验证事实

### 运行表现

- Rifle 与 Shotgun 在静止时 Fire/Reload 基本正常。
- Pistol 在静止时 Fire 正常；Reload 在本次手工重定向后异常。
- 三把枪在移动、跳跃或蹲伏期间叠加 Fire/Reload 时均可出现双臂扭曲。
- 异常主要集中在上半身、双臂、手和武器空间；下半身运动仍在继续。
- Equip/Unequip 武器飞到头顶的问题已由重新重定向动画解决，但橡皮手仍存在。

### 已知 Retargeter 边界

- 项目 IKRig/Retargeter 没有 `weapon_r` 专用 Chain。
- 既有批次已经捕获到 Retargeter 生成异常 `weapon_r` 位移的实例。
- `Scripts/Editor/Animation/SafeRetargetWriteback.py` 已验证可在保留正式 UObject 和外部引用的前提下，写回目标局部骨轨并单独迁移 Manny `weapon_r`。
- 因此“直接 Retarget 并 overwrite 正式资产”不是安全的默认流程。

### 当前 Git 现场

用户实验资产，当前均未暂存：

- `Content/Characters/Heroes/CC/MF/Animations/Actions/MM_Pistol_Reload.uasset`
- `Content/Characters/Heroes/CC/MF/Animations/Actions/MM_Pistol_Reload_Additive.uasset`
- `Content/Characters/Heroes/CC/MF/Animations/Weapons/Montages/AM_Pistol_Reload.uasset`

其他未暂存用户资产：

- `Content/Assets/Characters/CC/ShenWanYun/ShenWanYun.uasset`
- `Content/Assets/Characters/CC/ShenWanYun/ShenWanYun_Skeleton.uasset`
- `Content/Characters/Heroes/CC/MF/Animations/LinkedLayers/ABP_ItemAnimLayersBase.uasset`
- `Content/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers_Feminine.uasset`
- `Content/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base.uasset`
- `Content/Weapons/Pistol/ID_Pistol.uasset`

上述 Change 已经落盘。调查期间不得擅自 restore、覆盖或提交这些用户资产。本调查自己的单样本修改仅限 `Content/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd.uasset`。

## 当前假设及验证门槛

### H1：Montage 或引用 Sequence 缺少 Lyra 曲线/Notify/Timing

支持证据：

- 当前正式 Montage 多数由 AI 生成，不保证完整复制 Lyra 的 Slot、Segment、Notify、曲线与时序。
- Lyra Equip 参考资产明确使用 `DisableHandIK`、`ScaleDownWeaponR` 等曲线。

反证门槛：

- 当前 Montage 与 Lyra 参考在 Segment、Slot、曲线、Notify、Timing 上等价；且同一 Montage 在移动时仍异常。

### H2：AnimBP/Linked Anim Layer 没有按 Lyra 消费曲线或叠加顺序错误

支持证据：

- 同一武器动作在静止时正常、进入移动/跳跃/蹲伏叠加后异常。
- 问题跨三把武器出现，更像共享动画主链问题。

反证门槛：

- 项目与 Lyra 的相关 AnimGraph、Animation Layer、Function、Variable、Blend Mask 和曲线消费链语义等价；替换为忠实 Montage 后异常消失。

### H3：CC Skeleton/Virtual Bone/Socket/Post Process 与 Manny 不一致

支持证据：

- `weapon_r`、`weapon_l` 曾手工复制；Virtual Bone 由 AI 添加。
- Compatible Skeleton 不保证 Reference Pose Local Transform、Virtual Bone Source/Target、Socket 或 Post Process 等价。

反证门槛：

- Manny、男性 CC、女性 CC 的关键骨层级、关键 Local Transform、Virtual Bone 定义、Socket、Translation Retargeting 与 Post Process 均满足项目适配要求。

### H4：本次 Pistol Reload 是独立的 `weapon_r` 重定向故障

支持证据：

- 武器脱手紧随 `AM_MM_Pistol_Reload` 和两条 Sequence 的直接重定向发生。
- 项目此前已捕获 Retargeter 的异常 `weapon_r` 输出。

验证门槛：

- 对比 Manny 源与当前 MF 目标的 `weapon_r` 全帧 Local Transform；核对 Montage Segment、Skeleton、Additive/Base Pose 和 `ID_Pistol` 引用。

## 调查顺序

1. 只读审计三个代表样本：Pistol Reload、Rifle Reload、Shotgun Fire/Reload。
2. 对比 NewWorldOrder 与 Lyra 的 `ABP_Mannequin_Base`、`ABP_ItemAnimLayersBase` 和相关武器 Linked Layer。
3. 对比三套 Skeleton、SkeletalMesh、Virtual Bone、Socket、Post Process 与 Retarget Pose。
4. 根据证据选择单一最小修复；先修一个代表病例并做 PIE A/B。
5. 扩展到男女、三把武器、静止/移动/蹲伏/跳跃矩阵。

## 资产写入安全门槛

- 在完成语义审计前，不批量 overwrite `MM/Animations/Weapons/Montages` 或 `MF/Animations/Weapons/Montages`。
- 不使用 `EditorAssetLibrary.delete_asset`、Replace Reference 或 Force Delete。
- 不直接改 Lyra 或第三方插件资产。
- 不把当前 Pistol Reload 实验资产或其他用户 Change 混入调查提交。
- 需要重定向时先输出到隔离候选目录，验证后再更新正式资产。

## PIE 验收矩阵

| 角色 | 武器 | 动作 | 静止 | 移动 | 蹲伏 | 跳跃 |
|---|---|---|---|---|---|---|
| MM | Rifle | Fire/Reload | 待复测 | 待复测 | 待复测 | 待复测 |
| MM | Shotgun | Fire/Reload | 待复测 | 待复测 | 待复测 | 待复测 |
| MM | Pistol | Fire/Reload | 待复测 | 待复测 | 待复测 | 待复测 |
| MF | Rifle | Fire/Reload | 待复测 | 待复测 | 待复测 | 待复测 |
| MF | Shotgun | Fire/Reload | 待复测 | 待复测 | 待复测 | 待复测 |
| MF | Pistol | Fire/Reload | Fire 正常；Reload 异常 | 待复测 | 待复测 | 待复测 |

## 历史断点

- 8000：NewWorldOrder MCP 可用。
- 11000：Lyra MCP 已重新初始化并可用。
- 当时下一步：读取项目与 Lyra 的相关 AnimBP 图表、函数、变量和关键节点，优先追踪 `DisableLHandIK`、Slot 与移动状态叠加链。
- 该断点已由阶段二完成，保留用于说明调查演进。

## 2026-08-20 阶段一：代表 Montage 与 Pistol Reload 骨轨

### Lyra 11000 与项目 Manny 源资产

已对以下 Lyra 11000 资产和项目同路径 Manny 源资产做结构化复读：

- `AM_MM_Pistol_Reload`
- `AM_MM_Rifle_Reload`
- `AM_MM_Shotgun_Reload`
- `AM_MM_Shotgun_Fire`
- 上述 Montage 引用的非 Additive 与 Additive Sequence

当前项目 `/Game/Weapons/*/Animations` 和 `/Game/Characters/Heroes/Mannequin/Animations/Actions` 中的代表源资产，在以下已读取字段上与 Lyra 11000 一致：Skeleton、时长、帧率、帧数、Slot、Segment、Additive 类型、曲线名和曲线键。后续可把项目 Manny 源作为本轮只读对照，但涉及未读取字段时仍须回到 11000 复核。

### 正式 MF Montage 与 Lyra/Manny 源差异

| Montage | 正式 MF | Lyra/Manny 源 | 结论 |
|---|---|---|---|
| Pistol Reload | 5 Notify；Blend In/Out `0.1/0.35` | 5 Notify；`0.1/0.35` | 顶层结构一致 |
| Rifle Reload | 2 Notify；`0.25/0.25` | 5 Notify；`0.1/0.3` | 丢失 3 个 Notify，Blend 不一致 |
| Shotgun Reload | 2 Notify；`0.25/0.25` | 5 Notify；`0.15/0.35` | 丢失 3 个 Notify，Blend 不一致 |
| Shotgun Fire | 3 Notify；`0.25/0.25` | 3 Notify；`0/0.3` | Notify 数一致，Blend 不一致 |

VibeUE 当前可返回 Montage Notify 数量，但对 Montage 调用 `list_notifies/get_notify_info` 无法读取具体 Notify 类，因此具体 Notify 内容仍标记为待审计，不能仅凭数量推断功能。

### Rifle Reload 曲线

Lyra/Manny 源与正式 MF 的非 Additive、Additive Sequence 都存在 `DisableLHandIK`，4 个键完全一致：

- `0.333333 -> 0`
- `0.466667 -> 1`
- `0.666667 -> 1`
- `0.766667 -> 0`
- 插值均为 `Cubic`，Tangent Mode 均为 `User`

因此 Rifle Reload 的 `DisableLHandIK` 资产曲线不是当前差异；下一步必须确认项目 AnimBP 是否按 Lyra 相同方式消费该曲线。

### Pistol Reload 脱手根因证据

本次手工重定向后的正式 MF Pistol Reload Montage 顶层结构与 Lyra/Manny 源一致，但它引用的两条 MF Sequence 都产生了异常辅助骨轨：

| Sequence | 骨 | 最大 Local 位移误差 | 最小四元数点积 | 其他异常 |
|---|---|---:|---:|---|
| 非 Additive | `weapon_r` | `21.9800` | `0.003488` | 明显错误 |
| Additive | `weapon_r` | `21.9800` | `0.003488` | 与非 Additive 同样错误 |
| 两条 | `weapon_l` | `12.6019` | `0.980318` | 最大 Scale 误差 `1.55885` |
| 两条 | `ik_hand_l` | `17.4772` | `0.982084` | 明显偏移 |
| 两条 | `ik_hand_gun` | `5.30742` | `0.994413` | 存在偏移 |
| 两条 | `ik_hand_r` | 约 `1.75e-14` | `1.0` | 与源一致 |

`weapon_r` 第 0 帧源位移约 `(-7.15, 2.94, 0.26)`，MF 目标约 `(-5.41, 1.48, 18.92)`；旋转也几乎完全不同。图片中的武器脱手可由该差异直接解释，H4 已从“假设”升级为“已证实的主要原因”。

尚未执行修复。后续应复用安全写回流程修复两条正式 Sequence 的 `weapon_r`，同时评估 `weapon_l/ik_hand_l/ik_hand_gun` 是合理 Retarget 差异还是同一辅助骨链异常，不能只改 Montage 顶层参数。

## 2026-08-20 阶段二：ABP 语义对照与移动态辅助骨异常

### ABP Base 不是“少抄了一段主链”

对 11000 Lyra 与项目 Manny 的关键图表做了节点类型、标题、位置、Pin 类型、连接状态和默认值的规范化 SHA-256 对照。以下图表哈希逐项一致：

- `ABP_Mannequin_Base.AnimGraph`
- `BlueprintThreadSafeUpdateAnimation`
- `UpdateBlendWeightData`
- `FullBody_SkeletalControls`
- `FullBodyAdditives`
- `LeftHandPose_OverrideState`
- `ABP_ItemAnimLayersBase.BlueprintThreadSafeUpdateAnimation`
- `UpdateSkelControlData`
- `SetLeftHandPoseOverrideWeight`
- `FullBody_SkeletalControls`
- `FullBodyAdditives`
- `LeftHandPose_OverrideState`

项目 Manny Base 仅把原生父类从 `LyraAnimInstance` 适配为 `ShootMannequinAnimInstance`；已读取的动画图语义没有偏离 Lyra。男女正式 CC Base 的既有主链与项目 Manny 相同，只在末端增加已验收的 `ShoeAnimationLayer -> HairAnimationLayer`。男女正式 `ABP_ItemAnimLayersBase` 的 `UpdateSkelControlData`、`SetLeftHandPoseOverrideWeight` 和 14 节点 `FullBody_SkeletalControls` 也与项目 Manny/Lyra 哈希完全一致。

`UpdateSkelControlData` 确认按 Lyra 原逻辑读取 `DisableRHandIK`、`DisableLHandIK`，计算左右手 IK Alpha；`FullBody_SkeletalControls` 依次包含 `Hand IK Retargeting`、`Copy Bone(VB IK_Hand_L_weaponSpace -> ik_hand_l)` 和左右手 `Two Bone IK`。因此异常的辅助骨轨会被运行时链直接消费，不能把它当成“动画里不用的冗余骨”。

### 武器子层 Class Defaults

男女 Pistol/Rifle/Shotgun 正式武器层的布尔值、IK Alpha、FK Weight、Blend Weight 和主要动画引用，与 Manny 对应层一致；正式 Pistol/Rifle 额外配置了同侧 `LeftHandPose_Override` 资产，但 `EnableLeftHandPoseOverride=false`，当前默认不会参与混合。Shotgun 延续 Lyra 的专用左手覆盖机制。该差异暂不解释“所有枪只在移动时橡皮手”。

### 移动态辅助骨轨的强证据

对正式 CC Locomotion 代表样本与同名 Manny 源逐帧比较 Local Transform。此前批处理已让这些样本的 `weapon_r` 误差为零，但其余 IK/武器辅助骨并未一起安全写回：

| 样本 | 异常骨 | 最大 Local 位移误差 | 最小四元数点积 | 结论 |
|---|---|---:|---:|---|
| MM `MM_Rifle_Jog_Fwd` | `weapon_l` | `1078.7818` | `0.988195` | 严重损坏，末段持续增长至约 `(-582,-366,831)` |
| MF `MF_Pistol_Jog_Fwd` | `ik_hand_gun` | `118.2219` | `0.589141` | 严重损坏，源为单位 Local Transform，目标中段约 `(-27,2,115)` |
| MF `MF_Rifle_Jog_Fwd` | `ik_hand_l` | `8.0769` | `0.994790` | 有显著偏移，需结合骨架参考姿势判断 |
| MF `MM_Rifle_Crouch_Walk_Fwd` | `ik_hand_l` | `8.5682` | `0.997282` | 有显著偏移 |
| MF `MM_Rifle_Jump_Start` | `ik_hand_l` | `8.2743` | `0.993552` | 有显著偏移 |

`MM_Rifle_Jog_Fwd.weapon_l` 和 `MF_Pistol_Jog_Fwd.ik_hand_gun` 已明显超出任何合理的男女体型适配范围，并且它们正好只在移动底姿参与时出现。该证据与用户的条件矩阵“静止开火/换弹正常，叠加移动后双手扭曲”高度一致。当前主假设更新为：橡皮手主要来自正式 Locomotion Sequence 的辅助骨重定向损坏，ABP 只是忠实消费了坏轨道。

### 下一断点

1. 批量只读审计正式 Locomotion 中实际有 Manny 同名来源的 Sequence，统计 `weapon_l/ik_hand_gun/ik_hand_l/ik_hand_r` 严重异常范围。
2. 对照三套 Skeleton 的参考 Local Transform、Virtual Bone 与辅助骨父子关系，区分固定适配偏移与逐帧坏轨。
3. 先对一个移动代表样本做隔离候选修复和 PIE A/B；未通过前不全量覆盖。

### Locomotion 批量只读审计

已扫描所有正式 CC `Locomotion` 下存在同名 Manny 来源的 AnimSequence：MF `209` 个、MM `215` 个。按任一条件 `Local 位移误差 > 25`、四元数点积 `< 0.9`、Scale 误差 `> 0.5` 统计，MF 命中 `81` 条“资产-骨”异常，MM 命中 `82` 条。该数字包含 Unarmed 中当前武器链不会消费的辅助骨，不能直接当作待修文件数；但武器运行时引用已由 Asset Registry 复核：

- `ABP_PistolAnimLayers_Feminine` 直接依赖异常的 `MF_Pistol_Jog_Fwd/Start/Stop/Pivot` 等序列。
- `ABP_RifleAnimLayers` 直接依赖异常的 `MM_Rifle_Jog_Fwd`。
- MF Pistol 的大量 Idle/Walk/Jog/Turn 序列都出现约 `118-160` 的 `ik_hand_gun` 位移误差，不是单个文件偶发。
- MM Rifle 当前最严重的是 `MM_Rifle_Jog_Fwd.weapon_l` 末段异常；Rifle Lean 序列还残留明显 `weapon_r` 偏差。

### Skeleton 参考数据

三套 Skeleton 的 `weapon_r/weapon_l` 参考 Local Transform 一致，但 Retargeting Mode 不同：Manny 为 `AnimationScaled`，男女 CC 为 `Animation`。`ik_hand_gun` 的参考 Local Transform 会随 CC 体型变化，Manny 为 `Animation`，男女 CC 为 `AnimationScaled`；`hand_r` 则从 Manny 的 `AnimationScaled` 变为 CC 的 `AnimationRelative`。这说明几厘米的固定 IK 偏差不能直接按 Manny 原值覆盖，必须区分参考姿势适配；但 `1078` 和 `118-160` 的逐帧/整组异常远超参考骨架差异，仍可判定为坏轨。

当前下一步调整为：先修复并验证一个“无争议的逐帧尖峰”样本 `MM_Rifle_Jog_Fwd.weapon_l`；PIE A/B 通过后，再为 `ik_hand_gun` 设计考虑 CC 参考姿势的安全重建，而不是简单把所有 IK 骨逐帧抄成 Manny 数值。

## 2026-08-20 阶段三：男性 Rifle Jog 单轨道最小写回

在写入前重新复读 Git，正式目标 `MM_Rifle_Jog_Fwd.uasset` 仍是干净资产，未与用户现有修改重叠。通过 `SafeRetargetWriteback.migrate_mannequin_local_tracks` 只写回以下一条局部骨轨：

- 源：`/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd`
- 目标：`/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd`
- 唯一写回轨道：`weapon_l`

保存后的逐帧验证结果：

- 最大 Local 位移误差：从 `1078.781795` 降至 `2.31449e-7`
- 最小四元数点积：`0.9999999999998345`
- 最大 Scale 误差：`0`
- Git 只新增这一份 AnimSequence 修改；Montage、AnimBP、Skeleton、曲线、Notify 和用户实验资产均未被本次调用修改

该结果只证明安全工具可以消除已确认的坏轨，还不是运行时修复结论。

用户随后在分屏 PIE 中完成男性 Rifle 向前跑动时的视觉 A/B：静止 Fire/Reload 仍正常；向前跑动 Fire 仍有左臂扭曲；向前跑动 Reload 仍出现双臂和武器空间的灾难性拉伸。结论是本次单轨数据修复没有产生可见的实质改善。`weapon_l` 的 `1078.78` 位移是确定的坏数据，但它不是当前表现的充分根因，也尚未证明它是截图帧实际消费的关键轨道。

当前精确断点：保留该单样本为未提交候选，不扩展批量写回；运行时读取截图场景真正激活的 Locomotion Sequence、Reload Montage、IK Alpha 和 Disable Hand IK 曲线值，再只审计实际参与混合的辅助骨轨。若证明该样本并未参与当前状态，应移出修复提交；若参与但仍失败，则继续追踪混合链中的其他实际输入。

## 2026-08-20 阶段四：Pistol Reload 脱手修复后的静止橡皮手

### `weapon_r` 的视觉 A/B

正式 MF Pistol Reload 的非 Additive、Additive 两条 Sequence 仅写回 `weapon_r` 后，用户完成分屏 PIE 验收：手枪不再脱离右手，但女性角色静止换弹时的手臂橡皮形变仍存在。由此可将问题拆成两个独立层次：

- `weapon_r` 坏轨是手枪脱手的直接原因，修复有效。
- 静止换弹橡皮手不是 `weapon_r` 单轨能够解释的问题，不能继续把其他辅助骨无差别照抄 Manny。

这两份正式 Pistol Sequence 仍与用户本轮重定向实验重叠，当前只作为未提交候选保留；未完成最终视觉闭环前不得混入调查文档提交。

### Additive、曲线与资产级预览

Manny 源、正式 MM、正式 MF 的 Pistol Reload Base/Additive 六条 Sequence 已逐项复读：长度均为 `2.0s`，Base 均为非 Additive，Additive 均为 `Local Space Base`、`Local Anim Frame`、第 `0` 帧，Root Motion 设置一致。Additive 元数据不是当前差异。

UE 5.8 的动画数据使用 `AnimationSequencerDataModel`；通过具体 DataModel 子对象复读后确认：Pistol Reload 的 Manny Base/Additive 与正式 MF Base/Additive 都是 `0` 条 Float Curve。Pistol Reload 没有 `DisableLHandIK` 是 Lyra 原资产事实，不能把服务最初返回的空列表误判为目标资产漏迁，也不能未经 A/B 就把 Rifle 的曲线硬套给 Pistol。

正式 MF Pistol Reload Montage 在 Montage 编辑器中以两条真实 Slot Track 预览时，人物双臂没有出现运行时的灾难性拉伸；其 Base、Additive 单资产缩略预览也一致。Virtual Bone 姿势求值确认三套骨架都包含 `VB IK_Hand_L_weaponSpace`，MF 目标在 `1.0s` 时该 Virtual Bone 能得到有效世界变换，并非缺失或恒等失效。因此当前范围继续收窄到运行时 `HandIKRetargeting -> CopyBone -> TwoBoneIK` 叠加，以及 CC 手臂/Twist 骨架适配，而不是 Montage 容器、Additive 类型或 Virtual Bone 是否存在。

### 三枪对照的新边界

正式 MF Rifle Reload 的 Base/Additive 各有一条 `DisableLHandIK` 曲线，四键为第 `10/14/20/23` 帧的 `0/1/1/0`，与已有审计一致；正式 MF Shotgun Reload 与 Pistol Reload 的 Base/Additive 均为 `0` 条 Float Curve。Shotgun 静止换弹正常而 Pistol 静止换弹异常，说明“是否存在 DisableLHandIK”不是单独充分条件。若下一轮做关闭左手 IK 的实验，只能作为 Pistol 对 CC 骨架的适配性 A/B，不能写成 Lyra 原版缺漏。

三枪 Base Sequence 的 Manny -> MF 手臂局部旋转误差量级也接近：Pistol 并没有比 Rifle/Shotgun 多出数量级异常。Pistol 的特殊性更可能来自其姿势在 CC 前臂 Twist/TwoBoneIK 约束下进入不良区间，而不是普通 upperarm/lowerarm/hand 轨道出现了类似 `1078` 的尖峰。

### 工具禁区与下一断点

本阶段发现以下 UE 5.8/VibeUE 只读入口会在这些 AnimBP/运行时对象上触发原生访问冲突，后续不得重试：

- `AnimPoseExtensions.get_anim_pose_at_time(AnimMontage, ...)`：AnimMontage 不支持该评估入口，会触发引擎断言；只对 AnimSequence 使用。
- `BlueprintService.list_variables(AnimBlueprint, inherited=True)`：在正式 Item Anim Layer 上触发 `0xC0000005`。
- `ObjectIterator(AnimInstance)`：PIE 中遍历 AnimInstance 触发 `0xC0000005`。
- `ObjectTools.list_properties` 直接读取正式 AnimBlueprint 资产：MCP Transport 随后关闭，编辑器最终以 Python/ToolsetRegistry 调用栈崩溃。

崩溃前没有执行任何 `.uasset` 保存或曲线写入。编辑器重启并确认 8000 端为 NewWorldOrder 后，已通过验证安全的 `AnimSequenceService` 为正式 MF Pistol Reload Base/Additive 各加入一条临时 A/B 曲线：`DisableLHandIK` 在 `0.0s` 与 `2.0s` 均为 `1`，即整个 Montage 窗口关闭左手 TwoBoneIK。保存复读确认两条 Sequence 都只有这条新增曲线，键位为第 `0/60` 帧、值为 `1/1`；Montage、AnimBP、Skeleton 和其他武器没有被该调用修改。

该曲线明确是 CC 适配性实验，不是 Lyra 源数据迁移，当前与 `weapon_r` 修复一起保留为未提交视觉候选。下一断点是用户在分屏 PIE 中验证女性 Pistol 静止 Reload：若橡皮手消失，再缩窄曲线窗口并检查左手重新抓握；若无改善，立即移除两条临时曲线，停止辅助骨写回，转向 CC forearm twist 与 TwoBoneIK 约束的运行时取证。不再使用上述反射/遍历入口，也不批量覆盖八个 Montage。

## 2026-08-20 阶段五：重定向后的 `weapon_r` 批量修复与范围收口

用户提交 `2459cfe` 重新重定向男女八个角色 Montage 及其引用 Sequence 后，Montage 的声音、Notify 与 Timing 已回到 Lyra 结构，但 Retargeter 再次损坏了正式 Action Sequence 的动态 `weapon_r`。逐帧对比 Manny 同名源后，男女三枪 Fire/Reload 中有 16 条 Sequence 出现明显 Local Transform 偏差；例如 MF Pistol Fire 最大位移误差约 `13.39`、MM Pistol Reload 约 `63.69`，最低旋转四元数点积接近零。

本轮只使用 `SafeRetargetWriteback.migrate_mannequin_local_tracks(..., ["weapon_r"])` 写回明确列出的 `weapon_r` 轨道，未改 Montage 容器、曲线、Notify、Additive/Base Pose 或其他骨轨。16 条 Fire/Reload Sequence 保存后的最大位移误差为 `0`、最大缩放误差为 `0`、最小四元数点积约为 `1`。用户随后在分屏 PIE 中确认三枪 Fire/Reload 的武器脱手已经消失，开火和换弹恢复正常；女性 Pistol 静止 Reload 的右臂橡皮手仍存在，它与武器挂点脱离是两个问题。

Equip/Unequip 的真实引用关系也已复读：Pistol Equip 与 `AM_Generic_Unequip` 共用 `MM_Pistol_Equip`、`MM_Pistol_Equip_Additive`，Rifle 与 Shotgun Equip 共用 `MM_Rifle_Equip`、`MM_Rifle_Equip_Additive`。因此男女共 8 条 Equip Sequence 已用相同方式只修复 `weapon_r`，保存后同样得到位移误差 `0`、缩放误差 `0`、旋转点积约 `1`。这覆盖三枪 Equip 和丢弃最后一把武器时实际播放的 Generic Unequip；最终视觉复验留到编辑器中的真实切枪与丢枪输入。

本阶段不再继续扩大橡皮手调查。已知遗留问题记录为：女性 Pistol 静止 Reload 右臂扭曲，以及移动、跳跃、蹲伏叠加武器动作时可能出现的 CC 手臂/IK 适配问题。后续只有在它们重新成为版本阻断项时，才从 CC forearm twist、TwoBoneIK 和实际激活的 Locomotion Sequence 继续取证；不得把本次 `weapon_r` 修复误写成所有橡皮手问题已经解决。

### RTG 验证结论

用户创建的 `RTG_Mannequin_ChenHaoYu` 与 `RTG_Mannequin_ShenWanYun` 都沿用 Manny Retargeter 的 9 个 Op，只替换目标 IK Rig。两者没有 `weapon_r/weapon_l` 专用链或精确局部轨道复制步骤。曾在男性 RTG 的隔离候选中增加 `Pin Bones`，以 `CopyLocalPosition` 尝试复制 `weapon_r/weapon_l`；诊断输出仍有 `weapon_r` 位移误差约 `1.5682`、旋转点积约 `0.980638`，`weapon_l` 还产生约 `1.5588` 的缩放误差。失败 Op 已从 RTG 移除并保存，女性 RTG未修改。

UE 5.8 的 Pin Bone 此模式按当前全局子父向量工作，旋转模式也不是逐帧精确 Local Rotation；FK Chain 又没有 Local Translation 复制模式。因此当前 RTG 内建 Op 不能保证 Manny 动态 `weapon_r` 原样落到 CC。权威安全流程应是“Retarget 到隔离输出 -> 验证返回路径和 Skeleton -> 对明确的辅助骨执行局部轨道安全写回 -> 再验证正式 UObject”，而不是依赖 `overwrite_existing_files=True` 或在 RTG 中盲目复制全部 IK/Virtual Bone。Virtual Bone 由 Skeleton 关系派生，也不应当作普通动画轨道批量复制。

失败实验留下一个未跟踪诊断资产 `/Game/Developers/CodexDiagnostics/RetargetWeaponPinTest/ZZ_MM_Rifle_Reload_WeaponPinTest`。Asset API 复读为零引用，但编辑器仍持有对象，非 Force 删除失败；它不得进入提交，编辑器重启后由用户在 Content Browser 手工清理。

## 2026-09-10 阶段六：CC PostProcess 对橡皮手的阶段性结论

本节更新截至 2026-08-20 的历史状态，不删除此前关于 `weapon_r`、Locomotion 辅助骨和 `TwoBoneIK` 的取证。新的 CC 后处理记录见 [CC PostProcess 适配记录](../AnimationMigration/CC_PostProcess_适配记录.md)。

### 已确认的变化

- `ABP_ChenHaoYu_CombinedPostProcess` 和 `ABP_ShenWanYun_CombinedPostProcess` 已分别接入 ChenHaoYu、ShenWanYun 网格；它们保留原皱纹 AnimBP，再链接对应的 CC 姿势修正 ABP。
- 在 `MF_Pistol_Idle_ADS_AO_CD`、`MM_Pistol_Idle_ADS_AO_CD` 和默认姿势上，组合后处理启用/禁用的 A/B 已从正面、侧面和背面检查。代表性姿势中的手腕扭曲，以及之前达到整条手臂/腿崩坏级别的肘、膝错误，当前没有再复现。
- 之前把 Lyra `lowerarm_correctiveRoot`、`calf_correctiveRoot` 强行映射到 `elbowsharebone`、`kneesharebone` 的路径已关闭。当前只保留经过 CC 骨架语义确认的上臂、前臂、大腿和小腿 twist 分配。

### 对“橡皮手”的当前解释

当前最合理的阶段性解释是：旧问题不只来自 `weapon_r`。`weapon_r` 写回解决了武器脱手，而 CC PostProcess 补回了重定向结果缺失或不匹配的 twist/corrective 分配，因此同一条手臂变形链中的手腕扭曲和疑似橡皮手都可能同时改善。这个判断与本轮 A/B 一致，但仍属于根因推断，不能反向证明历史上每一种橡皮手都由同一个缺口造成。

任务状态因此标记为：`橡皮手目前暂时看上去已解决（阶段性），待扩展回归`。这不是永久关闭，也不覆盖本调查此前的历史结论。

### 尚未关闭的回归门槛

1. 男女 Rifle、Pistol、Shotgun 的 Fire、Reload、Equip、Unequip。
2. 静止之外的移动、跳跃、蹲伏、瞄准叠加，以及真实单人 PIE、分屏和 Listen Server。
3. 正面、背面、左右侧面和不同 LOD 下的手腕、肘、膝、脚踝、手部握枪接触。
4. 后处理启用/禁用的成对截图，确认基础动画异常、后处理异常和武器挂点异常不再互相混淆。

在上述回归完成前，不把本问题写成“所有动画已修复”，不删除旧调查证据，也不恢复对 Mannequin corrective 骨的盲目映射。
