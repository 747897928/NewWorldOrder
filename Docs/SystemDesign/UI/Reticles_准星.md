# 准星体系（Lyra → NewWorldOrder 迁移笔记）

最后更新：2025-11-02  
作者：Codex

本节梳理 Lyra 准星的组件结构、动画与数据来源，供 NewWorldOrder 重建命中/准星 HUD 时参考。Lyra 中三个主要准星蓝图：

| 武器 | Widget | 说明 |
| ---- | ------ | ---- |
| 步枪 | `W_Reticle_Rifle` | 标准四角准星，随着散布/ADS 缩放 |
| 霰弹 | `W_Reticle_Shotgun` | 四角 + 较大散布显示，命中覆盖更宽 |
| 手枪 | `W_Reticle_Pistol` | 类似步枪但尺寸更小 |

所有蓝图均继承 `ULyraReticleWidgetBase`，核心子部件一致：

1. **`Crosshairs` (`UCircumferenceMarkerWidget`)**  
   - `MarkerList`：4 个 Marker，位置角度 0° / -90° / 90° / 180°。  
   - `MarkerImage`：使用对应的材质实例（`MI_UI_Reticles_CrossHair_Rifle/S hotgun/Pistol`），控制单个准星角的样式。  
   - `Radius`：外圈半径；Tick 中通过 `ComputeMaxScreenspaceSpreadRadius` 与 ADS 散布值动态修改。  

2. **`SBOuterReticle` (SizeBox)**  
   - 通过 `SetWidthOverride` / `SetHeightOverride` 改变矩形范围，配合 `AimDownSights` 动画实现缩放。  

3. **`HitMarkerConfirmations` (`UHitMarkerConfirmationWidget`)**  
   - 处理命中提示（更换材质 / 帧动画）。  
   - `PerHitMarkerZoneOverrideImages` 针对 `Gameplay.Zone.WeakSpot` 等标签替换命中图片，例如手枪使用 `MI_UI_Reticles_HitMarkerConfirmation_Critical`。

4. **`EliminationMarker` (Image)**  
   - 显示击杀时的红色十字动画 `Elimination`。  

5. **消息绑定**  
   - `EventConstruct` 调用 `ListenForGameplayMessage`：  
     - ADS 通道（Lyra 的 `GameplayMessage.ADS`）用于准星缩放。  
     - Lyra 消灭消息（`Lyra.Elimination.Message`）用于播放击杀动画。  
   - `EventDestruct` 取消异步监听，防止野指针。

6. **Tick 逻辑**  
   - `ComputeMaxScreenspaceSpreadRadius` → `Crosshairs.SetRadius`。  
   - 根据散布比例计算 SizeBox 宽高，霰弹枪会使用更大的 120 / 160 / 80 / 140 等值。  
   - ADS / Elimination 自定义事件会播放对应动画，确保在动画正向播放时可停止并重置起始帧。

## 对应到 NewWorldOrder 的实施建议

1. **C++ 支撑**  
   - `UShootReticleWidgetBase` 现为 `ULyraActivatableWidget` 子类，内部 Tick 自动插值散布半径 / ADS / 击杀闪光；`UShootReticleWidget_Rifle/Shotgun/Pistol` 提供 Lyra 同款外观。  
   - `UShootCircumferenceMarkerWidget` + `SShootCircumferenceMarkerWidget` 渲染圆周角标；`UShootHitMarkerConfirmationWidget` 负责命中提示淡出。  
  - `UShootHUDReticleComponent`（挂在 `AShootPlayerController`）根据 ItemDefinition 的 `WeaponBasicConfig::ReticleWidgetClass` 实例化准星，并监听所有 `Message.UI.Reticle.*` 标签。该组件**不复制**且只运行在本地玩家控制器上，`UCombatComponent` 保持纯服务器权威以便后续复用/测试。  
   - `AHitscanWeaponInstance` 保持散布状态，并新增 `FireSequenceId` 递增字段，方便未来 TargetData 重放；投射物武器沿用 `AShootProjectileBase`。  

2. **UMG 结构复制**  
   - 根据上述部件创建三份准星 Widget，并复用相同的动画轨道与材质资源。  
   - `MarkerList`、`Radius`、材质引用沿用 Lyra 数值即可（步枪/手枪半径 24，霰弹半径根据实际调整）。

3. **GameplayMessage 适配**  
   - 新标签：`Message.UI.Reticle.HitNotify / ADS / Elimination`。HUD 组件统一监听，再调用 Widget 的 `HandleHitNotification / HandleAdsState / HandleElimination`。  
   - 命中提示：命中型武器在 `UShootGameplayAbility_Weapon_Fire::BroadcastReticleHitNotify` 中即时投影屏幕坐标、广播消息；火箭/榴弹由 `AShootProjectileBase` 在服务器统计爆炸结果后，通过 `AShootPlayerController::ClientReceiveReticleHitNotify` 转发至客户端。  
   - `FShootReticleHitNotifyMessage` 现携带 `SourceActor`、屏幕坐标及可选世界坐标，HUD 若收到世界坐标可自行 `ProjectWorldLocationToScreen`。  
   - ADS / Elimination 消息目前仅搭好通道，未来瞄准或击杀逻辑可直接广播。弱点贴图覆盖（`Gameplay.Zone.WeakSpot`）已在默认准星中配置 `MI_UI_Reticles_HitMarkerConfirmation_Critical`。

4. **后续扩展**  
   - 若需要更多武器类型，只需配置新的材质与 `MarkerList`。  
   - 可以将散布缩放逻辑抽成函数，以便在 C++ 层替换或共用（例如命中型 vs. 投射物型准星）。

## Lyra 蓝图可复用要点（2025-11-02）

- Tick 驱动散布  
  - `EventTick` 中调用 `ComputeMaxScreenspaceSpreadRadius()`，结果传给 `CrossHairs.SetRadius()`。  
  - 同时用 `Select` 节点按武器类型选择 SizeBox 的宽高（步枪 120/80、霰弹 160/140、手枪 96/64 等），再写入 `SOuterReticle.SetWidth/HeightOverride`。  
  - 若 ADS 禁用散布缩放，可在动画期间短路该 Tick 逻辑（Lyra 通过动画状态机上的布尔控制）。

- 消息订阅与取消  
  - 上述内容是 Lyra 原始蓝图的参考，不是项目运行时实现。Lyra Widget 中的 `Struct_UIMessaging` 是一个 ADS 消息结构，不是项目已有的 `ULyraUIMessaging`；后者只负责 CommonUI 确认/错误对话框，不能拿来作为准星消息类型。
  - 项目准星统一由 `UShootReticleWidgetBase` 在 `NativeConstruct` 注册 `Message.UI.Reticle.ADS` 的 `FShootReticleADSMessage` 与 `Message.UI.Reticle.Elimination` 的 `FShootReticleEliminationMessage`，并先以 Owning Pawn 过滤消息。迁入蓝图不再保留 `ListenForGameplayMessages`、`Struct_UIMessaging`、`FLyraVerbMessage` 或 `LyraPlayerState` Cast 节点。
  - 迁入准星的 ADS / Eliminate 自定义动画应实现 `UShootReticleWidgetBase.OnReticleADSVisualChanged` 与 `OnReticleEliminationVisual` 两个项目视觉回调。这样保留 Lyra 动画资产，同时不引入 Lyra Verb Message 的 PlayerState 复制体系或制造第二个消息源。

- 动画与自定义事件  
  - `AimDownSights`：推进/回退改变 SizeBox override 和 `CrossHairs` 材质参数，实现收缩/回弹。蓝图 `ADS(bool bOn)` 事件根据布尔值选择 `Play Animation Forward` 或 `Reverse`。  
  - `Elimination`：播放中心红色贴图动画 `EliminationMarker`（材质参数 `GlowAlpha/Sharpness`），并在播放前确保 `Stop Animation` 与 `Set Current Time` 重置到 0。  
  - 命中提示动画由 `HitMarkerConfirmations` 控件自身处理，只需在命中消息中调用 `HandleHitNotification`。

- Widget 树结构  
  - `Root` → `Primary` Overlay  
    - `SOuterReticle`（SizeBox）→ `CrossHairs`（Lyra 使用 `UCircumferenceMarkerWidget`，项目需替换为 `UShootCircumferenceMarkerWidget`）。  
    - `HitMarkerConfirmations`（Lyra 的命中控件，替换为 `UShootHitMarkerConfirmationWidget`）。  
    - `TargetDot`（中心点 Image，可按需保留）。  
    - `EliminationMarker`（Image，绑定到 `Elimination` 动画材质参数）。  
  - `CrossHairs.MarkerList` 数据：步枪为四角 ±90°，霰弹使用八个条目（含 45°/135°），手枪相同但半径更小。

- 变量与常量  
  - 保留 `AsyncAction`、`AsyncActionElimination` 变量用于保存监听器引用。  
  - `SpreadMultiplier`/`ADSZoomScale` 等可作为编辑时常量，Tick 逻辑可直接引用。  
  - 命中贴图覆盖：弱点 (`Gameplay.Zone.WeakSpot`) 指向不同材质实例，需在控件属性中设置 `PerHitMarkerZoneOverrideImages`。

- 后续实现建议  
  - HUD 组件负责把 `Gameplay.Message.ADS`、`Lyra.Elimination.Message`、命中消息转换为我们定义的 `FShootReticleHitNotifyMessage` 并转发给准星。  
  - 蓝图迁移时只需复制 Lyra 的动画轨道和节点布局，替换控件类并将消息标签更新为项目内的 `FShootGameplayTags` 值。

参考截图已记录在当前会话：`W_Reticle_Rifle`、`W_Reticle_Shotgun`、`W_Reticle_Pistol`。如需逐帧动画曲线，可导入 Lyra 的 UMG 资源或复刻动画轨道。

## 开发规范（2025-11-09 新增）

1. **优先在基类下沉共有逻辑**  
   - 命中型 / 投射物型准星都会调用的逻辑必须放在 `UShootReticleWidgetBase`、`UShootGameplayAbility_Weapon_Fire` 等父类，子类只保留差异化实现，避免重复 Bug。  
   - 如果需要扩展，先检查父类是否可以通过虚函数或配置结构支持，确认无冲突后再在子类 override。

2. **代码注解与蓝图指引**  
   - C++ 关键步骤必须提供行内中文注解，说明数据流和 Blueprint 接入点（例如：`// HUD 组件在此调用 HandleHitNotification`）。  
   - 新增供蓝图继承的类时，在头文件注释写明“蓝图应继承自 X 并在 Construct 中调用 Y”，并在 `Docs/SystemDesign/UI` 中同步记录流程，方便后续 AI / 人类开发者查阅。

3. **新增文件必须纳入 Git**  
   - 所有新增 C++ / Slate / 文档文件在实现完即执行 `git add <path>`，否则其它成员无法在远程仓库看到成果。  
   - 本文档所在目录（`Docs/SystemDesign/UI`）记录准星相关约定，若有新规范请追加本节并在提交信息中说明，确保知识可复用。

## 实际实现结构（2025-11-09）

### 武器 → 准星 UI 管线

1. **数据配置**  
- `WeaponBasicConfig::ReticleWidgetClass` 指向一个 `UCommonActivatableWidget`（推荐使用 `UShootReticleWidget_Rifle/Shotgun/Pistol` 或其蓝图子类）。  
   - 武器实例 (`ARangedWeaponInstance`) 仍负责记录散布、弹药、投射物配置；命中型武器通过 `AHitscanWeaponInstance::GetCalculatedSpreadAngle()` 暴露实时散布，投射物通过 `AShootProjectileBase` 负责爆炸。

2. **HUD 组件**  
   - `UShootHUDReticleComponent` 挂在 `AShootPlayerController` 上，开局订阅以下 Gameplay Message：  
     - `Message.UI.Reticle.HitNotify`（命中提示）  
     - `Message.UI.Reticle.ADS`（瞄准状态）  
     - `Message.UI.Reticle.Elimination`（击杀反馈）  
     - `Msg_Quickbar_ActiveIndexChanged`（武器槽切换）  
- 当 Quickbar 发生切换时，组件读取当前 `UShootWeaponInstance` → ItemDefinition 的 `WeaponBasicConfig::ReticleWidgetClass` 并生成激活状态的 `UCommonActivatableWidget`，自动调用 `UShootReticleWidgetBase::InitializeFromWeapon` 绑定武器实例。旧的准星 Widget 会被销毁，确保屏幕上只有一个准星。

3. **准星 Widget 基类 (`UShootReticleWidgetBase`)**  
   - 继承 `ULyraActivatableWidget`，内部持有以下可选子控件引用：  
     - `UShootCircumferenceMarkerWidget`（四角散布渲染）  
     - `UShootHitMarkerConfirmationWidget`（命中提示）  
     - `UImage`（中心点、击杀闪光）  
   - `NativeTick` 中做三件事：  
     1. `ComputeMaxScreenspaceSpreadRadius()` → `CrosshairWidget->SetRadius()`，实时体现散布。  
     2. 对目标 ADS Alpha 进行插值，驱动 `USizeBox` 调整大小，完成腰射/瞄准缩放。  
     3. 更新击杀闪光透明度。  
   - `HandleHitNotification`、`HandleAdsState`、`HandleElimination` 三个接口分别由 HUD 组件在收到对应 Gameplay Message 时调用。  
   - 若蓝图需要自定义结构，可继承 `UShootReticleWidgetBase` 并在 `LoadReticleAppearance()` 中覆写默认材质、Marker 布局或 ADS 尺寸。

4. **命中信息来源**  
   - 命中型武器：`UShootGameplayAbility_Weapon_Fire::BroadcastReticleHitNotify` 在本地命中后生成 `FShootReticleHitNotifyMessage`，包含真实命中点 + `SourceActor`。HUD 组件据此在本地立即显示命中。  
   - 投射物武器：服务器在 `AShootProjectileBase::ApplyRadialDamage` 统计所有命中 Actor 的世界坐标，通过 `AShootPlayerController::ClientReceiveReticleHitNotify` RPC 发送给本地玩家，再转发到 Gameplay Message 系统。  
   - 两种方式最终都落到 `UShootHitMarkerConfirmationWidget::HandleHitNotification`（命中提示）以及 `UShootReticleWidgetBase::HandleHitNotification`（散布 UI）。

5. **Lyra 对照**  
   - `UShootHUDReticleComponent` 对标 Lyra 的 `ULyraHUDLayout` + `ULyraReticleWidgetBase` 管线；我们没有引入 Lyra 的装备/物品系统，因此直接把逻辑绑定在 PlayerController 上。  
   - `UShootReticleWidget_Rifle/Shotgun/Pistol` 对应 `W_Reticle_Rifle/S hotgun/Pistol` 蓝图，但核心动画和散布逻辑已经在 C++ 中（更便于调试与继承）。  
   - 若未来需要完全复刻 Lyra 的装备层次，可在 `ARangedWeaponInstance` 中补充类似 `ULyraEquipmentInstance` 的生命周期，再将 HUD 监听权交给装备组件；当前实现优先满足本项目的简化流程。

### 投射物 GA 与模拟代理（补充说明）

- `UShootGA_Weapon_Fire_Projectile` 的 `NetExecutionPolicy = LocalPredicted`，因此只有“本地玩家 + 服务器”会运行 GA，模拟代理不会执行能力逻辑。  
- 视觉反馈依靠 GameplayCue 与 Actor 复制：  
  - 枪口火花：`ActivateAbility()` 在客户端立即 `K2_ExecuteGameplayCueWithParams`，服务器也会触发同一 Cue，并向所有客户端广播，所以模拟代理能看到。  
  - 投射物：`SpawnProjectile()` 仅在服务器生成 `AShootProjectileBase`，该 Actor `bReplicates=true` 且复制运动，所有客户端（含模拟代理）都能看到飞行轨迹、拖尾等。  
  - 爆炸：`AShootProjectileBase::BroadcastExplosionCue()` 只在服务器调用，Cue 会同步到每个客户端；HUD 命中提示则通过 `ClientReceiveReticleHitNotify` 回到拥有者，用于更新准星。  
- 结论：火箭筒/榴弹/手榴弹的效果完全符合 GAS 指南——模拟代理只消费 GameplayCue + 复制的投射物，并不会尝试在其上运行 GA。

### 新增/覆盖代码查阅

| 模块 | 主要文件 | 职责 |
| ---- | -------- | ---- |
| HUD 组件 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/ShootHUDReticleComponent.*` | 管理准星 Widget 生命周期、监听 Gameplay Message |
| 准星基类 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/ShootReticleWidgetBase.*` | 散布计算、ADS 插值、命中/击杀反馈 |
| 子类外观 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/ShootReticleWidgets.*` | 步枪/霰弹/手枪默认样式 |
| 命中提示控件 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/ShootHitMarkerConfirmationWidget.*` | 屏幕命中点淡出 |
| 圆周控件 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/ShootCircumferenceMarkerWidget.*` | 绘制四角 Marker |
| 能力命中广播 | `Source/NewWorldOrder/Public(UI|Private)/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.*` | Hitscan 命中消息 |
| 投射物命中广播 | `Source/NewWorldOrder/Public(UI|Private)/Weapons/Projectiles/ShootProjectileBase.*` | 爆炸命中消息 |

开发者若需要调试准星相关逻辑，可从 ItemDefinition 的 `WeaponBasicConfig::ReticleWidgetClass` 出发，顺藤摸瓜到 HUD 组件，再看 Ability/Projectile 的消息发送点。
