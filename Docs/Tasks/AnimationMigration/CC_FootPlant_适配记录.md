# CC FootPlant Control Rig 适配记录

更新：2026-09-10

## 背景与目标

两个 CC 版本的 `ABP_Mannequin_Base` 原先仍在 Control Rig 节点中引用 `/Game/Characters/Heroes/Mannequin/Rig/CR_Mannequin_FootPlant`。这个源 Control Rig 的预览网格和骨骼契约是 `SK_Mannequin`，不能作为 CC 角色的长期运行时依赖。

目标不是重搭一套 FootPlant 逻辑，而是保留 Lyra 已验证的节点图，复制出男女 CC 专用副本，使用对应 CC 骨架和网格，并只修正实际不兼容的骨骼引用。这样以后逐步移除 `/Game/Characters/Heroes/Mannequin` 时，CC 的脚部动画链不会再直接依赖这份源 Control Rig。

## 资产映射

| 作用 | ChenHaoYu | ShenWanYun |
| --- | --- | --- |
| CC FootPlant Control Rig | `/Game/Characters/Heroes/CC/MM/Rig/CR_ChenHaoYu_FootPlant` | `/Game/Characters/Heroes/CC/MF/Rig/CR_ShenWanYun_FootPlant` |
| CC 预览网格 | `/Game/Characters/Heroes/CC/Meshes/ChenHaoYu/ChenHaoYu` | `/Game/Characters/Heroes/CC/Meshes/ShenWanYun/ShenWanYun` |
| 使用的骨架 | `/Game/Characters/Heroes/CC/Meshes/ChenHaoYu/ChenHaoYu_Skeleton` | `/Game/Characters/Heroes/CC/Meshes/ShenWanYun/ShenWanYun_Skeleton` |
| 使用该 Rig 的 ABP | `/Game/Characters/Heroes/CC/MM/Animations/ABP_Mannequin_Base` | `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base` |

原始 `/Game/Characters/Heroes/Mannequin/Rig/CR_Mannequin_FootPlant` 未修改。

## 适配过程

1. 使用 AssetTools 复制源 Control Rig，保留原有节点、变量、控制器和图布局。
2. 将副本的 Preview Mesh 分别设置为 `ChenHaoYu`、`ShenWanYun`。
3. 将副本的骨骼层级导入对应 CC 网格的骨架，使 Control Rig 层级最终与 CC 骨架一致。
4. 复查 FootPlant 图实际使用的主链骨骼和控制器。`root`、`pelvis`、`thigh_l/r`、`calf_l/r`、`foot_l/r`、`ik_foot_root`、`ik_foot_l/r` 以及 `ball_r` 在两套 CC 骨架中均存在，不需要凭空重建 Two Bone IK、Trace 或坡度处理节点。
5. 复查发现 CC 没有 Lyra 的 `ik_ball_l`、`ik_ball_r`。源图中只有调试用的 `DrawSlope_2.Item` 使用 `ik_ball_r`，它不是姿势输出或 SetTransform 目标。两份副本将该引用改为 CC 的 `ball_r`，并移除层级中不属于 CC 骨架的两个 `ik_ball_*` 额外骨骼。
6. 重新编译两个 Control Rig，并把两个 CC `ABP_Mannequin_Base` 的 Control Rig 节点分别改为对应的 CC 副本。

这里没有把 `ik_ball_*` 强行伪造为 CC 骨骼，也没有把 `ball_r` 当作 Lyra 的变形 corrective 骨；`ball_r` 只用于保留原有调试斜率节点的合法 Item 引用。

## 为什么保留原节点图

FootPlant 图包含足部 Trace、坡度计算、脚部 IK、膝盖 Pole Vector、骨盆和躯干控制器等相互连接的节点。重新逐个创建会引入连线、缓存索引、默认值和控制器设置遗漏的风险。

本次只做以下定向修改：

- Control Rig 资产复制和重命名；
- Preview Mesh 和 CC 骨架层级替换；
- 缺失 `ik_ball_*` 的定向替代；
- ABP Control Rig 节点的资产类引用替换。

其余 140 个原节点和 7 个控制器保留，避免把“复制 Lyra 逻辑”和“重新设计 FootPlant”混成两个任务。

## 验证结果

- 两个 CC Control Rig 均为 `BS_UP_TO_DATE`。
- 两个副本均保留 140 个图节点和 7 个控制器。
- 两个副本各有 123 根骨骼，与对应 CC 骨架的 123 根骨骼一致。
- 图中所有显式 Bone/Control 引用均能在对应 CC Control Rig 层级中找到，缺失引用数为 0。
- 图中不再有 `ik_ball_l`、`ik_ball_r` 或 Lyra twist 骨引用。
- `DrawSlope_2.Item` 在两份副本中均为 `(Type=Bone,Name="ball_r")`。
- 男性 ABP 的 Control Rig 节点引用 `CR_ChenHaoYu_FootPlant`，目标骨架为 `ChenHaoYu_Skeleton`。
- 女性 ABP 的 Control Rig 节点引用 `CR_ShenWanYun_FootPlant`，目标骨架为 `ShenWanYun_Skeleton`。
- 两个 ABP 均编译为 `BS_UP_TO_DATE`，并且不再直接依赖源 `CR_Mannequin_FootPlant`。
- 两个 CC FootPlant 副本的直接依赖中没有 `/Game/Characters/Heroes/Mannequin` 包，只依赖对应 CC 网格。

## 依赖边界

这次只移除了两个 ABP 对 `CR_Mannequin_FootPlant` 的直接依赖，不代表 `ABP_Mannequin_Base` 已经完全脱离 Mannequin 目录。当前两个 ABP 仍可能通过父链、动画层接口、枚举、Notify 和其他 Lyra 共享资产依赖 `SK_Mannequin` 或其他 Mannequin 包。它们属于后续逐项迁移范围，不能因为 FootPlant 已替换就直接删除整个 Mannequin 文件夹。

后续继续迁移时，应分别检查：

- 该引用是否来自目标 ABP 的图节点、父类或接口契约；
- 是否只是编辑器预览/工具资产，还是运行时硬依赖；
- 能否复制出 CC 版本并保持类型契约；
- CC 是否有语义等价骨骼；没有等价物时应关闭对应输出或先协商，不要用名称相似的共享骨代替。

## 以后重新适配的操作清单

1. 先复制源 Control Rig，不修改 `/Game/Characters/Heroes/Mannequin` 源资产。
2. 设置 CC Preview Mesh，并以 CC 网格骨架为唯一层级基准。
3. 对原图每个 Bone/Control Item 做存在性检查。
4. 先区分变形输出、IK 目标和调试节点，再决定缺失骨骼的替代方式。
5. 只有在父子关系、参考姿势和蒙皮用途都能证明相近时，才使用 CC 替代骨；否则保留节点但关闭输出或移除非法 Item 引用。
6. 编译 Control Rig 后，再切换 ABP 节点引用；不要先删除源引用再排查副本。
7. 用 AssetRegistry 复查源 Control Rig 是否仍是 ABP 依赖，并记录仍然存在的其他 Mannequin 依赖。

