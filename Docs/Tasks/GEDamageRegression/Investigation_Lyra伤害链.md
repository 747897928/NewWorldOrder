# Lyra 伤害链调查

## 本机 Lyra 版本与位置

- 项目：G:/Documents/Unreal Projects/LyraStarterGame。
- EngineAssociation：5.8。
- 上游源码只读，未做修改。
- 本文件只引用源码与资产文件名；GE/GA 内部属性值已通过 Lyra 11000 MCP 验证，BaseDamage 来源仍待查。

## 关键源码路径

- Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.h / cpp
- Source/LyraGame/Weapons/LyraRangedWeaponInstance.h / cpp
- Source/LyraGame/Weapons/LyraWeaponInstance.h / cpp
- Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.h / cpp
- Source/LyraGame/AbilitySystem/Attributes/LyraCombatSet.h / cpp
- Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.h / cpp
- Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.h / cpp
- Source/LyraGame/AbilitySystem/LyraGameplayEffectContext.h / cpp
- Source/LyraGame/AbilitySystem/LyraAbilitySourceInterface.h
- Source/LyraGame/Equipment/LyraGameplayAbility_FromEquipment.h / cpp
- Source/LyraGame/Physics/PhysicalMaterialWithTags.h / cpp

## 射击能力结构

### C++ 基类职责

ULyraGameplayAbility_RangedWeapon：

- 继承 ULyraGameplayAbility_FromEquipment。
- GetWeaponInstance 通过 AbilitySpec 的 SourceObject 取得 ULyraRangedWeaponInstance。
- ActivateAbility 只绑定 TargetData delegate、更新武器开火时间，然后调用父类。
- StartRangedWeaponTargeting 执行 PerformLocalTargeting，生成 FLyraGameplayAbilityTargetData_SingleTargetHit。
- TargetData.UniqueId 来自 ULyraWeaponStateComponent 的未确认命中计数。
- OnTargetDataReadyCallback 中服务器通过 WeaponStateComponent 确认或替换命中，然后 CommitAbility，成功后调用 BlueprintImplementableEvent OnRangedWeaponTargetDataReady。
- 伤害应用不在 C++ 基类，在 Blueprint 事件图。

### 蓝图资产

- /Game/Weapons/GA_Weapon_Fire
- /Game/Weapons/Pistol/GA_Weapon_Fire_Pistol
- /ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto
- /ShooterCore/Weapons/Shotgun/GA_Weapon_Fire_Shotgun

Epic 官方文档确认 GA_Weapon_Fire 的 OnRangedWeaponTargetDataReady：

1. 对武器拥有者执行开火 GameplayCue，传入第一个命中作为参数。
2. 遍历所有命中，对每个命中位置执行 Impact GameplayCue。
3. Authority 时对每个目标应用伤害 GameplayEffect。

每把枪使用不同 GE：

- Pistol：/Game/Weapons/Pistol/GE_Damage_Pistol。
- Rifle：/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto。
- Shotgun：/ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun。
- Melee：/ShooterCore/Weapons/GE_Damage_Melee。
- Grenade：/ShooterCore/Weapons/Grenade/GE_Damage_Grenade。

Lyra\ 11000\ MCP\ 已确认：\n\n-\ 父蓝图\ /Game/Weapons/GA_Weapon_Fire\ 有变量\ GE_Damage，类型\ TSubclassOf<GameplayEffect>。\n-\ 子蓝图各自覆盖\ GE_Damage：\n\ \ -\ GA_Weapon_Fire_Pistol\ ->\ /Game/Weapons/Pistol/GE_Damage_Pistol\n\ \ -\ GA_Weapon_Fire_Rifle_Auto\ ->\ /ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto\n\ \ -\ GA_Weapon_Fire_Shotgun\ ->\ /ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun\n-\ 父蓝图默认值也指向\ GE_Damage_Pistol，但正式武器子类均显式覆盖。\n-\ 每个\ GE\ 都是\ Instant，Modifiers\ 为空，Executions\ 只有\ 1\ 个\ ULyraDamageExecution。\n-\ GE\ 的\ GameplayEffectTags\ 分别带\ DamageType\.Pistol\ /\ Rifle\ /\ Shotgun。\n-\ GameplayEffectParent_Damage_Basic\ 同样只有\ ULyraDamageExecution，无\ Modifier。
## 伤害 GE 与 Execution

### GameplayEffectParent_Damage_Basic

Lyra 主工程存在：

- /Game/GameplayEffects/Damage/GameplayEffectParent_Damage_Basic
- /Game/GameplayEffects/Damage/GE_Damage_Basic_Instant
- /Game/GameplayEffects/Damage/GE_Damage_Basic_Periodic
- /Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller

Epic 文档说明：

- GameplayEffectParent_Damage_Basic 及其子类由武器和手雷应用。
- 伤害通过 ULyraDamageExecution 转换 BaseDamage 为 Health。
- Execution 负责友伤过滤。
- GameplayCue.Character.DamageTaken 以 Health 变化作为 magnitude 驱动伤害数字。

### ULyraDamageExecution

本机 5.8 源码事实：

- 捕获 Source 的 ULyraCombatSet::BaseDamage，Snapshot 为 true。
- 从 FLyraGameplayEffectContext 提取 EffectCauser 和 HitResult。
- 无 HitResult 时回退到目标 ASC 的 AvatarActor。
- 通过 ULyraTeamSubsystem::CanCauseDamage 得到队伍倍率，友方为 0。
- 距离优先取 EffectContext 的 Origin，否则 EffectCauser 位置到 ImpactLocation。
- 如果 EffectContext 中有 ILyraAbilitySourceInterface：
  - 从 HitResult 取 PhysicalMaterial，调用 GetPhysicalMaterialAttenuation。
  - 调用 GetDistanceAttenuation(Distance)。
- DistanceAttenuation 钳到 0 以上。
- DamageDone = BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * DamageInteractionAllowedMultiplier。
- 输出 AddOutputModifier 到 ULyraHealthSet::Damage，Additive。

旧版本 Lyra 的 Execution 还捕获目标 CurrentHealth 并把输出钳制到 CurrentHealth；本机 5.8 版本输出到 Damage 元属性。

### BaseDamage 来源

代码事实：

- ULyraCombatSet::BaseDamage 默认值 0.0。
- C++ 中没有任何地方直接 SetBaseDamage 或 InitBaseDamage。
- 因此 BaseDamage 的非零值只能来自 GE 资产配置或蓝图初始化。
- 因此 BaseDamage 的非零值只能来自 GE 资产配置或蓝图初始化。
- Lyra 11000 MCP 已确认：三个武器 GE 均无 Modifier，Weapon AbilitySet 均无 GrantedGameplayEffects，WeaponInstance 蓝图没有 BaseDamage 变量。
- 仍未定位：Lyra 从哪里给 Source 的 ULyraCombatSet::BaseDamage 赋非零值。
- 这是后续最优先的未决问题。

## 效果上下文

ULyraGameplayAbility::MakeEffectContext 覆盖了 UGameplayAbility 默认实现：

1. Super::MakeEffectContext。
2. 从全局 AbilitySystemGlobals 拿到 FLyraGameplayEffectContext。
3. GetAbilitySource：
   - EffectCauser = AvatarActor。
   - SourceObject = AbilitySpec 的 SourceObject。
   - AbilitySource = Cast<ILyraAbilitySourceInterface>(SourceObject)。
4. EffectContext->SetAbilitySource(AbilitySource, SourceLevel)。
5. AddInstigator(OwnerActor, EffectCauser)。
6. AddSourceObject(SourceObject)。

因此 GA 蓝图只要调用 ApplyGameplayEffectSpecToTarget，WeaponInstance 会自动成为 AbilitySource，Execution 可以直接查询该武器的衰减和物理材质倍率。

ULyraGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec：

- 如果 Spec Context 带 HitResult。
- 把 HitResult.PhysMaterial 的 UPhysicalMaterialWithTags::Tags 追加到 Spec.CapturedTargetTags.GetSpecTags()。
- 这样 Execution 的 TargetTags 可以知道命中表面是 WeakSpot 还是其他材质。

## WeaponInstance 数据归属

ULyraRangedWeaponInstance 本身持有全部射击参数：

- BulletsPerCartridge。
- MaxDamageRange。
- BulletTraceSweepRadius。
- DistanceDamageFalloff。
- MaterialDamageMultiplier。
- SpreadExponent。
- HeatToSpreadCurve / HeatToHeatPerShotCurve / HeatToCoolDownPerSecondCurve。
- SpreadRecoveryCooldownDelay。
- bAllowFirstShotAccuracy。
- 瞄准、站立、蹲伏、跳跃散布倍率与过渡参数。

实现 ILyraAbilitySourceInterface：

- GetDistanceAttenuation：
  - 读取 DistanceDamageFalloff 曲线。
  - 无数据返回 1.0。
- GetPhysicalMaterialAttenuation：
  - 遍历 PhysMatWithTags->Tags。
  - 对每个 Tag 查 MaterialDamageMultiplier。
  - 所有命中项相乘。

Lyra 没有 ShootInventoryFragment_RangedWeaponConfig 这类 Fragment；武器数值在 WeaponInstance 蓝图和 GE 中。

## 弱点与物理材质

Lyra 资产：

- /Game/Characters/Heroes/PhysMat_Player
- /Game/Characters/Heroes/PhysMat_Player_WeakSpot

类为 UPhysicalMaterialWithTags，Tag 由资产配置。

Lyra 弱点倍率机制：

1. Trace 带 bReturnPhysicalMaterial。
2. HitResult.PhysMaterial 进入 EffectContext。
3. ULyraGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec 把材质 Tags 写入 CapturedTargetTags。
4. LyraDamageExecution 取 PhysMaterial，调用 WeaponInstance->GetPhysicalMaterialAttenuation。
5. WeaponInstance 的 MaterialDamageMultiplier 中配置 WeakSpot Tag 倍率，如 2.0。
6. 最终伤害乘该倍率。

Lyra 11000 MCP 已确认：

- PhysMat_Player：无 Tag。
- PhysMat_Player_WeakSpot：Gameplay.Zone.WeakSpot。
- WeaponInstance 的 MaterialDamageMultiplier：
  - Pistol：Gameplay.Zone.WeakSpot = 2.0
  - Rifle：Gameplay.Zone.WeakSpot = 1.5
  - Shotgun：Gameplay.Zone.WeakSpot = 1.75
- 2026-08-21 已通过 11000 的 `RuntimeFloatCurve.export_text()` 复读三把枪的实际 Key。距离单位为 UE cm：

| 武器 | DistanceDamageFalloff Key | 含义 |
|---|---|---|
| Pistol | 0→1.0；2000→1.0；2001→0.5；25000→0.5 | 20 m 前满伤，之后降至 50% |
| Rifle | 0→1.0；2800→1.0；2801→0.5 | 28 m 前满伤，之后降至 50% |
| Shotgun | 0→1.0；600→1.0；640→0.7；2000→0.7；2001→0.5 | 6 m 前满伤，6.4 m 后 70%，20 m 后 50% |

Pistol 的 2000→2001 和 Rifle 的 2800→2801 使用 Constant 插值；因此它们是刻意的阶跃，不应在迁移时擅自平滑为线性曲线。

## GameplayCue Notify

Lyra 资产：

- /ShooterCore/Weapons/Pistol/GCN_Weapon_Pistol_Fire
- /ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire
- /ShooterCore/Weapons/Shotgun/GCN_Weapon_Shotgun_Fire

官方职责：

- Fire Cue 负责枪口火光、音效、弹壳、相机反馈等客户端表现。
- 不负责权威伤害计算。
- Fire Cue 以第一个命中作为参数，再由 GCN 内部处理命中数组或逐目标 Impact。
- Impact Cue 对每个命中位置播放表面命中表现。

## 与当前项目的主要差异

| 项目 | Lyra | NewWorldOrder 当前 |
|---|---|---|
| 每枪伤害 GE | GE_Damage_Pistol / GE_Damage_RifleAuto / GE_Damage_Shotgun | 无资产，统一 UShootEffect_DamageSetByCaller |
| 基础伤害位置 | CombatSet BaseDamage（GE 只选择 Execution 与伤害类型 Tag） | Fragment BaseDamage |
| 距离衰减 | WeaponInstance DistanceDamageFalloff | Fragment DistanceFalloff，当前为空 |
| 物理材质倍率 | WeaponInstance MaterialDamageMultiplier | Fragment MaterialDamageMultiplier，当前为空 |
| 弱点倍率 | PhysMat Tag 映射到武器倍率表 | GA 内写死 WeakSpot + Marked 1.1 |
| EffectContext | FLyraGameplayEffectContext，自动 AbilitySource | 默认 Context，手动 AddSourceObject |
| 伤害计算 | ULyraDamageExecution | GA ApplyDamageToTarget 手动算 |
| 属性后处理 | HealthSet Damage 元属性 | AttributeSet IncomingDamage |
| GCN | Lyra 原生蓝图 | 项目 C++ 适配器 + 迁入蓝图 |

## 第一阶段可借鉴但不照搬的部分

- 保留每枪独立 Damage GE 的思路。
- 保留 WeaponInstance 实现 ILyraAbilitySourceInterface 的思路。
- 保留 PhysMat Tag 经 MaterialDamageMultiplier 进入伤害公式的思路。
- 不照搬 WeaponStateComponent。
- 不照搬 Lyra 完整 HealthSet / CombatSet 复制体系，除非后续 RPG 属性需要。
- 若引入 Execution，项目可在 ShootAttributeSet 上实现轻量版本，或复用现有 IncomingDamage。

## 2026-08-21：BaseDamage 数值来源复核

本节补充 11000 Lyra 编辑器与 Lyra 源码的只读复核结果，修正此前“GE 或 CombatSet BaseDamage”的模糊表述。

已确认：

1. `GameplayEffectParent_Damage_Basic`、`GE_Damage_Basic_Instant`、Pistol/Rifle/Shotgun 三个 `GE_Damage_*` 的 `Modifiers` 都为空；唯一 Execution 均为 `ULyraDamageExecution`。
2. `GA_Weapon_Fire` 的 `GE_Damage` 变量由各武器子蓝图分别设置为各自 `GE_Damage_*`；它只将 TargetData 应用到该 GE，不在图中写 `BaseDamage`。
3. `/ShooterCore/Game/HeroData_ShooterGame` 只引用 `AbilitySet_ShooterHero`；该 AbilitySet 的 `GrantedAttributes` 为空，唯一 Granted Gameplay Effect 为 `GE_IsPlayer`。
4. Pistol、Rifle、Shotgun 的 ShooterCore AbilitySet 均为 `GrantedAttributes=[]`、`GrantedGameplayEffects=[]`。
5. `B_Hero_Default` 自身的组件和变量中未发现 CombatSet 初始化入口。Lyra 的 `LyraCharacterWithAbilities` 和 `LyraPlayerState` 都会以 `CreateDefaultSubobject` 创建 CombatSet，但 `ULyraCombatSet::BaseDamage` 的构造默认值仍为 0；全量 C++ 搜索未找到 `SetBaseDamage`、`InitBaseDamage` 或对该属性的 `SetNumericAttributeBase` 写入。

因此，当前已审查的 Lyra ShooterCore 可见资产与 C++ 链路没有提供一个可迁移的“把 Source CombatSet.BaseDamage 设为非零”的事实入口。不能把 `ULyraDamageExecution` 的 Attribute Capture 原样复制到 NewWorldOrder；项目没有对应的、已验证的数值初始化链，直接复制将让伤害计算为 0。

后续设计应明确选择一个项目侧权威来源：

- 保留 WeaponInstance/配置资产作为基础伤害来源，再在项目 Execution 中读取；或
- 新建项目 CombatSet 与其明确的装备期初始化/清理生命周期。

这是一项实施前架构决策，不应从 Lyra 的空 Attribute Capture 假设中推断出来。
