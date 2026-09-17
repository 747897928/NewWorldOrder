# Rocket Launcher A：挂载弹头与换弹动画接入

## 当前目标

让 Rocket Launcher A 的可见火箭弹头成为一个可复制的投射物 Actor：

- 装备且弹匣有弹时，弹头挂在武器的 `AmmoSocket`；
- 开火时脱离并直接作为飞行投射物使用，不在 Fire GA 再生成第二个弹头网格；
- 换弹开始时按备弹生成一枚新的弹头并挂到角色左手，换弹提交通知或现有 `AN_Reload` 兜底时再转挂发射器；
- `AN_Reload` 到达提交点后，仍由现有弹药 StatTag 逻辑结算当前弹匣与备弹。

## 已核对的资产事实

- `/Game/Weapons/Rocket_Launcher_A/ID_Rocket_Launcher_A` 当前配置：
  - `Inventory.Ammo.Magazine = 1`
  - `Inventory.Ammo.MagazineCapacity = 1`
  - `Inventory.Ammo.Reserve = 12`
  - `Inventory.Ammo.ReserveCapacity = 12`
- `ProjectileWeaponConfig.ProjectileClass` 为 `ShootProjectileRocket`。
- `ProjectileConfig.ProjectileMesh` 为 `/Game/Weapons/Rocket_Launcher_A/Mesh/RocketLauncherA_Ammo`。
- `/Game/Weapons/Rocket_Launcher_A/Mesh/Rocket_Launcher_A` 已有 `AmmoSocket`，父骨骼为 `Ammo`，Socket 相对旋转为 Identity。
- `Reload_RocketLauncher_W` 长度为 2 秒；`Ammo` 骨骼在动画开头处于手边位置，在约 1.5 秒回到发射器内部。角色正式 Reload Montage 的 `AN_Reload` 提交点为约 1.746580 秒。

## 实现边界

- 新增 `UShootRocketLauncherWeaponInstance`，仅火箭筒使用；不在普通武器或通用弹匣换弹逻辑中写 Rocket 分支。
- 新增 `UShootGA_Reload_RocketLauncher`，复用 `UShootGameplayAbility_ReloadMagazine` 的蒙太奇、`GameplayEvent.ReloadDone` 和弹药结算语义；专用 GA 只在蒙太奇启动时创建/挂接弹头。
- `UShootGA_Weapon_Fire_Projectile` 增加可选的 `TakeProjectileForLaunch` 钩子；默认投射物返回空指针，仍走原来的 Deferred Spawn。
- `AShootProjectileBase` 增加 Held 状态：挂载时显示 `ProjectileMesh`、关闭碰撞/移动/拖尾；发射时由 `InitializeProjectile` 恢复正常飞行状态。
- `Rocket_Launcher_A` 骨骼内用于动画导向的 `Ammo` 网格由专用实例隐藏，避免和外部可复制弹头重叠；如果外部投射物创建失败则保留资产内置网格作为回退。
- CDO 初始化不读取 `FShootGameplayTags::Get()`；专用 Reload GA 使用 `DefaultGameplayTags.ini` 中的稳定 Tag 名称配合 `FGameplayTag::RequestGameplayTag(..., false)`。
- `UShootRocketLauncherWeaponInstance::PrepareRocketForReload()` 判断的是“当前弹匣为空且备弹大于 0”，不能复用只适用于已装填状态的 `CurrentAmmo > 0` 生成门槛；火箭筒是 1 发弹匣，打空后仍必须能准备下一枚弹头。
- 手部挂点使用可配置的 `RocketReloadHandSocketName`。当前运行时角色是 Mutable 生成的 SkeletalMesh，新增到基础 `ShenWanYun` 网格的自定义 Socket 不会可靠传播到运行时网格，因此正式配置使用始终存在的 `hand_l` 骨骼，并通过 `RocketReloadHandRelativeTransform` 调整旋转和位移。

## 接线与验收状态

已使用项目规定的 `Scripts/Build_Windows.ps1 -RestartEditor -WaitForReady -Map /Game/Maps/TestMap_ListenServer` 完成冷编译和重启，并核对：

1. 编辑器进程命令行包含完整且带引号的 `NewWorldOrder.uproject` 路径；VibeUE readiness signal 的 `projectFile` 和 `currentMap` 分别为本项目和 `/Game/Maps/TestMap_ListenServer`。
2. `/Game/Weapons/Rocket_Launcher_A/B_WeaponInstance_Rocket_Launcher_A` 的父类是 `ShootRocketLauncherWeaponInstance`，运行时实例可读到：
   - `RocketReloadHandSocketName = hand_l`；
   - `RocketReloadHandRelativeTransform` 已配置初始旋转/位移，可在蓝图 Details 中继续微调；
   - `RocketAmmoSocketName = AmmoSocket`。
3. `/Game/Weapons/Rocket_Launcher_A/AS_Weapon_Rocket_Launcher_A` 的整匣 Reload 条目使用 `ShootGA_Reload_RocketLauncher`；通用 Reload GA 未增加火箭筒分支。
4. 单人 PIE 运行态已确认：
   - 拾取火箭筒后为 `1/12`，开火后为 `0/11`；旧的 `ShootProjectileRocket` 脱离武器并成为飞行 Actor；
   - 换弹完成后回到 `1/11`，新的 `ShootProjectileRocket` 挂在 `B_Rocket_Launcher_A` 上，旧飞行弹头没有被重新利用；
   - 弹头网格使用 `RocketLauncherA_Ammo`，发射器的 `Ammo` 导向骨骼仍由专用实例隐藏；
   - `GameplayEvent.Reload.RocketInsert` 通知已写入女性角色正式 Rocket Reload Montage，现有 `AN_Reload -> GameplayEvent.ReloadDone` 仍负责弹药结算。
5. 手部视觉链已经从“不会生成”修正为“会生成但需要按角色动画微调姿态”。如果旋转仍需调整，修改 `B_WeaponInstance_Rocket_Launcher_A` 的 `RocketReloadHandRelativeTransform`；不要把 `RocketReloadHandSocketName` 改成基础网格中未传播到 Mutable 运行时网格的临时 Socket。

尚未在本轮重新完成双客户端远端复制、切枪中断和网络下的换弹中断验收；这些是后续网络回归项，不应被描述为已完成。当前已确认单人开火、备弹换弹、手部准备和回挂 `AmmoSocket` 的核心链路。
