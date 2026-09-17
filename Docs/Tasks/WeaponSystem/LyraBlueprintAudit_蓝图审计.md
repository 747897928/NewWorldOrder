# Lyra ShooterCore 蓝图审计

# 审计范围

2026-07-25 通过 LyraStarterGame 的 UE 5.8 MCP 端口 11000 只读完成。审计了 `/Game/Weapons`、`/ShooterCore/Weapons/Rifle`、`Pistol`、`Shotgun` 中的核心武器蓝图、Damage GE、AbilitySet 与 Reticle。

# 已修正的迁移判断

不应迁移整个 `/Game/Weapons` 根目录。该目录会携带全局 Audio、Effects、Characters、Tests、Spawnpad 和其他非射击系统内容。

也不应把所有武器蓝图一律排除。`B_Weapon` 表现链和 Damage GE 有明确迁移价值。正确做法是按父类和职责分为运行时表现迁移包、转换参考包和明确排除包。

# 运行时表现迁移包

以下蓝图均以 `Actor` 为父类，不依赖 Lyra Weapon C++ 作为父类：

- `/Game/Weapons/B_Weapon`
- `/Game/Effects/Blueprints/B_WeaponFire`
- `/Game/Effects/Blueprints/B_WeaponImpacts`
- `/Game/Effects/Blueprints/B_WeaponDecals`
- `/ShooterCore/Weapons/Rifle/B_Rifle`
- `/ShooterCore/Weapons/Pistol/B_Pistol`
- `/ShooterCore/Weapons/Shotgun/B_Shotgun`

`B_Weapon` 已验证包含 SkeletalMesh、枪口火焰、弹壳、Tracer、假投射物、表面命中、连续开火音频和本地 CustomStencil 表现。`B_WeaponImpacts` 与 `B_WeaponDecals` 使用 Niagara 数据通道和表面类型分流。

迁移后不能直接作为项目最终 WeaponActor。需要创建或使用项目 `AShootWeaponActor` 蓝图子类，保留枪口、开火音频、弹壳和 VFX 表现；移除或替换 Lyra TeamDisplayAsset 的队伍染色调用。PVE 项目不迁移 Lyra 的严格命中确认机制。

# Damage GE 迁移包

- `/Game/GameplayEffects/Damage/GE_Damage_Basic_Instant`
- `/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto`
- `/Game/Weapons/Pistol/GE_Damage_Pistol`
- `/ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun`

三种武器 Damage GE 都继承 `GE_Damage_Basic_Instant`，没有自定义图表。应迁移为数值和 SetByCaller/标签配置参考；接入前需映射到项目 AttributeSet 和项目伤害 GameplayTag，不能假定 Lyra 的属性路径可直接生效。

# 转换参考包

以下资产应阅读或可迁入单独的 Reference 目录，但不能作为本项目直接运行时资产：

- `/Game/Weapons/GA_Weapon_Fire`
  - 父类为 `LyraGameplayAbility_RangedWeapon`。
  - 包含 TargetData 处理、开火/命中 GameplayCue、服务器伤害、自动开火计时、失败开火 Montage 和可选物理场逻辑。
  - 项目已有 `UShootGameplayAbility_Weapon_Fire`，应以该蓝图作为行为对照补齐缺失表现，不复制 Lyra C++ 父类。
- `/ShooterCore/Weapons/*/GA_Weapon_Fire_*`
  - 均继承 `GA_Weapon_Fire_C`，图表只有 Activate/End 的父函数调用。
  - 项目对应 Rifle/Pistol/Shotgun GA 应以项目 C++ 基类创建或配置，不直接依赖这些蓝图。
- `/ShooterCore/Weapons/*/GA_Weapon_Reload_*`
  - 均依赖 `GA_Weapon_ReloadMagazine_C`。
  - 项目已有 `UShootGameplayAbility_ReloadMagazine` 和枪种子类，应转换其 Montage、时序和配置，不迁移为运行时父类。
- `/ShooterCore/Weapons/*/AbilitySet_Shooter*`
  - 类型为 `LyraAbilitySet`，项目不能直接使用。
  - 例如 `AbilitySet_ShooterPistol` 授予 Pistol Fire、Pistol Reload、AutoReload 三项能力；需要在项目 `UShootAbilitySet` 中建立等价条目。
- `/ShooterCore/Weapons/*/WID_*`、`ID_*`、`B_WeaponInstance_*`、`WeaponPickupData_*`
  - 分别依赖 Lyra EquipmentDefinition、InventoryItemDefinition、WeaponInstance 和 PickupDefinition 类型。
  - 项目有对应的 Equipment/Inventory/WeaponInstance 主线，应读取字段后转换为项目 DataAsset/Fragment，不直接保留 Lyra 父类。

# 暂不迁移的 UI 蓝图

`W_Reticle_Rifle`、`W_Reticle_Pistol`、`W_Reticle_Shotgun` 与 `W_AmmoCounter_*` 全部继承 `LyraReticleWidgetBase`。

`W_Reticle_Rifle` 已验证包含 ADS、击杀消息、命中消息、外圈准星半径和屏幕散布计算。应在项目 `UShootReticleWidgetBase` 与 CommonUI HUD 层改造完成后，手工复刻其控件树和图表；当前不迁移，避免缺失父类后形成不可用 Widget。

# 明确排除

- Lyra WeaponStateComponent、未确认命中队列和服务器命中替换。
- Tests、Spawnpad、WeaponPickupDefinition 运行时链路。
- 整个 Audio、Effects、Characters 根目录。
- 与当前三种参考武器无关的 Grenade、Healthpack、Launchpad 内容。

# 下一步迁移顺序

1. 先完成项目侧的持枪状态桥接，确保男女主 AnimBP 可接收装备和瞄准状态。
2. 再迁移一次性 Rifle 表现包：`B_Weapon` 表现链、`B_Rifle`、Rifle Mesh/Material/Texture/Sound/Animation、所需 Niagara 资产和 Rifle Damage GE。
3. 迁移完成后用 MCP 检查缺失类、引用和编译状态；只处理 Rifle 的错误。
4. 在项目中创建 Rifle ItemDefinition、EquipmentDefinition 和 `UShootAbilitySet` 等价资产，接到现有 Inventory/Equipment 主线。
5. Rifle 在单人、分屏、Listen Server 验收通过后，再按同一清单迁移 Pistol 与 Shotgun。
