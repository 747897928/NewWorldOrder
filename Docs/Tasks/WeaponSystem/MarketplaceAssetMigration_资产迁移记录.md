# 商城武器资产迁移记录

状态：第一阶段迁移、第二阶段审计和重定向器清理完成；Cascade 转换为 0；`Shotgun_A` 特殊武器第一纵切完成

日期：2026-08-24

## 1. 本次范围

本次只处理以下两个项目内商城资产目录的盘点、引用保护和目录迁移：

- `/Game/Assets/MilitaryWeapSilver`
- `/Game/Assets/FPS_Weapon_Bundle`

目标是把两个包中的全部真实资产放到 `/Game/Weapons` 下，并按武器网格所属枪族归档。此次没有实现逐发装填霰弹枪、狙击枪、榴弹发射器或火箭筒的能力链，也没有把 Cascade 强行转换成 Niagara。

资产操作全部通过项目 UE 5.8 MCP 的 Unreal Python、AssetRegistry 和 AssetTools 完成，没有读取或直接编辑 `.uasset` 二进制文件。

## 2. 迁移结果

迁移前的资产注册表盘点为：MilitaryWeapSilver 432 个资产，FPS_Weapon_Bundle 219 个资产。此前已经单独测试移动 1 个榴弹爆炸 Cascade 到目标目录；本批次继续移动其余 649 个真实资产，共计 650 个商城资产进入 `/Game/Weapons`。

迁移后重新扫描的结果：

- 两个源根目录中的真实资产数量均为 0。
- 源目录中仍有重定向器：MilitaryWeapSilver 432 个，FPS_Weapon_Bundle 221 个。它们不是第二份真实资产，用于让尚未 Fix Up 的旧引用继续解析。
- `/Game/Weapons` 共 763 个真实资产，其中原有项目资产 113 个，本次迁移资产 650 个。
- 迁移目标没有检测到路径冲突；同名骨骼位于不同来源目录时，分别放在 `Mesh` 和 `Animations`，没有覆盖。
- 已有项目目标 `/Game/Weapons/Assault_Rifle_A/Textures/Assault_Rifle_A_Diff` 没有被覆盖；源侧同名重定向器和用户当前修改继续保留。

### 枪族归档数量

| 目标枪族 | 迁移资产数 |
| --- | ---: |
| `Assault_Rifle_A` | 75 |
| `Grenade_Launcher_A` | 48 |
| `Rocket_Launcher_A` | 48 |
| `Knife_A` | 30 |
| `Pistols_A` | 58 |
| `Shotgun_A` | 53 |
| `Sniper_Rifle_A` | 53 |
| `AR4` | 15 |
| `G67_Grenade` | 13 |
| `KA74U` | 20 |
| `KA_Val` | 19 |
| `Ka47` | 19 |
| `M9_Knife` | 12 |
| `SMG11` | 29 |
| `Shared` | 158 |
| 合计 | 650 |

### 目标目录规则

每个枪族下按资产职责分为 `Mesh`、`Animations`、`Materials`、`Textures`、`Audio`、`Effects` 和 `ControlRig` 等目录；无法归属于单一枪族的通用商城依赖放在 `/Game/Weapons/Shared` 下。

用户明确要求的归档结果已核对：

- `/Game/Weapons/Grenade_Launcher_A/Mesh/GrenadeLauncherA_Ammo`
- `/Game/Weapons/Grenade_Launcher_A/Mesh/Grenade_Launcher_A`
- `/Game/Weapons/Rocket_Launcher_A/Mesh/RocketLauncherA_Ammo`
- `/Game/Weapons/Sniper_Rifle_A/Mesh/Sniper_Rifle_A_Skeleton`
- `/Game/Weapons/Sniper_Rifle_A/Mesh/SniperRifleA_Ammo`
- `/Game/Weapons/Ka47/ControlRig/SK_KA47_CtrlRig`
- `/Game/Weapons/Grenade_Launcher_A/Animations/Fire_GrenadeLauncher_W`

## 3. 引用和验证

迁移前已记录外部项目引用，包括 `BP_Negative_HealthPotion` 对 G67 网格的引用、`GCN_Weapon_Impact` 对步枪命中音效的引用、Assault Rifle AnimBP 和现有 Rifle Montage 对商城网格/动画/骨骼的引用。

迁移后已完成 AssetRegistry 扫描和目标资产加载检查。`BP_Negative_HealthPotion` 与 `GCN_Weapon_Impact` 已通过蓝图编译；Assault Rifle AnimBlueprint 已成功加载。目标网格、音频、动画和 Cascade 资产均能在目标路径解析。

第一阶段检查点时仍能在引用查询中看到源重定向器，例如旧 Rifle Montage 通过 `/Game/Assets/MilitaryWeapSilver/...` 指向新资产，因此当时没有自动删除。随后第二阶段完成外部引用评估，`f9ecbd76` 在用户继续操作后提交了精确重定向器清理。2026-08-25 再次重启编辑器并刷新 AssetRegistry 后，`/Game/Assets/MilitaryWeapSilver` 与 `/Game/Assets/FPS_Weapon_Bundle` 的注册资产数均为 0；目标 `/Game/Weapons` 资产继续可解析。不要根据第一阶段的历史数量重新恢复 653 个源重定向器。

## 4. Cascade 现状

本阶段没有创建或替换 Niagara，也没有删除 Cascade。MilitaryWeapSilver 中盘点到的 25 个 Cascade `ParticleSystem` 为：

```text
P_AssaultRifle_MuzzleFlash
P_AssaultRifle_Tracer_01
P_Grenade_Explosion_01
P_Grenade_MuzzleFlash_01
P_Grenade_Trail_01
P_Impact_Metal_Large_01
P_Impact_Metal_Medium_01
P_Impact_Metal_Small_01
P_Impact_Stone_Large_01
P_Impact_Stone_Medium_01
P_Impact_Stone_Small_01
P_Impact_Wood_Large_01
P_Impact_Wood_Medium_01
P_Impact_Wood_Small_01
P_Knife_RibbonTrail_01
P_Pistol_MuzzleFlash_01
P_Pistol_Tracer_01
P_RocketLauncher_Explosion_01
P_RocketLauncher_MuzzleFlash_Front_01
P_RocketLauncher_MuzzleFlash_Rear_01
P_RocketLauncher_Trail_01
P_Shotgun_MuzzleFlash_01
P_Shotgun_Tracer_01
P_SniperRifle_MuzzleFlash_01
P_SniperRifle_Tracer_01
```

`Fire_GrenadeLauncher_W` 的动画通知包含 `P_Grenade_MuzzleFlash_01` 和 `GrenadeLauncherA_Fire_Cue`。这证明迁移时必须把动画、特效和音频通知一起作为表现闭包处理；本阶段只移动它们，未改变通知内容。

下一阶段必须先查阅 Epic 官方文档和当前 UE 版本的迁移能力，逐项判断 Cascade 到 Niagara 是否能保持视觉、时序、材质和动画通知行为等价。无法证明等价的项目保留 Cascade，并提交人工决策清单，不得批量删除。

## 5. 尚未完成和安全边界

- 源重定向器已在后续提交 `f9ecbd76` 清理。重启后的 AssetRegistry 复核为两个源根目录各 0 个资产；本阶段没有用 PowerShell 递归删除目录。
- 若物理磁盘仍残留空目录，只能按 `AGENTS.md` 的精确路径安全规则处理；空目录本身不属于 Unreal 资产，不得用递归删除重新扩大范围。
- Cascade/Niagara 官方资料与逐项引用审计已完成；由于可证明一对一等价的项目为 0，转换和删除均未执行。
- `Shotgun_A` 的 ItemDefinition、AbilitySet、逐发 GA、弹药、角色/武器 Montage 和单人 PIE 第一纵切已完成；Listen Server、正式投放与动画修型仍未完成。狙击枪、榴弹发射器和火箭筒能力链尚未接入。
- 当前工作树中的 `BP_Negative_HealthPotion.uasset`、`GCN_Weapon_Impact.uasset`、`HomeMap.umap` 与 `Scripts/Editor/Animation/__pycache__/` 不属于 `Shotgun_A` 提交，必须继续排除。旧源侧 `Assault_Rifle_A_Diff` 路径已在 `f9ecbd76` 清理，目标 `/Game/Weapons/Assault_Rifle_A/Textures/Assault_Rifle_A_Diff` 继续存在。

## 6. 后续顺序

1. 保持 25 个 Cascade 不变，只有在单个代表样本具备视觉、时序、材质、LOD 与性能 A/B 证据后才提出 Niagara 替换。
2. 对 `Shotgun_A` 完成 Listen Server 双端弹药复制、打断预测、远端 Montage 与固定拾取点验收。
3. 用 Control Rig 或 Blender 为 MF/MM 分别制作“商城上半身 + Lyra/CC 下半身站姿”的离线烘焙动画，检查脚距、pelvis、重心、胸腔、肩肘和骨骼朝向后交用户视觉验收。
4. 以同样的单把参考武器方式依次接入狙击枪、榴弹发射器和火箭筒，不把三类武器逻辑混成一次不可验收的批量修改。

## 7. 第二阶段审计结果

2026-08-24 已在 NewWorldOrder 编辑器重启后完成复核：

- 源目录确认只有 432 + 221 = 653 个 `ObjectRedirector`，真实资产为 0。
- 91 个重定向器仍被 56 个源目录外包引用；Fix Up 会重存大量 `/Game/Weapons` 资产及三个既有 Assault Rifle 包，因此本阶段只做安全评估，没有自动批量 Fix Up 或删除。
- 25 个 Cascade 中 21 个有 3 个 LOD 距离，而 Epic 转换器只处理 LOD 0；`P_Knife_RibbonTrail_01` 又属于不支持的 AnimTrail/Beam 转换范围。
- 只有 6 个 Cascade 有源重定向器之外的真实使用者；其余 19 个仍作为未来武器接入候选保留。
- 现有 Niagara 资产只能作为语义候选，没有一项具备视觉、时序、材质和性能的一对一等价证据。
- 本阶段转换 0 项、删除 0 项，未修改动画 Notify。

完整证据、25 项逐表、官方资料和动画时间轴见 `Docs/Tasks/WeaponSystem/CascadeNiagaraAudit_审计报告.md`。

## 8. 清理与 Shotgun A 后续进展

- `f9ecbd76` 完成源重定向器的精确清理；2026-08-25 编辑器重启后两个源根目录 AssetRegistry 数量均为 0。
- `Shotgun_A` 已按现有项目模板建立独立 RuntimeOnly 武器闭环，逐发装填使用 `ShotgunStart/ShotgunLoop/ShotgunEnd` 和服务器逐发提交。
- 角色与武器 Montage Sync Follow、逐发弹药转移、非空打断开火和空仓阻止开火已通过单人 PIE；网络与美术验收仍明确保留。
- `P_Shotgun_MuzzleFlash_01` 继续由武器 Fire 动画 Notify 使用，未转换或删除。

完整实现边界与验证记录见 `Docs/Tasks/WeaponSystem/ShotgunPerShell_逐发装填接入.md`。
