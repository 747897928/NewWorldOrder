# CC PostProcess 适配记录

## 背景与目标

Lyra 的 `MF_Pistol_Idle_ADS_AO_CD` 等动画重定向到 CC 角色后，左手腕和前臂会出现扭曲。Lyra 的 `ABP_Manny_PostProcess`、`ABP_Quinn_PostProcess` 会在基础动画之后使用 Pose Driver 和 `CR_Mannequin_Procedural` 做扭转骨和 corrective 修正，因此本次工作的目标是保留这套成熟逻辑，把它迁移到 CC 的男、女骨架上。

本次不修改 `/Game/Characters/Heroes/Mannequin` 下的源资产，也不重新凭空搭建 Control Rig。实现方式是复制 Lyra 的 ABP 和 Control Rig，再只对 CC 骨架、骨骼名、Pose Asset 和 Control Rig 引用做定向适配，为以后逐步移除 Mannequin 依赖留出独立资产。

四个资产从 Mannequin 到 CC 的最终差异、失败中间方案、节点权重处理和以后重新适配的操作清单，见 [CC PostProcess 适配差异与排坑](CC_PostProcess_适配差异与排坑.md)。本记录保留验证结果和当前运行时接入状态；排坑文档用于接手者复现和维护。

## 已创建的 CC 资产

男角色 ChenHaoYu：

- `/Game/Characters/Heroes/CC/MM/Rig/ABP_ChenHaoYu_PostProcess`
- `/Game/Characters/Heroes/CC/MM/Rig/CR_ChenHaoYu_Procedural`
- `/Game/Characters/Heroes/CC/MM/Rig/Poses/Manny_*_pose`
- `/Game/Characters/Heroes/CC/MM/Rig/Poses/Manny_*_anim`

女角色 ShenWanYun：

- `/Game/Characters/Heroes/CC/MF/Rig/ABP_ShenWanYun_PostProcess`
- `/Game/Characters/Heroes/CC/MF/Rig/CR_ShenWanYun_Procedural`
- `/Game/Characters/Heroes/CC/MF/Rig/ABP_ShenWanYun_CombinedPostProcess`
- `/Game/Characters/Heroes/CC/MF/Rig/Poses/Quinn_*_pose`
- `/Game/Characters/Heroes/CC/MF/Rig/Poses/Quinn_*_anim`

男角色另外创建了组合入口：

- `/Game/Characters/Heroes/CC/MM/Rig/ABP_ChenHaoYu_CombinedPostProcess`

ABP 和 Control Rig 均已重新编译并保存，目标骨架分别是 `ChenHaoYu_Skeleton` 和 `ShenWanYun_Skeleton`。两份 Control Rig 保留了 Lyra 原有节点图，只把层级骨骼替换为对应 CC 网格的骨骼层级。

## 骨骼替换策略

CC 骨架没有 Lyra 的全部 corrective 骨骼，所以替换按“语义相近的 CC 骨骼优先，无法等价时禁用该修正”的原则处理。

| Lyra 骨骼 | CC 替代 | 说明 |
| --- | --- | --- |
| `upperarm_twist_01/02_{l/r}` | `cc_base_{l/r}_upperarmtwist01/02` | 上臂扭转 |
| `lowerarm_twist_01/02_{l/r}` | `cc_base_{l/r}_forearmtwist01/02` | 前臂、手腕扭转的主要替代 |
| `thigh_twist_01/02_{l/r}` | `cc_base_{l/r}_thightwist01/02` | 大腿扭转 |
| `calf_twist_01/02_{l/r}` | `cc_base_{l/r}_calftwist01/02` | 小腿扭转 |
| `lowerarm_correctiveRoot_{l/r}` | 不直接写入 `cc_base_{l/r}_elbowsharebone` | CC 的肘部共享骨不能证明等价；对应 Control Rig 旋转权重置零，Pose Driver 排除 |
| `calf_correctiveRoot_{l/r}` | 不直接写入 `cc_base_{l/r}_kneesharebone` | CC 的膝部共享骨不能证明等价；对应 Control Rig 旋转权重置零，Pose Driver 排除 |
| `calf_twistCor_02_r` | `cc_base_r_calftwist02` | 右侧小腿特殊引用 |

Pose Driver 的目标骨骼也已改为 CC 扭转骨：`hand_l/r` 驱动前臂扭转，`upperarm_l/r` 驱动上臂扭转，`thigh_l/r` 和 `calf_l/r` 驱动大腿、小腿扭转。`elbowsharebone` 与 `kneesharebone` 不再作为 corrective 输出目标。

## 没有一一对应的部分

以下 Lyra corrective 没有 CC 中可确认的等价骨骼：

- `upperarm_correctiveRoot_{l/r}`：暂时保留 Control Rig 节点，但映射到上臂扭转骨并把对应权重置为 `0.0`，避免把普通扭转骨当作 corrective 根骨直接施加错误变形。
- `thigh_correctiveRoot_{l/r}`：处理方式相同，暂时不启用该 corrective 根骨。
- Lyra 的 `clavicle` 和 `foot` corrective 驱动在 CC 中没有可靠的替代。对应 Pose Driver 节点没有删除，而是在 AnimGraph 中旁路其 `Pose` 输出；不能用空的 `OnlyDriveBones` 列表表示禁用，因为该列表为空时 UE 会驱动 Pose Asset 中的全部轨道。
- CC 没有 Lyra 的 `wrist_inner/outer`、`lowerarm_in/out/fwd/bck` 等附加骨；目前依赖 CC 自己的 `forearmtwist`、`upperarmtwist`、`thightwist` 和 `calftwist` 完成可验证的扭转分配。
- `cc_base_*_elbowsharebone` 和 `cc_base_*_kneesharebone` 虽然名称看起来像肘、膝修正骨，但它们与 Lyra correctiveRoot 的父子关系、绑定用途和姿势空间并不等价。实际 `MF/MM_Pistol_Idle_ADS_AO_CD` 评估中，直接写入它们会分别造成约 `102--138` 度的肘部旋转和约 `107--125` 度的膝部旋转，达到整条手臂/腿崩坏的级别，因此按“没有可靠替代”处理，而不是继续强行映射。

这些不是“已经证明完全等价”的替换。如果实际预览中仍有局部肩、肘、脚踝或大腿变形，需要根据 CC 的绑定权重和骨骼用途继续调整；若没有合适替代骨，应先确认是否接受关闭对应 corrective，而不是擅自把普通骨骼当作等价骨骼使用。

## 2026-09-10 复制后适配与组合接入

直接把独立的 `ABP_ShenWanYun_PostProcess` 设置到网格会替换原来的 `ShenWanYun_WrinkleAnimBlueprint`，因此会丢失面部皱纹处理链。当前采用组合方式：复制皱纹 ABP 作为基底，在其最终输出之后添加 `Linked Anim Graph`，调用独立的 CC 姿势修正 ABP。这样保留皱纹、曲线和原有面部 Control Rig，同时把手腕修正放在最后执行。

复制的 `CR_Mannequin_Procedural` 不能只按骨骼名字替换。Lyra 的 `SetTranslation` 节点把扭转骨当作独立分支，并用 `GlobalSpace` 插值写入位置；CC 的 twist 骨骼存在不同的父子关系和初始平移，继续执行这些节点会把 CC twist 局部平移推到约 16--39 个单位，造成网格明显变形。因此两个 CC Control Rig 均保留这些节点和原有旋转逻辑，但把 16 个 `SetTranslation.Weight` 默认值设为 `0.0`。这不是删除节点，后续如果确认某条 CC 骨骼需要位置分配，可单独恢复和调试对应节点。

两个 CC ABP 的 Pose Driver 已重新指向各自 CC Pose Asset，并按目标 Pose Asset 重建 `PoseTargets`。没有可靠替代骨骼的四个 clavicle/foot 驱动节点保留在图中但不再参与输出。

## 2026-09-10 膝部与肘部 corrective 回归修正

用户提供的动作姿势截图确认，之前把 Lyra 的 `lowerarm_correctiveRoot`、`calf_correctiveRoot` 分别替换成 CC 的 `elbowsharebone`、`kneesharebone` 是错误的语义替换。静态参考姿势看起来正常，不能证明持枪弯曲姿势中的整条手臂和腿正确。

- 两份 CC Control Rig 的 `SetRotation` 节点仍保留在原图中，但所有直接目标为 `cc_base_l/r_elbowsharebone`、`cc_base_l/r_kneesharebone` 的节点权重均改为 `0.0`。
- 两份 CC PostProcess ABP 的 Pose Driver `OnlyDriveBones` 已移除上述四类共享骨，只保留对应 CC 的 `forearmtwist`、`upperarmtwist`、`thightwist`、`calftwist`。
- 这次修正不是删除 Lyra 节点或重搭 Control Rig，而是保留复制来的节点图，关闭经动作姿势验证为错误的 CC 输出路径；CC twist 分配仍由原有节点链负责。
- `MF_Pistol_Idle_ADS_AO_CD` 与 `MM_Pistol_Idle_ADS_AO_CD` 的预览动作在启用和禁用后处理时均已对照检查。启用后处理时两侧不再出现之前的膝部内折、肘部异常旋转或整条肢体崩坏；禁用后处理时也没有把后处理错误误认为基础动画问题。

## 原理结论：后处理为何同时改善手腕与“橡皮手”

这里的“后处理”不是渲染阶段的画面滤镜，而是 Skeletal Mesh 在基础动画、重定向结果和皱纹动画之后执行的 AnimBP 姿势修正层。它处在最终骨骼姿势送入蒙皮之前，因此能够读取已经重定向到 CC 的主链姿势，再补写 CC 网格真正需要的扭转和 corrective 骨骼。

重定向器主要保证 `root/pelvis/spine/upperarm/lowerarm/hand/thigh/calf/foot` 等主链能够传递动作。它不会自动证明 Manny 的辅助骨在 CC 中具有相同的父子关系、参考姿势、Retargeting Mode、姿势空间和蒙皮用途。CC 网格如果缺少有效的前臂和小腿扭转分配，主链两端虽然看起来都在正确位置，中间的蒙皮仍会把旋转集中到错误区域，形成手腕糖纸式扭曲、肘部塌陷、手部拉长以及类似“橡皮手”的视觉结果。手部 IK 或 `TwoBoneIK` 继续约束手掌时，还可能把这种错误变形放大，所以问题不一定只出现在手腕本身。

本次组合后处理的职责可以概括为：

1. Pose Driver 读取 `hand/upperarm/thigh/calf` 等驱动骨的姿势，在 Pose Asset 中查找对应的 corrective 姿势，并把结果限制到 CC 的 `forearmtwist`、`upperarmtwist`、`thightwist` 和 `calftwist` 等真实替代骨。
2. 复制而来的 CC Control Rig 在输入姿势之后分配这些扭转和姿势修正，使旋转沿 CC 的骨骼链和蒙皮权重展开，而不是强行把 Lyra 的 corrective 根骨值写进语义不等价的 CC 共享骨。
3. `ABP_*_CombinedPostProcess` 先保留原来的皱纹 AnimBP，再通过 `Linked Anim Graph` 调用 CC 姿势修正 ABP。因此它不是用一个新后处理替换面部皱纹，而是在原有面部链之后增加身体骨骼的变形修正。

这解释了为什么一个后处理入口可能同时改善手腕和疑似橡皮手：它们可能共享同一类“主链重定向成功，但 twist/corrective 分配不正确”的底层缺口。不过这只是基于当前 A/B 结果的根因推断，不代表所有历史橡皮手现象都已被证明来自同一条链。`weapon_r` 写回解决的是武器或挂点脱手；后处理解决的是角色肢体的姿势分配，两者必须分开记录。

这套职责与 Unreal 官方对 Pose Driver、Animation Blueprint 中 Control Rig 以及重定向动画边界的说明一致：

- [Pose Driver in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-driver-in-unreal-engine?lang=en-US)
- [Control Rig in Animation Blueprints in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-in-animation-blueprints-in-unreal-engine?lang=en-US)
- [Using Retargeted Animations in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-retargeted-animations-in-unreal-engine)

## 2026-09-10 橡皮手问题的阶段性结论

本轮在 `MF_Pistol_Idle_ADS_AO_CD`、`MM_Pistol_Idle_ADS_AO_CD` 和默认姿势上，对男女网格分别执行了组合后处理启用/禁用的对照，并从正面、侧面和背面检查。启用组合后处理后，之前的手腕扭曲和整条肢体级别的膝、肘错误没有再出现；禁用后处理时也没有把基础姿势问题误判成后处理修复。男性网格已经确认使用 `ABP_ChenHaoYu_CombinedPostProcess`，女性网格使用 `ABP_ShenWanYun_CombinedPostProcess`。

因此当前可以在任务中这样标记：

> 橡皮手：目前暂时看上去已解决（阶段性结论）。

这里的“暂时看上去已解决”有明确边界：当前证据主要覆盖代表性的 Pistol ADS 姿势和默认姿势，尚未覆盖三把武器全部 Fire/Reload/Equip/Unequip、移动/跳跃/蹲伏叠加、真实 PIE 输入、不同 LOD 以及男女所有运行入口。在这些回归完成前，不能把任务改成“橡皮手永久解决”，也不能删除原调查记录。

任务层的当前状态和后续回归入口记录在 [橡皮手与武器动画叠加链调查](../LyraShooterCoreAdaptation/Investigation_橡皮手与武器动画叠加链.md) 与 [Lyra ShooterCore 适配状态](../LyraShooterCoreAdaptation/Status_状态.md) 中。

## 验证结果

- 两份 CC Control Rig 编译成功，图中直接引用的骨骼均能在 CC 层级中找到，没有遗留的 Lyra 骨骼名。
- 两份 CC Control Rig 编译成功，`SetTranslation` 的 16 个位置写入节点均已验证为零权重；旋转修正节点和原节点图仍保留。
- 两份 CC Control Rig 中四类 `elbowsharebone/kneesharebone` 旋转输出均已验证为零权重；两份 CC PostProcess ABP 的 Pose Driver 白名单也不再包含这些共享骨。
- 两份 CC PostProcess ABP 编译成功，均引用对应的 CC Control Rig；没有 CC 等价输出的 Pose Driver 已通过图连接旁路，而不是用空 `OnlyDriveBones` 误当禁用。
- 每份 ABP 都引用对应目录下的 14 个 CC Pose Asset，没有交叉使用男、女 Pose Asset；其中女角色 ABP 已修正为引用 Quinn Pose Asset 迁移到 CC 后的版本。
- 28 个目标 Pose Asset 的 `retarget_source_asset` 已清除 Mannequin 引用；对应 `_anim` 序列的骨架是 CC 骨架，retarget source 是对应 CC 网格。
- 通过 Asset Registry 检查，四个新 ABP/Control Rig 包没有 `/Game/Characters/Heroes/Mannequin` 包依赖。
- 以 `MF_Pistol_Idle_ADS_AO_CD` 的 CC 动画姿势做资产级评估：修正前 CC twist 局部平移最大变化约 `38.8`，修正后最大变化约 `1.97`；组合 ABP 与原皱纹 ABP 的输出差异只来自 CC 姿势修正链。
- 对男女 `MF/MM_Pistol_Idle_ADS_AO_CD` frame 0 的组合输出检查显示，剩余修正只落在 CC twist 骨，不再直接旋转肘、膝共享骨；启用/禁用后处理的前、侧、背视图切换均已执行，未再观察到整条肢体级别的错误。
- 原始 `ABP_Manny_PostProcess`、`ABP_Quinn_PostProcess`、`CR_Mannequin_Procedural` 未修改。

## 当前运行时接入状态

ShenWanYun 网格当前已配置：

- `/Game/Characters/Heroes/CC/Meshes/ShenWanYun/ShenWanYun` 的 Post Process Anim Blueprint 为 `ABP_ShenWanYun_CombinedPostProcess`

ChenHaoYu 网格当前已配置：

- `/Game/Characters/Heroes/CC/Meshes/ChenHaoYu/ChenHaoYu` 的 Post Process Anim Blueprint 为 `ABP_ChenHaoYu_CombinedPostProcess`

这两个组合 ABP 分别以旧的 `ShenWanYun_WrinkleAnimBlueprint`、`ChenHaoYu_WrinkleAnimBlueprint` 为基底，再调用对应独立 CC PostProcess ABP。旧皱纹 ABP 保留为备份，不与新的融合入口重名。编辑器网格预览显示男女组合后处理均正在运行，网格没有出现直接替换独立 ABP 时的异常状态。

后续仍需在实际游戏姿势、不同 LOD 和左右手武器动作中确认左腕视觉结果；若某个 CC twist 或 corrective 骨骼仍有局部问题，应继续在对应副本中调整，不要恢复对 Mannequin 资产的运行时引用。
