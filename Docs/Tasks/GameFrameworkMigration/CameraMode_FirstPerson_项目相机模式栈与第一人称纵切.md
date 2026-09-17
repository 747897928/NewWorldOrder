# 项目相机模式栈与第一人称纵切

## 状态

- 开始日期：2026-08-28
- 当前状态：代码与资产配置完成，等待用户补充最终分屏/重生视觉回归
- 前置：现有共享按住 ADS 、Sniper Scope 与单 `FollowCamera`/`CameraBoom` 运行正常

## 目标

- 在 `AShootCharacter` 上保留第三人称 `FollowCamera`，新增独立 `FirstPersonCamera`；两者各自保存 FOV、PostProcess 与未来专属渲染配置。
- 引入 Pawn 级 `UShootCameraModeStackComponent`，选择基础第三/第一人称相机，并把临时 ADS 层应用到当前视图。
- Experience 数据负责默认视角和是否允许切换；当前玩法可允许按 V 切换，未来生化模式 Experience 可默认并锁定第一人称。
- 使用 CommonUI + Enhanced Input + GAS 既有输入链，不在 C++ 硬编码 V 或手柄按键。
- 保持本地分屏隔离：视角与 Owner 可见性以各自 Pawn/LocalPlayer 为边界。

## 设计决策

### 为什么本项目使用两个相机

- Lyra CameraMode 的主要价值是将“什么玩法状态需要什么视图”与 Character 解耦，再由一个栈求最终视图。
- Lyra 当前参考实现没有本项目这种“完整第三人称配置 + 未来第一人称专属渲染规则”的双视图需求；不应机械复制其单输出实现。
- 本项目明确选择两个 `UCameraComponent`：`FollowCamera` 只保留第三人称 Boom/肩射参数，`FirstPersonCamera` 单独保存第一人称 Transform、FOV、PostProcess 和未来不可见规则。切回第三人称时不需要逐项恢复被第一人称覆盖过的属性。
- 任一时刻只有一个相机 Active。模式栈原子切换 Active 状态、Head Owner No See 和俯仰限制，不把多个活动相机同时交给 `AActor::CalcCamera` 猜测。
- 第一/第三人称本轮采用端点切换，不做同 Actor 两个 CameraComponent 之间的伪平滑插值；ADS 仍保留现有权重插值。这样不会出现肩后过渡阶段已经隐藏 Head 的短暂“无头角色”。

### 模式栈结构

```text
Experience Camera Policy
  -> Base Perspective: ThirdPerson or FirstPerson
  -> Temporary Mode: weapon ADS weight 0..1
  -> UShootCameraModeStackComponent
  -> ThirdPerson: CameraBoom + FollowCamera
  -> FirstPerson: Head bone + FirstPersonCamera
```

- 第三人称基线在 `BeginPlay` 从 `BP_ShootCharacter` 的 `FollowCamera/CameraBoom` 默认值捕获，不在 C++ 引用具体 `/Game/` 资产。
- 第一人称使用 `AShootCharacter.FirstPersonCamera`，按项目决定附加到 `GetMesh()` 的 `head` 骨骼；相对位置、旋转、FOV 与 PostProcess 在 `BP_ShootCharacter` 的组件 Details 中直接调优。
- 项目接受复用第三人称全身动画时 Head 骨骼可能带来的晃动和局部穿模。后续 AI 不得在没有新产品决策时擅自改挂 Root、增加专用手臂或复制第二套动画体系。
- ADS GA 仍在拥有端与服务器同步写入 `UShootRangedWeaponInstance.AimingAlpha`；只有本地控制端把同一权重交给 CameraModeStack。
- 退出 ADS 回到当前基础相机，因此第一人称下 Sniper 开镜、松开后不会被强制切回第三人称。
- 第一人称不复用第三人称的 SpringArm/Socket 端点。当前项目没有专用第一人称动画和逐武器机瞄数据，因此普通枪不激活 ADS；仅 `EShootWeaponItemCategory::Sniper` 保留已有全屏 Scope。
- 第三人称所有武器继续使用现有肩射 ADS。未来若决定给普通枪增加机瞄，应把支持方式升级为显式武器数据，由 CameraMode 消费，不把相机挂在武器骨骼上。

用户提供过一个仅供参考、尚未验证的机瞄思路：在每把武器骨架增加 `AimPoint` Socket，通过 IK/相机相对变换让机械瞄具中心对齐屏幕中心。它可以作为未来逐武器数据与资产调优的输入，但不是当前权威实现；不能直接把 CameraComponent 附到枪上，否则动画抖动、武器切换和服务器射线方向更难保持一致。若以后为普通枪做机瞄，只需把当前“Sniper 分类允许第一人称 ADS”升级成 WeaponInstance/ItemDefinition 显式配置，双相机模式栈无需推翻。

### Mesh 可见性

- Mutable 角色当前把 Head 和 Body 分为 `GetMesh()` 与 `BodyMesh`。
- 第一人称只对 Head Mesh 设置 `Owner No See`，Body 与武器仍保留；相机仍可跟随被 Owner 隐藏的 Head 组件骨骼变换。
- 进入第一人称时在同一游戏帧先设置 Head Owner No See 与俯仰限制，再激活 `FirstPersonCamera`；退出时先激活 `FollowCamera` 再恢复 Head。没有跨 0.2 秒过渡的无头阶段。
- 不使用 `HideBoneByName`：它是组件全视图状态，会破坏同一进程中其他本地分屏玩家的观察结果。
- 项目明确不制作专用第一人称手臂、姿势或动画；第一人称继续使用完整第三人称 Body、换装、武器与动画链。
- 因第三人称动画不保证视线与机械瞄具精确对齐，个别枪械的眼前穿模属于后续资产调优，不会反过来引入一套专用第一人称网格。

### 角色朝向

- 持枪第三人称与第一人称都必须跟随控制器 Yaw。
- 第一人称空手也跟随控制器；只有第三人称空手恢复 `bOrientRotationToMovement=true`。
- Character 保存“当前持枪”与“本地第一人称”两个独立事实，由一个函数合并计算旋转策略，避免切枪覆盖相机模式。
- 当前第一人称已经立即使用控制器 Yaw，不等待第三人称 Turn-in-Place 动画完成。若个别 AnimBP 仍短暂播放转向表现，记录为第三人称动画复用的非阻塞视觉遗留，不为本轮增加第一人称专用动画。

## 输入与 Experience 配置

- 新 Tag：`InputTag.Camera.TogglePerspective`。
- 新 InputAction：`IA_ToggleCameraPerspective`，键盘映射 V；手柄键位本轮不猜测。
- `DA_ShootInputConfig` 将 InputAction 映射到 Tag；角色既有 `UShootInputComponent -> ASC` 链路不增加键盘分支。
- Experience 新增 `CommonAbilitySet`、`DefaultCameraPerspective`、`bAllowCameraPerspectiveSwitch`。
- `CommonAbilitySet` 授予 `UShootGA_ToggleCameraPerspective`；GA 为 `LocalOnly`，不复制纯视觉偏好。
- 玩家主动选择的视角保存在本地 `AShootPlayerController`，它比 Pawn 生命周期长；死亡重生的新 Pawn 会继续使用原选择，不再无条件退回 Experience 默认第三人称。

## 本轮不做

- 不创建生化模式、专用地图或 Experience Tile UI；它们是后续 story。
- 不猜测手柄按键。
- 不创建专用第一人称手臂/武器网格或动画集；这是当前确定的项目约束，不是待补功能。
- 不在没有逐武器 AimPoint 调优的情况下假装完成普通枪机械瞄具精确对齐。
- 不修改 Lyra 或引擎源码。
- 不在正式玩法代码中增加“冻结 AI”规则。相机与输入测试若受敌人持续攻击、死亡重生干扰，应使用测试地图专用 AI 行动开关；死亡期间按键无效不能误判为 Enhanced Input 失效。

## 已知视觉遗留

- Sniper 全屏 Scope 的右缘在特定构图下会看到一小段自身枪管。它与已修复的 Tracer 持续光柱、枪口火焰和镜内烟雾不是同一问题；当前证据更接近第一人称完整身体/武器复用后进入 Scope 相机视锥，或 Scope 遮罩边缘未完全覆盖。
- 该问题不影响射线、伤害、Scope 进入退出或其他玩家观察结果。本轮不在缺少可靠运行时回读时贸然把整把武器设为 Owner No See；后续应在 Scope UI 遮罩、Sniper 武器相对姿态和仅 Owner 的开镜可见性三者中做视觉对比后再选最小修复，不能删除枪口火焰。

## 验收标准

1. Windows Editor Target 冷编译通过，项目启动不进入 Project Browser。
2. 允许切换的 Experience 中，V 原子切换第三/第一人称两个相机端点，不存在中间倍率；两套相机配置互不覆盖。
3. 第一人称普通枪不会进入伪机瞄；Sniper 按住/松开 Scope 正常并回到第一人称基础视角，第三人称下所有现有 ADS 仍回原第三人称端点。
4. 切枪、空手与第一人称组合不会互相覆盖角色朝向。
5. 双本地玩家可独立切换；玩家 A 的头部只对 A 的视图隐藏，玩家 B 仍能看到 A 的完整角色。
6. 不允许切换的 Experience 中，同一 InputAction 不会改变视角。
7. 第一人称死亡重生后仍保持该 LocalPlayer 的第一人称偏好；第三人称死亡重生同理。

## 验证记录

- 2026-08-29 Windows Editor Target 冷编译成功，`UnrealEditor-NewWorldOrder.dll` 生成完成。
- 运行时已回读两套相机组件：第三人称 `FollowCamera` 附加 `CameraBoom`，第一人称 `FirstPersonCamera` 附加 `CharacterMesh0.head`；V 输入可在两者之间切换，进入第一人称时 Head `Owner No See` 生效。
- 用户视觉验收确认 Head 附着后的第一人称构图正常，不需要固定向左旋转。
- 在修复前的重生回归中发现 PlayerController 偏好读取时序和 Pawn 内保存 Pitch 原值会导致重生回第三人称、Pitch 泄漏；现已把偏好重读延迟到本地控制 Pawn 就绪，并把 Pitch 原值迁移到 PlayerController 生命周期。修复后已重新冷编译；最终死亡重生、双本地玩家和远端 Owner 的人工视觉回归仍待完成。
