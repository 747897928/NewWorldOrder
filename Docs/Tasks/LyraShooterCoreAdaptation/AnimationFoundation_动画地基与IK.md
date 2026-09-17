# Lyra 动画地基、虚拟骨骼与武器 IK

## 文档目的

本文记录 NewWorldOrder 把 Lyra 动画层迁到 CC 男女主骨架时必须保留的骨架契约。它既是开发笔记，也是后续重新重定向、新增枪械和玩家验收时的检查清单。

本文只描述已经通过 Lyra 资产、项目资产和运行节点核对的事实。`weapon_r`、虚拟骨骼和主 AnimBP 类型迁移已经完成；方向切换、脚步、握枪、分屏和 Listen Server 的当前验收结果以 `Verification_验收.md` 的最新日期章节为准，不能再从本文的历史诊断段落推断未完成。

## 一条完整的姿势链

```text
主 AnimBP 采集移动状态
  -> Idle / Start / Cycle / Stop / Pivot / Jump / Fall / Land
  -> 当前武器 Linked Anim Layer 提供对应动画
  -> Orientation Warping 调整移动朝向
  -> Stride Warping 调整步幅
  -> AimOffset 和左手武器姿势
  -> FullBody_SkeletalControls
       HandIK Retargeting
       weapon space 左手目标
       TwoBoneIK
       FootPlacement
       LegIK
  -> 表演 Pose Slot
  -> HairAnimationLayer
  -> ShoeAnimationLayer
  -> 最终角色姿势
```

任何一段都不能脱离前后契约单独照搬。动画重定向成功只代表目标骨架能播放序列，不代表辅助骨骼、虚拟骨骼、IK 目标和曲线也已迁移。

## Lyra 的三个虚拟骨骼

Lyra Mannequin Skeleton 当前有以下虚拟骨骼：

- `VB IK_Hand_R_chestSpace`：`spine_05 -> hand_r`
- `VB IK_Hand_L_chestSpace`：`spine_05 -> hand_l`
- `VB IK_Hand_L_weaponSpace`：`weapon_r -> hand_l`

虚拟骨骼不是蒙皮骨骼，也不会凭空创造一个新的动画轨道。它由“Source 骨骼坐标系”和“Target 骨骼位置”在运行时计算一个辅助变换。

`VB IK_Hand_L_weaponSpace` 的含义可以读成：把左手 `hand_l` 表达到 `weapon_r` 的坐标系中。Lyra 随后把这个变换复制到 `ik_hand_l`，TwoBoneIK 再让左臂追随该目标。这样身体、右手和武器发生运动时，左手仍能稳定跟随武器空间，而不是只播放一条容易漂移的手臂动画。

## 逐武器左手姿势的数据入口

2026-08-12 重新读取 11000 端 Lyra 后确认，官方没有在 WeaponInstance 或 ItemDefinition 上增加通用 `LeftHandGripOffset`。逐武器差异由 `ABP_ItemAnimLayersBase` 已有的 `LeftHandPose_OverrideState` 承担：

1. `LeftHandPose_Override` 是 AnimSequence 数据入口。
2. `EnableLeftHandPoseOverride` 决定是否启用。
3. `SetLeftHandPoseOverrideWeight` 根据启用状态与动画曲线计算权重。
4. `LeftHandPose_OverrideState` 只把指定姿势按骨骼混入当前 Pose。
5. 混合结果继续进入下游 `weapon_r -> ik_hand_l -> TwoBoneIK`，不会绕开原 IK 契约。

Lyra Rifle/Pistol 关闭该覆盖；Shotgun 启用并引用 `MM_Shotgun_Idle_Hipfire`。项目男女正式 CC 已逐项对齐：Rifle/Pistol 关闭，Shotgun 启用并引用同侧 `MM_Shotgun_Idle_Hipfire` 或 `MF_Shotgun_Idle_Hipfire`。男女 Base 的三个相关图均为 `UpToDate`，节点数与连接数分别为 `14/12`、`6/5`、`8/7`。

因此新增枪械需要特殊左手握姿时，应在该枪的同侧 Linked Anim Layer 中配置一条同骨架握姿序列；不要在 `UShootAnimInstance` 恢复旧的世界 Socket 转 Bone Space 代码，也不要新增与动画层并行的 C++ 左手 Transform。只有动画层机制无法表达运行时模块化配件变化时，才另行设计配件级数据入口，并先明确它如何合成到唯一的 `ik_hand_l` 目标。

## weapon_r 为什么不能用 ik_hand_gun 代替

Lyra 的 `weapon_r` 是 `hand_r` 的子骨骼，`ik_hand_gun` 属于独立 IK 层级。二者职责不同：

- `weapon_r`：动画中的武器空间参考，拥有独立动画运动。
- `ik_hand_gun`：手部 IK 目标层级的父节点，用于把左右 IK 手目标组织在同一枪械 IK 空间。

本轮对 Lyra Rifle 动画的实际采样已经证明：

- `MM_Rifle_Idle_Hipfire` 和 `MM_Rifle_Jog_Fwd` 都包含 `weapon_r` 轨道。
- `weapon_r` 相对 `hand_r` 的变换会随时间变化，并非一条静态参考姿势。
- 正式 CC MM/MF 重定向动画没有 `weapon_r` 轨道。

因此以下两种“快速修复”都不正确：

- 直接创建 `ik_hand_gun -> hand_l` 的同名虚拟骨骼。
- 把 AnimGraph 中的 `weapon_r` ModifyBone 或 CopyBone 改指向 `ik_hand_gun`。

前者会丢失真实 weapon space 运动；后者还会缩放或破坏整条手部 IK 层级。正确迁移必须同时补齐物理辅助骨骼 `weapon_r`、对应动画轨道，再创建精确的 `weapon_r -> hand_l` 虚拟骨骼。

## FullBody_SkeletalControls 的实际顺序

项目正式 CC `ABP_ItemAnimLayersBase` 与 Lyra 原图拓扑一致，当前核对到的关键节点为：

1. `HandIKRetargeting`
   - FK：`hand_r`、`hand_l`
   - IK：`ik_hand_r`、`ik_hand_l`
   - 随动层级：`ik_hand_gun`
   - 由 `DisableHandIKRetargeting` 曲线控制反向 Alpha。
2. `CopyBone`
   - Source：`VB IK_Hand_L_weaponSpace`
   - Target：`ik_hand_l`
   - 复制 Component Space 的 Translation 和 Rotation。
3. 左右 `TwoBoneIK`
   - 右手链：`hand_r` 追随 `ik_hand_r`。
   - 左手链：`hand_l` 追随 `ik_hand_l`，并从 Effector 取得旋转。
4. `FootPlacement`
   - 根：`ik_foot_root`
   - 骨盆：`pelvis`
   - 左右脚：`foot_l/r`、`ik_foot_l/r`、`ball_l/r`
   - 结合地面追踪结果和 Plant 状态修正脚底接触。
5. `LegIK`
   - 以 `ik_foot_l/r` 为目标解算 `foot_l/r` 腿链。
   - 由 `DisableLegIK` 曲线控制反向 Alpha。
6. `ModifyBone weapon_r`
   - 由 `ScaleDownWeaponR` 曲线控制，用于特定姿势缩小武器参考骨。

迁移前 CC Base 的编译警告恰好是 `VB IK_Hand_L_weaponSpace` 和 `weapon_r` 缺失。这不是可忽略的命名噪声，而是左手脱离护木、开火橡皮手臂的重要结构性证据。当前该骨架契约已补齐；若 PIE 仍扭曲，应继续查动画状态、Warping 输入、ControlRig/IK 权重和 Montage 叠加，不应再次用替代骨名打补丁。

## Linked Layer 主 AnimBP 类型契约

IK Retargeter 会复制 AnimBP 图和动画资产，但不会自动把蓝图类类型引用迁移到目标目录。项目曾出现以下隐蔽状态：

- 正式 Item Layer 位于 `/Game/Characters/Heroes/CC/MM|MF`，Target Skeleton 也正确。
- `GetMainAnimBPThreadSafe` 的返回签名和 DynamicCast 仍指向 `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base`。
- 图内变量、函数 MemberReference 和 Property Access 已解析输出也继续保存旧类。
- 编辑器错误只显示两个同名的 `ABP Mannequin Base Object Reference` 不兼容，肉眼难以区分实际包路径。

运行时后果是 Linked Layer 每帧取得主 AnimInstance 后 Cast 失败，速度、方向、蹲伏、跳跃和 IK 数据无法可靠传入武器层。正确迁移必须同时处理：

1. 所有 Function Result 的 `UserDefinedPins` 返回类型。
2. 主 DynamicCast 的目标类型。
3. 指向旧主类变量和函数的 `FMemberReference`。
4. 同名旧类的 Property Access 输出 Pin。
5. 函数签名改变后的所有调用节点刷新。

项目维护的 `AnimationAssetFixer.RetargetMainAnimBlueprintType` 按上述顺序执行，并允许从中断后的“Result 仍旧、Cast 已新”状态继续。

## Linked Layer 父类继承契约

复制或重定向数据型 AnimBP 时，目标 Skeleton 和节点动画覆盖正确，并不代表父 AnimBP 已经迁移。项目曾出现以下第二层隐蔽状态：

- 正式 CC Rifle/Pistol/Unarmed 的 Target Skeleton 和动画覆盖均已指向 CC。
- 它们的 Blueprint Parent Class 仍是 Mannequin `ABP_ItemAnimLayersBase`。
- 正式 CC Shotgun 又继续继承 Mannequin Rifle 子层。
- 因此运行时真正实例化的子层会绕过已修复的正式 CC Base，导致同名类类型不兼容、旧循环状态和错误主 AnimBP 数据继续进入姿势链。

正确父类链必须按性别完全闭合：

```text
CC/MM ABP_ItemAnimLayersBase
  -> CC/MM Rifle | Pistol | Unarmed
  -> CC/MM Rifle -> CC/MM Shotgun

CC/MF ABP_ItemAnimLayersBase
  -> CC/MF Rifle | Pistol | Unarmed
  -> CC/MF Rifle -> CC/MF Shotgun
```

2026-08-06 已把男女八个正式子层重定父类到上述同侧 CC 链，并保留原有 CC Node Asset Override。随后重新编译 `/Game/Characters/Heroes/CC` 下全部 18 个 AnimBP/动画接口资产，均为 `BS_UP_TO_DATE`。运行时也已确认男女 Shotgun 的 Linked Layer 实例分别来自同侧正式 CC 路径。

## 一次性状态的循环规则

`StanceTransition` 和 `IdleBreak` 依赖自动剩余时间规则返回 `Idle`，其 Sequence Player 必须是非循环。若勾选 Loop，UE 5.8 会明确警告自动过渡源是循环动画，并可能让角色停在过渡或扫视姿势。

项目维护的 `AnimationAssetFixer.SetSequencePlayerLooping` 只接受精确 ABP、图名和节点对象名；命中不唯一或类型不符就拒绝修改。2026-08-06 已只对正式 CC 男女 Base 的这两个一次性节点关闭循环，重新编译后对应警告消失。

## Orientation Warping 与 Stride Warping

`Orientation Warping` 解决的是“动画朝向”和“角色实际移动方向”不一致的问题。例如角色面向镜头前方却向左移动时，可以旋转腿部和根部运动方向，而不用为任意角度都制作一条动画。

`Stride Warping` 解决的是“动画原始步幅”和“游戏实际速度”不一致的问题。它根据速度、播放速率和腿链调整脚步距离，减少滑步。

两者的输入错了会产生完全不同但外观相似的问题：

- Orientation 输入或 Pivot 状态错：快速反向时身体扭转、卡在极端侧倾或腿部折叠。
- Stride 比例、速度单位或脚 IK 契约错：双脚靠拢、踮脚、膝盖被强拉、移动像滑行。
- FootPlacement/LegIK 在错误基础姿势上继续求解：会把原本的小偏差放大成明显畸变。

所以排查顺序必须是：先确认状态机和基础动画，再确认 Orientation/Stride 输入，最后启用 FootPlacement/LegIK。不能用更多 IK 去补偿错误的 locomotion。

## Jog Lean Additive 的正确重建流程

男女 Rifle 的 `MM_Rifle_Jog_Leans_Left` 与 `MM_Rifle_Jog_Lean_Right` 曾在目标 CC 目录中保留源 Additive 标记后直接重定向。结果不是普通的侧倾调参误差，而是目标骨架的绝对关键帧被错误解释成 Additive 差值：女性 Left/Right 一度姿势和字节近似相同，骨盆、脊柱出现约 100 到 168 度异常旋转，快速反向时会全身折叠。

单变量 A/B 已排除 Orientation Warping、FootPlant ControlRig、RootYaw/Aim；把 Rifle Jog Lean 的 Apply Additive Alpha 临时设为 0 后折叠消失，直接锁定到 Lean 源资产。正式修复不是继续叠加补丁，而是重建损坏资产：

1. Manny 到 CC IK Retarget 时设置 `retain_additive_flags=False`，先得到目标骨架的正常绝对姿势。
2. 检查目标 Left、Center、Right 姿势互异且旋转合理。
3. 只在目标 Left/Right 上恢复 `Local Space` Additive。
4. 男女分别使用同侧 CC Center AnimSequence 作为 Base Pose。
5. 恢复 Start、Cycle、Pivot 图中的 Apply Additive Alpha 为 1，编译正式主 AnimBP。

已按该流程覆盖并验证四个正式资产：

- `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/MM_Rifle_Jog_Leans_Left`
- `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/MM_Rifle_Jog_Lean_Right`
- `/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Leans_Left`
- `/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Lean_Right`

2026-08-08 已在 TestMap 给男女角色装备 Rifle，并把键鼠 P1 仅在 PIE 运行时依次控制男女 Pawn，以真实 `IA_Move` 执行连续左右快速反转、前后混合和 Pivot。三张阶段截图位于 `Saved/Diagnostics/LeanFix_Phase_07.png`、`LeanFix_Phase_15.png`、`LeanFix_Phase_23.png`；女性灾难性全身折叠未再复现。该项可以退出根因调查，后续按 Foot Controls、Hand Controls 顺序继续。

## Foot Controls 单变量结论

2026-08-08 已在正式男女 CC Item Base 上依次对 `FootPlacement`、`LegIK`、`Stride Warping` 做单变量 A/B；每次只关闭一个节点，截图后立即恢复并重新编译。三项关闭均未单独消除玩家看到的踮脚、双脚靠近或滑步感，因此不能把任一运行节点当作唯一根因，也不应增加新的 IK fallback。

`GroundDistance` 运行采样符合 Lyra 语义：地面为 `0`，跳起约为 `193`，接近落地约为 `49.7`，落地后恢复 `0`。CC 的 `ik_foot_l/r`、`foot_l/r`、`ball_l/r` 契约存在，目标 Jog 的脚部相位和轨迹也保持一致；剩余观感应归入 CC 比例、参考站距和 Retarget Pose 调优。诊断证据位于：

- `Saved/Diagnostics/FootAB_Baseline.png`
- `Saved/Diagnostics/FootAB_FootPlacementOff.png`
- `Saved/Diagnostics/FootAB_LegIKOff.png`
- `Saved/Diagnostics/FootAB_StrideWarpingOff.png`

所有 A/B 节点和 Stride Warping 连线均已恢复，男女正式蓝图保持 `BS_UP_TO_DATE`。

## Fire、Reload 与 weapon-space 轨道修复

用户验收中的 Shotgun 开火橡皮手并非 TwoBoneIK 骨长被拉伸。单变量取证确认男女运行时上臂和前臂长度始终正常，`HandIK Retargeting` 关闭也不能消除问题。直接根因是正式 CC Action Fire 资产在重定向后丢失了 Lyra 源动画中逐帧变化的 `weapon_r` 轨道，同时六个正式 Fire Montage 被错误放在 `UpperBodyAdditive`，而 Lyra 源 Montage 使用 `FullBodyAdditivePreAim`。

正式修复包含两部分：

1. 从匹配的 Manny Rifle、Pistol、Shotgun Fire 源动画逐帧采样完整局部姿势，把 `weapon_r` 写回男女六个 CC Fire AnimSequence。
2. 把男女六个 CC Fire Montage 的槽恢复为 Lyra 的 `FullBodyAdditivePreAim`，使 Montage 在 Aim 和最终 HandIK/CopyBone/TwoBoneIK 之前参与姿势链。

随后又核对到 Reload、Reload Additive、Equip 和 Equip Additive 也丢失了同一条动态 `weapon_r`。已按 Lyra 源动画逐帧补回以下正式资产：

- 男女 Rifle、Pistol、Shotgun 的 `Reload` 与 `Reload_Additive`，共 12 个 AnimSequence。
- 男女 Rifle、Pistol 的 `Equip` 与 `Equip_Additive`，共 8 个 AnimSequence；Shotgun Equip 按既有配置复用 Rifle Equip 双轨。

逐帧验证的最大位移差为 `0`，旋转四元数点积下限大于 `0.9999999999999`。`weapon_l` 在这些源动画中基本保持参考姿势常量，不承担当前 weapon-space 动态驱动，因此没有为了“轨道数量一致”做无效复制。

此前真实 Enhanced Input -> GAS 验收曾覆盖男女 Shotgun Fire，以及女性 Rifle/Pistol Fire、三枪 Reload，但用户随后在正式 Montage 预览、切枪瞬间和移动开火中稳定复现双肘回折。骨长稳定只能排除缩放拉伸，不能证明最终姿势正确；因此下面旧截图只作为调用链与骨长证据，不再作为 Hand Controls 通过证据：

- `Saved/Diagnostics/HandFix_Gameplay_Shotgun_Male.png`
- `Saved/Diagnostics/HandFix_Gameplay_Shotgun_Female_Clean.png`
- `Saved/Diagnostics/HandFix_Gameplay_Rifle_Female_Clean.png`
- `Saved/Diagnostics/HandFix_Gameplay_Pistol_Female_Clean.png`
- `Saved/Diagnostics/HandFix_Gameplay_Shotgun_Reload_Female.png`
- `Saved/Diagnostics/HandFix_Gameplay_Rifle_Reload_Female.png`
- `Saved/Diagnostics/HandFix_Gameplay_Pistol_Reload_Female.png`

曾在女性截图左侧出现的灰色“巨臂”经可见性和空间 A/B 证明来自 TestMap 中与女性相距仅 `68.25 cm` 的 EnemyBot 进入相机近裁剪，不是女性骨骼再次拉伸。把敌人仅在 PIE 中移开后，三枪干净截图不再出现该几何体；地图出生点占用仍需单独修正。

### Fire Additive 的第二次根因纠错

2026-08-09 用户直接打开 7 个正式 Montage 后发现，错误姿势已经存在于 Montage/源动画预览，而不是只在运行时 IK 叠加后出现。进一步对照 Lyra 11000 确认 Rifle、Pistol、Shotgun Fire 源序列都是 Mesh Space Additive，并分别以自身第 `22/24/28` 帧作为 Base Pose。目标 CC 序列虽然保留了这些元数据，但它们此前带着源 Additive 标记直接重定向，存在与 Jog Lean 相同的“目标绝对关键帧被当作 Additive 差值”风险。

当前正式重建契约：

1. Manny Fire 到对应 CC 男女目标执行 IK Retarget，必须设置 `retain_additive_flags=False`。
2. 目标 Fire 恢复 Mesh Space Additive、`ABPT_ANIM_FRAME`、同名目标序列自引用 Base Pose，以及 Rifle/Pistol/Shotgun 的参考帧 `22/24/28`。
3. 重定向会覆盖后补的辅助骨轨道，随后必须重新逐帧迁移 `weapon_r`。
4. Fire Montage 保持原路径与引用，Slot 必须是 `FullBodyAdditivePreAim`。
5. Shotgun Equip 恢复与同侧 Rifle Equip 一致的双轨：`UpperBody` 基础动作加 `UpperBodyAdditive` Additive 动作；诊断期移除基础轨不能进入正式交付。
6. 只有真实输入产生弹药扣减、记录到正确 Montage，并通过男女静止/移动/快速切换截图，才可重新标记通过。

### 2026-08-13 男性 Shotgun Fire 最终纠错

旧的动态 `weapon_r` 与 Montage Slot 修复是必要地基，但不是用户截图中“开火时枪飞到头顶”的最后根因。再次对比 Lyra、女性 CC 和男性 CC 的同名 `MM_Shotgun_Fire` 后发现：

- Lyra 与女性 CC：`Mesh Space Rotation Offset Additive`、`Anim Frame` Base Pose、自身第 `28` 帧。
- 男性 CC：错误保存为 `AAT_NONE / ABPT_NONE`，无 Base Pose。

该男性绝对姿势随后进入 `FullBodyAdditivePreAim`，会被当作加法数据再次叠加，从而把手臂和武器推到头顶。正式修复只恢复动画资产的 Lyra 元数据：Mesh Space Additive、自身 Base Pose、第 `28` 帧；不改 C++、Equipment、Socket、Actor Transform 或 AnimGraph。

修复后同一 `0.12s` 冻结帧恢复肩部持枪。分屏真实 `IA_Attack` 使 Shotgun `7 -> 6`，角色和武器 Montage 同时位于 `0.178007s`；Listen Server 远端客户端 `8 -> 7` 后，客户端与服务器模拟代理均同步在约 `0.342s`。因此本问题已从 Actor Transform 调查中退出。

## Equip、Unequip 与短 Montage 淡出修复

项目现已按 Lyra `B_WeaponInstance_Base` 的正式顺序接通武器表现：选取男女对应 Item Anim Layer，调用 `LinkAnimClassLayers`，再在角色主 Mesh AnimInstance 播放 ItemDefinition 为该 Skeleton 配置的 Equip/Unequip Montage。`UShootWeaponInstance::OnEquipped/OnUnequipped` 负责入口，`UShootRangedWeaponInstance` 从 `FShootWeaponCharacterMontageSet` 选择 Fire、Reload、Equip 或 Unequip；女性 Mutable 主 Mesh 使用瞬态 Skeleton 时，仍通过同 Pawn 的正式身体组件匹配 `ShenWanYun_Skeleton`，不会退回 Manny 或跨性别 Montage。

2026-08-09 对用户新截图重新取证后，修正了此前“Equip 双肘只是官方姿势，无需处理”的结论。Lyra 11000 的 Rifle/Pistol/Shotgun `WID_*` 均使用 `weapon_r` 与 Z 轴 `-90` 度相对旋转；项目旧配置却是 `weapon_socket_hand_r + Identity`。三份正式 `BP_Equipment_*` 已对齐 Lyra。对齐后，横移 Fire 的枪体和双手空间稳定，但 Equip 开头 `0.0-0.6s` 在 CC 最终表现上仍不可接受。

项目因此在 `UShootWeaponInstance` 增加数据字段 `EquipMontageStartPosition`，三把 `B_WeaponInstance_*` 均配置为 `0.6s`。运行时仍在同一 `OnEquipped` 调用链即时链接动画层和显示武器，只把 Montage 起播位置裁到已逐帧验证的安全区间；没有永久锁定左手 IK，也没有按性别、枪型或玩家编号写条件分支。2026-08-12 复读发现 MM `AM_Shotgun_Equip` 当前在 `0.0001s` 有一枚 `AN_ShootPlayWeaponMontage`，因此旧的“男女六个 Equip Montage 全部零 Notify”结论不再成立；该用户动画资产未在本轮修改，另行纳入 Shotgun 开火旋转调查。

2026-08-12 又从 11000 端完整复读了 Lyra 原生 Equipment/WeaponInstance、`B_WeaponInstance_Base` EventGraph/宏、WeaponInstance CDO、WID 挂点和 Montage/Sequence 元数据。项目的 Item Anim Layer 选择与 Link 顺序、`weapon_r/-90`、双轨 Additive 契约、Generic Unequip 参数和动态 `weapon_r` 均与 Lyra 对齐。分屏男女 Rifle/Pistol/Shotgun 真实拾取与 Listen Server 远端 Rifle 真实拾取在 `0.6001s` 均未复现骨骼拉伸；把同一动作查看到 `0.3s` 时出现的双肘外翻来自 Lyra 源动作本身。Generic Unequip 在分屏与网络两端的 `0.25s` 姿势也保持正常，Host/另一 LocalPlayer 没有串播。因此当前 `0.6s` 是有逐帧证据的 CC 表现适配，不是调用链或重定向错误的替代品。

男女 Generic Unequip 由同侧 Pistol Equip 的 `0.1-0.6` 秒片段组成。最初副本虽然 Track、Section 和片段区间正确，却错误保留了长 Equip Montage 的 `RateScale=1.1`、Blend In `0.25/Hermite Cubic`、Blend Out `0.40/Hermite Cubic`。在总长仅 `0.5` 秒时，自动 Blend Out 约在 `0.06` 秒触发，造成“第二帧消失”的假象。与 Lyra 11000 原资产逐字段对照后，男女正式资产均恢复为：

- `RateScale=0.9`
- Blend In：`0.20`、`Cubic`
- Blend Out：`0.30`、`Cubic`
- `BlendOutTriggerTime=-1`、自动 Blend Out 开启
- Default Section：`0.0-0.5`

帧级验证中，男女 Montage 均从 `0.0` 连续推进到约 `0.23` 秒才按 Lyra 设置进入 Blend Out。真实输入验收进一步覆盖：

- 男性本地玩家：键盘 `G -> IA_WeaponDrop -> ASC/GAS -> RuntimeOnly QuickBar -> OnUnequipped`，ActiveIndex 变为 `-1` 的同帧进入 MM `AM_Generic_Unequip`。
- 女性本地玩家：手柄 `Gamepad_DPad_Down` 走同一 IA/GAS 链，ActiveIndex 变为 `-1` 的同帧进入 MF `AM_Generic_Unequip`。
- 两者均连续多帧保持正确 Montage，手臂比例正常；截图为 `Saved/Screenshots/WindowsEditor/ScreenShot00001.png` 和 `ScreenShot00002.png`。
- 真实 `IA_WeaponNext` 已覆盖男女 Rifle/Pistol/Shotgun Equip、快速连续切换和丢枪后自动装备下一槽；最后一枪丢弃后进入同侧正式空手层。

TestMap2 Listen Server 初次回归时，远端装备出现“服务器播放 Equip、客户端只切动画层但不播 Montage”。原因不是动画资产或 RPC 缺失，而是两条复制流没有顺序保证：`FShootEquipmentList::PostReplicatedAdd` 可能先调用客户端 `OnEquipped`，装备子对象的 `Instigator` 随后才到达；具体 Montage 又必须经 `Instigator -> UShootInventoryItemInstance -> ItemDefinition -> RangedWeaponConfig` 解析。

正式修复在 `UShootEquipmentInstance::OnRep_Instigator` 中转发一个受保护的 `OnInstigatorReplicated` 扩展点。`UShootWeaponInstance::OnEquipped` 只有在 Equip Montage 为空且 Instigator 尚未到达时才记录一次性待补状态；回调到达后只补播 Montage，不再次 Link Anim Layer、不重复显隐武器，也不重入蓝图 `OnEquipped`。若 Instigator 先到，则正常 `OnEquipped` 直接播放且不设置待补标记。因此该处理同时覆盖两种复制顺序，不依赖 Timer、Multicast 或固定延迟。

修复后的 Listen Server 帧级验收结果：

- 远端 Shotgun 权威拾取：服务器第 `6` 帧进入 MF `AM_Shotgun_Equip`，客户端第 `8` 帧复制到 ActiveIndex `0` 并播放相同 Montage。
- 客户端真实 Slate 输入按约 `20` 帧间隔执行 `Shotgun -> Rifle -> Shotgun -> Rifle`，服务器模拟代理与客户端本地 Pawn 在第 `17/37/57` 帧同步进入对应 Equip Montage。
- 真实 `G` 丢当前枪后自动装备剩余 Shotgun；再次 `G` 丢最后一枪时，两端于第 `88` 帧同时进入 MF `AM_Generic_Unequip`，ActiveIndex 均为 `-1`。

## CC 适配的正确顺序

1. 在男女 CC 主 SkeletalMesh/Skeleton 上补齐 `weapon_r`，父级必须是 `hand_r`，参考局部变换以 Lyra 为基准。
2. 把 Lyra 源动画的 `weapon_r` 局部轨道采样并写入对应 CC 动画；不得只写一帧静态姿势。
3. 添加三个精确虚拟骨骼，尤其是 `weapon_r -> hand_l`。
4. 编译男女 Base、Rifle、Pistol、Shotgun、Unarmed 层，要求缺骨骼警告为零。
5. 先关闭 HandIK/FootPlacement 做基础移动 A/B 验证，再逐项恢复。
6. 在 Rifle 通过后扩展 Pistol 和 Shotgun；新增枪械沿用动画层和配置，不修改主 AnimBP 武器分支。
7. 验证 Mutable 重建、头发层、鞋子层和表演 Pose Slot 未被破坏。

## 玩家验收重点

- 空手和持枪分别连续左右、前后反转至少 20 次，无骨架折叠或卡帧。
- 跳跃经过 JumpStart、Fall、Land；蹲伏同时改变碰撞体和可见动画。
- Rifle、Pistol、Shotgun 的左手稳定接触正确握持区域，开火和换弹不拉伸。
- 本地分屏两名角色同时执行动作，动画层、武器和视口互不串线。
- Listen Server 的模拟代理也能看到一致姿势，不能依赖仅本地 Controller 存在的数据。

## 当前状态

- 已完成：男女 CC `ABP_ItemAnimLayersBase` 内遗留的 Mannequin AimOffset 和 Rifle Idle fallback 已替换为正式 CC 资产，并经编辑器重启验证持久化。
- 已完成：证明 `weapon_r` 是独立运动轨道，排除用 `ik_hand_gun` 代替的错误方案。
- 已完成：男女 CC 同步添加 `weapon_r` 和三个精确虚拟骨骼；Rifle、Pistol、Shotgun 的 Idle/Fire/Reload 以及 Rifle/Pistol Equip 所需正式 CC 资产已迁移动态 `weapon_r`，并逐帧核对。
- 已完成：男女 Item Base 的主 AnimBP 返回类型、成员引用和 Property Access 已迁移到同侧 CC 主 AnimBP。
- 已完成：男女 Rifle/Pistol/Unarmed/Shotgun 子层已进入同侧 CC 父类链；一次性 StanceTransition/IdleBreak 不再循环；18 个正式 AnimBP/动画接口资产编译错误为零。
- 已完成：男女 Rifle Jog Lean 已按“`retain_additive_flags=False` 生成绝对姿势，再恢复目标侧 LocalSpace Additive 和 CC Center Base Pose”重建；真实 `IA_Move` 快速反向未再复现女性全身折叠。
- 已完成：FootPlacement、LegIK、GroundDistance、Stride Warping 单变量 A/B；所有诊断开关和图连线均已恢复。
- 已完成：男女 Shotgun Fire 以及女性三枪 Fire/Reload 已走真实 Enhanced Input 和 GAS 验收；男性 Shotgun Fire 最终根因是目标 Sequence 丢失 Mesh Space Additive 与自身第 28 帧 Base Pose，已修复并完成分屏、Listen Server 回归。
- 已完成：三枪 Equipment 已对齐 Lyra `weapon_r/-90`，三把 WeaponInstance 已使用数据化 `EquipMontageStartPosition=0.6s` 跳过 CC Equip 的异常前段。`0.4s` 中间值因女性 Shotgun 仍有单臂横伸而舍弃；女性 `0.6/0.8/1.0s` 采样证明 `0.6s` 是首个自然持枪点。男女均以约 `300cm/s` 横移触发 Shotgun Fire，双手保持稳定。
- 已完成：男女三枪 Equip、快速切枪、丢枪后自动装备和最后一枪 Unequip/空手已走真实 Enhanced Input/GAS；Generic Unequip 的过早淡出已按 Lyra 精确参数修复。正式 `0.6s` 配置下，女性 Rifle 起播、三枪十次快速切换和 Shotgun 横移 Fire 均已补齐真实输入截图。
- 已完成：逐武器左手握把复读确认使用 Lyra 原生的 Linked Anim Layer 姿势覆盖。Rifle/Pistol 保持关闭，男女 Shotgun 分别引用同侧正式 Idle Hipfire；不新增第二套 C++ Transform 或性别硬编码。
- 已完成：TestMap2 Listen Server 远端模拟代理与客户端本地 Pawn 的 Equip/Unequip、快速切枪和最后一枪空手同步回归。
- 已完成：TestMap 出生点已修正；男女 Foot Controls 又补做了四向连续反转、Jump/落地和 Crouch 的最终玩家视角回归，截图 `HighresScreenshot00077-00090.png` 未见双脚绑定、膝盖逆折或落地悬空。
- 已完成：Equip 起播、正式 CC 姿势数据、角色到武器 Montage 同步、18 个 AnimBP 编译、分屏与 Listen Server 回归均已选择性提交并推送。下一阶段只处理经分类后仍影响运行时的 Mannequin 依赖，不重做已归档玩法矩阵。
