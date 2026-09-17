# 动画辅助骨轨修复

更新：2026-09-13

## 适用症状

从 Manny 动画重定向到正式 CC 骨架后，如果出现以下现象，先审计辅助骨局部轨道，不要立即修改 AnimBP、Socket 或武器 Actor Transform：

- 武器在 Fire、Reload、Equip 或 Unequip 中脱手、飞头、突然旋转或缩放。
- 静止时正常，叠加移动、跳跃或蹲伏后双手被拉长、扭曲。
- 动画视觉预览大体正常，但运行时 TwoBoneIK、手部 IK 或武器挂点开始消费辅助骨后异常。
- 轨道只在末帧或少数中间帧出现极大位移尖峰。

本项目已经验证：IK Retargeter 的身体重定向成功，不等于 `weapon_r`、`weapon_l` 和 IK 手骨的动态 Local Transform 被精确复制。给 RTG 增加 Pin Bone 也不能保证逐帧 Local T/R/S 与 Manny 源一致。

## 权威源与正式目标

- 权威源：`/Game/Characters/Heroes/Mannequin/Animations` 下与正式动作同名、同帧率、同关键帧数的 Manny AnimSequence。
- 正式目标：`/Game/Characters/Heroes/CC/MM/Animations` 或 `/Game/Characters/Heroes/CC/MF/Animations` 下的 CC AnimSequence。
- 安全脚本：`Scripts/Editor/Animation/SafeRetargetWriteback.py`。
- 调用入口：`migrate_mannequin_local_tracks(mannequin_source_path, formal_anim_path, track_names)`。

不要直接复制 `.uasset`，不要用文件系统覆盖正式资产，也不要用 `overwrite_existing_files=True`。UE 5.8 的覆盖式重定向可能 Replace References、Force Delete 正式包，再把临时包改名；这会破坏外部引用和 Git 审计。

## 白名单原则

每次只回写已经证明异常、且语义上应沿用 Manny 动态轨道的辅助骨。候选白名单如下：

- `weapon_r`
- `weapon_l`
- `ik_hand_gun`
- `ik_hand_l`
- `ik_hand_r`

不要把候选列表一次性全写入。先逐骨、逐动作比较源目标 Local T/R/S；只有误差超限或运行时节点实际消费该骨时才加入本次 `track_names`。身体骨、Root、Pelvis、Spine、手臂 Twist 骨和虚拟骨不属于默认白名单。

虚拟骨是否需要处理必须单独审计。虚拟骨通常由 Skeleton 定义而不是 AnimSequence 原始骨轨驱动，不能假设 RTG 或本脚本会复制它们。

## 标准修复流程

1. 保存 Git 检查点，确认正式目标和用户其他动画改动的归属。
2. 读取源/目标 AnimSequence 的 Skeleton、帧率、关键帧数、Additive 类型和 Base Pose。
3. 对候选辅助骨逐帧比较 Local Translation、Rotation、Scale，记录最大位移误差、最小四元数点积和最大缩放误差。
4. 只把确认异常的骨名传给 `migrate_mannequin_local_tracks`。
5. 脚本通过正式资产自己的 `AnimationDataController` 写入指定轨道，不替换 UObject 或包路径。
6. 复读 Skeleton、Additive、Base Pose、曲线、Notify、Sync Marker、Montage 引用，确认非白名单数据未被改动。
7. 在 PIE 分别验证静止、移动、跳跃、蹲伏下的 Fire、Reload、Equip、Unequip；同时观察武器和左右手。
8. 只选择性暂存实际修复的 AnimSequence 与本任务文档，提交并推送。

## 验证阈值

脚本当前默认采用：

- Local Translation 最大误差不超过 `1.0e-4`。
- Local Rotation 四元数绝对点积不低于 `0.999999`。
- Local Scale 最大误差不超过 `1.0e-5`。

任何一项超限都不能写成“修复完成”。数值验证通过只证明辅助骨轨与源一致，仍需 PIE 证明运行时视觉正常。

## 已验证边界

- 男性 `MM_Rifle_Jog_Fwd.weapon_l` 曾出现约 `1078.8` 的局部位移误差；白名单回写后降到浮点误差范围。
- 女性 `MF_Pistol_Jog_Fwd.ik_hand_gun` 曾出现约 `118.2` 的位移误差，旋转点积约 `0.589`。
- 给 RTG 添加 `weapon_r`、`weapon_l` Pin Bone 的实验仍留下 `weapon_r` 位移误差约 `1.5682`、旋转点积约 `0.980638`，以及 `weapon_l` 缩放误差约 `1.5588`，因此不能把 RTG Pin 视为精确复制方案。
- Fire、Reload、Equip 的 `weapon_r` 已按上述流程修复并通过用户视觉验收；Unequip 复用 Equip 底层动作的活动链也已验证。

## 2026-08-23 Rifle Grenade Toss 修复

用户重新重定向四个正式 Rifle Grenade Toss Action Sequence 后，目标资产的动态 `weapon_r` 轨道出现丢失/错误。按同名 Manny 源逐帧执行 `SafeRetargetWriteback.migrate_mannequin_local_tracks(..., ["weapon_r"])`，没有重定向或覆盖 Montage 容器：

- `/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Rifle_GrenadeToss`
- `/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Rifle_GrenadeToss_Additive`
- `/Game/Characters/Heroes/CC/MM/Animations/Actions/MM_Rifle_GrenadeToss`
- `/Game/Characters/Heroes/CC/MM/Animations/Actions/MM_Rifle_GrenadeToss_Additive`

四个源/目标均为 30 帧、30 FPS；普通序列保持 `AAT_NONE`，Additive 序列保持 `AAT_LOCAL_SPACE_BASE`。写回后 `weapon_r` 的最大局部位移误差为 `0`，最大缩放误差为 `0`，最小旋转四元数点积约为 `1`。本轮未主动修改曲线、Notify、Timing、Additive/Base Pose、Montage 或其他骨轨。

## 2026-08-23 MF Aim Offset 修复

MF Rifle Aim Offset 容器被 `/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/ABP_RifleAnimLayers_Feminine` 直接引用。复读发现 MF 的 Rifle 与 Unarmed 两组 Aim Offset 样本均有重定向后的动态 `weapon_r` 偏差，而 MM 对应样本与 Manny 源一致。已按同名 Manny 源逐帧修复以下两组各 15 个正式 MF `AnimSequence` 的 `weapon_r`，没有修改 Aim Offset 容器、其他骨轨或 Additive/Base Pose：

- `MM_Rifle_Idle_Hipfire_AO_CC/CD/CU/LC/LD/LU/LBU/LBD/LBC/RC/RD/RU/RBC/RBD/RBU`
- `MM_Unarmed_Idle_Ready_AO_CC/CD/CU/LC/LD/LU/LBU/LBD/LBC/RC/RD/RU/RBC/RBD/RBU`

修复前 MF Rifle 样本 `weapon_r` 最大局部位移误差约 `78.78`、最低旋转点积约 `0.00165`；MF Unarmed 样本最大位移误差约 `28.57`、最低旋转点积约 `0.0224`。30 个样本写回后最大位移误差均为 `0`，最大缩放误差均为 `0`，最低旋转点积约为 `1`；所有样本仍保持 `AAT_ROTATION_OFFSET_MESH_SPACE`、30 FPS、2 帧和 ShenWanYun Skeleton。PIE 已启动确认编辑器运行正常，但本轮未自动拾取武器，因此不把 Rifle 持握视觉写成已验收。

## 2026-09-13 Body Control Rig 武器控制器

为了在 Sequencer 中直接给 `weapon_r`、`weapon_l` 的装备姿态打关键帧，已在以下两个正式 Body Control Rig 中增加可动画控制器：

- `/Game/Characters/Heroes/CC/MF/Rig/CR_ShenWanYun_Body`
- `/Game/Characters/Heroes/CC/MM/Rig/CR_ChenHaoYu_Body`

控制器和求解链约定如下：

- `weapon_r_ctrl` 显示名为 `weapon_r`，父级为现有骨骼 `hand_r`；`weapon_l_ctrl` 显示名为 `weapon_l`，父级为现有骨骼 `hand_l`。
- 两个控制器均为 `EULER_TRANSFORM`、`ANIMATION_CONTROL`，初始偏移取对应 `weapon_r`、`weapon_l` 的局部变换，因此控制器初始姿态与装备挂点重合。
- `Forwards Solve` 在主身体求解完成后，将 `weapon_r_ctrl`、`weapon_l_ctrl` 的 Global Transform 写入对应武器骨骼；`Backwards Solve` 将武器骨骼回写控制器，供烘焙和反向匹配使用。
- 这次只增加动画制作层控制器和 RigVM 连线，没有新增骨骼、Socket 或运行时武器 Actor 变换。运行时装备仍沿用现有 `weapon_r` 挂点及其既有朝向约定。

Sequencer 使用时，在对应 Control Rig 轨道上给 `weapon_r_ctrl` 或 `weapon_l_ctrl` 的 Transform 通道打关键帧；不要把 `ik_hand_gun` 当作 `weapon_r` 的替代控制器。若重新生成 Body Control Rig，必须保留这两个控制器、它们在 `hand_r`/`hand_l` 下的初始偏移，以及前后向求解中的武器骨骼同步节点。

## 2026-09-14 蹲伏 Locomotion 辅助轨修复

此前 MF/MM 两侧各 65 个蹲伏 Locomotion 目标曾缺少 `weapon_l`；补回固定 CC 参考姿势后，PIE 左手仍向右偏。重新比较同名 Mannequin 源和 CC 目标发现，`weapon_l` 已逐帧一致，真正异常在左手 IK 和武器参考空间输入：

- `ik_hand_l`：两侧 65/65 均有重定向差异。
- `ik_hand_gun`：两侧 65/65 均有重定向差异。
- `weapon_r`：两侧各 21 个 Unarmed 蹲伏目标没有轨道，评估时退回 CC Skeleton 参考姿势。

当前 Linked Layer 的运行时消费链是 `VB IK_Hand_L_weaponSpace -> CopyBone(ik_hand_l) -> TwoBoneIK(hand_l Effector)`；该虚拟骨骼的空间又与 `weapon_r` 相关，因此不能只盯着名字为 `weapon_l` 的轨道。部分站立 CC MF 动作没有显式 `weapon_l` 轨道，站立轨道也不是可靠的蹲伏替代物。

本轮使用本文件顶部规定的 `migrate_mannequin_local_tracks`，没有做文件系统覆盖：

- MF/MM 各 65 个蹲伏目标写回 `ik_hand_l`、`ik_hand_gun`。
- MF/MM 各 21 个缺少 `weapon_r` 的 Unarmed 蹲伏目标补回 `weapon_r`。
- `weapon_l` 不重复覆盖，因为它已经与同名源一致。

所有源/目标关键帧数量和帧率一致；每个写回资产均通过逐帧 Local T/R/S 阈值验证，目标 Skeleton 和其他元数据保持不变。最终 130 个目标的四条辅助轨均与同名源逐帧一致。静态修复不等价于 PIE 视觉验收；下一步必须在安全地图验证蹲伏静止、移动、进入和退出状态。

## 禁止事项

- 禁止为了修一个辅助骨，批量覆盖整个正式动画目录。
- 禁止在没有源/目标帧数和帧率一致证据时写轨道。
- 禁止把全骨架写回接口用于局部问题。
- 禁止修改 AnimationAssetFixer；它不是本流程的轨道修复工具。
- 禁止仅凭 Montage 曲线、Socket 或 AnimBP 表象断言根因。
- 禁止在验证失败后提交诊断副本、重复后缀包或 `Content/Developers` 临时资产。

## 维护结论

以后从 Manny 重定向新动作时，默认流程应是“RTG 负责身体姿势，辅助骨逐轨审计，异常轨按白名单回写”。不能承诺一次 RTG 输出永远不会再出现 `weapon_r`、`weapon_l` 或 IK 手骨损坏。
