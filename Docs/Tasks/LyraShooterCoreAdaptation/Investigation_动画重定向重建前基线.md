# 动画重定向与 Montage 重建前基线

最后更新：2026-08-20

# 目的

本笔记保存用户批量重新重定向 MM/MF 动画前的事实、待验证假设和接续顺序。上下文压缩或切换会话后从这里继续，不重新把已验收系统列为待办。

# 已关闭任务

- 准星：用户验收通过。
- MF/MM GameplayTag Property Map：用户验收通过；每次 PIE 前编译仅作为回归门禁。
- Hair/Shoe Anim Layer：用户验收通过。
- 补给站：用户已确认临时 WidgetComponent 与临时 EventGraph 清理完成，LocalPlayer 提示、HUD 进度条、取消、补弹和碰撞范围均正常。

# 当前阻断资产

## AM_Generic_Unequip

路径：`/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_Generic_Unequip`

- 丢弃最后一把枪进入空手时实际播放。
- 男性角色在 Montage 持续期间整个上半身消失，结束后恢复。
- 两个分屏视角观察结果一致，不是本地摄像机或 HUD 问题。
- 这证明 Generic Unequip 不能直接按“无用资产”删除。应先确认 `ID_* -> Equipment/WeaponInstance -> Character Montage` 的真实读取链，再重建或在确认重复后退役。

## AM_Pistol_Equip

路径：`/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_Pistol_Equip`

- 用户确认资产编辑器预览缺少正常人物，运行时角色扭曲并伴随武器飞头。
- 不能继续只补曲线或 Preview Mesh；先确认 Skeleton、Montage Segment、底层 AnimSequence、Slot、Section、曲线和 Notify。

# 当前根因候选

按调查顺序排列：

1. 旧 Retarget 输出中的 `weapon_r` 位移、旋转或缩放轨道损坏。
2. 底层 AnimSequence 的骨骼缩放或 Additive/Base Pose 数据损坏。
3. `ScaleDownWeaponR` 虽与 Lyra 曲线数值一致，但项目 AnimBP 的消费节点或目标骨架语义不同。
4. `DisableLHandIK` 的曲线值、最终 IK Alpha 或消费时机不正确。
5. Character Montage 与 Weapon Montage 没有在相同位置同步。
6. Weapon Actor 或 Weapon Mesh 的 Relative Transform 被二次叠加。
7. Manny 动作落到写实 CC 比例后的美术造型问题。只有前六项均被运行时证据排除后，才进入 Control Rig/DCC 修型。

`sfx_WeaponSwap_nl_meta_Preset` 是声音 Notify，Timing 面板中的第二条标记来自该 Notify。它们不能解释上半身消失，也不能作为 Transform 修复手段。

# 用户批量重定向边界

- 来源：`/Game/Characters/Heroes/Mannequin/Animations`。
- 目标：保持原目录关系，分别覆盖 `/Game/Characters/Heroes/CC/MM/Animations` 与 `/Game/Characters/Heroes/CC/MF/Animations` 下对应的正式 AnimSequence。
- 用户负责编辑器中的批量重定向；Codex 不在用户操作期间并发保存这些动画资产。
- 用户完成后，第一步只读取 Git 变更清单，不立即修 Montage。
- 必查字段：目标 Skeleton、Additive 类型、Additive Base Pose、Root Motion、曲线、Notify、Sync Marker、压缩设置、`weapon_r`、IK Bones 和 Retarget Root。
- AnimMontage、AimOffset、AnimBP、ControlRig 不能因为 AnimSequence 批量输出而假定自动正确；它们在基础序列确认后单独重建或重新装配。

# 运行时单帧采样要求

在 Equip、Unequip 异常帧同时记录：

- Character Montage 名称和 Position。
- Weapon Montage 名称和 Position。
- `weapon_r`、`hand_r`、`hand_l`、`ik_hand_gun`、`ik_hand_l` Transform。
- `ScaleDownWeaponR`、`DisableLHandIK` 和最终左手 IK Alpha。
- Weapon Actor 与 Weapon Mesh Relative Transform。
- Actor、CharacterMesh0、CharacterBodyMesh0 的 Scale。
- 男性本地视角与另一 LocalPlayer 观察视角截图。

# 重定向后接续顺序

1. 生成 Git 资产变更清单，区分 MM、MF、AnimSequence、Montage、AimOffset 和工具资产。
2. 先检查 `RTG_ChenHaoYu`、`RTG_ShenWanYun` 的 Root、Arm、IK 与 `weapon_r` 策略。
3. 选择 Pistol Equip、Rifle Equip、Shotgun Fire、普通 Locomotion 和 Additive AimOffset 做小样本资产检查。
4. 编译 MF/MM `ABP_Mannequin_Base`，确认没有 GameplayTag `[None]`。
5. 在原路径重建男性 `AM_Pistol_Equip`。
6. 复读 Generic Unequip 调用链；保留则在原路径重建，确认重复才清除引用并退役。
7. PIE 逐帧完成 `weapon_r`、IK、Montage 同步和 Weapon Actor 单变量 A/B。
8. 修复后再验收 Rifle/Pistol/Shotgun 的 Equip、切枪、丢最后一把枪和空手恢复。
9. 独立提交并推送动画重定向、Montage 重建和运行时修复，不把 GE/GCN 伤害审计混入同一提交。

# 后续但不打断当前动画工作

- 姿势库将来支持 Male/Female/通用目录策略；底层兼容骨架播放能力不按性别限制。
- 武器伤害需要对照 Lyra 11000，恢复每枪 GE、Physical Material WeakSpot 和 GameplayCue/GCN 职责。
- Niagara 伤害数字已有可见结果，但仍需跨武器、WeakSpot、分屏和 Listen Server 完整验收。
# 2026-08-18 按需重定向执行断点

本次正式 CC 动画按需重定向已经完成 Git、源资产和正式目标目录的初始基线复读。Mannequin 源资产恢复此前已经独立提交为 `468b6d4`、`b379e8e`，当前工作区无未提交源恢复修改。本任务不删除 `/Game/Characters/Heroes/Mannequin/Animations`，也不把整个源目录批量复制到 CC。

按四个正式目标目录实际扫描后，MF 有 294 个目标 `AnimSequence`，其中 290 个按完整相对路径对应 Mannequin 源，1 个由正式 Additive Base Pose 明确指向 Locomotion 源，3 个 `SplashPose_Quinn_1/10/11` 仍是 Manny Skeleton 且无正式引用，最终 MF 正式处理集合为 291 个。MM 有 382 个目标 `AnimSequence`，其中 379 个按完整相对路径对应 Mannequin 源，3 个由 AimOffset 基础姿势链明确指向 Locomotion 源，最终 MM 正式处理集合为 382 个。没有使用模糊名称匹配。详细字段和映射边界见 `Investigation_CC动画按需重定向清单.md`。

首轮 MM 代表样本实际调用 `RTG_ChenHaoYu`。`MM_Pistol_Equip`、`MM_Rifle_Equip`、`MM_Shotgun_Fire`、`MM_Rifle_Idle_ADS`、`MM_Rifle_Jog_Fwd`、`MM_Rifle_Idle_ADS_AO_CC` 已被输出到原正式目标路径；调用超出 MCP 单次 30 秒上限后，已先复读资产状态确认没有重复资产，再继续检查，未盲目重复执行。`MM_Shotgun_Fire` 与 `MM_Rifle_Idle_ADS_AO_CC` 的 MeshSpace Additive 和目标 self Base Pose 已按源元数据恢复。

样本检查显示 Root、Pelvis、Spine、clavicle、upperarm、lowerarm、hand、ik_hand_gun、ik_hand_l、IK Foot 和 Scale 没有同类异常；但 `MM_Rifle_Jog_Fwd` 中帧的目标 `weapon_r` 局部位移约为 502，而源同帧约为 6.9，属于明确异常。项目 IKRig/Retargeter 的链条没有 `weapon_r` 专用 Retarget Chain，现有 Retargeter 不足以保留该武器辅助轨道。已停止批量覆盖，不再将该异常扩散到其余正式序列。

随后使用仓库已有 `Scripts/Editor/Animation/MigrateLyraWeaponRBoneTrack.py` 对 6 个 MM 代表样本做了局部 Pose 逐帧轨道修复，并在首/中/末帧比较源目标 weapon_r；复验最大平移误差为 0、最小四元数点积约为 1，Root、Pelvis、Spine、四肢、手部 IK、IK Foot、Scale、Root Motion 和 Additive 全部通过。随后使用 `RTG_ShenWanYun` 完成 MF 的 Pistol Equip、Rifle Equip、Rifle Idle ADS、Pistol Additive AimOffset 和 Quinn Pose 样本验证；5 个样本的 weapon_r 最大平移误差均为 0，Scale、Root Motion、主骨骼、IK 和目标 Skeleton 全部通过。MF AimOffset 的源 Base Pose 是 Locomotion/Pistol Frame 0，已通过 Unreal 原生 `ref_pose_seq` 明确映射到同侧 CC Locomotion Base Pose。当前断点已前移到 MF 全量小批次处理。期间新输出目标偶发出现 `save_loaded_asset=False`，复读后按路径 `save_asset` 成功；若后续重现则停批记录。已知两个损坏 MM Montage 仍只记录为后续可重建对象，本任务不会在缺少旧 Segment、Section、Slot、曲线、Notify 和引用方记录时覆盖它们。

MF 全量试运行继续完成 4 批；因首批长序列按关键帧预算单独成批，实际完成并复验 7 个新 AnimSequence，连同 5 个代表样本和此前提交的 10 个目标序列，当前 MF 正式集合完成 22/291，剩余 269 个。处理 Pistol AimOffset 基础姿势链时，同目录其他 AO AnimSequence 也出现工作树改写；它们仍属于正式集合但尚未完成本任务的 weapon_r/元数据复验，不计入完成数，下一批会按原路径重新覆盖。`AO_MF_Pistol_Idle_ADS` 容器和 `AM_Shotgun_Reload` Montage 已精确恢复，未提交容器/Montage。

本断点随后完成 4 个 MF AimOffset 小批，共 8 个正式 `AnimSequence`：`MF_Pistol_Idle_ADS_AO_LC`、`MF_Pistol_Idle_ADS_AO_LD`、`MF_Pistol_Idle_ADS_AO_LU`、`MF_Pistol_Idle_ADS_AO_RBC`、`MF_Pistol_Idle_ADS_AO_RBD`、`MF_Pistol_Idle_ADS_AO_RBU`、`MF_Pistol_Idle_ADS_AO_RC`、`MF_Pistol_Idle_ADS_AO_RD`。每个目标均使用 `RTG_ShenWanYun` 在原正式路径 overwrite，随后按既有 `weapon_r` 迁移脚本逻辑逐帧修复辅助轨并保存复读。首/中/末帧 `weapon_r` 误差、Root/Pelvis/Spine、clavicle/upperarm/lowerarm/hand、`ik_hand_gun`、`ik_hand_l`、IK Foot、Scale、Root Motion、Additive 类型/Base Pose、曲线、Notify、Sync Marker 均通过，未生成重复后缀资产。MF 进度由 `22/291` 更新为 `30/291`，剩余 `261`。编辑器额外改写的 `AO_MF_Pistol_Idle_ADS` AimOffset 容器已恢复；本批只允许 8 个目标序列和两份调查文档进入提交。下一断点：检查精确 Git 暂存清单并提交推送后，继续剩余 MF 序列。

随后尝试单独处理 MF `MF_Pistol_Idle_ADS_AO_RU` 时，`run_batch_retarget` 超过 300 秒未返回。Unreal 日志记录 Retargeter 生成了重复资产 `MF_Pistol_Idle_ADS_AO_RU1`，并对正式 `MF_Pistol_Idle_ADS_AO_RU` 记录了 `Force Deleting 1 Package(s)`；这违反本任务不得 Force Delete、不得生成重复后缀资产的边界。该调用已停止，`AO_RU` 不计入 `30/291` 完成数，`RU1` 不得提交。当前桥接请求也因编辑器内部任务未返回而阻塞；在 Unreal Asset API 恢复前不再发起重定向或删除操作。

2026-08-19 只读复核 UE 5.8 `UIKRetargetBatchOperation` 源码后确认，`overwrite_existing_files=True` 的覆盖语义本身就是：先生成数字后缀副本，再调用 `ForceReplaceReferences`、`ForceDeleteObjects`，最后把新副本重命名为旧名称。`RU1` 与日志中的 Force Delete 因此属于引擎固定实现，而非只有脏资产才触发的偶发异常。后续正式 CC 动画批处理永久禁用该参数。下一断点是用户授权重启 NewWorldOrder 编辑器后，通过 Unreal API 复读 `RU/RU1` 对象、引用和 Skeleton；随后仅在非正式测试资产上验证“独立临时目录重定向、检查返回路径、用 AnimSequence 数据控制器写回现有正式对象、非 Force 清理零引用临时资产”的替代链。该链路未验证前，不继续剩余 `261` 个 MF 或 `382` 个 MM。

# 2026-08-20 RTG 辅助骨实验结论

用户从 `/Game/Characters/Heroes/Mannequin/Rig/RTG_Mannequin` 复制了两份 Retargeter，并分别把目标 IK Rig 指向男女 CC：

- `/Game/Assets/Characters/CC/ChenHaoYu/RTG_Mannequin_ChenHaoYu`
- `/Game/Assets/Characters/CC/ShenWanYun/RTG_Mannequin_ShenWanYun`

两份资产都保留源 Retargeter 的 9 个 Op，没有 `weapon_r/weapon_l` 专用 Chain。男性资产上的隔离 `Pin Bones` 实验使用 `weapon_r -> weapon_r`、`weapon_l -> weapon_l`、Source Skeleton、Copy Local Position 与 Copy Scale。诊断输出仍出现 `weapon_r` 最大位移误差约 `1.5682`、最小旋转点积约 `0.980638`，`weapon_l` 最大缩放误差约 `1.5588`，不满足精确轨道迁移门槛；失败 Op 已撤销，女性资产没有被修改。

结论是 UE 5.8 当前 Retarget Op 组合不能直接表达“逐帧原样复制辅助骨 Local Translation + Local Rotation + Local Scale”。不能为了追求 RTG 内一键完成而接受厘米级武器偏差，也不能盲目把全部 `ik_hand_*` 或 Virtual Bone 当作 Manny 原值复制，因为 CC 参考姿势与比例存在合法差异。后续 RTG 工具的正式方案固定为：输出到隔离目录，验证目标 Skeleton/帧数/元数据，再调用 `SafeRetargetWriteback.migrate_mannequin_local_tracks` 对白名单辅助骨写回正式对象，并逐帧验证；`overwrite_existing_files=True` 继续永久禁用。

本次 Fire/Reload 16 条与 Equip 8 条正式 CC Action Sequence 已用该白名单写回流程修复 `weapon_r`。这证明后处理方案有效，不代表剩余全量 MF/MM 动画已经完成重定向，也不恢复此前暂停的 `261` 个 MF 或 `382` 个 MM 批次。
