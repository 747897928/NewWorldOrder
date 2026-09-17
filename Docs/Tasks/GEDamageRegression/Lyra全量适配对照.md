# Lyra 全量适配对照

## 用途

- 这是“尽量抄 Lyra，不适配才改”的对照表。
- 执行 AI 先看这张表，知道每个系统在 Lyra 的哪个文件/蓝图，以及项目当前有没有对应物。
- 具体代码和蓝图事实见 Lyra关键代码摘录.md。

## 总原则

- 优先把 Lyra 的代码结构搬到项目。
- 只在项目已有明确架构差异处改：
  - PlayerState 持有 ASC。
  - Controller 持有 RuntimeOnly QuickBar。
  - Pawn 只做表现桥。
  - 已明确不引入 WeaponStateComponent 的旧决策需要重新确认。
- 如果某个系统项目已有等价物，先比较差异，再决定覆盖或保留。

## 伤害与 GE

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| 每枪伤害 GE | /Game/Weapons/GA_Weapon_Fire 变量 GE_Damage；GE_Damage_Pistol / GE_Damage_RifleAuto / GE_Damage_Shotgun | 无每枪 GE，统一 UShootEffect_DamageSetByCaller | 每枪创建 GE，Fire GA 从 WeaponInstance 或 GA 变量选择 |
| 伤害计算 | ULyraDamageExecution.cpp | GA ApplyDamageToTarget 手算 | 新增 UShootDamageExecution，GA 不再手算 |
| GE 基础伤害 | Lyra 从 Source ULyraCombatSet::BaseDamage 捕获 | Fragment BaseDamage | 迁到 UShootEffect_DamageBase::BaseDamage |
| 属性输出 | ULyraHealthSet::Damage | UShootAttributeSet::IncomingDamage | 保留项目 IncomingDamage |

## 距离衰减

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| 数据字段 | ULyraRangedWeaponInstance::DistanceDamageFalloff | ShootInventoryFragment_RangedWeaponConfig::DistanceFalloff | 迁到 UShootRangedWeaponInstance |
| 计算函数 | ULyraRangedWeaponInstance::GetDistanceAttenuation | UShootRangedWeaponInstance::GetDistanceAttenuation | 改成读实例字段 |
| 曲线资产 | B_WeaponInstance_Pistol / Rifle / Shotgun | 三把枪 Fragment 当前为空 | 从 Lyra WeaponInstance 蓝图复制曲线 |

## 弱点伤害

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| PhysMat Tag | /Game/Characters/Heroes/PhysMat_Player_WeakSpot | /Game/Blueprints/Character/PhysMat_Player_WeakSpot | Tag 已一致：Gameplay.Zone.WeakSpot |
| 倍率表 | ULyraRangedWeaponInstance::MaterialDamageMultiplier | ShootInventoryFragment_RangedWeaponConfig::MaterialDamageMultiplier | 迁到 UShootRangedWeaponInstance |
| 倍率数值 | Pistol 2.0 / Rifle 1.5 / Shotgun 1.75 | 当前为空 | 配置到 WeaponInstance 蓝图 |
| 计算入口 | ULyraDamageExecution -> GetPhysicalMaterialAttenuation | GA 内 WeakSpot + Marked 10% | 删除 GA 内 10% 硬编码，走 Execution |
| TargetTags | ULyraGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec 把 PhysMat Tags 写入 CapturedTargetTags | 项目未做 | 可抄，用于 Execution/Tag 后续扩展 |

## 准星 Reticle

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| Reticle 基类 | Source/LyraGame/UI/Weapons/LyraReticleWidgetBase.h/cpp | Source/NewWorldOrder/Public/UI/Weapons/ShootReticleWidgetBase.h/cpp | 项目已有等价物，对齐 Lyra 方法名与逻辑 |
| 圆周角标 | Source/LyraGame/UI/Weapons/CircumferenceMarkerWidget.h / SCircumferenceMarkerWidget.cpp | Source/NewWorldOrder/UI/Weapons/ShootCircumferenceMarkerWidget.* | 项目已有等价物 |
| 命中确认 | Source/LyraGame/UI/Weapons/HitMarkerConfirmationWidget.h / SHitMarkerConfirmationWidget.cpp | Source/NewWorldOrder/UI/Weapons/ShootHitMarkerConfirmationWidget.* | 项目已有等价物 |
| Reticle 配置 | Source/LyraGame/Weapons/InventoryFragment_ReticleConfig.h | ShootInventoryFragment_WeaponBasicConfig::ReticleWidgetClass | 项目已用 BasicConfig，保留 |
| 准星注册 | Lyra 用 WeaponStateComponent + W_WeaponReticleHost | UShootHUDReticleComponent + ShootReticleHostWidget | 项目已按 LocalPlayer HUD 注册，保留 |

Lyra Reticle 关键方法：

- InitializeFromWeapon
- ComputeSpreadAngle
- ComputeMaxScreenspaceSpreadRadius
- HasFirstShotAccuracy

项目 ShootReticleWidgetBase 已有同名方法，基本是从 Lyra 抄的，不需要重写。

## 命中提示 HitMarker

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| 服务器命中确认 | ULyraWeaponStateComponent::ClientConfirmTargetData | UShootGameplayAbility_Weapon_Fire::BroadcastReticleHitNotify + AShootPlayerController::ClientReceiveReticleHitNotify | 项目已用简化消息链 |
| 命中区域 | ULyraWeaponStateComponent::AddUnconfirmedServerSideHitMarkers 里从 PhysMat Tags 找 Gameplay.Zone | BroadcastReticleHitNotify 里 WeakSpot Tag | 可对齐 Lyra 的 HitZone 规则 |
| 客户端绘制 | SHitMarkerConfirmationWidget | SShootHitMarkerConfirmationWidget | 项目已有等价物 |

## WeaponStateComponent

Lyra 位置：

- Source/LyraGame/Weapons/LyraWeaponStateComponent.h/cpp
- Controller 组件。
- 职责：
  - Tick 当前武器。
  - 管理未确认命中标记。
  - 服务器确认后把命中标记同步给客户端。
  - 记录最近一次造成伤害的时间。

项目当前决策：

- UShootQuickBarComponent.cpp 注释明确“不引入该组件”。
- UShootGameplayAbility_Weapon_Fire 用简化 TargetData UniqueId=0。
- 准星和命中提示已有自己的消息链。

建议：

- 如果目标是“基本全抄 Lyra”，需要重新评估是否引入 ULyraWeaponStateComponent。
- 如果不引入，继续使用项目现有消息链。
- 如果引入，需要把 BroadcastReticleHitNotify 换成 WeaponStateComponent 的 AddUnconfirmedServerSideHitMarkers / ClientConfirmTargetData。

## 后坐力 Recoil

Lyra 现状：

- Lyra C++ 中没有独立 CameraRecoil 类。
- 后坐力/镜头表现主要在 B_Weapon Fire GameplayCue、动画、相机模式中。
- 没有从 C++ 里直接 AddYawInput / AddPitchInput 的通用武器后坐力代码。

项目当前：

- UShootRangedWeaponInstance::GetLocalCameraRecoil。
- UShootGameplayAbility_Weapon_Fire::ApplyLocalCameraRecoil。
- 数据在 Fragment 的 LocalCameraPitch/YawRecoilMin/Max。

建议：

- 如果跟随 Lyra，需要确认 Lyra B_Weapon Fire 蓝图里的后坐力实现，并判断是否替换项目 C++ 本地相机后坐力。
- 如果项目需要保留手感，可保留当前 C++ 本地后坐力，但把数据迁到 WeaponInstance 蓝图。
- 当前文档不直接删除项目后坐力，因为 Lyra 没有同构 C++ 可抄。

## 投射物与爆炸

| 项目 | Lyra 位置 | 项目当前位置 | 建议 |
|---|---|---|---|
| 投射物 | /ShooterCore/Weapons/Grenade/*、GA_Grenade | UShootGA_Weapon_Fire_Projectile + AShootProjectileBase | 项目已有，暂不重构 |
| 爆炸伤害 | GE_Damage_Grenade | ShootProjectileBase::CalculateExplosiveDamage + SetByCaller | 后续可迁移到 Execution |

## 执行建议顺序

1. 先按 Implementation_迁移方案.md 完成武器直射伤害 GE。
2. 再把距离衰减和弱点倍率迁到 WeaponInstance。
3. 对比项目 Reticle 与 Lyra Reticle，补齐差异。
4. 对比项目 HitMarker 与 Lyra HitMarker，补齐 HitZone 规则。
5. 由用户决定是否引入 WeaponStateComponent。
6. 由用户决定后坐力是否跟随 Lyra B_Weapon 表现，或保留项目 C++ 后坐力。
7. 最后做全量 PIE：单人、分屏、Listen Server。
