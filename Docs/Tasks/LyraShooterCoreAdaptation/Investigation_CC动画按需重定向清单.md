# CC 动画按需重定向清单

更新时间：2026-08-18

## 当前断点

本任务严格使用 Unreal Editor MCP/Python API 操作资产，不读取或修改 `.uasset` 文件。当前项目工作区无未提交的源资产恢复修改；Mannequin 源资产恢复已经由此前独立提交 `468b6d4`、`b379e8e` 完成，本任务不把源恢复混入 CC 重定向提交。

当前已完成按需扫描、MM 代表样本验证和 MF 代表样本验证，但尚未开始全量覆盖。首轮 MM 代表样本重定向的正式 Retargeter 输出中，`MM_Rifle_Jog_Fwd` 的 `weapon_r` 轨道出现明显异常；已立即停止批量操作，并用项目已有的局部轨道修复链修复后复验通过。异常没有扩散到未处理的正式序列。

MM 和 MF 代表样本均已通过；下一断点是分小批次开始 MF 全量 `AnimSequence` 重定向，并在每批执行 Additive/Base Pose 原生元数据恢复和 `weapon_r` 轨道迁移。

MF 全量试运行块先完成 4 批、8 个新序列；因首批长序列按关键帧预算单独成批，实际完成并复验的是 7 个新序列，均通过 Skeleton、Additive/Base Pose、Root Motion、曲线/Notify/Sync Marker 数量、Scale 和 `weapon_r` 首/中/末帧误差检查。包括此前已验证并提交的 15 个 MF 序列在内，当前已完成 MF 正式集合 22/291，剩余 269 个。处理 Pistol AimOffset 基础姿势链时，Retargeter 还让同目录其他 AO AnimSequence 出现了工作树改写；这些序列仍属于正式集合但尚未完成本任务的 weapon_r/元数据复验，不计入完成数，下一批会按原路径重新覆盖。`AO_MF_Pistol_Idle_ADS` 容器和 `AM_Shotgun_Reload` Montage 已精确恢复，未提交任何容器/Montage。

## 资产边界与重定向器

| 项目 | 路径 |
| --- | --- |
| Mannequin 源 | `/Game/Characters/Heroes/Mannequin/Animations` |
| MF 目标 | `/Game/Characters/Heroes/CC/MF/Animations/{Actions,AimOffsets,Locomotion,Poses}` |
| MM 目标 | `/Game/Characters/Heroes/CC/MM/Animations/{Actions,AimOffsets,Locomotion,Poses}` |
| MF Retargeter | `/Game/Assets/Characters/CC/ShenWanYun/RTG_ShenWanYun` |
| MM Retargeter | `/Game/Assets/Characters/CC/ChenHaoYu/RTG_ChenHaoYu` |
| MF 目标 Skeleton | `/Game/Assets/Characters/CC/ShenWanYun/ShenWanYun_Skeleton` |
| MM 目标 Skeleton | `/Game/Assets/Characters/CC/ChenHaoYu/ChenHaoYu_Skeleton` |

目标序列只按正式目标目录中实际存在的 `AnimSequence` 建立；缺少目标副本时，只有在正式 Base AnimBP、Item Anim Layer、武器 Linked Layer、AimOffset、BlendSpace、角色 Montage 或角色到武器同步链中发现明确引用，才加入待补齐集合。按四个正式目录实际扫描，MF 有 294 个目标序列，其中 290 个按完整相对路径直接对应 Mannequin 源，另有 1 个由正式 Additive Base Pose 明确指向 Locomotion 源的同名基础序列，3 个零引用 Quinn Pose 跳过，最终正式处理集合为 291 个。MM 有 382 个目标序列，其中 379 个按完整相对路径直接对应 Mannequin 源，另有 3 个由 AimOffset 基础姿势链明确指向 Locomotion 源的同名基础序列，最终正式处理集合为 382 个。没有使用模糊名称匹配。

`AimOffset`、`BlendSpace`、`AnimMontage`、`AnimBlueprint`、`AnimLayerInterface`、`AnimationModifier` 和 `AnimNotify` 不作为 AnimSequence 盲目重定向对象。容器引用在序列完成后单独复读和修复。

## 初始资产计数

以下是 Asset Registry 扫描到的正式目标目录初始计数。`AnimSequence` 是可以重定向的序列；其他类型仅作为容器或运行时引用检查对象。

| 性别 | 目录 | AnimSequence | AimOffset/BlendSpace | AnimBlueprint | AnimMontage |
| --- | --- | ---: | ---: | ---: | ---: |
| MF | Actions | 13 | 0 | 0 | 0 |
| MF | AimOffsets | 46 | 3 | 0 | 0 |
| MF | Locomotion | 209 | 1 | 4 | 0 |
| MF | Poses | 26 | 0 | 0 | 0 |
| MM | Actions | 57 | 0 | 0 | 0 |
| MM | AimOffsets | 80 | 3 | 0 | 0 |
| MM | Locomotion | 215 | 1 | 4 | 0 |
| MM | Poses | 30 | 0 | 0 | 0 |

全量集合复读结果：MF `291` 个处理、`3` 个零引用 Manny Skeleton 遗留跳过、`0` 个关键帧/帧率契约不一致；MM `382` 个处理、`0` 个跳过、`0` 个关键帧/帧率契约不一致。MF 源 Additive 类型分布为 None 231、LocalSpaceBase 11、MeshSpace 48（另 1 个特殊基础序列按非 Additive 源处理）；MM 为 None 269、LocalSpaceBase 30、MeshSpace 80。所有已扫描可处理目标当前目标 Skeleton 分别为 `ShenWanYun_Skeleton` 和 `ChenHaoYu_Skeleton`。

三条特殊 MM 映射和一条特殊 MF 映射不是模糊匹配，而是由正式目标 Additive/AimOffset 资产的 `ref_pose_seq` 和同名基础姿势链明确确定：

| SourcePath | TargetPath | 依据 |
| --- | --- | --- |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/MF_Pistol_Idle_ADS` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_CC` 的源 Base Pose 链 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/MM_Pistol_Idle_ADS` | `/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Pistol_Idle_ADS` | 正式 Pistol AimOffset 基础姿势同名 Locomotion 链 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Idle_ADS` | `/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS` | 正式 Rifle AimOffset 基础姿势同名 Locomotion 链 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/MM_Unarmed_Idle_Ready` | `/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Unarmed_Idle_Ready` | 正式 Unarmed AimOffset 基础姿势同名 Locomotion 链 |

MF 目标序列中有 3 个已知旧资产仍使用 Manny Skeleton：

- `/Game/Characters/Heroes/CC/MF/Animations/Poses/SplashPose_Quinn_1`
- `/Game/Characters/Heroes/CC/MF/Animations/Poses/SplashPose_Quinn_10`
- `/Game/Characters/Heroes/CC/MF/Animations/Poses/SplashPose_Quinn_11`

这 3 个资产在当前状态调查中已经确认无正式引用，属于人工清理遗留；本任务不删除、不 Force Delete，也不把它们混入正式 CC 重定向数量。其余 MF 序列初始 Skeleton 为 `ShenWanYun_Skeleton`，MM 序列初始 Skeleton 为 `ChenHaoYu_Skeleton`。

## 映射记录字段

全量映射记录按一条目标 `AnimSequence` 一行维护，字段固定为：

| 字段 | 记录规则 |
| --- | --- |
| SourcePath | Mannequin 源 `AnimSequence` 完整对象路径 |
| TargetPath | 正式 MF/MM 目标 `AnimSequence` 完整对象路径 |
| Source Skeleton | 重定向前从源资产读取 |
| Target Skeleton | 重定向后从目标资产复读 |
| 引用它的正式容器 | 由 Asset Registry 引用扫描及正式 ABP/AimOffset/BlendSpace/Montage 复读得到；无直接容器时记录 `无直接正式容器引用` |
| Additive 类型 | 源资产类型与目标复原后的类型 |
| Additive Base Pose | 源 Base Pose、目标侧对应 Base Pose；LocalSpace 内建 Anim Frame 记录为空路径并注明 Frame 0 |
| Root Motion | 源与目标的 `enable_root_motion` 复读值 |
| 曲线 | `AnimSequenceService.list_curves` 名称清单及数量 |
| Notify | `AnimSequenceService.list_notifies` 名称/时间清单及数量 |
| Sync Marker | `AnimSequenceService.list_sync_markers` 名称/时间清单及数量 |
| 是否覆盖已有资产 | 正式目标需要更新，但禁止再使用 `overwrite_existing_files=True`；UE 5.8 会以 Force Replace + Force Delete 实现所谓覆盖。后续必须经独立临时输出和 AnimSequence 数据写回链更新现有正式对象 |

正式目标序列当前均有唯一同名 Mannequin 源序列；因此本阶段不存在“同名多源待人工选择”的映射。全量重定向完成后，将在本节后追加机器复读结果和失败映射，而不是用名称推测容器关系。

## 代表样本验证记录

首轮 MM 样本使用 `RTG_ChenHaoYu`，当时输出路径保持正式目标路径，使用了 `overwrite_existing_files=True`、`retain_additive_flags=False`。Retargeter 输出后，Additive 元数据按源资产记录恢复；容器本身没有被覆盖。2026-08-19 复读 UE 5.8 引擎实现后，已确认该 overwrite 参数内部会 Force Replace 引用并 Force Delete 旧资产，因此历史样本结果保留，但后续批次禁止继续使用同一路径。

| SourcePath | TargetPath | Source/Target Skeleton | Additive | Base Pose | Root Motion | 曲线/Notify/Sync | 覆盖 | 当前结果 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Pistol_Equip` | `/Game/Characters/Heroes/CC/MM/Animations/Actions/MM_Pistol_Equip` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Rifle_Equip` | `/Game/Characters/Heroes/CC/MM/Animations/Actions/MM_Rifle_Equip` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Shotgun_Fire` | `/Game/Characters/Heroes/CC/MM/Animations/Actions/MM_Shotgun_Fire` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | MeshSpace → MeshSpace | 源 self → 目标 self | false | 0/0/0 | 是 | 元数据已恢复；Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Idle_ADS` | `/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Idle_ADS` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd` | `/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | None → None | 空 | true | 0/0/0 | 是 | Retargeter 输出中帧 weapon_r 约 502；既有轨道修复后 Root、Pelvis、手臂、IK、Scale、Root Motion、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_CC` | `/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_CC` | `SK_Mannequin` → `ChenHaoYu_Skeleton` | MeshSpace → MeshSpace | 源 self → 目标 self | false | 0/0/0 | 是 | 元数据已恢复；Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |

### MF 样本

| SourcePath | TargetPath | Source/Target Skeleton | Additive | Base Pose | Root Motion | 曲线/Notify/Sync | 覆盖 | 当前结果 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Pistol_Equip` | `/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Pistol_Equip` | `SK_Mannequin` → `ShenWanYun_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Rifle_Equip` | `/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Rifle_Equip` | `SK_Mannequin` → `ShenWanYun_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MF_Rifle_Idle_ADS` | `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/MF_Rifle_Idle_ADS` | `SK_Mannequin` → `ShenWanYun_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_CC` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_CC` | `SK_Mannequin` → `ShenWanYun_Skeleton` | MeshSpace → MeshSpace | 源 Locomotion/Pistol Frame 0 → 目标 Locomotion/Pistol Frame 0 | false | 0/0/0 | 是 | 原生 `ref_pose_seq` 已明确修复到同侧 CC Locomotion Base Pose；Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/Poses/QuinnIntro_Blockout/QuinnIntro_BlockOut_Pose1_Quinn` | `/Game/Characters/Heroes/CC/MF/Animations/Poses/QuinnIntro_Blockout/QuinnIntro_BlockOut_Pose1_Quinn` | `SK_Mannequin` → `ShenWanYun_Skeleton` | None → None | 空 | false | 0/0/0 | 是 | Root、Pelvis、四肢、IK、Scale、weapon_r 通过 |

MF 样本中，Retargeter 新输出路径刚生成时，`save_loaded_asset` 偶发返回 false；复读确认目标包已存在且仅目标包脏后，使用 `EditorAssetLibrary.save_asset` 重试成功。该 API 时序异常没有导致映射失败或重复资产，已作为批处理实现注意事项记录；后续每批保存后必须复读 Asset Registry/目标包并在必要时按路径重试一次，失败则停批。

### 异常判断

项目 IKRig/Retargeter 的源、目标链条都没有 `weapon_r` 专用 Retarget Chain；现有 Retargeter 负责角色骨骼链和手部 IK，但不会可靠保留项目的武器辅助骨轨。`MM_Rifle_Jog_Fwd` 的目标 `weapon_r` 中帧异常而角色骨骼链正常，符合这一已知边界。因此不能把首轮 Retargeter 输出直接批量视为合格。

项目已有的 `Scripts/Editor/Animation/MigrateLyraWeaponRBoneTrack.py` 已在 6 个 MM 代表样本上执行局部 Pose 逐帧复读，并对首/中/末帧做误差检查；`weapon_r` 最大平移误差为 0、最小四元数点积约为 1。复验确认 Root、Pelvis、脊柱、clavicle、upperarm、lowerarm、hand、weapon_r、ik_hand_gun、ik_hand_l、IK Foot、Scale、Root Motion 和 Additive 全部通过，允许继续 MF 代表样本。

## 容器与遗留引用初始扫描

Asset Registry 初始扫描未发现正式 MF/MM CC 容器、Base AnimBP、Linked Layer 或 CC Montage 直接引用 Mannequin `AnimSequence`。全局仍有 16 条旧 `/Game/Weapons/Pistol|Rifle|Shotgun/Animations/AM_MM_*` Montage 引用 Mannequin 源序列，这些不属于正式 CC 运行链，需在最终报告中作为排除的遗留引用列出，不为清零依赖而扩大本任务范围。

重定向后仍必须复读：

- AimOffset Samples 与 Preview Base Pose；
- BlendSpace Samples；
- Additive Sequence Base Pose；
- 正式 Montage Segment、Section、Slot、曲线和 Notify；
- 正式 ABP 与 Linked Layer 的动画节点引用。

已知损坏的 MM Montage `/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_Generic_Unequip` 与 `/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_Pistol_Equip` 本阶段只在基础序列健康后记录为“可以重建”，不覆盖、不删除、不 Force Delete。

## 后续阶段

1. 以 MF 291 个正式目标 AnimSequence 为边界，按小批次重定向；每批立即恢复 Additive/Base Pose、复读 Skeleton/Root Motion，并迁移 `weapon_r`。
2. MF 批次保存、复读、编译验证后独立提交并推送。
3. 以 MM 382 个正式目标 AnimSequence 为边界，按小批次完成同一链路；已验证的 6 个 MM 样本不重复调查。
4. 单独复读和修复 AimOffset、BlendSpace、Additive Base Pose、Montage Segment 与正式 ABP/Linked Layer 引用。
5. 编译男女 Base AnimBP、Item Anim Layer、Rifle/Pistol/Shotgun/Unarmed Linked Layer；再用 PIE 截图验证运行时资产名和视觉结果。

## 2026-08-18 MF AimOffset 批次断点

本批使用 `RTG_ShenWanYun`，仅覆盖以下 8 个正式 MF `AnimSequence`，没有覆盖 AimOffset 容器：

| SourcePath | TargetPath | Source Skeleton | Target Skeleton | Additive/Base Pose | Root Motion | 曲线/Notify/Sync | 覆盖 | 结果 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LC` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LC` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | `weapon_r` 首/中/末帧误差通过；Root、Pelvis、Spine、手臂、手部 IK、IK Foot、Scale 通过 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LD` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LD` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LU` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_LU` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBC` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBC` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBD` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBD` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBU` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RBU` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RC` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RC` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |
| `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RD` | `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RD` | `SK_Mannequin` | `ShenWanYun_Skeleton` | MeshSpace；源/目标同侧 Base Pose 链 | 按源复读 | 按源复读 | 是 | 同上 |

本批每个序列均先由 Retargeter 处理主骨骼，再按 `Scripts/Editor/Animation/MigrateLyraWeaponRBoneTrack.py` 的既有逻辑逐帧迁移 `weapon_r`。复读结果没有发现除 `weapon_r` 外的骨骼、Scale、IK、Root Motion 或 Additive 异常；目标目录没有生成 `_Retargeted`、`_Copy` 或 `_New` 资产。MF 正式处理集合进度更新为 `30/291`，剩余 `261` 个。编辑器额外脏化的容器 `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/AO_MF_Pistol_Idle_ADS` 已恢复，未纳入本批提交。下一断点为继续处理剩余 MF AimOffset/Locomotion/Actions/Poses 序列，并保持本批的逐目标复读链路。

### AO_RU 异常断点

继续处理 `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RU` 时，单资产 `run_batch_retarget` 调用超过 300 秒未返回。编辑器日志显示 Retargeter 生成了 `/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MF_Pistol_Idle_ADS_AO_RU1`，随后记录 `Force Deleting 1 Package(s)` 并指向正式 `MF_Pistol_Idle_ADS_AO_RU`。2026-08-19 只读复核 UE 5.8 `UIKRetargetBatchOperation::OverwriteExistingAssets` 后确认：数字后缀副本、`ForceReplaceReferences`、`ForceDeleteObjects`、再重命名回正式路径正是 `overwrite_existing_files=True` 的固定实现，不是仅由脏资产触发的偶发错误。该序列本批不计入完成数，不能直接提交 `RU` 或 `RU1`；必须等 Unreal MCP 恢复后通过 Asset API 读取两者实际对象、引用和 Skeleton，再用非 Force 的单资产操作处理现场。后续 MF/MM 批处理彻底禁用原生 overwrite，改为独立临时目录输出、验证返回路径，再研究并验证 AnimSequence 数据写回现有正式对象的链路。下一断点仍是完成 `RU/RU1` 状态复读；在临时输出与数据写回链通过非正式资产验证前，不恢复批处理。
