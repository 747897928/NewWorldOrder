# 武器系统收尾审计

## 状态

- 审计日期：2026-08-28
- 审计范围：库存 Fragment、Equipment、WeaponInstance、Pickup、GameplayCue、AutoReload、特殊投射物配置、Cascade 与 Niagara
- 当前基线：火箭筒和榴弹发射器已经由玩家验收，投射物会从枪口朝准星目标飞行；实体投射物使用 `WorldDynamic` 对象类型，火箭撞地爆炸，榴弹与地板碰撞、反弹并按引信工作
- P0 状态：2026-08-27 已完成 Rocket/Grenade Listen Server 模拟代理、服务器权威伤害和 Rifle/Pistol/Shotgun 主线回归
- 2026-08-28 检查点：已接入共享 AutoReload、共享按住 ADS、Sniper Scope、武器与弹药图标消息链；已删除无消费者的 Equipment Montage 字段和无引用旧 AnimInstance 类
- ADS 已修正 Lyra 输入触发器、眼高 Pivot 和退出过渡生命周期；玩家已验收按住进入、松开退出和相机完整恢复，冷编译与 PIE 回归均通过
- Sniper 枪口表现已完成资产级归一：移除 Montage 重复声音、枪口火焰和独立 Cascade Tracer，只在底层 Sequence 保留一次声音与一次狙击枪专用枪口火焰；新增按 Owner 视图隐藏 ADS 枪口粒子的项目 AnimNotify。玩家已验收开镜拥有者不见烟雾、其他玩家仍见枪口火焰，贴在枪口前的持续光柱也已消失
- 玩家已完成 Shotgun_A 远程拥有客户端与 Sniper 完整 Listen Server 回归：AutoReload、逐发复制、Montage/开火打断、ADS/Scope/退出和分屏 Owner 可见性均通过

## 总结结论

- 当前武器系统仍有收尾任务，不能回答“没有需要完成的任务”。
- 弹药当前值与容量已进入 StatTag 单一数据源迁移；详细映射、接口审计和验收边界见 `AmmoStatTagMigration_弹药StatTag单一数据源与接口精修.md`。
- `UShootEquipmentDefinition_Weapon` 上无消费者且七个 `BP_Equipment_*` CDO 均为 `None` 的 `EquipMontage`、`UnequipMontage` 已删除；实际装备动画继续来自 `RangedWeaponConfig.CharacterMontages`。
- `B_WeaponInstance_*`、`BP_Equipment_*`、`BP_WeaponPickup_*` 职责不同，不属于可合并的重复资产。
- 项目已经有 GCN，火箭和榴弹的 Fire、Detonate 均有独立 GameplayCue Notify；“项目没有 GCN”不符合当前资产事实。
- AutoReload 已实现为独立共享 OnSpawn GA，并接入全部七把武器；Shotgun_A 仍使用自己的逐发装填 GA，共享被动只负责在空仓且有备弹时激活它，不伪造输入、不接管逐发提交时序。
- 长期方向应为 Niagara-first；但火箭和榴弹必须作为独立、原子化迁移任务完成视觉等价、联机验证和旧 Cascade 删除，不能把步枪的通用 Niagara 生搬过去，也不能留下新旧双链路长期并存。

## 当前资产职责边界

### ItemDefinition 与 Fragment

- `ID_*` 是物品定义入口。
- `ShootInventoryFragment_EquippableItem` 指向装备定义。
- `ShootInventoryFragment_WeaponBasicConfig` 提供 WeaponId、准星、掉落拾取类等通用配置。
- `ShootInventoryFragment_RangedWeaponConfig` 提供射击、换弹、角色动画、射程和本地后坐力配置；弹药当前值和容量属于 ItemInstance StatTags。
- 火箭和榴弹额外使用 `ShootInventoryFragment_ProjectileWeaponConfig`，提供投射物类、速度、重力、反弹、引信、伤害和尾迹配置。
- `ShootInventoryFragment_SetStats` 在创建 ItemInstance 时写入当前弹药等 StatTag 状态。

当前七个 `ID_*` 没有发现完全空置的 Fragment。现有 Fragment 不能按“字段看起来相似”直接删除。

### Equipment、WeaponInstance、装备 Actor 与 Pickup

```text
ID_*
  -> EquippableItem.EquipmentDefinition
  -> BP_Equipment_*
       -> InstanceType = B_WeaponInstance_*
       -> ActorsToSpawn = B_* 装备表现 Actor

BP_WeaponPickup_*
  -> 世界中的交互与掉落表现
  -> 拾取后授予 ID_*
  -> 掉落时保存当前 StatTag 快照
```

- `BP_Equipment_*` 描述装备时创建什么运行时实例和表现 Actor。
- `B_WeaponInstance_*` 是装备期间的 UObject 运行时逻辑与配置，负责动画层、扩散、热量、伤害 GE、衰减和材质倍率等。
- `B_*` 是装备到角色身上的武器表现 Actor。
- `BP_WeaponPickup_*` 是世界拾取和掉落 Actor，不承担装备期间的武器实例职责。

结论：这四层存在显式职责边界，不应合并。

## 弹药字段审计

### 迁移后的字段职责

- `SetStats.InitialItemStats[Inventory.Ammo.Magazine]`：创建物品时的当前弹匣弹药。
- `SetStats.InitialItemStats[Inventory.Ammo.Reserve]`：创建物品时的当前备用弹药。
- `SetStats.InitialItemStats[Inventory.Ammo.MagazineCapacity]`：弹匣容量。
- `SetStats.InitialItemStats[Inventory.Ammo.ReserveCapacity]`：备用弹药容量。

运行时当前弹药和容量都从关联 ItemInstance 读取；Reload 和补给站不再读取 Ranged Fragment 中的第二份容量数据。

### 当前 CDO 数据

| 武器 | Ranged 容量 | SetStats 初始当前值 | 结论 |
|---|---:|---:|---|
| Rifle | 30 / 60 | 30 / 60 | 当前相同 |
| Pistol | 12 / 48 | 12 / 48 | 当前相同 |
| Shotgun | 8 / 24 | 8 / 24 | 当前相同 |
| Shotgun_A | 8 / 24 | 8 / 24 | 当前相同 |
| Sniper | 10 / 30 | 10 / 30 | 当前相同 |
| Grenade Launcher | 6 / 18 | 6 / 18 | 当前相同 |
| Rocket Launcher | 4 / 12 | 4 / 12 | 当前相同 |

### 已选定的迁移方向

优先采用更接近 Lyra 的单一 StatTag 数据源，但必须分阶段迁移：

1. 增加 `Inventory.Ammo.MagazineCapacity` 和 `Inventory.Ammo.ReserveCapacity`。
2. 将 Reload、AmmoStation、CanActivate、HUD 等容量消费者迁移到 ItemInstance StatTag。
3. 为七个 `ID_*` 补齐容量 StatTag，并允许当前弹药小于容量。
4. 用自动审计验证所有武器的当前值不超过容量。
5. 完成 CDO、单机、Listen Server 和掉落快照回归后，才删除 `RangedWeaponConfig.MagazineSize` 与 `MaxReserve`。

`AmmoPerShot` 在七把武器上均为 `1`，Lyra 也不把该规则放在武器 Fragment。项目固定一次开火扣一个弹药单位，Shotgun 多弹丸由 `BulletsPerCartridge` 表达，因此本轮删除该字段。

## 装备动画字段审计

### 已完成的高置信度清理

- `UShootEquipmentDefinition_Weapon` 继承自 `UShootEquipmentDefinition`；无消费者的 `EquipMontage`、`UnequipMontage` 已从该武器子类删除。
- `UShootEquipmentDefinition_Weapon` 本身继续保留：它在构造函数中把父类 `UShootEquipmentDefinition.InstanceType` 默认设为 `UShootRangedWeaponInstance`，为武器装备蓝图提供语义分类和错误配置兜底；当前七个蓝图虽显式覆盖具体实例类型，但为删除空壳而批量重设父类没有实际收益。
- 当前装备和卸下动画仍由 `UShootRangedWeaponInstance::FindCharacterMontage` 从 `RangedWeaponConfig.CharacterMontages` 解析。
- 无引用的 `UShootAnimInstance` 与 `UShootAnimInstanceBase` 已删除；删除由用户复核引用后确认。

### 保留的兼容字段

- 受保护 Lyra 射击主线中的 `CharacterFireMontage`、`CharacterReloadMontage` 已完成 BlueprintService 与 AssetRegistry 父类审计，不能按零消费者删除。
- 七把正式武器都使用 `ID_*` 显式骨架 Montage；`GA_Weapon_Reload_Rifle/Pistol/Shotgun` 的 `CharacterReloadMontage` 均为 `None`，且三个蓝图没有变量或函数图表覆盖。
- 遗留 `/Game/Blueprints/Weapons/Assault_Rifle_A/GA_AssaultWeapon_File` 仍把 `AM_Fire_Rifle_Ironsights` 写入 `CharacterFireMontage`；同目录 `GA_AssaultWeapon_Reload` 仍把 `AM_Reload_Rifle_Ironsights` 写入 `CharacterReloadMontage`。
- 两个遗留 GA 当前没有外部 Asset Referencer，但“遗留资产候删”与“基类字段无消费者”不是同一结论。按受保护主线和多资产删除规则，本轮保留字段与资产；若产品确认整套 Assault_Rifle_A 旧 GA 可废弃，应先单独删除或迁移该资产闭包，再重新审计字段。

## 武器动画样式与特殊武器审计

### 结论

- Lyra 源码没有枪型 `AnimationStyle` 枚举；`ULyraWeaponInstance` 直接保存 `EquippedAnimSet` 与 `UneuippedAnimSet`，再按 Cosmetic Tag 从 `FLyraAnimLayerSelectionSet` 选择唯一 Linked Layer。
- 项目现行 `FShootAnimLayerSelectionSet`、`UShootWeaponInstance::EquippedAnimSet` 与 `UnequippedAnimSet` 已采用同一架构。旧的 `EShootWeaponAnimationStyle` 和 `UShootRangedWeaponInstance::GetAnimationStyle()` 已不在源码或资产调用链中，不应恢复，也不应扩展 Rocket、Grenade、Sniper 枚举值。
- `Cosmetic.AnimationStyle.Feminine` 是角色外观标签，只用于在同一武器姿势中选择女性 Layer；它不是武器类型枚举。

### 七把武器的资产事实

| WeaponInstance | 角色 Equipped Layer | 武器自身 AnimBP | Fire/Reload Montage |
|---|---|---|---|
| Rifle | Rifle 男/女 | `ABP_Weap_Rifle` | Rifle 独立配置 |
| Pistol | Pistol 男/女 | `ABP_Weap_Pistol` | Pistol 独立配置 |
| Shotgun | Shotgun 男/女 | `ABP_Weap_Shotgun` | Shotgun 独立配置 |
| Shotgun_A | Shotgun_A 男/女独立入口 | `ABP_Weap_Shotgun_A` | Shotgun_A 逐发装填独立配置 |
| Sniper_Rifle_A | Sniper_Rifle_A 男/女独立入口 | `ABP_Sniper_Rifle_A` | Sniper 独立配置 |
| Grenade_Launcher_A | Grenade_Launcher_A 男/女独立入口 | `ABP_Grenade_Launcher_A` | Grenade Launcher 独立配置 |
| Rocket_Launcher_A | Rocket_Launcher_A 男/女独立入口 | `ABP_Rocket_Launcher_A` | Rocket Launcher 独立配置 |

Shotgun_A、Sniper、Grenade Launcher、Rocket Launcher 现已各有男女独立 Linked Layer 资产，并由对应 `B_WeaponInstance_*` 的 `EquippedAnimSet` 直接引用。当前 Layer 内容分别从 Shotgun 或 Rifle 基线复制，因此视觉暂时无差异，但后续替换专用待机、移动、瞄准或持枪姿势时不再影响 Rifle/Shotgun 主线。每个新资产还写入 `NewWorldOrder.AnimationLayerIntent` 元数据，明确它是特殊武器后续动画入口。

特殊武器 Layer 解决角色基础姿势隔离；每把武器网格上的机械动画仍由各自 `ABP_*` 承担，角色 Fire/Reload 仍由 `ID_*` 的 `RangedWeaponConfig.CharacterMontages` 按男女骨架独立选择。仅替换火箭筒换弹动作时，修改它已经独立存在的武器 AnimBP 或角色 Reload Montage 即可，不需要增加 C++ 枚举。

## GameplayCue 与 GCN

项目资产注册表中已经存在武器 GCN。特殊武器包括：

- `GCN_Weapon_Rocket_Fire`
- `GCN_Weapon_Rocket_Detonate`
- `GCN_Weapon_GrenadeLauncher_Fire`
- `GCN_Weapon_GrenadeLauncher_Detonate`
- `GCN_Weapon_Grenade_Detonate`

特殊 Fire GCN 当前负责 Camera Shake 与 Force Feedback。`UShootGameplayCueNotify_WeaponFire::OnExecute` 对投射物武器跳过通用 `B_WeaponFire` 的 Shell Eject、Muzzle Flash、Tracer Niagara 图表，以避免空引用和重复表现。

因此：

- 火箭和榴弹的薄 GA 子类只负责选择各自 Fire GameplayCueTag，仍有明确语义，不是无用重复。
- 特殊投射物由服务端权威生成并复制；模拟代理不会运行 GameplayAbility，其尾迹和爆炸表现必须依靠复制 Actor、组件和 GameplayCue 可见。
- 不应重新让特殊武器进入通用步枪 `B_WeaponFire` Niagara Actor。

## 七把武器开火音频收敛

- Pistol 与基础 Shotgun 的 GCN 使用 `BurstEffects` 每发生成一次各自 MetaSound；Rifle 的 GCN 使用 `B_Weapon.TriggerFireAudio` 复用常驻 AudioComponent 并逐发发送 Trigger。半自动枪不再误用 Rifle 持久触发链，修复“第一发正常、后续隔多发才响”的结构性原因。
- `Shotgun_A` 共享 Shotgun Fire GA/GCN，因此使用正式 Shotgun MetaSound，不新建第二条声音链。
- 商城动画遗留的 `RifleA_Fire_Cue`、`PistolA_Fire_Cue`、`ShotgunA_Fire_Cue` 已从对应 Fire AnimSequence 移除，避免与 GCN 正式枪声叠加。枪口粒子与 `Shotgun_Reload_Cue` 保持不动。
- Sniper、Rocket Launcher、Grenade Launcher 继续使用各自动画的专用 Fire Sound Notify；其 Fire GCN 不配置通用枪声。Rocket/Grenade 的爆炸声仍归 Detonate Cue，不把范围爆炸表现塞进发射声入口。
- `/Game/System/Audio/WeaponAudioFunctions` 保留为已迁入的参考辅助库，但当前正式枪声链不从 C++ 调用它，也不迁入或替换项目 GameInstance。`/Game/Blueprints/System/BP_ShootGameInstance` 继续继承项目 `UShootGameInstance`；大厅选择音频由该项目蓝图自己保存。
- 曾有一版 C++ 通用反射适配器枚举 BlueprintFunctionLibrary 的 UFunction 参数并调用 `ProcessEvent`；Shotgun PIE 在 Blueprint reinstance 后于参数遍历处空指针崩溃。该危险适配器已删除。`EarlyReflections/WhizBy` 仍查询 Player 0，未来若恢复必须改成 GCN 类型安全节点或显式 Listener 的原生 API，不能恢复通用反射调用。
- 资产配置、七把武器矩阵和新增武器接入规则见 `WeaponFireAudio_开火音频接入规范.md`。当前已完成资产/CDO/编译级验证；四种运行形态的最终听感需要玩家耳听，不能由日志检查冒充完成。

## ADS 与武器开火表现验收

### ADS 退出与相机端点

- 根因不是 `WaitInputRelease` 未收到松开，而是 `UShootGA_Weapon_Aim::EndAbility` 在 `Super::EndAbility` 之前注册退出 Timer；父类结束 Ability 时清理了该异步状态，导致 Scope 能关闭，但 SpringArm/FOV 永久停在 ADS 端点。
- 当前顺序为：先清理瞄准标签、移动状态与 Scope 消息，调用 `Super::EndAbility` 完成 GAS 生命周期，再从本次激活缓存的 Character/Weapon 启动反向过渡。
- 普通 Rifle、Pistol、Shotgun、Shotgun_A、Grenade Launcher、Rocket Launcher 的 ADS 端点为 FOV 70、SpringArm 150；Sniper 的 Scope 端点为 FOV 30、SpringArm 75。退出始终回到激活时捕获的原始 FOV、SpringArm、TargetOffset 与 SocketOffset，不硬编码默认端点。
- 玩家已验收长按右键、松开右键和相机恢复；修复同时覆盖快速点击、Ability 取消与切枪结束时的同一退出路径。
- 对照 Lyra 的 `GA_ADS` 与 Reload 实现，Lyra 没有要求有弹药才允许 ADS，Reload 也不会强制取消 ADS；项目当前保持这两个行为，不额外制造隐式耦合。

### Sniper 枪口与 Tracer 配置边界

- `Tracer` 是命中扫描武器的曳光弹视觉：伤害仍由瞬时射线判定，Niagara 使用枪口和 `ImpactPositions` 表现可见弹道，不承担伤害或实体投射物移动。
- `B_Weapon` 父蓝图现有 `Enable Tracer FX` 配置，默认开启，并把该值传给 `B_WeaponFire.Enable Tracer`。Sniper 关闭该值后，通用 Niagara Tracer 确实没有生成；玩家仍看到的持续亮光柱来自 `AM_Sniper_Rifle_A_Fire_W` 自身的 `P_SniperRifle_Tracer_01` Cascade Notify，不能再把两条入口混为一谈。
- 审计确认底层 `Fire_SniperRifle_W` Sequence 原有一次 Fire Sound 和误用的 `P_AssaultRifle_MuzzleFlash`，其 Montage 又在同一时间复制了一次 Fire Sound、添加 `P_SniperRifle_MuzzleFlash_01` 和 `P_SniperRifle_Tracer_01`。UE 的 Montage Tick 会分别收集 Montage Notify Track 和 Slot Segment 内 Sequence 的 Notify，因此这些事件会叠加执行，不是编辑器列表的显示重复。
- 当前归一为单一动画入口：Montage 不再保存直接 Notify；底层 Sequence 只保留一次 Fire Sound 和一次 `P_SniperRifle_MuzzleFlash_01`。独立 Cascade Tracer 已移除，错误的 Assault Rifle 枪口火焰已替换为 Sniper 专用资产；没有删除 Sniper 枪口火焰。
- `B_Weapon` 新增 `Enable Muzzle Flash FX`，默认开启并传给 `B_WeaponFire.Enable Muzzle Flash`。Rifle、Pistol、Shotgun、Shotgun_A 继续继承开启；Sniper 关闭该值，避免通用 Niagara 枪口火焰与动画专用 Cascade 再叠一层。EventGraph 中文注解同时说明 Muzzle 与 Tracer 的职责边界。
- Rifle、Pistol、Shotgun、Shotgun_A 显式开启；Sniper 显式关闭；Rocket Launcher 与 Grenade Launcher 显式关闭。火箭和榴弹由真实 Projectile Actor、尾迹与专用 GameplayCue 表现，不使用命中扫描 Tracer。
- `UShootAnimNotify_PlayScopedMuzzleFlash` 继承 `UAnimNotify_PlayParticleEffect`，仍生成同一份 Sniper Cascade。若武器 Owner Pawn 的 ASC 当前持有 `Event.Movement.ADS`，它只把生成的粒子组件设为 `Owner No See`；开镜玩家自己的全屏镜视图不再渲染枪口烟雾，另一个本地分屏玩家、远端玩家和模拟代理仍可看到。不能用“本地直接不生成粒子”替代，否则同一进程的其他分屏视图也会丢失表现。
- 该清理不修改 Sniper 已验收的 FOV 30 / SpringArm 75 端点。扩大 FOV 只会改变放大倍率，不能修复重复特效或按视图可见性。
- 玩家已完成最终视觉验收：开镜拥有者不见枪口烟雾，其他玩家仍能看到枪口火焰；独立 Cascade Tracer 移除后，贴在枪口前的持续光柱不再出现。
- `Owner No See` 只解决同一粒子组件对不同观察视图的可见性，不等于已经实现第一/第三人称切换。项目级 CameraMode、第一人称相机和角色 Mesh 可见性仍是独立后续任务。

## AutoReload 实现

### 当前实现

- 新增共享 `UShootGA_Weapon_AutoReload`，使用 OnSpawn 本地被动轮询。
- Rifle、Pistol、Shotgun、Shotgun_A、Sniper、Grenade Launcher、Rocket Launcher 的 AbilitySet 均已授予 AutoReload。
- GameplayTag 已存在 `Ability.Type.Passive.AutoReload`。
- 项目 `UShootGameplayAbility` 已支持 `ActivationPolicy = OnSpawn`，ASC 也会在 ActorInfo 完成后重试 OnSpawn Ability。
- 迁移笔记记录了 Lyra Rifle、Pistol、Shotgun AbilitySet 均包含 AutoReload；项目早期有意延后该能力。

### 已落实的实现边界

- SourceObject 必须是当前 WeaponInstance。
- 当前轮询 WeaponInstance 预测弹药；弹匣为空、备用弹药大于零且超过交互延迟时，请求激活同一 SourceObject 的 Reload Ability。
- 不伪造键盘输入，不为不同输入设备写硬编码分支。
- 服务端最终负责换弹权威；不能依赖 Simulated Proxy 执行 GA。
- 正式换弹能力通过 `UShootGameplayAbility_ReloadMagazine` 或 `UShootGA_Reload_ShotgunPerShell` 类型识别，不依赖原生 CDO 初始化过早而可能为空的 AssetTag；同时要求 Reload Spec 与 AutoReload 使用同一 `UShootRangedWeaponInstance` SourceObject。
- `UShootGA_Reload_ShotgunPerShell` 继续独占 InsertShell、循环、结束和非空弹匣开火打断时序；AutoReload 只在空仓且有备弹时激活该正式 Spec。若同一 Spec 已运行，轮询直接视为已处理，不重复发送激活请求。
- Equipment 卸载会由 `FShootAbilitySet_GrantedHandles::TakeFromAbilitySystem` 对旧装备授予的每个 Handle 调用 `ClearAbility`，因此切枪会移除旧 WeaponInstance 的 AutoReload 和 Reload Spec，不留下继续触发旧武器换弹的被动实例。
- Listen Server Host 运行时已验证 `0/24 -> 8/16`，证明空仓会自动进入 Shotgun_A 的完整逐发装填；`0/0` 连续观察 3 秒保持 Reload 未激活且弹药不变。该结论不替代远端拥有客户端复制验收。

## 特殊投射物代码与字段审计

### 应保留

- `ResolveFireDirectionFromTargetData`：承担服务端从客户端 TargetData 解析目标点、范围限制和方向校验，不能用单纯枪口 ForwardVector 替代。
- 火箭和榴弹的薄 Fire GA 子类：选择不同 GameplayCueTag。
- `bFavorHighArc`：榴弹弹道求解的可配置策略，当前虽为 false，但存在合法变化维度。
- `bRotationFollowsVelocity`：投射物姿态规则，当前两种武器都使用。
- `bApplyMaterialMultipliers`：伤害规则开关，当前虽为 false，但有实际消费者。
- Rocket 子类强制关闭反弹：表达火箭语义不变量，可抵御资产误配置。
- `TrailSystem` 与 `TrailParticleSystem`：在 Niagara 迁移完成前分别承载 Niagara 与 Cascade 兼容路径。

### 已完成的安全精简

- `ProjectileTargetResolved` 是方向故障期间遗留的诊断日志；最终联机回归通过后已从普通 `Log` 降为 `Verbose`，保留低成本排障信息但不再污染默认输出。
- 其他投射物移动诊断已经是 `Verbose`，可保留作为低成本排障信息。

不建议继续压缩 `UShootGA_Weapon_Fire_Projectile` 的权威校验、目标数据和弹道分支。它们分别覆盖网络安全、火箭直射与榴弹抛射，不属于同义重复。

## Cascade 到 Niagara 决策

### 审计事实

- 当前审计发现 25 个 Cascade 资产，其中 21 个有三个 LOD。
- 自动转换器主要覆盖 LOD0，Beam、AnimTrail、Ribbon 等模块存在不支持或部分支持。
- 六个 Cascade 仍由动画或运行时配置引用；其余资产需要进一步确认是否只是未被引用的库存内容。
- 火箭前喷口、后喷口和榴弹枪口位于不同 Socket 或 Notify 时间，不能用步枪通用 Muzzle Flash 替换。
- 当前没有经过视觉验收的 1:1 Niagara 等价资产。

### 决策

- 长期架构采用 Niagara-first。
- 当前不进行盲目批量转换，也不保留永久的新旧双链路。
- 火箭与榴弹应在一个独立任务中完成原子迁移：新 Niagara 资产、Notify 替换、尾迹切换、联机视觉回归、依赖归零、删除旧 Cascade、删除 Cascade 兼容字段和组件。

### 原子迁移验收

1. 火箭前后喷口的 Socket、时序、方向与原表现一致。
2. 火箭和榴弹尾迹在拥有客户端、服务端和 Simulated Proxy 上均只出现一次。
3. 榴弹反弹、引信和爆炸不因 VFX 迁移改变。
4. 近景、远景和最低目标画质完成视觉与性能对比。
5. AssetRegistry 确认旧 Cascade 无运行时引用后删除资产。
6. 删除 `TrailParticleSystem`、`UParticleSystemComponent` 等过渡代码，不给后续维护者留下两套入口。

## 收尾任务与优先级

### P0 正确性与联机闭环（已完成）

1. 已完成最终 Listen Server 双端回归：远端拥有客户端通过真实输入开火，服务器生成唯一权威投射物；Host、拥有客户端和模拟代理看到同一投射物、尾迹和爆炸，且不重复播放。
2. 已确认 Rocket/Grenade 对目标 ASC 的伤害由服务器权威结算；客户端 TargetData 只在距离与角度校验后用于解析发射方向。
3. 已回归 Rifle、Pistol、Shotgun 的通用 Niagara Shell Eject、Muzzle Flash、Tracer、Fire GameplayCue 和弹药消耗，特殊武器分支没有影响 Lyra 主线。
4. 已把最终状态回写 `STATUS.md` 与特殊武器实施文档；较早的“待 PIE 验证”描述已标记为历史验证边界。

### P1 当前正确性收尾

1. 已完成 ADS 玩家视角验收：长按进入、松开退出、Scope 关闭和相机原始端点恢复均正常；快速点击、连续点击和切枪取消共用同一反向过渡路径。
2. 已修复 Sniper 的 `NS_Tracer`、`NS_MuzzleFlash`、`NS_ShellEject` 空引用；进一步确认持续光柱来自动画 Cascade Tracer，而不是已关闭的通用 Niagara Tracer，并完成通知归一、专用枪口火焰保留和 ADS Owner 视图烟雾过滤。冷编译、重启反射、蓝图编译、资产复读及玩家开镜/其他玩家视觉验收均通过。
3. Shotgun_A 的“动作播放但弹药不增加”已修复：删除错误的 `WaitInputRelease`，Listen Server Host 已验证单发、连续逐发及非空弹匣开火打断；共享 AutoReload 也已接入并验证空仓有备弹时正常激活，零备弹不会误触发。切枪移除旧被动已由 Equipment 授予句柄清理调用链确认；玩家随后完成远端拥有客户端的逐发复制、Montage、开火打断和模拟代理表现验收。
4. Sniper 已完成远端玩家 ADS、Scope、开火、退出、其他玩家枪口表现及分屏/远端 Owner 可见性回归。

### P2 高置信度冗余清理

1. 已删除 `UShootEquipmentDefinition_Weapon.EquipMontage`、`UnequipMontage` 和已确认无引用的旧 AnimInstance 类。
2. 已把 `ProjectileTargetResolved` 从普通 `Log` 降为 `Verbose`。
3. 已确认 `UShootEquipmentDefinition_Weapon` 仍提供默认 `UShootRangedWeaponInstance` 与武器语义边界，保留该类型。
4. 已完成受保护主线兼容 Montage 字段审计：两个无外部 Referencer 的 Assault_Rifle_A 遗留 GA 仍保存非空兜底 Montage，因此字段不是零消费者；本轮不删资产、不改受保护类。
5. 已完成 `EShootWeaponAnimationStyle` 审计：旧枚举与 Getter 已不存在，七把武器使用 Lyra 式动画层数据配置；不恢复枪型分支，特殊武器是否新增独立角色 Layer 留给实际美术姿势需求决定。

### P3 Niagara 原子迁移

1. 为 Rocket 与 Grenade 制作专用 Niagara 枪口、前后喷口、尾迹和所需爆炸等价资产。
2. 完成动画 Notify、Projectile Fragment、GCN 和联机表现替换。
3. 依赖归零后删除旧 Cascade 与过渡组件，不长期保留两套实现。

### P4 独立的非阻断事项

- 弹药 StatTag 单一数据源与无消费者接口精修已完成，以当前七个 `ID_*` CDO 实际值为准，没有恢复较早的 Rifle `5/15` 历史值。
- Shotgun_A 的功能与联机闭环已完成；动画混合抛光由玩家另行交给动画 AI，不属于本收尾任务。
- 项目级 CameraMode 栈与第一人称基础纵切已落地为双 `UCameraComponent`：`AShootCharacter.FollowCamera` 继续附加到 `CameraBoom` 并只保存第三人称参数，`AShootCharacter.FirstPersonCamera` 附加到角色 `CharacterMesh0.head` 并独立保存第一人称 FOV、Transform、PostProcess 与未来专属渲染规则。模式栈仍是唯一的视角状态裁决者；进入第一人称时仅对拥有者隐藏独立 Head Mesh，切回第三人称时恢复可见。详细边界与验收状态以 `Docs/Tasks/GameFrameworkMigration/CameraMode_FirstPerson_项目相机模式栈与第一人称纵切.md` 为准。
- 武器拾取 `WorldLabel` 是否保留由产品视觉决定，不按冗余代码处理。
- Redirector 修复属于资产治理任务，不与武器运行逻辑混在同一提交。

## 禁止直接执行的清理

- 不直接删除 SetStats 中的当前弹药值。
- 弹药容量迁移期间不提交“旧 Fragment 字段已删但 `ID_*` 容量 Tag 尚未写入”的中间状态。
- 不合并 `B_WeaponInstance_*` 与 `BP_WeaponPickup_*`。
- 不把火箭和榴弹重新接入通用步枪 `B_WeaponFire` Niagara 图表。
- 不为 AutoReload 伪造键盘或手柄输入。
- 不修改受保护的 Rifle、Pistol、Shotgun 射击与换弹主线，除非后续证据证明必须修改并先向用户说明完整影响面。
- 不在 Niagara 等价性和依赖归零前删除仍被引用的 Cascade 资产。
