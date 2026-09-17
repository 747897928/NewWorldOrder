# 动画迁移任务交接

更新：2026-09-13
状态：正式 CC 运行时目录切换和旧 CC Animations 清理已完成；Equip 结构已与 Lyra 对齐；CC PostProcess 已完成独立复制、骨骼适配和男女 Wrinkle 组合接入；四个正式 Base/Linked Layer ABP 的 Mannequin Skeleton/Profile 依赖已清理，全局 Mannequin 源资产清理和男性专用姿势修型仍未完成

辅助骨轨道的诊断、白名单 Local T/R/S 回写、安全门槛和验证阈值见 `AuxiliaryBoneTrackRepair_辅助骨轨修复.md`。以后重定向出现武器脱手、飞头或 IK 手骨扭曲时，从该文档断点继续，不要重新猜测 Actor Transform。

## 目标

把 /Game/Characters/Heroes/Mannequin/Animations（Lyra 官方动画）通过 IK Retarget 重定向到自定义角色，输出到：
- /Game/Characters/Heroes/CC/MM（ChenHaoYu 骨架）
- /Game/Characters/Heroes/CC/MF（ShenWanYun 骨架）

目录结构镜像 Mannequin/Animations。

## 已完成（已推送）

- 首轮动画数据重定向：CC/MM 422 资产（396 AnimSequence、3 AimOffset、1 BlendSpace1D、12 ABP），CC/MF 293 资产（267 AnimSequence、3 AimOffset、1 BlendSpace1D、12 ABP）。无性别资产（Bench、SmearPoses）两边双份；MF 侧补齐 Feminine 图层回退引用的 MM_* 动画。
- 2026-08-02 复核发现首轮脚本遗漏 MF 的 13 个 `MM_*` 武器动作。现已用 `RTG_ShenWanYun` 补齐 Pistol/Rifle/Shotgun 的 Fire、Reload、Equip 与对应 Additive，均绑定 `ShenWanYun_Skeleton`；MF 目录当前至少增加到 306 个资产。
- 在 `/Game/Characters/Heroes/CC/MM|MF/Animations/Weapons/Montages` 分别新增 9 个正式 CC Montage：三把武器各自的 Fire、Reload、Equip。Fire 使用 `UpperBodyAdditive`；Reload/Equip 使用 `UpperBody + UpperBodyAdditive`。Shotgun Equip 当前显式复用 Rifle 双手装备动作，但保留独立 Montage，便于后续替换。
- 上述 31 个新增资产已通过 AssetRegistry 依赖扫描，对 `/Game/Characters/Heroes/Mannequin` 的直接依赖为 0。运行时 `ID_*` 已切换到 `/Game/Characters/Heroes/CC/MM|MF/Animations/Weapons/Montages`。
- 15 个 AnimBlueprint 复制到 MM/MF 两侧，Target Skeleton 分别改为 ChenHaoYu_Skeleton / ShenWanYun_Skeleton。
- 8 个武器 AnimLayer 蓝图（Pistol/Rifle/Shotgun/Unarmed × 普通/Feminine）CDO 动画引用替换完成。
- 图层子图节点引用（RotationOffsetBlendSpace、SequenceEvaluator、BlendSpacePlayer）全部替换为 CC 路径。
- 运行时验证：全部 ABP 图节点对 Mannequin 的动画引用为 0。
- 2026-08-17 已确认 `ABP_ItemAnimLayersBase` 并未漏掉源图：排除 Lyra 导出中自动生成的 `ExecuteUbergraph`、`__AnimFunc`、`__AnimFunc_MERGED` 后，双方源图拓扑一致。禁止再依据原始导出节点总数整体替换正式 AnimBP。
- 男女 6 个正式 Equip Montage 已补齐 Lyra 精确曲线插值/切线及换枪 PlaySound Notify。男性 Rifle/Pistol 的 4 条底层 Equip Sequence 已在导出阶段关闭手臂 IK 求解后重新重定向，避免陈浩宇比例下的额外肘部过弯；Additive 元数据和 Skeleton 已复读正确。修正后仍存在的写实 CC 橡皮感需要专用动作修型，不是缺少 Timing 或 Generic Unequip 导致。

## 2026-09-10 CC PostProcess 独立复制与适配

- 四个源资产到 CC 的逐项差异、`elbowsharebone/kneesharebone` 错误映射的原因、16 个 `SetTranslation` 的关闭原因、Pose Driver 白名单陷阱和以后重新适配的操作清单，集中记录在 [CC_PostProcess 适配差异与排坑](CC_PostProcess_适配差异与排坑.md)；不要只根据早期对话中的中间方案操作。
- 已从 Lyra 复制 `ABP_Manny_PostProcess`、`ABP_Quinn_PostProcess` 和 `CR_Mannequin_Procedural`，生成 ChenHaoYu 与 ShenWanYun 各自的 CC 版本；原始 Mannequin 资产未修改。
- 两份 Control Rig 保留原节点图，层级改为对应 CC 网格骨骼；Lyra 的 upperarm/lowerarm/thigh/calf twist 已映射到 CC 的 `cc_base_*_*twist01/02`。实测确认 CC 的 `elbowsharebone`/`kneesharebone` 不能等价承接 Lyra 肘膝 corrective，因此相关旋转节点保留但置零，Pose Driver 也排除这些共享骨。
- CC 没有可靠等价物的 upperarm/thigh correctiveRoot、clavicle/foot corrective 暂不启用；没有把普通骨骼假定成等价 corrective。详细映射、验证结果和后续接入步骤见 `CC_PostProcess_适配记录.md`。
- 新 ABP/Control Rig 已编译，四个独立 CC 包对 `/Game/Characters/Heroes/Mannequin` 的直接包依赖为 0；皱纹组合入口另外复制到 CC Rig 目录，用于在保留面部处理的前提下接入姿势修正。
- 2026-09-10 已修正复制 Control Rig 对 CC twist 层级不兼容的全局平移写入：两个 CC Control Rig 的 `SetTranslation` 节点保留但置零权重；无可靠 CC 替代的 clavicle/foot Pose Driver 通过 AnimGraph 旁路，避免空 `OnlyDriveBones` 被误解为禁用。
- 已创建 `ABP_ShenWanYun_CombinedPostProcess` 和 `ABP_ChenHaoYu_CombinedPostProcess` 作为皱纹基底加 CC 姿势修正的组合入口。旧的男女 `*_WrinkleAnimBlueprint` 保留为备份；ShenWanYun、ChenHaoYu 网格均已切换到各自组合入口并在编辑器预览中确认后处理正在运行。
- 直接给网格设置独立 CC PostProcess 会覆盖 Wrinkle AnimBP，是“使用后直接不行”的结构性原因；后续新增姿势修正必须接在对应 Wrinkle 链之后，不能再次直接替换 Mesh Post Process Anim Blueprint。

## 2026-09-10 CC FootPlant Control Rig 独立迁移

- 两个 CC `ABP_Mannequin_Base` 已分别从源 `CR_Mannequin_FootPlant` 切换到 `CR_ChenHaoYu_FootPlant`、`CR_ShenWanYun_FootPlant`；源 Control Rig 未修改。
- 两份副本保留原 140 个节点和 7 个控制器，骨骼层级改为对应 CC 的 123 根骨骼。CC 缺少 Lyra 的 `ik_ball_l/r`，已将仅用于调试斜率节点的 `ik_ball_r` 改为 CC 的 `ball_r`，并移除两个额外骨骼。
- 两个 ABP 和两个 Control Rig 均编译为 `BS_UP_TO_DATE`；ABP 已不再直接依赖源 `CR_Mannequin_FootPlant`。完整复制、适配和依赖边界见 [CC FootPlant Control Rig 适配记录](CC_FootPlant_适配记录.md)。

## 2026-09-13 CC AnimBP Skeleton/Profile 依赖清理

- 四个正式 CC Base/Linked Layer ABP 的 Target Skeleton 已复读为 `ShenWanYun_Skeleton`（MF）和 `ChenHaoYu_Skeleton`（MM）。
- 主 Base 的 `Layered Bone Blend` 使用对应 CC Skeleton 上真正的 `UpperBodyLowerBodySplitMask` Blend Mask；同名 Weight Profile 不能替代它。
- 两个 CC Skeleton 已补齐 `UpperBodyMask`、`LowerBodyMask`、`LeftFingersMask`、`UpperBodyLowerBodySplitMask` 和 `FastFeet`。其中四个 `*Mask` Profile 均为真正的 `BlendMask`，`FastFeet` 为 `TimeFactor`；具体语义和 23 根 Split Mask 同名骨匹配记录见 [CC AnimBP 骨架依赖清理记录](CC_AnimBP_SkeletonDependencyCleanup_骨架依赖清理.md)。
- MF/MM Base 的 Mannequin `FastFeet` 状态机过渡已分别改为 Shen/Chen 的 CC Profile；Linked Layer PivotSM 各两条残留过渡也已改写。
- 通过 `AnimationAssetFixer.force_structural_compile_and_save` 结构化编译并保存后，四个 ABP 的图运行时 Mannequin 命中为 0，Asset Registry 不再返回 `/Game/Characters/Heroes/Mannequin` 依赖。
- 2026-09-13 PIE 回归已定位并修复：迁移脚本曾把源 Profile 未记录的目标骨骼错误写成显式 `0.0`；源 Profile 的未记录骨骼实际默认权重是 `1.0`。两个 CC Skeleton 的四个 `*Mask` 已恢复为“同名复制源值、未匹配保留默认 1.0”的稀疏语义，共纠正 644 个错误权重。这个错误会在 PIE 的 Main ABP → Linked Layer → CopyBone/TwoBoneIK 链中截断左手和武器辅助空间，故预览正常而运行时异常。具体 `weapon_l` 链见 [CC AnimBP 骨架依赖清理记录](CC_AnimBP_SkeletonDependencyCleanup_骨架依赖清理.md)。
- 仅修改了项目定制 `Plugins/AnimationAssetFixer`；没有修改 VibeUE、Unreal Engine 或 Lyra/第三方插件。旧 Mannequin 源资产和零引用旧武器源 Montage 尚未删除。

## 2026-09-14 Walk/Jog/Start/Stop 选择逻辑调查

用户观察到角色普通移动时似乎没有播放 Walk，并怀疑 `BaseWalkSpeed=300`、`BaseRunSpeed=600` 或移动速度过高导致 Walk 被跳过。本轮只做静态图、资产引用和安全运行时采样，没有修改 AnimBP、角色速度或任何动画资产。

### 资产确实存在且引用正确

CC MF 的空手动画层 `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers_Feminine` 的 CDO 复读结果为：

```text
Walk_Cardinals.forward = /Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_Walk_Fwd
Jog_Cardinals.forward  = /Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_Jog_Fwd
Jog_Start_Cardinals.forward = .../MF_Unarmed_Jog_Fwd_Start
Jog_Stop_Cardinals.forward  = .../MF_Unarmed_Jog_Fwd_Stop
```

MF/MM 两侧 16 个 CC 武器/空手 Linked Layer 均已复读：`Walk_Cardinals`、`Jog_Cardinals`、`Jog_Start_Cardinals`、`Jog_Stop_Cardinals`、`ADS_Start_Cardinals`、`ADS_Stop_Cardinals` 各有完整四方向引用，引用资产均使用对应 CC Skeleton，没有发现 Walk、Start 或 Stop 资产遗漏。

### 当前选择条件不是速度二选一

`ABP_ItemAnimLayersBase.UpdateCycleAnim` 当前语义为：

```text
IsCrouching == true  -> Crouch_Walk_Cardinals
否则 IsADS == true   -> Walk_Cardinals
否则                 -> Jog_Cardinals
```

Walk/Jog 的选择索引实际来自 `GameplayTag_IsADS`。两个正式 MF/MM Base 的 `GameplayTagPropertyMap` 均将 `Event.Movement.ADS` 映射到该变量；`UShootGA_Weapon_Aim` 激活时添加该标签，结束时移除。因此非 ADS 普通移动看到 `MF_Unarmed_Jog_Fwd` 是当前 Lyra 风格链路的预期行为，不是因为速度太快而错过 `MF_Unarmed_Walk_Fwd`。

`GetMainAnimBPThreadSafe.Displacement Speed` 只连接到 `Set Playrate to Match Speed`，用于调整动画播放速率，不负责 Walk/Jog 分支。角色的 `BaseWalkSpeed=300`、`BaseRunSpeed=600` 只改变移动组件的最大速度和播放速率匹配。

### Start/Stop 结论

普通非 ADS 移动使用 `Jog_Start_Cardinals` / `Jog_Stop_Cardinals`；ADS 移动使用 `ADS_Start_Cardinals` / `ADS_Stop_Cardinals`，后者保存的是 Walk 起步/停止动作。主 Base 状态机通过 `HasAcceleration` 进入 Start，通过速度和动画剩余时间转入 Cycle/Stop；Start/Stop 是短暂过渡状态，操作时不容易被肉眼捕捉，但配置链存在且完整。

安全运行时采样使用真实 `IA_Move` 输入确认移动组件和动画变量会更新：角色出现非零速度、加速度，`HasVelocity`、`HasAcceleration`、`DisplacementSpeed` 和方向变量均正常变化。采样过程中曾出现的零速度帧来自测试地图碰撞阻挡，不是输入链或 Walk 资产缺失。

### 当前决策和以后若改变设计的方案

当前不修改，因为这会改变 Lyra 现有的“非 ADS Jog、ADS Walk”语义。如果产品设计明确要求“普通移动 300 使用 Walk，按住 Shift 到 600 才使用 Jog”，以后应引入动画可读的明确 `IsRunning` 状态，而不是用 `DisplacementSpeed` 猜阈值；并同时修改 `UpdateCycleAnim`、`UpdateStartAnim`、`UpdateStopAnim`，保证 Walk/Jog 的 Cycle、Start、Stop 使用同一模式。速度数值先保持 300/600 不变，待分支逻辑验证后再单独调播放速率。

## 2026-09-14 蹲伏武器辅助骨轨修复

用户反馈迁移后的蹲伏持枪姿势中左手向右偏移。此前已为 MF/MM 两侧各 65 个蹲伏序列补回 `weapon_l` 轨道，但 PIE 视觉仍未恢复，因此本轮重新按运行时消费链审计，而不是继续复制站立 `weapon_l`。

### 为什么没有复制站立 `weapon_l`

复读结果显示，男女两侧 65 个蹲伏目标的 `weapon_l` 与同名 Mannequin 蹲伏源已经逐帧一致：最大局部位移误差约 `2.31e-7`，最小旋转四元数点积约 `0.9999999999998345`，缩放误差为 `0`。这条源蹲伏 `weapon_l` 本身是固定辅助骨参考姿势。

站立 `weapon_l` 也不能作为通用替代：部分 CC MF 站立动作甚至没有显式 `weapon_l` 轨道，评估结果只是 CC Skeleton 参考姿势；把它复制到蹲伏目标不会改变已存在的有效姿势，也不能解决左手 IK 目标偏移。因此没有把站立轨道批量覆盖到蹲伏动作。

### 实际异常和运行时相关性

当前 CC Linked Layer 的左手链为：

```text
VB IK_Hand_L_weaponSpace
    -> CopyBone 写入 ik_hand_l
    -> TwoBoneIK 使用 ik_hand_l 作为 hand_l Effector
```

该虚拟骨骼的参考空间又与 `weapon_r` 相关；`ik_hand_gun` 也参与武器手部对齐。对同名 Mannequin 蹲伏源与 CC 目标逐帧比较发现：

- `ik_hand_l`：MF/MM 两侧 65/65 全部存在重定向差异，最大局部位移误差分别约 `23.28` / `18.61`。
- `ik_hand_gun`：MF/MM 两侧 65/65 全部存在重定向差异，最大局部位移误差分别约 `9.96` / `6.97`。
- `weapon_r`：两侧各有 21 个 Unarmed 蹲伏目标缺少轨道；缺失时评估为 CC 参考姿势，与源动态辅助轨不一致，最大局部位移误差约 `6.81`。
- `weapon_l`：两侧 65/65 已经一致，不是本轮继续覆盖的对象。

因此“左手偏向右侧”更符合左手 IK/武器参考空间输入没有按源动作写回，而不是单独的 `weapon_l` 缺轨问题。

### 本轮写回内容

使用 `Scripts/Editor/Animation/SafeRetargetWriteback.py` 的 `migrate_mannequin_local_tracks`，只对同名 Mannequin 蹲伏源和正式 CC 目标做局部辅助轨写回：

- MF：65 个蹲伏序列写回 `ik_hand_l`、`ik_hand_gun`；另给 21 个缺失 `weapon_r` 的 Unarmed 序列补回 `weapon_r`。
- MM：65 个蹲伏序列写回 `ik_hand_l`、`ik_hand_gun`；另给 21 个缺失 `weapon_r` 的 Unarmed 序列补回 `weapon_r`。
- `weapon_l` 没有重复写入。
- 所有 130 对源/目标的关键帧数量和帧率一致；未使用重定向覆盖、替换 UObject 或文件系统覆盖。
- 每个写回资产均通过脚本的逐帧 Local T/R/S 验证；没有修改曲线、Notify、Additive/Base Pose、Montage 或其他身体骨轨。

最终完整复读结果：男女两侧各 65 个目标的四条辅助轨均存在并与同名源逐帧一致；最大位移误差为浮点误差范围，最小旋转点积不低于 `0.999999999999386`，最大缩放误差为 `0`；目标 Skeleton 仍为 ShenWanYun/ChenHaoYu，编辑器脏包为 `0`。

### 当前验收边界

这是对蹲伏辅助骨输入的静态和逐帧数据修复，尚未把“PIE 视觉已经最终正确”写成完成。下一步应在 `TestMap_SplitScreen` 或无攻击 AI 的安全地图中验证 Rifle、Pistol、Shotgun 和 Unarmed 的蹲伏静止/移动姿势，重点观察左手握持、武器方向和进入/退出蹲伏。如果视觉仍偏，再继续检查 `VB IK_Hand_L_weaponSpace` 的运行时空间和后处理链，不应回退到批量复制站立 `weapon_l`。

## 2026-09-10 肘膝动作姿势回归修正

- 用户截图确认之前的“肘膝语义替代”在持枪弯曲动作中会造成整条手臂和腿崩坏；静态参考姿势正常不代表动作姿势正确。
- 已在男女 CC Control Rig 中关闭直接写入 `cc_base_l/r_elbowsharebone`、`cc_base_l/r_kneesharebone` 的四类 `SetRotation` 输出，并从男女 PostProcess Pose Driver 的 `OnlyDriveBones` 中移除这些共享骨。原节点图保留，CC twist 骨的旋转分配保留。
- 男女 `MF/MM_Pistol_Idle_ADS_AO_CD` 已做组合后处理启用/禁用对照；启用时不再出现膝部异常内折、肘部大角度扭转或整条肢体崩坏，禁用时也未发现基础重定向本身存在同级别错误。

## 临时插件 AnimationAssetFixer

- 位置：Plugins/AnimationAssetFixer（Editor 模块）
- 用途：替换 AnimBlueprint 图节点内动画引用，覆盖 Python 无 API 的节点类型（RotationOffsetBlendSpace / SequenceEvaluator）。
- 接口：
  - replace_node_asset_references(abp_path, old_to_new_prefix[], compile_and_save) → 替换节点数
  - list_node_asset_references(abp_path) → 列出全部图节点的"图名|节点类|资产路径"
  - force_structural_compile_and_save(abp_path) → 结构化编译并保存，刷新嵌套 AnimGraph 修改后的生成类烘焙数据
- 已用其完成本次任务；可保留（后续同类型工作可直接复用）或删除（删除插件 + 移除 .uplugin 即可）。

## 2026-08-02 正式目录切换与旧目录删除

- `DefaultGame.ini` 的男女主 AnimClass 已切换为正式 MM/MF `ABP_Mannequin_Base`。
- `BP_ShootCharacter` 默认空手层、Rifle/Pistol/Shotgun 的装备与卸下层、男性展示 AnimBP 均已切换为正式 MM/MF 层。
- 新编辑器进程真实 PIE 已证明女性主 AnimInstance 与 Unarmed Linked Layer 都来自正式 MF 目录。
- 外部引用扫描为零后，已删除 `/Game/Assets/Characters/CC/ChenHaoYu/Animations`、`/Game/Assets/Characters/CC/ShenWanYun/Animations` 和 `/Game/Developers/Codex/AnimPoseProbe`。
- 删除后重新编译正式男女主、两个 ItemLayersBase、八个武器层、角色、展示和三把武器实例，共 17 个蓝图，失败数为 0。
- 相关提交：`1cd6507` 为正式运行时入口切换；其后的删除提交记录旧目录物理删除。

## 剩余（Mannequin 类型/类引用，按白名单保留，不影响当前功能）

以下引用指向 Mannequin，当前仍属待处理的共享逻辑资产。它们不是“删除 Mannequin 目录也安全”的永久白名单；删除源目录前必须迁移或给出明确保留理由：

- AnimEnum_CardinalDirection / AnimEnum_RootYawOffsetMode（枚举）
- AnimStruct_CardinalDirections（结构体）
- AnimNotifies/TransitionToLocomotion（AnimNotify 类）
- LinkedLayers/ALI_ItemAnimLayers（接口类）
- ABP_Mannequin_Base（父链/引用，见 ABP_ItemAnimLayersBase 与武器层的依赖）
- Meshes/SK_Mannequin（骨架）
- Rig/CR_Mannequin_FootPlant、Rig/RTG_Mannequin（ControlRig / 重定向器）

后续仍必须完成：

- 完成三把 `ID_*` 正式 CC Montage 的真实开火、换弹回归。当前 Reload Montage 缺少 `GameplayEvent.ReloadDone` Notify，不能把“路径已切换”写成“换弹已完成”。
- 当前 Equip 已进入 QuickBar 实际切槽链：武器切武器时由新 Equip 替换旧 Unequip，切到空手时完整播放 `AM_Generic_Unequip`。若后续要求“旧武器完整收起后再生成新武器”的严格串行表现，必须另建统一的可取消权威状态机，不能在两个生命周期回调中分别盲播。
- 重新扫描 15 个 AnimBP 的父类、接口、ControlRig、枚举和 Skeleton 类型依赖。此前“图节点动画引用为 0”只覆盖节点资产引用，不等于整个 Mannequin 目录可删除。
- 在不同武器姿势和实际 PIE 中继续验证 CC 左腕视觉结果；若发现局部骨骼仍不匹配，继续修改 CC 副本并记录替代骨骼，不恢复 Mannequin 运行时资产引用。

## 相关提交

- e8de4df Lyra动画重定向到CC角色（MM/MF）+ ABP迁移
- 8694997 新增临时插件 AnimationAssetFixer
- 后续 77ac87e 等为误提交清理
