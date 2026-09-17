# Weapons System Reference

## Data Pipeline
- 主数据表：`WeaponStats.csv` → `DT_WeaponStats`。字段包含 `BaseDamage`、`MagazineSize`、`FireRateRPM`、扩散参数、`BulletsPerShot`、`CritMultiplier`、`ReloadTime`、`ReloadType`、`WeaponTier`。
- 衰减曲线表：`WeaponFalloffCurves.csv` → `DT_FalloffCurves`，定义四个关键点（距离 cm / 倍率），运行时插值得到 `FRuntimeFloatCurve`。
- 敌人血量：`EnemyStatsPerChapter.csv`，按章节/难度设置 HP，供平衡验证。
- 弹药配置：`AmmoConfig.csv`，指定最大携带量与不同敌人掉落数量。
- `ARangedWeaponInstance` 在 `LoadWeaponData` 中根据 `WeaponID` 拉取以上数据并填充实例字段。
- 原始 CSV 存放于 `剧情和策划/NewOrder_UE5_CSVs_vFinal/` 目录（`Weapons.csv`、`AmmoConfig.csv`、`WeaponFalloffCurves.csv` 等）。修改数值时先编辑 CSV，再导入/生成对应 DataTable。

## Falloff Presets
- `Curve_CloseRange`（霰弹）：0–5m 100%，10m 50%，20m 10%，强制近身。
- `Curve_MidRange`（步枪/冲锋枪）：0–20m 100%，50m 70%，80m 40%。
- `Curve_LongRange`（狙击）：0–30m 100%，60m 90%，100m 75%。
- `Curve_NoFalloff`（爆炸类）：全程 100%/最远 60%，用于榴弹、火箭。

## Weapon Slots & Attachments
- 主武器槽：初始 1 把；Lv20 解锁第二主武器槽。
- 插槽命名（与 `UWeaponDefinition` 保持一致）：
  - `weapon_socket_hand_r` / `_l`：右手/左手持武器。
  - `weapon_socket_spine_back_r` / `_l`：背部挂点。
  - `weapon_socket_thigh_r`：手枪。
  - `weapon_socket_pelvis_melee`：近战。

## Combat Loop Constraints
- 切枪：按 `3` 键，动画 1.5 秒期间无法瞬切；服务器驱动。
- 弹药：主武器独立携带，拾取量按敌人类型与感知属性调整。
- 关键公式：
  - 距离伤害：`FinalDamage = BaseDamage * DistanceFalloff(Distance)`。
  - 弱点伤害：`CriticalDamage = FinalDamage * CritMultiplier * (1 + PerceptionBonus)`。
  - 护甲穿透：`EffectiveArmor = ArmorValue * (1 - StrengthPenetration)`。

## Balancing Targets
- 目标 TTK：0.3–0.7 秒；主武器占最终伤害 80%，技能提供 20% 倍增。
- 武器族群定位：
  - AR：中距离全能。
  - SMG：高射速近战。
  - SG：5 米爆发。
  - SR：远距离精准。
  - LMG：持续火力/压制。
  - 爆炸武器：战术工具，弹药稀缺。

## Implementation Notes
- `GrantAbilitiesToOwner` 时设置 `Spec.SourceObject = WeaponInstance`。
- 弹药管理与扩散恢复在服务器执行；OnRep 通过 `UGameplayMessageSubsystem` 广播到 UI。
- 换弹方式：`ReloadType = Magazine`（整匣） 或 `Single`（逐发），对应 GA/动画流程。
- 需要新武器时：更新 CSV → 重新生成 DataTable → 配置 `UWeaponDefinition` → 验证挂点与动画。
