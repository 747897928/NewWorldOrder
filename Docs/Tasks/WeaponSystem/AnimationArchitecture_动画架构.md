# Lyra ShooterCore 动画架构与项目适配

# 文档职责

本文是首批 Rifle、Pistol、Shotgun 与空手动画迁移的权威实现笔记。后续修改角色 AnimBP、武器 AnimLayer、换枪生命周期或 CC 重定向前，必须先核对本文。

审计时间：2026-08-17

审计来源：

- LyraStarterGame UE 5.8 MCP 端口 11000。
- NewWorldOrder UE 5.8 MCP 端口 8000。
- Lyra `Source/LyraGame/Animation`、`Cosmetics`、`Weapons` 与 `Feedback/ContextEffects` 官方源码。

# 不可破坏的项目边界

- `/Game/Blueprints/Character/BP_ShootAnimInstance_F` 的 `DefaultGroup.UpperBodyAdditive` Slot 由 `/Game/UI/Pose/W_PoseLibraryEntry` 经 `WBP_GameMenu` 播放表演动作。若改名，必须同时更新全部调用者。
- `HairAnimationLayer` 与 `ShoeAnimationLayer` 是 Mutable 头发物理、鞋子和高跟鞋表现的末端动画层，不能删除。
- 可见 CC AnimBP 的最终顺序必须保持为：基础 locomotion pose -> `ALI_ShootWeaponLayers.FullBody_Aiming` -> `UpperBodyAdditive` -> FootPlant/Control Rig -> `ShoeAnimationLayer` -> `HairAnimationLayer` -> Output Pose。Shoe 位于落脚求解之后，避免高跟鞋骨盆和脚部补偿被 Control Rig 覆盖；Hair 必须最后读取已经包含动作、武器、鞋履补偿的最终身体姿势。
- `/Game/Blueprints/Character/BP_ShootAnimInstance_F` 与 `/Game/Blueprints/Character/BP_ShootAnimInstance_M_Lyra` 分别绑定沈婉芸、陈浩宇正式 CC Skeleton；运行时不再使用 Mannequin 隐藏源 Mesh 或 `Retarget Pose From Mesh`。
- 动画表现状态只读取当前 Pawn 的 `UShootEquipmentManagerComponent`，不从 Controller QuickBar 反查；这样 Listen Server 的模拟代理也能得到正确持枪层。HUD 和输入仍从各自 Owning PlayerController 的 QuickBar 读取，禁止使用 `GetFirstPlayerController` 或 Player 0。
- 武器是 Controller 上 RuntimeOnly QuickBar 的战斗会话状态；动画层是 Pawn 表现状态。角色重生、切换或重新 Possess 后必须把当前武器层重新应用到新 Pawn，但不得把 RuntimeOnly 写入 PlayerState SaveGame。
- 不修改 `Plugins/` 或 Unreal Engine 源码。

# 已完整审计的 Lyra 动画资产

- `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base`
- `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_CopyPose`
- `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Retarget`
- `/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ALI_ItemAnimLayers`
- `/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/ABP_PistolAnimLayers`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/ABP_PistolAnimLayers_Feminine`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers_Feminine`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Shotgun/ABP_ShotgunAnimLayers`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Shotgun/ABP_ShotgunAnimLayers_Feminine`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers_Feminine`

# Lyra 的职责分层

```text
ABP_Mannequin_Base
  采集角色、移动、视角、GameplayTag 和 GroundDistance
  维护通用 locomotion 状态机与跳跃状态
  调用 ALI_ItemAnimLayers 的各个入口
            |
            v
ABP_ItemAnimLayersBase
  实现全部 ALI 入口
  提供共享的 Start/Cycle/Stop/Pivot、AimOffset、Jump/Fall、HandIK、LegIK
  只通过动画变量读取一套动画集
            |
            v
Rifle/Pistol/Unarmed 子 AnimBP
  主要覆盖动画资产变量，不复制整套图表逻辑
            |
            v
Shotgun 子 AnimBP
  继承 Rifle 男女层，只替换 Shotgun 左手和特有姿势
```

此分层的核心价值是资源生命周期。主 AnimBP 不直接引用所有枪械动画；当前武器通过 `LinkAnimClassLayers` 只链接一套 Item Anim Layer，切枪时替换，空手时链接 Unarmed。这样新增武器主要新增动画数据子蓝图，不需要修改角色主 AnimGraph 或 C++ 枚举分支。

# ABP_Mannequin_Base

## 父类和依赖

- 父类：`ULyraAnimInstance`。
- 直接脚本依赖：GameplayTags、AnimGraph、AnimGraphRuntime、LyraGame、ControlRig、PropertyAccessNode、MovieSceneAnimMixer。
- `ULyraAnimInstance` 用 `FGameplayTagBlueprintPropertyMap` 将 ASC Tag 自动映射到 AnimBP Bool，并从 `ULyraCharacterMovementComponent::GetGroundInfo()` 写入 `GroundDistance`。
- NewWorldOrder 没有 Lyra CharacterMovementComponent，不能原样保留这条 GroundInfo Cast。项目父类必须用标准 `UCharacterMovementComponent` 和项目 ASC 提供等价输入，或在项目 AnimBP 中替换对应 Property Access。

## 更新模型

- EventGraph 为空。
- `BlueprintThreadSafeUpdateAnimation` 通过 Property Access 读取 Pawn、MovementComponent 和 AnimInstance 已缓存字段。
- 线程安全更新内计算世界位置、世界旋转、速度、加速度、局部速度、移动方向、根部偏航、瞄准方向、跳跃/下落状态、上一帧状态和 linked layer 变化。
- 不能在 AnimGraph 工作线程直接调用会遍历 Controller、QuickBar、Actor 或 UObject 的普通 BlueprintPure Getter。
- 项目当前 `UsesRifleLocomotion`、`UsesPistolLocomotion`、`UsesShotgunLocomotion`、`GetWeaponForwardSpeed`、`GetWeaponRightSpeed` 告警正是旧图违反该边界的结果。修复方式是删除旧按武器 BlendByBool 主线并接入动画层，不是给这些函数盲加 `BlueprintThreadSafe`。

## 主要状态与变量

Lyra 主实例维护约 52 个变量，职责可分为：

- 运动采样：WorldLocation、WorldRotation、WorldVelocity、WorldAcceleration、Velocity、Acceleration、LocalVelocity、LocalAcceleration、Speed2D、GroundDistance。
- 方向和旋转：LocalVelocityDirectionAngle、CardinalDirection、RootYawOffset、AimYaw、AimPitch、TurnYawCurveValue。
- 状态：IsOnGround、IsCrouching、IsJumping、IsFalling、IsADS、IsFiring、IsReloading、IsDashing、IsMelee、JumpStart、JumpApex、FallLand。
- 动画层生命周期：LastLinkedLayer、LinkedLayerChanged。切换武器层后，Locomotion 状态机会走恢复路径，避免把上一把武器的播放时间和状态直接带入下一层。

项目第一阶段不伪造尚不存在的 Aim Ability Tag。只有 ASC 中出现正式、可复制的 Aim 状态后，才把它映射到动画变量。

## LocomotionSM

地面主状态：

- Idle
- Start
- Cycle
- Stop
- Pivot

空中主状态：

- JumpStart
- JumpStartLoop
- JumpApex
- FallLoop
- FallLand

状态机还使用 Alias/Conduit 组织跳跃入口，并在 linked layer 发生变化时触发恢复。每个状态不是直接播放某把枪的动画，而是调用 `ALI_ItemAnimLayers` 对应入口。

# ALI_ItemAnimLayers

官方接口共有 14 个入口：

- `FullBodyAdditives`
- `FullBody_IdleState`
- `FullBody_StartState`
- `FullBody_CycleState`
- `FullBody_StopState`
- `FullBody_PivotState`
- `FullBody_Aiming(PreAimPose, AimYaw, AimPitch)`
- `FullBody_JumpStartState`
- `FullBody_JumpApexState`
- `FullBody_FallLandState`
- `FullBody_FallLoopState`
- `FullBody_JumpStartLoopState`
- `FullBody_SkeletalControls(InPose)`
- `LeftHandPose_OverrideState(InputPose)`

项目第一纵切没有复制 Lyra 全部 14 个入口，而是建立唯一项目接口
`/Game/Blueprints/Character/Animation/ALI_ShootWeaponLayers`，当前只保留
`FullBody_Aiming`。Rifle、Pistol、Shotgun、Unarmed 的正式 CC Linked Layer 都实现此接口。
这是首批三把武器的已验收契约；后续只有在迁入 Start/Cycle/Stop/Pivot、Jump/Fall 或
IK 功能时才按需扩展，不能把 `/Game/Characters/Heroes` 下的 Lyra 接口作为运行时依赖。

# ABP_ItemAnimLayersBase

## 父类和依赖

- 父类：`UAnimInstance`，不是 `ULyraAnimInstance`。
- 直接脚本依赖：AnimGraph、AnimGraphRuntime、PropertyAccessNode、AnimationLocomotionLibraryRuntime、AnimationWarpingRuntime 和对应编辑器节点模块。
- 共享约 63 个数据变量，子 AnimBP 通过 Class Defaults 覆盖资产，不复制逻辑图。

## 数据变量

主要配置组：

- Walk/Jog 的 Forward、Backward、Left、Right 动画集。
- Idle、ADS Idle、Start、Stop、Pivot。
- JumpStart、JumpStartLoop、JumpApex、FallLoop、FallLand。
- AimOffset、TurnInPlace、FullBodyAdditive、LeftHandPose。
- StrideWarping 与 OrientationWarping 参数。
- DisableHandIK、播放速率上下限、上半身覆盖和左右手 IK 开关。

## 图表职责

- Idle：内部 `IdleSM`。
- Start/Cycle：Sequence Evaluator/Player、Layered Blend Per Bone、Orientation Warping、Stride Warping。
- Stop：Sequence Evaluator 与上半身覆盖。
- Pivot：内部 PivotSM 与上半身覆盖。
- Aiming：输入姿势叠加 AimOffset。
- FullBodyAdditives：独立 Additive 状态机。
- Jump/Fall：每个阶段都允许武器姿势覆盖。
- SkeletalControls：手部 IK、腿部 IK、Foot Placement、武器/根骨 Modify Bone、手部 Retarget 和虚拟骨骼复制。
- LeftHandPose：用 Layered Blend Per Bone 叠加武器左手姿势。

ItemBase 通过线程安全 Property Access 读取主 AnimBP。项目适配必须维持“主实例负责状态，Linked Layer 负责动画资产和骨骼控制”的方向，不能让每个武器层自己查 Controller 或 QuickBar。

# 枪械与性别多态

- `ABP_RifleAnimLayers`、`ABP_PistolAnimLayers`、`ABP_UnarmedAnimLayers` 直接继承 `ABP_ItemAnimLayersBase`。
- Feminine 版本同样继承 ItemBase，只覆盖女性动画集。
- `ABP_ShotgunAnimLayers` 继承男性 Rifle 层；Feminine 版本继承女性 Rifle 层。Shotgun 复用长枪 locomotion，只覆盖霰弹枪特有左手姿势和少量资产。
- Unarmed 将 `DisableHandIK` 设为 true。
- 武器子 AnimBP 基本没有独立 EventGraph 或 AnimGraph；差异放在数据默认值里。这是新增枪械无需改 C++ 的关键。

# 正式 CC 骨架与离线重定向

- `ABP_Mannequin_CopyPose` 的 AnimGraph 只有 `Copy Pose From Mesh`，用于附着 Mesh 从父 Mesh 复制姿势。
- `ABP_Mannequin_Retarget` 在 EventGraph 从 Owner Character Mesh 缓存源组件，AnimGraph 使用 `Retarget Pose From Mesh`。目标 AnimBP 必须绑定目标骨架和正确 IK Retargeter。
- NewWorldOrder 保留 `/Game/Characters/Heroes/Mannequin/Rigs/IK_Mannequin_NWO`、
  `/Game/Assets/Characters/CC/ShenWanYun/RTG_ShenWanYun` 与
  `/Game/Assets/Characters/CC/ChenHaoYu/RTG_ChenHaoYu` 作为编辑器迁移来源。
- Rifle、Pistol、Shotgun 的 locomotion、Fire、Reload、Equip 已离线重定向到沈婉芸和陈浩宇正式骨架。
  男女身高和骨架不同，因此各自维护 BlendSpace、Montage 和 Linked Layer，不能共用一套
  CC 目标动画假装兼容。
- 正式动作 Montage 位于 `/Game/Characters/Heroes/CC/MM|MF/Animations/Weapons/Montages`。
  2026-08-02 新增的三类 Fire/Reload/Equip 共 18 个 Montage 均只引用同侧 CC 动画；其中
  Shotgun Equip 暂时复用同侧 Rifle Equip 动作。运行时 Fire/Reload 仍使用旧 CC 目录，必须在
  单独切换引用并完成 PIE 回归后，才能删除 `/Game/Assets/Characters/CC/.../Animations`。
- 运行时只有正式可见 `CharacterMesh0`。`AShootCharacter` 不创建隐藏 Mannequin 动画源，
  主 AnimBP 也没有 `Retarget Pose From Mesh`。`/Game/Characters/Heroes` 只允许作为迁移源，
  关键运行时资产的直接依赖审计必须为零。

## 2026-08-17 Equip 资产复核

- 不能用导出的原始节点总数判断 `ABP_ItemAnimLayersBase` 是否漏迁。Lyra 导出中包含额外生成的 `ExecuteUbergraph`、`__AnimFunc` 和 `__AnimFunc_MERGED` 图；排除这些生成图后，Lyra 与项目正式源图均为 15 个动画图、4 个状态机、11 个状态、15 个转换，各源图直接节点数一致。当前不需要整体替换 Item Anim Layers。
- 男女 Rifle、Pistol、Shotgun 共 6 个正式 Equip Montage 已与 Lyra 参考补齐两条 Slot Track、`ScaleDownWeaponR`、`DisableLHandIK` 的精确插值/切线，并在约 `0.03s` 添加 `/Game/Audio/MetaSounds/sfx_WeaponSwap_nl_meta_Preset` PlaySound Notify。编辑器中的第二条 Timing 标记来自该声音 Notify，不是第三条动画轨道，也不参与手臂 IK。
- `AM_Generic_Unequip` 只在武器切到空手时使用。武器切武器时，旧武器卸下后会立即进入新武器 Equip；单独播放 Generic Unequip 未复现同一问题，因此它不是当前橡皮手根因。
- 运行时采样证明 Montage 曲线有效：Equip 中 `DisableLHandIK=1`，Linked Layer 的左手 IK Alpha 降为 0，右手保持 1，与 Lyra 一致。曲线和 Notify 对齐后仍有的写实 CC 手臂观感，不能再归因于“缺 Timing/缺 SFX/曲线未生效”。
- 男性 Rifle/Pistol 的 4 条底层 Equip Sequence 已从 Manny 源重新离线重定向，重定向导出阶段关闭 `Run IK Rig` 的手臂求解，保留 FK、Pelvis、Root Motion、Curve Remap 和末端 Pin Bones；Shotgun 继续复用 Rifle Equip。这样消除了 IK 求解给陈浩宇短臂额外增加的约 8 度肘部过弯，同时保持 Additive 类型、Local Anim Frame 0 和正式 Skeleton 不变。女性未复现该额外过弯，不做无证据重定向。
- 正面逐帧 A/B 显示，修正后的男性正式 Montage 已与 Manny 源动作的肘、腕轨迹基本一致；剩余“橡皮手”主要是 Manny 原换枪动作的交叉手臂姿势落到写实 CC 比例与蒙皮后的造型问题。若仍不满足美术验收，下一步应制作 CC 专用 Equip 动画或在 DCC/Control Rig 中做姿势修型，不能继续用 C++、音效 Notify 或 GameplayTag 补丁掩盖。

## GameplayTag 属性映射的持久化约束

- 男女正式 `ABP_Mannequin_Base` 的 ADS、WeaponFire、Reload、Dash、Melee Bool 由父类 `UShootAnimInstance` 使用 `FGameplayTagBlueprintPropertyMap` 驱动。
- 映射不仅序列化 GameplayTag 和属性名，还序列化目标蓝图变量 GUID。复制另一性别蓝图的映射、变量重建后继续使用旧 GUID，或只在当前内存实例中修正，都会在包重载/蓝图 Reinstance 后退化成 `property [None]`，随后 PIE 被编译失败阻断。男女 AnimBP 并不互斥。
- 修复或改图后必须分别用当前蓝图自身变量 GUID 重建映射，并按“MF 编译 -> MM 编译”至少重复两轮；然后关闭/重载两个包，再各编译一次并确认 `BS_UP_TO_DATE`。每次自动 PIE 验收前也必须先编译这两个正式 Base AnimBP，不能把长时间无响应直接归因于 PIE。

# 武器动画层选择与生命周期

Lyra 的 `FLyraAnimLayerSelectionSet` 不是动画类数组，而是：

- 有序 `LayerRules`。每条规则包含一个 Layer Class 和全部必须满足的 Cosmetic Tags；首条匹配规则获胜。
- `DefaultLayer`。没有规则匹配时使用。

`ULyraWeaponInstance` 分别持有：

- `EquippedAnimSet`
- `UneuippedAnimSet`

`PickBestAnimLayer` 根据装备状态和外观标签选择一套 Layer Class。`B_WeaponInstance_Base` 在 `OnEquipped`、`OnUnequipped` 中获取 Pawn 的综合外观标签，然后调用 `LinkAnimClassLayers`；卸下枪械时不是简单 Unlink，而是链接 Unarmed 层。

Rifle、Pistol、Shotgun 的默认规则是：

- DefaultLayer：男性层。
- `Cosmetic.AnimationStyle.Feminine`：女性层。
- UnequippedAnimSet：相同规则选择 Unarmed 男/女层。

项目当前实现：

- 将当前 `FShootAnimLayerSelectionSet::LayerClasses` 替换为 Lyra 等价的 `LayerRules + DefaultLayer + SelectBestLayer`。
- `UShootWeaponInstance` 使用 `EquippedAnimSet` 与 `UnequippedAnimSet`，不再对可见 Character Mesh 遍历 Apply/Remove。
- 外观标签从当前 Pawn 的 `UMutableAppearanceComponent` 或 PlayerState 已恢复的角色身份构建；默认男、女性标签规则可配置，不能按 LocalPlayer 索引硬编码。
- 动画层目标是正式可见 CC Mesh。每次只链接一个选中的正式 CC 层；不能把男性、女性、
  Rifle、Pistol、Shotgun 全部同时 Link。
- 空 QuickBar、新 Pawn 初始化、丢掉最后一把枪、角色切换后都必须显式进入正确性别的 Unarmed 层。
- Mutable 异步生成最终 SkeletalMesh 后会重建 AnimInstance；`AShootCharacter` 监听
  `UMutableAppearanceComponent::OnMutableSkeletalMeshUpdated`，再从 Pawn EquipmentManager
  重放当前武器层，避免外观完成后退回空手。
- 该重放不能只按 Layer 类缓存并跳过。Mutable 创建的是新的 AnimInstance，即使目标 Layer
  类与更新前相同，新实例也尚未链接该层；装备、卸装和 Mutable 完成入口都必须对当前可见
  AnimInstance 调用一次 `LinkAnimClassLayers`。

# 项目主 AnimBP 重构边界

`BP_ShootAnimInstance_F` 当前旧 `Unequipped` 状态机同时包含：

- Unarmed BlendSpace。
- Rifle/Pistol/Shotgun 三套 2D BlendSpace。
- 三个 `Blend Poses by Bool`。
- `GetWeaponForwardSpeed`、`GetWeaponRightSpeed` 与三个 `Uses*Locomotion` 调用。
- 旧 Jump/Fall/Land/Crouching 分支。

这些节点属于候删旧主线，用户已经授权破坏性重构。它们不能继续与新 AnimLayer 主线并存，否则会形成两套武器状态来源和两套 locomotion。

第一阶段已落地状态：

- 旧 `Crouching` 与 `Unequipped` 状态机不再进入最终 Output。
- AnimGraph 不再调用上述五个线程不安全 Getter，相关编译告警为零。
- 正式 CC AnimBP 直接输出本骨架 locomotion，并调用项目唯一 `ALI_ShootWeaponLayers`。
- Rifle/Pistol/Shotgun/Unarmed 通过正式 CC linked layer 切换。
- 项目运行时不存在隐藏 Mannequin 动画源或 `Retarget Pose From Mesh`。
- Fire、Reload 与表演 Pose 的 CC Montage 继续在可见 Mesh 的 `UpperBodyAdditive` 播放；
  武器自身 Montage 由项目 C++ `UShootAnimNotify_PlayWeaponMontage` 从 Pawn EquipmentManager
  找到当前 WeaponActor 后播放并 `MontageSync_Follow`，不依赖旧 CombatComponent Notify 蓝图。
- Shoe 与 Hair 层仍在 Slot 和 FootPlant/Control Rig 之后，正式顺序为 `ShoeAnimationLayer -> HairAnimationLayer -> Output`；普通换衣不会重新初始化或丢失 linked layer。
- Equip 已进入实际 QuickBar 切槽链。武器切武器时，新武器 Equip 会替换旧武器同 Slot 的 Unequip；
  只有切到空手时才完整播放 `AM_Generic_Unequip`。这是当前已验证的即时切换策略，不是串行的
  “旧武器完整收起 -> Actor 切换 -> 新武器完整取出”。以后若改成严格串行，必须由一个权威状态机
  统一管理 Actor 生命周期和可取消时序，不能重新在 `OnEquipped`/`OnUnequipped` 两端各自盲播。

2026-07-31 运行时验证：

- TestMap 双本地玩家分别为男、女正式 AnimBP，Rifle/Pistol/Shotgun/Unarmed 层互不串线。
- 男 Rifle、女 Pistol、男 Shotgun 的角色 Reload Montage 与武器 Montage 时间轴位置完全一致。
- TestMap2 Listen Server 中，服务器主机 Rifle Fire 同时出现在服务器本地 Pawn 与客户端远端代理；
  两端角色 Montage 和武器 Montage 的采样位置均为 `0.130000s`。
- `BP_ShootCharacter`、男女主 AnimBP、三把 WeaponInstance 和八个正式 CC Layer 的直接依赖中
  不包含 `/Game/Characters/Heroes`。

## 商城 Shotgun A 动画边界

- `/Game/Weapons/Shotgun_A` 是逐发装填参考武器，不替换既有 `/Game/Weapons/Shotgun` 的整匣换弹配置。
- 角色 Reload Montage 使用 `ShotgunStart/ShotgunLoop/ShotgunEnd`，武器 Reload Montage 使用相同段落和时长。角色 Montage 上的项目 Notify 子类仍走 `UShootAnimNotify_PlayWeaponMontage`，不是让角色骨骼直接驱动武器的 `Ammo`、`Ammo_Empty`、`Grip_Bone`、`Clip_Bone`、`Slide_Bone` 或 `Trigger_Bone`。
- 武器 AnimBP 只提供 RefPose 到 `DefaultSlot` 的播放通道；机械骨骼变化来自武器自身 Montage。单人 PIE 慢放采样中，角色与武器在 0.16000、1.58333、2.63333 秒的位置一致，证明 `MontageSync_Follow` 没有被新的 Start/Loop/End 结构破坏。
- 当前 MF 动画保留商城逐发装填上半身与原下半身；MM 是独立离线重定向结果。它们是功能验证资产，不是最终美术资产。
- 最终修型目标是分别为 MF/MM 离线烘焙一份动画：保留商城上半身的装弹/开火手部动作，同时匹配现有 Lyra/CC 动画的双脚距离、pelvis 高度与重心、胸腔、肩和肘。不得用运行时 AnimBP 分层冒充已完成，也不能只按帧数机械拼接；必须检查源骨骼朝向、接缝连续性、手与枪贴合、Root/Pelvis 轨迹和 Notify 时序，再由用户视觉验收。
- 逐发弹药提交由角色 Montage 上的 `UShootAnimNotify_InsertShell` 完成；武器动画上的声音、枪口 Cascade 与护木机械动作继续作为同步表现闭包。修改角色或武器 Montage 的段落、播放率或长度时，必须同时复核两个 Montage 和提交 Notify，不允许只改一侧时间轴。

完整时序和运行证据见 `ShotgunPerShell_逐发装填接入.md`。

# FootstepEffectTagModifier

Lyra 原 `FootstepEffectTagModifier` 依赖项目源码类型：

- `UAnimNotify_LyraContextEffects`
- `FLyraContextEffectAnimNotifyVFXSettings`
- `FLyraContextEffectAnimNotifyAudioSettings`
- `FLyraContextEffectAnimNotifyTraceSettings`
- Context Effects Interface、Component、Library、Subsystem 与 Settings。

NewWorldOrder 当前没有上述源码、Context Effects Library 或运行时组件，只有孤立迁入的 Modifier，因此失效 Cast、SetParameters 和 Make Struct 节点是确定的缺依赖，不是刷新节点即可修好。

处理原则：

- 动画主线不为修复一个编辑器 Modifier 盲目引入整套无数据可播放的 Context Effects 系统。
- 若首批 locomotion 动画已有可用项目脚步 Notify，则将 Modifier 改为项目 Notify。
- 若后续确实迁移 Lyra 的表面标签、声音、Niagara Library，则在 `Source/NewWorldOrder` 复制并改造为 `Shoot` 命名的项目类，再重建 Modifier 图；禁止修改插件源码。
- 在作出删除决定前先检查动画资产是否引用该 Modifier、是否已有替代 Notify。无引用且无运行时资源时可删除这个孤立候删资产，但必须记录引用审计结果。

# 蓝图迁移依赖

Lyra 十三个资产的直接模块依赖已由 MCP 实测：

- 主 Base：GameplayTags、AnimGraph、AnimGraphRuntime、ControlRig、PropertyAccessNode、MovieSceneAnimMixer。
- Retarget：IKRig、IKRigDeveloper。
- ItemBase：AnimGraph、AnimGraphRuntime、PropertyAccessNode、AnimationLocomotionLibraryRuntime、AnimationWarpingRuntime 与对应编辑器节点。
- 子层：AnimGraphRuntime、AnimationWarpingRuntime。

迁移前必须确认项目启用对应引擎插件/模块。允许修改 `.uproject` 和项目 `Build.cs` 声明依赖，不允许修改插件或引擎源码。

# 验收矩阵

- 编译：C++ 冷编译生成基础 DLL；全部相关 AnimBP、ALI、Modifier 为 UpToDate；线程安全告警为零。
- 单人：空手、拾取三枪、逐枪切换、移动八方向、跳跃、开火、换弹、丢最后一枪回空手。
- 性别：单人最后选择角色使用对应层；本地分屏两名玩家性别互斥并分别使用男/女层，不按 PlayerIndex 写死。
- 分屏：两个 LocalPlayer 的 AnimLayer、Montage、QuickBar 和准星互不串线。
- Listen Server：服务器和客户端都看到正确持枪、开火、换弹和空手；动画层选择不依赖仅本地存在的对象。
- 外观回归：头发物理、鞋子/高跟鞋层和表演 Pose Slot 全部保留。
