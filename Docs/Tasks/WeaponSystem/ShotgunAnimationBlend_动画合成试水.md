# Shotgun_A 角色动画离线合成试水任务

## 任务目的

本任务只验证 AI 能否用 Blender 5.2 或 Unreal Editor 的 Sequencer、Control Rig，把商城霰弹枪动画中需要的上半身动作，合成到项目现有 Lyra/CC 角色动画的下半身站姿上，并烘焙成新的 AnimSequence。

本任务不是 Shotgun_A Gameplay Ability、弹药、拾取、网络或武器机械动画任务。不要修改 C++、Gameplay Ability、Input、武器蒙太奇同步逻辑，也不要用运行时 AnimBlueprint 分层混合替代离线动画合成。

先只做一个最小试样：MF Reload。该试样通过视觉验收后，再决定是否继续 MM Reload、MF Fire、MM Fire。

## 项目与安全边界

- Unreal 项目：NewWorldOrder，EngineAssociation 以 `NewWorldOrder.uproject` 为准。
- 开工前完整阅读 `AGENTS.md`、`Docs/DevelopmentNotes/MCP_踩坑记录.md`、`Docs/Tasks/WeaponSystem/AnimationArchitecture_动画架构.md`。
- Unreal 资产只能通过 UE MCP、Python、AssetRegistry、Sequencer、Control Rig 或编辑器 UI 读取和编辑，禁止读取或直接编辑 `.uasset` 二进制。
- 不得覆盖、移动、重命名或删除任何源动画。
- 所有试验产物先放在 `/Game/Developers/AnimationTrials/Shotgun_A`。未经用户验收，不得替换正式 Shotgun_A 动画引用。
- Blender 中间文件、FBX 和诊断截图放在项目内 `Saved/AnimationTrials/Shotgun_A`，不要写到项目外。
- 不要删除源动画中的 Notify、Sync Marker、Curve 或 Montage Section。试样阶段如未能原样转移，应在交付报告中逐项列出，不得假装完成。

## 必须理解的现有播放架构

- 角色与武器并不是由同一套骨骼动画驱动。
- 角色蒙太奇中的 `/Game/Characters/Heroes/Mannequin/Animations/AnimNotifies/AN_PlayWeaponMontage` 及其 Shotgun_A 子类负责触发武器蒙太奇。
- 武器蒙太奇使用 Montage Sync Follow 跟随角色蒙太奇，驱动 Shotgun_A 自身的 `Ammo`、`AmmoEject`、`Clip_Bone` 等机械骨骼。
- 因此本任务只修角色动画姿态，不要把武器机械骨骼动画烘焙到角色骨架，不要改变角色与武器蒙太奇的同步契约。
- 最终角色动画的长度、关键动作时刻与蒙太奇段落必须能继续和武器动画同步。若试样改变时长，必须给出精确的新旧时长和时间映射，不得直接替换正式资产。

## 输入资产

### MF Reload 最小试样

- 上半身动作来源：`/Game/Assets/Animations/Reload/Reload_Shotgun_Ironsights`
- 对应武器机械动画：`/Game/Weapons/Shotgun_A/Animations/Reload_Shotgun_Ironsights_W`
- 下半身、pelvis 和整体重心参考：`/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Shotgun_Reload`

### 后续 Reload MM

- 上半身动作来源仍为 `Reload_Shotgun_Ironsights`。
- 下半身参考需要从 CC/MM 正式动作库中选取与 Lyra Shotgun idle、reload 衔接自然的版本；不得因为文件名带 MF 就断言男性不能使用。项目男女角色骨架兼容，但最终仍需分别烘焙 MF 和 MM 输出。

### 后续 Fire

- 上半身动作来源：`/Game/Assets/Animations/Fire/Fire_Shotgun_Ironsights`
- 下半身、pelvis 和整体重心参考：`/Game/Characters/Heroes/CC/MF/Animations/Actions/MM_Shotgun_Fire`
- 对应武器机械动画及正式蒙太奇由接手者通过 AssetRegistry 和 Montage 依赖查询确认，不可仅凭文件名猜测。

## 用户要解决的视觉问题

商城角色动画的动作内容可用，但它的全身姿态和项目 Lyra/CC 动画不一致。当前可见问题包括：

- 双脚距离过宽，动作开始或结束时会突然从 Lyra idle 的窄站姿跳到宽站姿，再跳回去。
- pelvis 高度偏低，重心和项目现有 Shotgun idle、fire、reload 不连续。
- pelvis、root 的水平位置或朝向可能漂移。
- 胸腔、肩、肘、手腕为了完成装弹或开火动作发生变化，不能简单把 `spine_01` 以上全部覆盖后就认为合成正确。
- 左右手必须继续合理握持枪体、插入霰弹；不能出现手掌穿模、手腕反折、肘部跳变或枪口瞬移。
- 动作首尾必须能自然接回项目现有 Shotgun idle、走路、蹲下、跳跃后的持枪姿态。

验收时不要只看某一帧，也不要只比较帧数。必须连续播放并观察双脚、pelvis、胸腔、肩、肘、手腕、武器相对位置和动作首尾。

## 最小输出

先产出：

`/Game/Developers/AnimationTrials/Shotgun_A/MF/AS_MF_Shotgun_A_Reload_BlendTrial_01`

要求：

- 使用和 MF 正式角色动画相同的 Skeleton，不得创建新 Skeleton。
- 保留 `Reload_Shotgun_Ironsights` 的装弹上半身语义。
- 下半身站姿、双脚距离、pelvis 高度和重心尽量匹配 `MM_Shotgun_Reload`，并能与项目 Shotgun idle 自然衔接。
- 先保持原始 Reload 动画的总时长和采样率。若工具链无法做到，停止替换正式资产，只输出试样与差异报告。
- 试样必须是烘焙后的 AnimSequence，不得依赖运行时 AnimBP 分层、额外 Blueprint Tick 或每帧 Control Rig 求解。

同时输出一份记录：

`Docs/Tasks/WeaponSystem/ShotgunAnimationBlendTrialReport_动画合成试水报告.md`

报告至少包含：

- 使用 Blender 5.2 还是 UE Sequencer、Control Rig。
- 输入资产、Skeleton、帧率、帧数、时长。
- 采用的骨骼边界和每个例外骨骼的处理方式。
- root、pelvis、左右脚、spine、clavicle、upperarm、lowerarm、hand 的合成策略。
- 是否改变了 Root Motion、Curve、Notify、Sync Marker。
- 首帧、中间装弹帧、末帧截图，以及连续播放视频或等价的逐帧证据。
- 已知问题和不能自动证明的事项。

## 推荐方法 A：Unreal Editor、Sequencer、Control Rig

1. 复制源动画到 `/Game/Developers/AnimationTrials/Shotgun_A/MF`，不要直接打开正式资产做破坏性编辑。
2. 在 Sequencer 中并排放置上半身来源、下半身参考和试样角色，确认三者使用兼容骨架和相同世界朝向。
3. 先对齐 root、pelvis 和动作首帧，不要立即复制整条骨骼层级。
4. 以参考动画的 root、pelvis、腿、脚为底；以商城动画的胸腔、手臂、手指为动作来源。
5. 对 spine 链不要使用一个固定切点生硬覆盖。至少检查 `spine_01` 到胸腔的扭转如何从 pelvis 过渡到双臂动作，必要时在关键帧上做渐进权重或手工修正。
6. 检查手部与枪体、装弹口的空间关系；若缺少持枪约束，可临时用 Control Rig 约束辅助编辑，但最终结果必须 Bake 到 AnimSequence。
7. 烘焙前后分别记录骨骼朝向、root/pelvis 轨迹和总时长。
8. 保存试样后关闭 Sequencer，重新加载资产并播放，确认结果不依赖未保存的 Control Rig 或 Sequencer 状态。

## 推荐方法 B：Blender 5.2

1. 从 Unreal 导出两个角色动画时，使用同一 Skeleton、相同参考姿势、统一帧率和 FBX 轴设置。
2. 不要在 Blender 中重建、改名或重新排序骨骼；不要应用会改变骨骼 Rest Pose 的操作。
3. 先验证往返测试：不做任何动画修改，将一个源动画导入 Blender 再导回测试目录，确认骨骼朝向、比例、root、pelvis、手脚没有变化。
4. 往返测试通过后，再以 Lyra/CC 参考动画的 root、pelvis、腿和脚为底，合入商城动画的胸腔、手臂、手指动作。
5. spine 链必须做连续过渡；不要只按骨骼名称把下半身和上半身硬拼。
6. 保留原帧率、时间范围和首尾帧。不要自动重采样到不同帧率，也不要自动简化关键帧后直接交付。
7. 导回 Unreal 时选择现有 MF Skeleton，关闭新 Skeleton 创建，导入到试验目录。
8. 在 Unreal 中和两个源动画并排播放，检查骨骼朝向、足底滑动、pelvis 跳变、手部与枪体穿模以及首尾姿态。

## 验收矩阵

| 检查项 | 通过条件 | 不通过示例 |
|---|---|---|
| Skeleton | 使用正式 MF Skeleton，无新增 Skeleton | 导入时生成新 Skeleton 或骨骼重命名 |
| 动作语义 | 每次抓取、送弹、手部回位仍清楚 | 手穿过机匣、霰弹位置不合理、动作丢失 |
| 双脚 | 距离和朝向接近 Lyra/CC Shotgun 参考，首尾无跳变 | 宽站姿突然出现或消失 |
| pelvis | 高度、重心和朝向连续 | pelvis 下沉、侧移、首尾弹跳 |
| spine 过渡 | pelvis 到胸腔扭转连续 | 切骨点出现折线、胸腔突然旋转 |
| 手臂 | 肩、肘、腕自然，双手仍与枪体一致 | 手腕反折、肘部跳变、手掌漂离枪体 |
| root | 无非预期位移或旋转 | 角色滑动、原地动作产生位移 |
| 时间 | 试样与源 Reload 时长、帧率一致 | 未记录的重采样或时长变化 |
| 运行时边界 | 产物是独立烘焙 AnimSequence | 依赖运行时 AnimBP 或 Sequencer |
| 证据 | 有首帧、关键装弹帧、末帧和连续播放证据 | 只提交资产，不提供视觉证据 |

## 停止条件

出现以下任一情况时，不要覆盖正式资产，保留试样并在报告中说明：

- 无法确认骨骼本地轴或 FBX 往返导致姿态变化。
- 需要改变 Skeleton、Rest Pose 或骨骼层级才能继续。
- 无法保持动作时长，导致角色与武器 Montage Sync Follow 可能失配。
- 手部动作与枪体无法同时满足，必须大幅改动武器机械动画。
- 无法消除明显脚滑、pelvis 跳变、spine 断层或手腕反折。
- 无法提供连续播放证据，仅能给出静态截图。

## 后续产出命名建议

只有 MF Reload 试样通过后，才继续：

- `AS_MF_Shotgun_A_Reload_Final`
- `AS_MM_Shotgun_A_Reload_Final`
- `AS_MF_Shotgun_A_Fire_Final`
- `AS_MM_Shotgun_A_Fire_Final`

正式路径和 Montage 替换由主任务在用户验收后统一决定。试水任务不得自行替换当前 `AM_MF_Shotgun_A_Reload`、`AM_MM_Shotgun_A_Reload`、`AM_MF_Shotgun_A_Fire`、`AM_MM_Shotgun_A_Fire` 的引用。

## 交付检查

- 试样 AnimSequence 可以在 Unreal 中独立加载和播放。
- 没有新增 Skeleton。
- 源动画未发生修改。
- 报告完整记录输入、方法、骨骼边界、时间信息、视觉证据和已知问题。
- `git status` 中只包含本任务试验资产与报告，未混入其他用户修改。
- 完成后精确提交并推送远程，报告 commit hash。
