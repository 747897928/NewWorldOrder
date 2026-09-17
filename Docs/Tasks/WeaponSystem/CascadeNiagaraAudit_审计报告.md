# 商城武器 Cascade 与 Niagara 第二阶段审计

状态：审计完成；转换 0 项；删除 0 项；Fix Up Redirectors 仅完成安全评估

日期：2026-08-24

## 1. 审计范围与结论

本阶段从提交 `273e4db1eae09a3ceada8dd4132d9eee35450578` 继续，不重复第一阶段的 650 个真实资产迁移，也不实现逐发装填霰弹枪、狙击枪、榴弹发射器或火箭筒的完整能力链。

结论：

- UE 5.8.1 仍包含 Cascade Editor、`UParticleSystem` 和 Cascade 运行时组件。Epic 将 Cascade 称为 legacy，并说明未来计划弃用和移除，但当前官方 5.8 文档没有证明它已经弃用或不能继续运行。
- Epic 的 Cascade To Niagara Converter 是 Beta、默认关闭的编辑器插件。官方将转换结果定义为升级起点，要求逐项查看 Niagara Log、错误和警告；它不是视觉与行为等价证明。
- 25 个商城 Cascade 中有 21 个配置了 3 个 LOD 距离，而官方转换器只处理 LOD 0；自动转换会丢失 LOD 1、LOD 2 的行为信息。
- `P_Knife_RibbonTrail_01` 虽然只有 1 个 LOD，但 AnimTrail/Beam Renderer 属于官方明确不支持的转换类型。
- 其余 3 个单 LOD tracer 没有真实运行时引用，也没有与现有 Niagara 候选的逐帧、材质、时序和性能等价证据。
- 因此本阶段可证明等价的转换项为 0，建议转换项为 0。25 个 Cascade 全部保留；19 个无真实引用项只标记为未来武器接入或人工清理候选，不删除。

## 2. Epic 官方资料结论

### 2.1 产品状态

- [Niagara Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-niagara-effects-for-unreal-engine) 将 Niagara 定义为 Unreal Engine 的 next-generation VFX system，核心由 System、Emitter、Module 和 Parameter 构成。
- [UE 5.8 Cascade API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Cascade) 仍列出 Cascade Editor 插件、模块和类。
- [UE 5.8 Animation Notifies](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine) 同时支持 `Play Particle Effect` 的 legacy Cascade 和 `Play Niagara Particle Effect`；二者是不同 Notify 类型。
- [UE 5.8 Cascade To Niagara Converter](https://dev.epicgames.com/documentation/unreal-engine/cascade-to-niagara-effects-converter-plugin-for-unreal-engine) 写明该工具会持续更新，直到 Cascade 在未来版本被弃用和移除。该表述证明存在未来淘汰路线，但不能改写成“UE 5.8 已弃用 Cascade”。

### 2.2 转换器边界

本机 UE 5.8.1 的 `CascadeToNiagaraConverter.uplugin` 为 `IsBetaVersion=true`、`EnabledByDefault=false`。项目 `NewWorldOrder.uproject` 没有显式启用该插件，本阶段没有修改插件状态。

Epic 官方列出的关键限制：

- Event Generator、EventReceiver、Particle Attractor、Source Movement、Emitter Initial/Direct Location、Seeded Modules 不支持。
- Beam 与 AnimTrail Renderer 不支持。
- Ribbon Renderer 只部分支持，Ribbon UV 不保证等价。
- Cascade Emitter LOD 只转换 LOD 0。
- 转换后必须打开 Niagara Editor 检查 Niagara Log、依赖错误、跳过模块和警告。

### 2.3 性能判断

[Niagara Scalability and Best Practices](https://dev.epicgames.com/documentation/unreal-engine/scalability-and-best-practices-for-niagara) 与 [Measuring Performance in Niagara](https://dev.epicgames.com/documentation/unreal-engine/measuring-performance-in-niagara) 不支持“迁到 Niagara 就一定更快”的结论：

- Niagara 提供 Effect Type、按平台 Scalability、Emitter/System/Renderer 覆盖、Pooling 和更灵活的 CPU/GPU Simulation Target。
- 每个 Niagara System Instance 和 Emitter 仍有 CPU 开销；GPU 与 CPU 哪个更快取决于粒子数量、平台和当前 CPU/GPU 瓶颈。
- 性能结论必须来自代表性玩法中的 Unreal Insights、Niagara Debugger、过绘和目标硬件测量。

因此 Niagara 的长期维护、可扩展性和可缩放能力更强，但本项目不能以性能猜测替代转换前后的实测。

## 3. 重启后的重定向器复核

NewWorldOrder 编辑器在审计前单独重启；Lyra 编辑器没有关闭。重启后的 AssetRegistry 已完成同步搜索，`is_loading_assets=false`。

| 路径 | 真实资产 | ObjectRedirector |
| --- | ---: | ---: |
| `/Game/Assets/MilitaryWeapSilver` | 0 | 432 |
| `/Game/Assets/FPS_Weapon_Bundle` | 0 | 221 |
| 合计 | 0 | 653 |

引用结果：

- 91 个源重定向器仍有源目录外引用。
- 这些引用来自 56 个唯一包：53 个位于迁移后的 `/Game/Weapons`，3 个是项目已有包：
  - `/Game/Blueprints/Weapons/Assault_Rifle_A/ABP_Assault_Rifle_A`
  - `/Game/Blueprints/Weapons/Assault_Rifle_A/AM_Fire_Rifle_W_Montage`
  - `/Game/Blueprints/Weapons/Assault_Rifle_A/AM_Reload_Rifle_Ironsights_W_Montage`
- 典型情况是目标 Cascade、材质和网格仍通过源重定向器解析已迁移的材质、纹理、Skeleton、SoundCue 或 Mesh。

Fix Up Redirectors 的预期影响是重存至少上述 56 个引用包，并删除一批或全部 653 个重定向器。项目安全规则要求多个文件或非空目录的删除由用户人工处理，因此本阶段不自动执行 Fix Up，也不调用批量删除。

用户在 Content Browser 人工执行 Fix Up 后，必须重新验证：

1. 两个源根目录真实资产和重定向器均为 0。
2. `/Game/Weapons` 资产的依赖中不再出现两个源根目录。
3. 三个项目既有 Assault Rifle 包直接依赖新路径。
4. 下节 6 条动画的 Notify 名称、类型、触发时间和声音闭包不变。
5. Git 状态没有卷入用户原有纹理、Python 缓存或无关资产。

## 4. 25 个 Cascade 逐项审计

表中“真实引用”排除了资产自身对应的源重定向器。Niagara 候选只表示用途相似，不表示一对一等价。

| Cascade | 真实引用与时序 | LOD | Niagara 语义候选 | 结论 |
| --- | --- | --- | --- | --- |
| `P_AssaultRifle_MuzzleFlash` | `Fire_Rifle_W` 0.029011s；`Fire_SniperRifle_W` 0.027763s | 3 | `NS_WeaponFire_MuzzleFlash_Rifle`、`NS_MuzzleFlash` | 保留；多 LOD，且一项被两种武器复用 |
| `P_AssaultRifle_Tracer_01` | 无 | 3 | `NS_WeaponFire_Tracer`、`NS_BulletTracer` | 保留；未来枪械候选，无等价证据 |
| `P_Grenade_Explosion_01` | 无 | 3 | `NS_Grenade_Explosion`、`NS_Explosion` | 保留；名称相似不证明材质、爆炸节奏等价 |
| `P_Grenade_MuzzleFlash_01` | `Fire_GrenadeLauncher_W` 0.014037s | 3 | `NS_MuzzleFlash` | 保留；动画、粒子和声音必须一起迁移 |
| `P_Grenade_Trail_01` | 无 | 3 | `NS_Grenade_Trail`、`NS_RocketTrail` | 保留；未来投射物候选，无等价证据 |
| `P_Impact_Metal_Large_01` | 无 | 3 | `NS_Impact_Metal` | 保留；尺寸和粒子量级未验证 |
| `P_Impact_Metal_Medium_01` | 无 | 3 | `NS_Impact_Metal` | 保留；尺寸和粒子量级未验证 |
| `P_Impact_Metal_Small_01` | 无 | 3 | `NS_Impact_Metal` | 保留；尺寸和粒子量级未验证 |
| `P_Impact_Stone_Large_01` | 无 | 3 | `NS_ImpactConcrete`、`NS_Impact_Concrete` | 保留；Stone 与 Concrete 不能直接视为同材质表现 |
| `P_Impact_Stone_Medium_01` | 无 | 3 | `NS_ImpactConcrete`、`NS_Impact_Concrete` | 保留；Stone 与 Concrete 不能直接视为同材质表现 |
| `P_Impact_Stone_Small_01` | 无 | 3 | `NS_ImpactConcrete`、`NS_Impact_Concrete` | 保留；Stone 与 Concrete 不能直接视为同材质表现 |
| `P_Impact_Wood_Large_01` | 无 | 3 | `NS_Impact_Wood` | 保留；尺寸和木屑材质未验证 |
| `P_Impact_Wood_Medium_01` | 无 | 3 | `NS_Impact_Wood` | 保留；尺寸和木屑材质未验证 |
| `P_Impact_Wood_Small_01` | 无 | 3 | `NS_Impact_Wood` | 保留；尺寸和木屑材质未验证 |
| `P_Knife_RibbonTrail_01` | 无 | 1 | `NS_SimpleRibbonTrail` | 保留；AnimTrail/Beam 转换不受支持 |
| `P_Pistol_MuzzleFlash_01` | `Fire_Pistol_W` 0.009046s | 3 | `NS_MuzzleFlash` | 保留；多 LOD且动画时间轴已在用 |
| `P_Pistol_Tracer_01` | 无 | 1 | `NS_WeaponFire_Tracer`、`NS_BulletTracer` | 保留；无逐帧或性能等价证据 |
| `P_RocketLauncher_Explosion_01` | 无 | 3 | `NS_Explosion` | 保留；无 Rocket 专属一对一系统 |
| `P_RocketLauncher_MuzzleFlash_Front_01` | `Fire_RocketLauncher_W` 0.014262s | 3 | `NS_MuzzleFlash` | 保留；必须保留前后喷口的独立时序和 Socket |
| `P_RocketLauncher_MuzzleFlash_Rear_01` | `Fire_RocketLauncher_W` 0.005850s | 3 | `NS_MuzzleFlash` | 保留；必须保留前后喷口的独立时序和 Socket |
| `P_RocketLauncher_Trail_01` | 无 | 3 | `NS_RocketTrail` | 保留；未来投射物候选，无等价证据 |
| `P_Shotgun_MuzzleFlash_01` | `Fire_Shotgun_W` 0.162210s | 3 | `NS_MuzzleFlash` | 保留；多 LOD且动画时间轴已在用 |
| `P_Shotgun_Tracer_01` | 无 | 1 | `NS_WeaponFire_Tracer_Shotgun`、`NS_BulletTracer` | 保留；无逐帧或性能等价证据 |
| `P_SniperRifle_MuzzleFlash_01` | 无；当前 Sniper 动画使用 Assault Rifle 版本 | 3 | `NS_MuzzleFlash` | 保留；未来 Sniper 接入前先决定复用关系 |
| `P_SniperRifle_Tracer_01` | 无 | 1 | `NS_WeaponFire_Tracer`、`NS_BulletTracer` | 保留；无逐帧或性能等价证据 |

数量汇总：

- 有真实引用：6 个 Cascade，涉及 6 条动画；其中 `P_AssaultRifle_MuzzleFlash` 被两条动画引用。
- 只有源重定向器引用：19 个。
- 3 LOD：21 个。
- 1 LOD：4 个。
- 可证明一对一 Niagara 等价：0 个。
- 本阶段转换：0 个。
- 本阶段删除：0 个。

## 5. 动画、特效与声音 Notify 闭包

6 条开火 AnimSequence 均能加载。粒子 Notify 全部为 `AnimNotify_PlayParticleEffect`，声音全部为 `AnimNotify_PlaySound`；Chance 为 1，Trigger on Server 为 true，Trigger on Follower 为 false。

| 动画 | 声音 Notify | 粒子 Notify |
| --- | --- | --- |
| `Fire_Rifle_W` | `RifleA_Fire_Cue` 0.028989s | `P_AssaultRifle_MuzzleFlash` 0.029011s |
| `Fire_SniperRifle_W` | `SniperRifleA_Fire_Cue` 0.026865s | `P_AssaultRifle_MuzzleFlash` 0.027763s |
| `Fire_GrenadeLauncher_W` | `GrenadeLauncherA_Fire_Cue` 0.014024s | `P_Grenade_MuzzleFlash_01` 0.014037s |
| `Fire_Pistol_W` | `PistolA_Fire_Cue` 0.009045s | `P_Pistol_MuzzleFlash_01` 0.009046s |
| `Fire_RocketLauncher_W` | `RocketLauncherA_Fire_Cue` 0.005833s | Rear 0.005850s；Front 0.014262s |
| `Fire_Shotgun_W` | `ShotgunA_Fire_Cue` 0.162186s；`Shotgun_Reload_Cue` 0.645357s | `P_Shotgun_MuzzleFlash_01` 0.162210s |

Epic 的 Cascade 与 Niagara 粒子 Notify 是不同类。未来若某项通过人工 A/B 验收，不能只把资产引用改成一个 `NS_*`：

1. 将对应 `AnimNotify_PlayParticleEffect` 替换为 `AnimNotify_PlayNiagaraEffect`。
2. 保留原触发时间、Track、Socket、Attached、Location/Rotation/Scale Offset。
3. 保留同一时间点附近的声音 Notify，不改变 Grenade、Rocket 前后喷口和 Shotgun Reload 的相对顺序。
4. 编译和保存动画后重启编辑器复读 Notify。
5. 在单人、双本地玩家和 Listen Server 中验证每端触发次数、附着位置、声音和粒子同步。

本阶段没有修改任何 Notify 或动画资产。

## 6. 本地未提交资产判断

- `Content/Assets/MilitaryWeapSilver/Weapons/Textures/Assault_Rifle_A_Diff.uasset` 是用户原有修改，本阶段未触碰。
- `Content/Blueprints/Actor/Potion/BP_Negative_HealthPotion.uasset` 与 `Content/Blueprints/GameplayCueNotifies/GCN_Weapon_Impact.uasset` 在接手前已经修改。重启后两者均为 `BS_UP_TO_DATE`、编辑器内 `is_dirty=false`；前者直接依赖 `/Game/Weapons/G67_Grenade/Mesh/SM_G67`，后者直接依赖迁移后的 Rifle Impact SoundCue。
- 上述两个 Blueprint 的 LFS 对象大小只发生小幅变化，符合编译/重存产物特征，但没有文本化语义 diff 可以证明“只有元数据变化”。因此继续保留在工作区，不纳入本阶段提交，也不覆盖或删除。
- `Scripts/Editor/Animation/__pycache__/` 保持未跟踪，不纳入提交。

## 7. 后续门槛

1. 用户人工执行 Fix Up Redirectors 后，按第 3 节重新扫描和验收。
2. 若决定迁移某个 Cascade，先只选一项代表样本；打开转换后的 Niagara Editor，解决所有 Log 错误，并保留旧 Cascade 做并排 A/B。
3. 代表样本必须通过近景、远景、不同 Scalability、Socket 附着、动画时序、Listen Server 和代表性性能测量。
4. 只有样本等价后，才能提出同族批量迁移；任何不支持模块、LOD 差异或表现差异都返回人工确认。
5. 四类特殊武器的 GAS、弹药、投射物和网络能力链继续作为独立任务，不混入资产迁移提交。
