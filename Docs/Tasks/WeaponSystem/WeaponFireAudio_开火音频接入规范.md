# 武器开火音频接入规范

## 目标

- 每次权威开火只产生一条枪声入口，避免 GCN 与动画 Notify 重复播放。
- 连续自动武器与单发武器分别使用适合自身触发语义的音频链。
- 新增武器只需配置数据和资产，不按 WeaponId 在 C++ 增加分支。
- 保留 Standalone、Listen Server、模拟代理和 SplitScreen 的逐视图、逐听者验收入口。

## 当前调用链

1. Fire GA 发送对应的 `GameplayCue.Weapon.*.Fire`。
2. `UShootGameplayCueNotify_WeaponFire` 解析本次 Cue 的 `UShootWeaponInstance`，只从该武器的 Equipment SpawnedActor 找表现 Actor。
3. 命中扫描武器继续调用 `/Game/Weapons/B_Weapon.Fire` 生成枪口、弹壳、Tracer、Impact 与 Decal。
4. 投射物武器在通用 `B_Weapon.Fire` 之前退出；枪口和发射声由各自武器动画 Notify 负责，Projectile Actor 负责飞行与爆炸表现。
5. `/Game/System/Audio/WeaponAudioFunctions` 是已迁入但当前不由 C++ 运行时调用的参考辅助库。项目不得为它迁入 `B_LyraGameInstance`；大厅音频继续由继承 `UShootGameInstance` 的 `/Game/Blueprints/System/BP_ShootGameInstance` 管理。

## 七把武器配置矩阵

| 武器 | Fire Cue | 枪声唯一入口 | GCN 配置 | 动画 Fire Sound Notify |
|---|---|---|---|---|
| Pistol | `GameplayCue.Weapon.Pistol.Fire` | GCN 单发 MetaSound | `BurstSounds=MSS_Weapons_Pistol_Fire`，`bUsePersistentFireAudio=false` | 不得再承担正式枪声 |
| Rifle | `GameplayCue.Weapon.Rifle.Fire` | GCN 持久 AudioComponent Trigger | `FireSound=MSS_Weapons_Rifle_Fire`，`bUsePersistentFireAudio=true` | 不得再承担正式枪声 |
| Shotgun | `GameplayCue.Weapon.Shotgun.Fire` | GCN 单发 MetaSound | `BurstSounds=MSS_Weapons_Shotgun_Fire`，`bUsePersistentFireAudio=false` | 不得再承担正式枪声 |
| Shotgun_A | 共享 Shotgun Fire Cue | 共享 Shotgun GCN 单发 MetaSound | 不创建第二套 Shotgun_A GCN | 武器机械动画可保留，正式枪声不得重复 |
| Sniper | `GameplayCue.Weapon.Sniper.Fire` | `Fire_SniperRifle_W` 专用 Sound Notify | 不配置 `FireSound`、`BurstSounds` 或持久触发 | `SniperRifleA_Fire_Cue` |
| Rocket Launcher | `GameplayCue.Weapon.Rocket.Fire` | `Fire_RocketLauncher_W` 专用 Sound Notify | 只保留 Camera Shake/Force Feedback | `RocketLauncherA_Fire_Cue` |
| Grenade Launcher | `GameplayCue.Weapon.GrenadeLauncher.Fire` | `Fire_GrenadeLauncher_W` 专用 Sound Notify | 只保留 Camera Shake/Force Feedback | `GrenadeLauncherA_Fire_Cue` |

Pistol、Rifle、Shotgun_A 的商城武器动画原本也带 Fire Sound Notify，现已只移除 `RifleA_Fire_Cue`、`PistolA_Fire_Cue`、`ShotgunA_Fire_Cue` 三条旧正式枪声。枪口粒子和 `Shotgun_Reload_Cue` 仍保留；Sniper、Rocket、Grenade Launcher 的专用 Fire Sound Notify 也保持不动。该边界仍需以 PIE 的声音调试列表和实际听感共同验收，不能仅凭资产名称判断。

`WeaponAudioFunctions.EarlyReflections/WhizBy` 仍沿用 Lyra 的 Player 0 查询，只能表达单一本地听者。曾经用 C++ 通用反射枚举蓝图函数参数并 `ProcessEvent`，在 Blueprint reinstance 后于 Shotgun Cue 触发空指针崩溃，因此该危险适配器已删除。当前基础枪声不依赖这些辅助函数。未来若恢复，必须在 GCN 蓝图使用类型安全节点，或建立项目原生、显式接收 LocalPlayer/Listener 的 API；不得恢复通用反射调用，也不能继续依赖 Player 0。

## 新增武器决策

### 自动武器

- 如果 MetaSound 设计为常驻实例并通过 Trigger Parameter 接收每发事件，配置 `bUsePersistentFireAudio=true` 和 `FireSound`。
- `BurstSounds` 保持为空。
- 动画中不再放正式枪声 Notify。

### 半自动或单发命中扫描武器

- 默认使用 GCN `BurstSounds` 每发生成一次 MetaSound。
- `bUsePersistentFireAudio=false`，`FireSound=None`。
- 动画中不再放正式枪声 Notify。
- 若美术资产必须保留自己的专用 Sound Cue，则反过来保持 GCN 无声音；两条路径只能选一条。

### 投射物或特殊机械武器

- 可以保留专用动画 Sound Notify，但对应 GCN 必须保持无正式枪声。
- 投射物 Fire GCN 不进入通用命中扫描 `B_Weapon.Fire`，避免重复枪口、Tracer 和空 Niagara 引用。
- 爆炸声音属于 Detonate Cue，不与发射声合并。

## 为什么不能把所有武器都接入 Rifle Trigger

`B_Weapon.TriggerFireAudio` 会缓存一个 AudioComponent：第一次调用生成声音组件，后续只执行 Trigger Parameter。这只适用于为持续触发设计的 MetaSound。把一次性 Pistol 或 Shotgun MetaSound 放入该链，会出现第一发正常、后续 Trigger 没有重新启动声音，听起来像漏声或隔几发才响。

## Concrete 脚步 MetaSound 注册键故障

- 日志中的 `Could not find graph with registry graph key ... sfx_Character_FS_Concrete_nl_meta` 与武器 Fire GCN 无关；它由 `/Game/ContextEffects/CFX_DefaultSkin` 在角色或 AI 脚步触发时调用。
- 旧结论“只重存 `sfx_Character_FS_Concrete_nl_meta` 即可修复”不完整。真正的失败链是 `Concrete/Land/Glass -> sfx_Character_FS_Base_nl_meta -> MS_GatedWavePlayer`：Patch 重存后资产注册类 ID 已变为 `95ED24A94DD3C6D61290EC967C532443`，但 `FS_Base` 仍保存旧依赖 ID `92DD0EAA49EA051A42CC42ABBF16F94A`。因此引擎先无法构建 `FS_Base`，随后所有引用它的脚步 MetaSound 都找不到有效 registry graph，并最终在引擎的无效图防御检查处触发 `Graph.IsValid()` ensure。
- 本轮通过 Unreal 编辑器工具只修正 `FS_Base` 文档中的一条旧 Patch 依赖记录，并按 `MS_GatedWavePlayer -> FS_Base -> Concrete/Land/Glass` 顺序重新注册、验证；没有修改引擎源码，也没有删除或静音脚步资产。
- 若以后迁移或升级 MetaSound 后再次出现同类错误，必须从日志中最早的 `node class ... does not exist in registry` 向上游追踪 Patch/公共 Base，核对引用节点类 ID 与被引用资产的 `AssetClassID`。修复后按依赖拓扑顺序重存和注册，再启动实际 PIE 验证；不要只重存最终报错的叶子资产，也不要通过删除脚步 Notify、静音 Context Effects 或替换为任意 SoundWave 掩盖注册问题。

## 验收清单

- Standalone：每次 Pistol、Rifle、Shotgun、Shotgun_A 开火均有一次且只有一次对应枪声。
- Listen Server：Host 与远端 Owner 都能听到自己的每发枪声；旁观者听到空间化远端枪声，不出现双声。
- 模拟代理：其他玩家的枪声跟随正确 Pawn/武器位置，不绑定到 Player 0。
- SplitScreen：两个 LocalPlayer 的枪声和听者参数互不串线。
- 特殊武器：Sniper、Rocket、Grenade Launcher 的发射声各自匹配武器；Rocket/Grenade 爆炸声只在 Detonate 链出现。
- 连续射击：Rifle 至少连续打完整个弹匣，逐发触发稳定；Pistol 连续点击与 Shotgun 连续射击不得出现“第一发有声、后续隔多发才有声”。
- 打开声音调试列表核对同一发没有同时出现正式 MetaSound 与旧商城 Fire Cue；最终听感仍由玩家耳听验收。

## 本轮验证状态

- 项目冷编译通过，六个 Fire GCN 蓝图均重新编译为 `BS_UP_TO_DATE`。
- 删除通用反射适配器后，在 `TestMap_ListenServer` 对 Shotgun 连续注入 6 次开火输入，编辑器保持存活，原 `ExecuteWeaponAudioFunction` 空指针崩溃未复现。
- `MS_GatedWavePlayer` 与 `FS_Base` 的类 ID 重新对齐后，在 `TestMap_ListenServer` 实际运行约两分钟：本次运行新增日志中 `Graph.IsValid`、旧 `92DD0EAA`、`Could not find graph with registry graph key`、`Failed to build MetaSound` 和 `Cannot create node` 均为 0。
- 上述是技术链路验证，不代替玩家耳听。Pistol、Rifle、Shotgun、Shotgun_A 的连续听感，以及七把武器在 Standalone、Listen Server、模拟代理和 SplitScreen 下的空间化结果仍需人工逐枪验收。

## 维护约束

- 不从 NiagaraExamples 等孤立样例目录随意借用名称相似的 Load、Fire 或机械声音。
- 不为新增武器迁入 Lyra GameInstance。项目已有 `UShootGameInstance` 与 `BP_ShootGameInstance`。
- 不在 GA、WeaponInstance 或 WeaponId 分支中硬编码声音资产路径。
- GCN CDO、武器 AnimSequence Notify 和 Detonate Cue 是检查一把新武器时必须同时审计的三个入口。
