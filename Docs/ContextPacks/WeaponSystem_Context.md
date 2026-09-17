# 武器系统上下文包

本文用于上下文压缩或更换会话后快速恢复 NewWorldOrder 的 Lyra ShooterCore 项目化适配。它只保存当前权威边界和精确断点，不替代任务包中的完整证据。

## 必读入口

1. `Docs/Tasks/LyraShooterCoreAdaptation/Overview_总览.md`
2. `Docs/Tasks/LyraShooterCoreAdaptation/Status_状态.md`
3. `Docs/Tasks/LyraShooterCoreAdaptation/Verification_验收.md`
4. `Docs/Tasks/LyraShooterCoreAdaptation/AnimationFoundation_动画地基与IK.md`
5. `Docs/DevelopmentNotes/LyraShooterCore_武器迁移笔记.md`
6. `Docs/DevelopmentNotes/MCP_踩坑记录.md`

## 不可改变的架构边界

- 修改武器、装备、动画、UI、网络生命周期或玩法代码前，先在 Lyra 11000 读取对应 C++、Blueprint EventGraph 和资产配置，再决定原样迁移、项目层适配或明确舍弃。
- PlayerState 只保存 Persistent 仓库、角色存档与出战配置；Controller 管理 RuntimeOnly QuickBar；Pawn 只承担当前 Combat、Equipment 和 `B_Weapon` 表现桥。
- QuickBar、Equipment 与 HUD 只引用 `UShootInventoryManagerComponent` 管理的 ItemInstance/WeaponInstance。副本拾取不得写回账号 QuickBar。
- 保留项目的 TargetData 回传与服务器按自身场景重新 Trace/结算；`CommitAbility/ApplyCost` 负责能力提交与消耗。不引入 Lyra WeaponStateComponent、命中确认队列或独服反作弊链。
- 玩家私有 HUD 片段按目标 LocalPlayer 注册到 UIExtension 插槽；禁止 `GetFirstPlayerController`、全局 `AddToViewport` 或世界共享进度 Widget。
- 正式角色动画骨架为 `ChenHaoYu_Skeleton` 与 `ShenWanYun_Skeleton`。逐武器差异走同侧 Linked Item Anim Layer 或武器配置，不在主 AnimBP/C++ 增加性别硬编码分支。
- `/Game/Characters/Heroes/Mannequin` 当前仍是迁移源和部分类型/工具链依赖，未经引用审计和替代资产验收不得删除。

## 已闭环的玩家路径

- 固定点 PressToInteract 拾取 Rifle、Pistol、Shotgun，切枪、丢枪、自动装备下一把和最后一枪后的空手状态。
- Rifle `30/60`、Pistol `12/48`、Shotgun `8/16` 的射击、整弹匣换弹与单机、分屏、Listen Server 复制。
- 每 LocalPlayer 独立 QuickBar、准星、交互提示/三秒补给进度和本地后坐力。
- 服务器权威逐弹丸 Trace/伤害、Lyra Heat 散布、队伍友伤过滤、敌人受伤死亡与 TestMap 重生。
- 男女正式 CC Linked Layer、Equip/Unequip、Lean、Foot/Hand IK、逐武器左手姿势覆盖和 Lyra Niagara 伤害数字。
- 角色 Fire/Reload 与武器 SkeletalMesh Montage 已统一走原生 `UShootAnimNotify_PlayWeaponMontage`；分屏与 Listen Server 实测角色、武器时间轴一致。
- 男性 Shotgun Fire 飞枪根因是目标 `MM_Shotgun_Fire` 丢失 Mesh Space Additive 元数据，绝对姿势被叠加进 `FullBodyAdditivePreAim`。恢复自身第 28 帧 Base Pose 后，真实开火不再飞到头顶；Actor 挂点始终为 `weapon_r/-90`。

## 当前精确断点

1. 已完成：MM 31 个正式 Additive Base Pose 已改为同侧 ChenHaoYu；MF 两个 AimOffset 已接入 30 个同侧 ShenWanYun 样本与正式 Preview Base，采样网格和 Additive 契约保持不变。
2. 已完成：男女三枪 6 个 Fire 与 6 个 Reload 角色 Montage 均配置唯一原生同步 Notify；Fire 的旧 BP Notify 已移除，Reload 保留原结算 Notify。分屏与 Listen Server 实测角色和武器 Montage 同步。
3. 已完成：男性 `MM_Shotgun_Fire` 恢复与 Lyra/女性一致的 Mesh Space Additive、自身第 28 帧 Base Pose。真实 `IA_Attack` 弹药 `7 -> 6` 时角色和武器 Montage 均 Active，画面不再飞枪。
4. 已完成：18 个正式 CC AnimBP/动画接口重新编译为 `BS_UP_TO_DATE`；Equip/Unequip 的基础与 Additive 元数据同 Lyra/男女一致，中间帧未复现骨长拉伸。
5. 已完成：冷启动复读确认 18 个正式 CC AnimBP/动画接口均为 `BS_UP_TO_DATE`，目标 Skeleton 全部正确。正式目录的 1097 条直接 Mannequin 依赖边中，1065 条来自曲线压缩设置、AnimationModifier 或 Transition Notify；其余为 Lyra 枚举/结构、动画层类型、ControlRig、IK Retargeter 和编辑器 Skeleton 契约，不是运行姿势回退。
6. 已完成：六个相关 AnimBP 的父类均为项目原生 `ShootMannequinAnimInstance` 或引擎 `AnimInstance`；男女 `ALI_ItemAnimLayers` 各保留 14 个 Lyra 动画层函数。正式玩家姿势链当前保持 `MANNY_REF_POSE=0`、`MANNY_AIM_REFS=0`。
7. 用户手工清理清单：`/Game/Weapons` 下 8 个零引用旧 Montage，以及 MF Poses 下 `SplashPose_Quinn_1/10/11`。它们涉及多个文件，不得在文件系统批量删除 `.uasset`。
8. 下一断点：不再为清零路径复制 Lyra 共享类型和工具资产；从任务包中选择新的玩家可验收功能时，继续先在 Lyra 11000 读取对应实现，再做项目层最小适配。

## 验证与编译边界

- Widget/Blueprint Compile 与 C++ 编译是两条链。纯 `.cpp` 函数体小改可用 Live Coding；新增或修改反射声明、模块或 Build.cs 后关闭编辑器冷编译。
- 冷编译使用 `Scripts/Build_Windows.ps1`；与 Lyra 编辑器并开时带 `-NoHotReloadFromIDE`，防止生成临时 `-000N.dll`。
- 最终回归固定为 HomeMap 单人、TestMap_SplitScreen 本地双人和 TestMap_ListenServer。结束后恢复 HomeMap、`PIE_Standalone`、客户端数 1、单进程。
- 每个小阶段选择性暂存、提交并推送；排除 `.codex_mcp_request.json`、`.reasonix/`、`Build/`、`reasonix.toml`、动画用户改动和项目外压缩包。
