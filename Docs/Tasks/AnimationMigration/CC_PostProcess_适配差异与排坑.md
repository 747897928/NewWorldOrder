# CC PostProcess 适配差异与排坑

更新：2026-09-10
适用范围：`ABP_Manny_PostProcess`、`ABP_Quinn_PostProcess`、`CR_Mannequin_Procedural` 迁移到 ChenHaoYu 与 ShenWanYun 的全过程

## 先给结论

这四个资产不能通过“复制后换 Target Skeleton”完成适配。最终方案仍然是复制 Lyra 的成熟节点图，但对 CC 做了四类定向差异处理：

1. 将 Pose Driver、Pose Asset、Control Rig 和 AnimBP 的骨架、骨骼引用切换为对应 CC 版本。
2. 将 Lyra 中语义相近且经过动作姿势验证的 twist 骨映射到 CC 的 `upperarmtwist`、`forearmtwist`、`thightwist`、`calftwist`。
3. 对 CC 没有可靠等价物的 corrective 输出保留原节点作为可追踪结构，但关闭权重、移除驱动白名单或旁路 Pose 输出；不把普通共享骨强行冒充 Lyra correctiveRoot。
4. 保留原有皱纹 AnimBP，使用 `CombinedPostProcess` 在皱纹链之后接入 CC 姿势修正，避免直接替换网格上的 Post Process Anim Blueprint。

因此“复制原图”是正确方向，但“只换骨架”不是完整操作。最终运行时使用的是 CC 独立姿势修正资产的组合入口；Lyra 原始 Mannequin 资产没有被修改，也不是 CC 网格的运行时后处理引用。

## 四个源资产与最终 CC 资产

| Lyra 源资产 | 最终 CC 资产 | 用途 | 最终运行时状态 |
| --- | --- | --- | --- |
| `ABP_Manny_PostProcess` | `/Game/Characters/Heroes/CC/MM/Rig/ABP_ChenHaoYu_PostProcess` | 男性 CC 姿势修正层 | 由 `ABP_ChenHaoYu_CombinedPostProcess` 间接调用 |
| `ABP_Quinn_PostProcess` | `/Game/Characters/Heroes/CC/MF/Rig/ABP_ShenWanYun_PostProcess` | 女性 CC 姿势修正层 | 由 `ABP_ShenWanYun_CombinedPostProcess` 间接调用 |
| `CR_Mannequin_Procedural` | `/Game/Characters/Heroes/CC/MM/Rig/CR_ChenHaoYu_Procedural` | 男性 CC Control Rig | 被 `ABP_ChenHaoYu_PostProcess` 引用 |
| `CR_Mannequin_Procedural` | `/Game/Characters/Heroes/CC/MF/Rig/CR_ShenWanYun_Procedural` | 女性 CC Control Rig | 被 `ABP_ShenWanYun_PostProcess` 引用 |

额外的最终入口是：

- `/Game/Characters/Heroes/CC/MM/Rig/ABP_ChenHaoYu_CombinedPostProcess`
- `/Game/Characters/Heroes/CC/MF/Rig/ABP_ShenWanYun_CombinedPostProcess`

这两个组合 ABP 不是第五、六个独立修正逻辑，而是“原皱纹 AnimBP + 对应 CC 姿势修正 ABP”的运行时包装层。网格应该挂组合入口，而不是直接挂独立的 `ABP_ChenHaoYu_PostProcess` 或 `ABP_ShenWanYun_PostProcess`，否则会覆盖原来的皱纹处理链。

## Mannequin 与 CC 的最终差异

### 骨架和节点图

Mannequin 版本的 Control Rig 使用 Manny 的层级、参考姿势、局部/全局空间关系和 corrective 骨骼。CC 版本保留原节点拓扑、驱动关系和处理顺序，但将目标骨架改为对应 CC Skeleton，并逐项替换能确认语义相近的骨骼引用。

不能把“节点数量相同”当成“行为相同”。Control Rig 节点是在目标骨架的父子关系和参考姿势上求值的；同一个节点在 CC 上可能因为父骨、初始平移、蒙皮权重或姿势空间不同而产生完全不同的结果。

### 骨骼映射

最终启用的主要映射如下：

| Lyra/Manny 语义 | CC 最终骨骼 | 处理结论 |
| --- | --- | --- |
| `upperarm_twist_01/02_{l/r}` | `cc_base_{l/r}_upperarmtwist01/02` | 启用上臂扭转分配 |
| `lowerarm_twist_01/02_{l/r}` | `cc_base_{l/r}_forearmtwist01/02` | 启用前臂、手腕扭转分配 |
| `thigh_twist_01/02_{l/r}` | `cc_base_{l/r}_thightwist01/02` | 启用大腿扭转分配 |
| `calf_twist_01/02_{l/r}` | `cc_base_{l/r}_calftwist01/02` | 启用小腿扭转分配 |
| `calf_twistCor_02_r` | `cc_base_r_calftwist02` | 保留右侧小腿特殊引用 |

以下内容不是可用的等价映射：

| Lyra corrective | 看起来相似的 CC 骨骼 | 最终处理 |
| --- | --- | --- |
| `lowerarm_correctiveRoot_{l/r}` | `cc_base_{l/r}_elbowsharebone` | 不等价；Control Rig 旋转权重置零，Pose Driver 排除 |
| `calf_correctiveRoot_{l/r}` | `cc_base_{l/r}_kneesharebone` | 不等价；Control Rig 旋转权重置零，Pose Driver 排除 |
| `upperarm_correctiveRoot_{l/r}` | CC 普通上臂/twist 骨 | 没有可靠等价物；节点保留但不启用 corrective 输出 |
| `thigh_correctiveRoot_{l/r}` | CC 普通大腿/twist 骨 | 没有可靠等价物；节点保留但不启用 corrective 输出 |
| Lyra `wrist_inner/outer`、`lowerarm_in/out/fwd/bck` | CC 中无直接对应附加骨 | 不伪造一对一映射，使用已验证的 CC twist 骨承担可验证的扭转分配 |
| Lyra clavicle/foot corrective | CC 中没有可靠替代 | 保留 Pose Driver 节点，但在 AnimGraph 旁路其 Pose 输出 |

`elbowsharebone` 和 `kneesharebone` 在 CC 骨架中确实存在，但“名字像肘/膝修正骨”不代表它们与 Lyra 的 correctiveRoot 处于相同父子关系、姿势空间或蒙皮用途。最终方案中它们不是 corrective 替代骨，只是被保留在骨架里但不参与这条后处理输出。

## Control Rig 具体改动

### 保留的内容

- 保留 `CR_Mannequin_Procedural` 的原有节点图、处理阶段和大部分旋转分配逻辑。
- 保留能映射到 CC twist 骨的 `SetRotation` 和驱动链。
- 保留无法暂时验证的节点作为结构线索，避免以后重新搭建时丢失原始意图。
- 只在 CC 专用副本中修改，不修改 Lyra 的 `CR_Mannequin_Procedural`。

### 关闭的内容

1. CC twist 骨的父子关系和初始平移不同于 Manny。Lyra 的 16 个 `SetTranslation` 节点使用全局空间插值，直接照搬会把 CC twist 局部平移推到约 `16--39` 个单位，造成网格明显变形。因此这 16 个节点保留，但 `Weight` 默认值全部置为 `0.0`。
2. 直接目标为 `cc_base_l/r_elbowsharebone`、`cc_base_l/r_kneesharebone` 的四类 `SetRotation` 输出全部置为 `0.0`。它们不能承接 Lyra 的肘、膝 correctiveRoot。
3. 对没有可靠替代骨骼的 upperarm/thigh corrective 输出，不删除节点，而是关闭其有效权重，防止普通 twist 骨被误当成 corrective 根骨。

这不是“把 Control Rig 重搭一遍”，而是保留复制图、关闭已证伪的输出路径。以后如果发现某个 CC 专用骨确实需要位置分配，应只恢复对应的单个节点并重新做 A/B，不能一次性恢复全部 `SetTranslation`。

## AnimBP、Pose Driver 和 Pose Asset 具体改动

### Pose Asset

- 男性 ABP 使用 `/Game/Characters/Heroes/CC/MM/Rig/Poses` 下的 CC Pose Asset。
- 女性 ABP 使用 `/Game/Characters/Heroes/CC/MF/Rig/Poses` 下由 Quinn 迁移得到的 CC Pose Asset。
- 每份 CC 姿势修正 ABP 使用对应性别的 14 个 Pose Asset，共 28 个目标 Pose Asset。
- 对应 `_anim` 序列使用 CC Skeleton 和对应 CC 网格作为 retarget source；`retarget_source_asset` 中的 Mannequin 引用已清除。
- 两个 ABP 的 `PoseTargets` 按实际目标 Pose Asset 重建，不能只换 Pose Asset 路径而保留旧目标骨列表。

### Pose Driver

最终 Pose Driver 只允许驱动经过 CC 骨架和动作姿势验证的 twist 骨：

- `hand_l/r` 驱动 `forearmtwist`。
- `upperarm_l/r` 驱动 `upperarmtwist`。
- `thigh_l/r` 驱动 `thightwist`。
- `calf_l/r` 驱动 `calftwist`。

`elbowsharebone`、`kneesharebone` 已从 `OnlyDriveBones` 中移除。这里有一个 UE 配置陷阱：`OnlyDriveBones` 为空不等于“禁用驱动”，在当前节点语义下可能变成驱动 Pose Asset 中的全部轨道。因此不能用清空列表代替关闭逻辑；无可靠替代的 Pose Driver 必须在 AnimGraph 旁路 Pose 输出，或使用明确的有效白名单。

### 皱纹组合

直接把独立 CC PostProcess 设置给 ShenWanYun 或 ChenHaoYu 网格，会替换原来的 `*_WrinkleAnimBlueprint`，导致“使用后直接不行”以及面部皱纹链丢失。最终做法是：

```text
角色基础姿势
    -> 原有 Wrinkle AnimBP
    -> Linked Anim Graph
    -> 对应性别的 CC PostProcess ABP
    -> CC Control Rig / Pose Driver
    -> 最终蒙皮
```

旧的 `ShenWanYun_WrinkleAnimBlueprint` 和 `ChenHaoYu_WrinkleAnimBlueprint` 仍作为备份保留；网格的正式 Post Process 引用指向两个 `CombinedPostProcess`。

## 早期方案为什么会失败

| 早期做法或现象 | 实际原因 | 最终修正 |
| --- | --- | --- |
| 只替换骨架后编译通过 | 编译只证明引用存在，不证明父子层级、参考姿势和蒙皮语义等价 | 逐项核对骨骼语义、Pose Asset 和 Control Rig 输出 |
| 用 `elbowsharebone` 替代 `lowerarm_correctiveRoot` | CC 共享骨与 Lyra correctiveRoot 的父子关系和姿势空间不等价 | Control Rig 权重置零，Pose Driver 移除 |
| 用 `kneesharebone` 替代 `calf_correctiveRoot` | 同上，动作弯曲时会产生大幅膝部错误旋转 | Control Rig 权重置零，Pose Driver 移除 |
| 只修膝盖不修手臂 | `elbowsharebone` 仍然被写入约 `100` 度以上，手臂仍会局部凹折 | 同时关闭 elbow/knee 两类共享骨 |
| 保留 Lyra `SetTranslation` 全部权重 | CC twist 的父子关系和初始平移不同，全局插值把局部位置推偏 | 16 个位置写入节点保留但权重置零 |
| 把独立 CC PostProcess 直接给网格 | 覆盖原 Wrinkle AnimBP | 新建 `CombinedPostProcess`，在皱纹链之后链接 |
| 认为空 `OnlyDriveBones` 就是不驱动 | UE 可能把空列表解释为驱动 Pose Asset 全部轨道 | 使用明确白名单或 AnimGraph 旁路 |
| 只在静态参考姿势观察“正常” | 静态姿势没有覆盖持枪弯曲、IK 和 corrective 触发区间 | 用 Pistol ADS 等动作姿势做启用/禁用 A/B |

错误映射造成的量化证据：

- `elbowsharebone` 直接写入时约产生 `102--138` 度肘部旋转。
- `kneesharebone` 直接写入时约产生 `107--125` 度膝部旋转。
- 未关闭 `SetTranslation` 前，CC twist 局部平移变化最大约 `38.8`；关闭后最大变化约 `1.97`。

## 最终验证结果和边界

已确认的结果：

- 两个 CC Control Rig 编译成功，原有节点图保留，目标骨骼引用已切换到对应 CC 层级。
- 两个 CC PostProcess ABP 编译成功，分别引用对应的 CC Control Rig 和 CC Pose Asset。
- 四个独立 CC ABP/Control Rig 包没有 `/Game/Characters/Heroes/Mannequin` 的直接包依赖。
- `MF_Pistol_Idle_ADS_AO_CD` 和 `MM_Pistol_Idle_ADS_AO_CD` 的代表性姿势中，启用组合后处理后不再出现之前的手腕扭曲、肘部大角度旋转、膝部内折或整条肢体崩坏。
- 已执行后处理启用/禁用以及正面、侧面、背面检查；剩余修正主要落在 CC twist 骨，不再直接旋转肘膝共享骨。

仍然需要扩展回归的内容：

- 三把武器全部 Fire、Reload、Equip、Unequip。
- 移动、跳跃、蹲伏、瞄准叠加，以及真实单人 PIE、分屏和 Listen Server。
- 不同 LOD、左右侧面和真实手部与武器的接触。

因此当前任务可写成“CC PostProcess 已完成适配，橡皮手目前暂时看上去已解决，待扩展回归”，不能写成“所有动画和所有 LOD 永久解决”。

## 以后重新适配时的操作清单

1. 先复制资产到 CC 专用目录，保留源 Mannequin 资产不动；不要从空白 Control Rig 重建。
2. 设置目标 Skeleton 后，逐项检查 Control Rig 的骨骼引用、父子关系、参考姿势和空间类型；不要因为同名就直接替换。
3. 先只启用确认等价的 upperarm/forearm/thigh/calf twist 骨，所有 correctiveRoot、共享骨和附加骨单独验证。
4. 复制作姿势 Asset 和 `_anim` 序列，清理 Mannequin retarget source，重建 PoseTargets 与 `OnlyDriveBones`。
5. 对 Control Rig 的 `SetTranslation` 和 corrective `SetRotation` 逐节点检查权重；没有证据时保留节点但置零，不删除节点。
6. 如果没有可靠 CC 替代骨，记录为“无可靠替代、关闭输出”，不要擅自用 `elbowsharebone` 或 `kneesharebone` 顶替。
7. 如果角色已有皱纹、头发或鞋履后处理，必须复制成组合入口，不能直接覆盖网格原有 Post Process Anim Blueprint。
8. 至少用一个静态姿势和一个持枪弯曲姿势做 PostProcess 启用/禁用 A/B，并从正面、侧面、背面检查肘、膝、腕、脚踝和整条肢体。
9. 通过编译、引用、资产级骨骼输出检查后，再做 PIE；只把确认过的 CC 资产提交，不能恢复 Mannequin 运行时引用。

## 对旧中间结论的更正

以下说法需要按最终方案理解，而不能直接照抄：

```text
CC 没有部分 Lyra 附加骨，已使用 forearmtwist、elbowsharebone 等替代。
```

最终准确说法是：

```text
CC 没有 Lyra 的部分附加骨；已使用经过验证的 upperarmtwist、forearmtwist、thightwist、calftwist 承担可验证的扭转分配。
elbowsharebone、kneesharebone 虽然存在于 CC 骨架，但不能等价替代 Lyra correctiveRoot，最终保留节点结构并关闭 Control Rig 旋转输出，同时从 Pose Driver 白名单中移除。
```

因此，旧对话中提到“使用 elbowsharebone 等替代”的内容属于适配过程中的错误中间方案；最终修复正是通过撤销这条语义错误映射完成的。

## 相关文档

- [CC PostProcess 适配记录](CC_PostProcess_适配记录.md)：资产清单、当前运行时接入、验证结果和原理说明。
- [动画迁移任务交接](Overview_交接.md)：迁移任务总状态和后续 Mannequin 依赖清理边界。
- [橡皮手与武器动画叠加链调查](../LyraShooterCoreAdaptation/Investigation_橡皮手与武器动画叠加链.md)：橡皮手历史取证、`weapon_r` 修复和扩展回归门槛。
- [动画辅助骨轨修复](AuxiliaryBoneTrackRepair_辅助骨轨修复.md)：重定向后 `weapon_r`、IK 手骨等辅助轨道的独立排查流程。
