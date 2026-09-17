# 弹药 StatTag 单一数据源与接口精修

## 状态

- 开始日期：2026-08-28
- 当前状态：已完成
- 范围：七个 `ID_*`、`UShootInventoryFragment_RangedWeaponConfig`、`UShootRangedWeaponInstance`、开火弹药 Cost、Reload 与补给站容量消费者
- 不包含：武器数值重平衡、动画融合、Rocket/Grenade Cascade 迁移、第一人称 CameraMode

## 目标

- 当前弹药和容量统一存在 `UShootInventoryItemInstance` 的 StatTagStack，不再由 Ranged Fragment 保存第二份容量数据。
- 新建武器只需在 `UShootInventoryFragment_SetStats.InitialItemStats` 配置四个弹药 Tag，不需要在多个 Fragment 间手工对齐。
- 删除已确认没有 C++ 或蓝图消费者的弹药兼容接口，降低 `UShootRangedWeaponInstance` 的理解成本。

## 权威字段

`UShootInventoryFragment_SetStats.InitialItemStats` 上的四个 Tag 是弹药初始状态和容量的唯一配置入口：

| Tag | 职责 |
|---|---|
| `Inventory.Ammo.Magazine` | 创建 ItemInstance 时的当前弹匣弹药 |
| `Inventory.Ammo.Reserve` | 创建 ItemInstance 时的当前备用弹药 |
| `Inventory.Ammo.MagazineCapacity` | 弹匣容量 |
| `Inventory.Ammo.ReserveCapacity` | 备用弹药容量 |

`UShootInventoryFragment_RangedWeaponConfig` 只保留射程、角色 Montage、本地后坐力等远程武器规则，不再保存 `MagazineSize`、`MaxReserve`、`AmmoPerShot`。

## AmmoPerShot 决策

- 当前七把武器的 `AmmoPerShot` 都是 `1`。
- Lyra 源码没有对应的武器 Fragment 字段；可变消耗数量属于 Ability Cost 职责。
- Shotgun 的多弹丸由 `BulletsPerCartridge` 表达，一次扣一发。
- 因此项目开火 Cost 固定消耗一个 `Inventory.Ammo.Magazine`，删除 `AmmoPerShot`。未来若某个技能需要多单位消耗，应在该 Ability Cost 资产上配置，不应恢复 Fragment 重复字段。

## 迁移清单

迁移前通过 Unreal Editor CDO 回读确认了当前资产实际值。这些值优先于较早审计文档里的历史数值，本轮不做数值重平衡。

| ItemDefinition | Magazine | Reserve | MagazineCapacity | ReserveCapacity |
|---|---:|---:|---:|---:|
| `ID_Rifle` | 30 | 60 | 30 | 60 |
| `ID_Pistol` | 12 | 48 | 12 | 48 |
| `ID_Shotgun` | 8 | 24 | 8 | 24 |
| `ID_Shotgun_A` | 8 | 24 | 8 | 24 |
| `ID_Sniper_Rifle_A` | 10 | 30 | 10 | 30 |
| `ID_Grenade_Launcher_A` | 6 | 18 | 6 | 18 |
| `ID_Rocket_Launcher_A` | 4 | 12 | 4 | 12 |

## 接口审计

- 通过 Unreal BlueprintService 扫描 `/Game/Weapons`、`/Game/Blueprints/Weapons`、`/Game/UI/Weapon`、`/Game/Characters`、`/Game/Effects`。
- 扫描覆盖 111 个蓝图、389 个非空 Graph。
- 以下旧公开接口在蓝图图表与 C++ 中均为零消费者，本轮删除：
  - `HasEnoughAmmo`
  - `IsMagazineFull`
  - `IsMagazineEmpty`
  - `ConsumeAmmo`
  - `ConsumeAmmoPredicted`
  - `ReloadAmmoPredicted`
  - `GetAmmoPerShot`
  - `GetCurrentSpreadAngle`
  - `CalculateSpreadOffset`
  - `GetSpawnedRangedActor`
  - `GetWeaponActorForwardVector`
- `ReloadAmmo`、`AddMagazineAmmo`、`AddReserveAmmo`、`RefillAmmoToCapacity`、`GetMagazineSize` 仍有正式消费者，保留并改为读取容量 StatTag。

## 验收标准

1. Windows Editor Target 冷编译通过。
2. 七个 `ID_*` 都持有四个弹药 Tag，且当前值不超过容量。
3. `RangedWeaponConfig` CDO 不再暴露三个旧弹药字段。
4. Reload 仍以 `MagazineCapacity - Magazine` 计算需要量，补给站把两类弹药补到容量 StatTag。
5. Rifle、Pistol、Shotgun、Shotgun_A、Sniper、Grenade Launcher、Rocket Launcher 的开火每次只扣一发。
6. QuickBar 弹药数、换弹、AutoReload、掉落 StatTag 快照与 Listen Server 复制行为不回归。

## 回滚边界

- 不用历史文档值覆盖玩家已在 `ID_*` 上调整的当前数值。
- 如果资产迁移失败，应先恢复上表四个 StatTag，不恢复已删除的 Fragment 双数据源。
- 编译与资产迁移完成前，不单独提交一个无容量 Tag 的中间状态。

## 完成记录

- `Scripts/Build_Windows.ps1` 已在关闭 NewWorldOrder Editor 后完成冷编译；UHT、5 个模块编译单元、`UnrealEditor-NewWorldOrder.lib` 和基础 `UnrealEditor-NewWorldOrder.dll` 全部通过，退出码为 0。
- 重启后通过 Unreal MCP 批量迁移、编译和保存七个 `ID_*`。
- 逐资产复读确认四个弹药 Tag 数值与上表一致，当前值均不超过容量。
- 新反射环境中 `magazine_size`、`max_reserve`、`ammo_per_shot` 三个旧属性对七把武器均不存在；资产保存后 Dirty Content 与 Dirty Maps 均为空。
