# NewWorldOrder 交接审计与下一阶段代办

更新日期：2026-08-25

依据：

- `C:\Users\wizard\.codex\attachments\eb63efc5-2889-4e31-b153-8df3256648c2\pasted-text.txt`
- 本轮早期武器迁移提示词
- `Docs/Tasks/WeaponSystem/STATUS.md`
- `Docs/Tasks/WeaponSystem/GrenadeHealthpack_实施记录.md`
- `Docs/Tasks/GEDamageRegression/Status_状态.md`
- `Docs/Tasks/AnimationMigration/Overview_交接.md`
- 当前 `origin/main` 与本轮 PIE/编译证据

## 一、需求主线

项目要在 Lyra 派生的武器系统上继续扩展，不是简单把商城资产复制到工程里。最终目标是：

1. 保持现有 `/Game/Weapons` 的职责和目录习惯。
2. 将 `MilitaryWeapSilver` 与 `FPS_Weapon_Bundle` 中真正需要的资产整理、迁移到项目目录。
3. 处理老包中的 Cascade 依赖，确认可替换的表现已迁到 Niagara，并通过引用审计决定保留或删除。
4. 把商城包提供的枪型接入项目武器能力，而不是只迁移网格：逐发装填霰弹枪、狙击枪、榴弹发射器、火箭筒都属于后续武器系统主线。
5. 以资产引用、GameplayCue、GAS 能力、QuickBar、Equipment 和角色动画层的完整闭环作为完成标准。

当前已完成的是手雷与拾取式回血包的第一轮项目化，不应重新把回血包做成武器或重新复制 QuickBar 槽位。

## 二、交接文件 8 项待办核对

| 编号 | 原待办 | 状态 | 当前结论 |
| --- | --- | --- | --- |
| 1 | 手动 PIE 验收：手雷、治疗包、男女主血条、出副本技能、回合重开 | 部分完成 | 手雷投掷/弹体/爆炸、治疗包低血拾取、满血不可用、护盾先扣、回血包重生已通过自动化和用户验收；男女主切换血条、出副本技能消失、`InitializeRoundForAll` 蓝图接线仍需独立验收。 |
| 2 | 弹体可见性和爆炸表现调优 | 部分完成 | 已有数据驱动投射物网格、轨迹和爆炸 Niagara；速度与引信已为可观察测试值。弹体尺寸、遮挡、近距离观感仍需人工调优，不能宣称最终美术完成。 |
| 3 | `W_GrenadeCooldown` 初始 50% 冷却 | 已完成 | 初始按钮可用状态已修复，冷却完成后亮起，用户已验收；相关修复见 `f5105363`、`667e59ff`、`7738ca17`。 |
| 4 | 手雷距离衰减伤害 | 未完成 | `/Game/Weapons/Grenade/Curve_GrenadeDamage` 已存在，但尚未接入正式伤害计算链。需要明确曲线行、命中距离来源、SetByCaller 或 Execution 的落点，并补近点/远点验收。 |
| 5 | 手雷 GameplayCue 爆炸表现与多端表现 | 部分完成 | 单机已有直接 Spawn 的爆炸 Niagara 和音效表现；项目化 `GameplayCueNotify` 以及联机多端一致性仍需核对，不能把 standalone 表现等同于网络完成。 |
| 6 | 手雷/治疗包完成后的音效、特效、AudioModulation 引用审计 | 已完成 | 已审计并保留有引用的 `CB_*` 与 `PP_Default*`；迁移音效、特效、回血包材质均已有引用或已在提交历史中。当前遗留的 Potion、GameplayCue、HomeMap 与 Python `__pycache__` 修改继续由各自任务所有者处理。 |
| 7 | `MilitaryWeapSilver` / `FPS_Weapon_Bundle` 整理 | 资产迁移、审计与重定向器清理完成；特殊武器接入进行中 | 已完成 650 个真实商城资产迁移、第二阶段引用/Cascade 审计以及 `f9ecbd76` 的精确重定向器清理；重启后两个源根目录 AssetRegistry 资产数均为 0。25 个 Cascade 中 0 项具备一对一 Niagara 等价证据，未转换或删除。`Shotgun_A` 逐发装填第一纵切已完成单人 PIE，人工 A/B、联机验收和其余三类特殊武器仍未完成。详见 `Docs/Tasks/WeaponSystem/MarketplaceAssetMigration_资产迁移记录.md`、`CascadeNiagaraAudit_审计报告.md` 与 `ShotgunPerShell_逐发装填接入.md`。 |
| 8 | 女主 X 医疗站 | 未完成 | `AShootSkillMedicalStation` 骨架可复用，医疗站高级规则、技能冷却 GE、pad 视觉和完整使用验收尚未完成。回血包拾取不等于医疗站完成。 |

## 三、早期武器迁移目标核对

| 早期目标 | 状态 | 说明 |
| --- | --- | --- |
| 先读 Lyra 和项目武器系统，按 Lyra 思路理解现有链路 | 基本完成 | 现有三把基础武器的 ItemDefinition、Equipment、QuickBar、Fire/Reload、动画层和表现链已有项目化审计与运行时证据。后续迁移仍必须以当前项目实现为准，不能直接复制 Lyra 蓝图父类。 |
| 按 `/Game/Weapons` 目录整理 `MilitaryWeapSilver` | 完成 | 真实资产已按 Assault Rifle、Grenade Launcher、Rocket Launcher、Knife、Pistols、Shotgun、Sniper Rifle 和 Shared 归档；`f9ecbd76` 清理源重定向器，重启后源根目录 AssetRegistry 数量为 0。 |
| 按 `/Game/Weapons` 目录整理 `FPS_Weapon_Bundle` | 完成 | AR4、G67_Grenade、KA74U、KA_Val、Ka47、M9_Knife、SMG11 及 Shared 资产已按网格族归档；`f9ecbd76` 清理源重定向器，重启后源根目录 AssetRegistry 数量为 0。特殊武器逻辑按单把纵切继续推进。 |
| 全面从 Cascade 倒向 Niagara | 审计完成，转换未开始 | 25 项已逐一列出引用方、LOD、动画时序和语义候选；21 项多 LOD、1 项 AnimTrail，且全部缺少视觉与行为等价证据。本轮结论为转换 0、删除 0，后续只允许从单个代表样本开始人工 A/B。 |
| 集成逐发装填霰弹枪 | 第一纵切完成 | `Shotgun_A` 已形成独立 ItemDefinition、Equipment、WeaponInstance、WeaponActor、AbilitySet、Start/Loop/End Montage、逐发 Notify 和单人 PIE 弹药/打断/同步闭环。Listen Server、正式投放以及商城上半身与 Lyra/CC 下半身的离线动画修型仍待后续。 |
| 集成狙击枪 | 未完成 | `ShootGA_Reload_Sniper.cpp` 存在，但尚未接入完整武器目录和正式能力配置。 |
| 集成榴弹发射器 | 未完成 | `ShootGA_Weapon_Fire_GrenadeLauncher.cpp` 存在，但尚未完成武器资产、投射物、伤害和表现闭环。手雷能力不是榴弹发射器能力。 |
| 集成火箭筒 | 未完成 | `ShootGA_Weapon_Fire_RocketLauncher.cpp` 存在，但尚未完成武器资产、投射物、伤害和表现闭环。 |

## 四、已完成成果，不要重复施工

1. 手雷不是回血包的替代品；回血包是拾取即用的交互对象，不占 QuickBar 武器槽。
2. 手雷独立挂在 `HUD.Slot.ExtraEquipment`，不再复制为 QuickBar 槽位。
3. 手雷初始进入副本时应立即可用；冷却结束后按钮恢复亮态。
4. 手雷爆炸当前已有 standalone 的 Niagara/音效表现；后续只处理 Cue 网络闭环和验收缺口。
5. 火焰危险区是自动 Overlap，踩上去持续扣血，不需要按 F；当前测试 GE 已走 `IncomingDamage`，由正式伤害链按护盾优先吸收。
6. 护盾伤害规则已经修复为先扣 `Shield`，溢出部分再扣 `Health`；不要重新写一条绕过 `IncomingDamage` 的测试扣血路径。
7. `BP_HealthpackPickup` 已支持拾取即用、满血不提供交互、消耗后隐藏和禁用碰撞、默认 30 秒服务器重生；`RespawnDelay=0` 表示一次性销毁。
8. AudioModulation 的 `CB_*` 和 `PP_Default*` 不是垃圾修改，已确认存在项目 Audio Classes/ParameterPatches 引用，应保留。
9. 回血包材质和模型已在历史提交中，不要因为当前 diff 中看不到就重新复制或删除。
10. CC `MF`/`MM` 的 `Animations/Poses` 目录按用户要求不参与无引用清理；其余已完成一轮无引用动画清理。

相关提交：

- `7738ca17`：护盾先扣、回血包默认 30 秒重生、测试危险区正式伤害链。
- `2bddc20d`：已引用 AudioModulation 资产和迁移音频提交。
- `2612ddb4`：回血包美术资产已在历史中提交。
- `261942a4`：CC 无引用动画清理，排除 Poses。
- `aa55f91e`：手雷音效与回血包反馈闭环。
- `dd81fb58`：手雷初始可用状态修复链。

## 五、建议分流

### 当前 Codex 可直接接的窄任务

1. 手雷距离衰减：只改正式伤害链和所需的最小资产配置，先读 `Curve_GrenadeDamage`、当前 Damage Execution、Lyra 对应 GE，再实现近点/远点可验证的曲线采样。
2. 手雷 GameplayCue 联机闭环：确认当前 Cue 标签、Cue Manager 扫描路径、音效和 Niagara 的网络触发方式，只在证据明确时补项目适配，不复制 Lyra 无关资产。
3. 医疗站第一纵切：在已有 `AShootSkillMedicalStation` 骨架上补冷却 GE、pad 视觉和最小完整验收；不要修改回血包拾取链。
4. 文档和人工验收陪跑：可以整理日志、截图和验收表，但不能把没有人工完成的男女切换、真实双端、出副本和回合重开写成已通过。

### Luna Max 适合接的资产任务

1. 商城包第一阶段：资产盘点、引用清单、目录整理、精确移动/复制、初步 Cascade/Niagara 清单和可证实的一对一替换。
2. 在引用审计证明安全后，按精确对象路径处理无用资产；不能使用递归删除，也不能把“没有看到引用”当成删除证据。
3. 资产迁移完成后输出 manifest、引用差异和需要人工判断的 Cascade 项，由当前 Codex 复核；如果发现表现不等价或跨系统依赖，再升级到 Terra/Sol。

### Terra High 或 Sol High 适合接的任务

1. Cascade→Niagara 中需要判断视觉等价性、重建参数或修改多个蓝图/GameplayCue 的部分。
2. 四类特殊武器接入：逐发霰弹枪、狙击枪、榴弹发射器、火箭筒的跨资产闭环，涉及 GAS、弹药、动画、投射物、Cue、网络和 QuickBar。
3. CharacterSwitching、AnimationMigration、SessionUI 等跨系统任务的最终收口和实机验收。它们不是当前商城资产整理的隐含步骤，不应与迁移任务混在同一批修改里。

Sol High 的价值主要在跨系统最终判断和高代价验收，不代表所有资产整理都必须使用 Sol High。

### 不应交给无人值守批处理的事项

- 直接递归删除商城目录或项目外目录。
- 未经 Asset Registry/引用方确认就批量删除 Cascade、材质、音效或动画。
- 直接读取或修改 `.uasset` 二进制文件。
- 修改 Lyra、第三方插件或 Unreal Engine 源码。
- 用“看起来像没引用”替代重启编辑器后的依赖复核和蓝图编译。

## 六、给 Luna Max 的实现提示词

适用场景：把商城资产的第一阶段整理交给 Luna Max，思考强度选择 Max。当前 Codex 在 Luna Max 完成后核对 diff、引用清单和验证证据；如果出现非机械性的 Niagara 重建或跨系统武器逻辑，再升级到 Terra High 或 Sol High。

```text
任务：完成 NewWorldOrder 的商城武器资产迁移第一阶段，不要重新实现已经完成的手雷/回血包功能。

OBJECTIVE
阅读 NewWorldOrder/AGENTS.md、Docs/Tasks/TaskAudit/NextWork_代办与提示词.md，以及 WeaponSystem、AnimationMigration、GEDamageRegression 的相关 STATUS/Requirements/Overview。理解当前 Lyra 派生武器链后，处理：
1. 审计 /Game/Assets/MilitaryWeapSilver 和 /Game/Assets/FPS_Weapon_Bundle。
2. 参考现有 /Game/Weapons、/Game/Audio、/Game/Effects 的实际组织方式，提出并执行最小必要的目录整理和迁移。
3. 列出两个商城包中的 Cascade 资产、所有引用方、可否转换到 Niagara、转换后的引用替换结果和不能自动转换的项目。
4. 只对证据充分的 Cascade 项执行 Niagara 迁移；转换失败或表现不等价的项目先保留并列入阻塞清单，不要假装完成。
5. 为后续逐发装填霰弹枪、狙击枪、榴弹发射器、火箭筒保留清晰的资产边界，但本批次不要擅自把四类武器能力全部实现，除非当前资产审计显示某项是迁移闭包的必要步骤。

FILES AND OWNERSHIP
- 允许修改：NewWorldOrder/Content 下明确属于上述两个商城包的迁移目标、必要的项目适配 C++/蓝图配置、相关 Docs/Tasks 文档。
- 禁止修改：Plugins/Lyra、Plugins/CommonUser、Plugins/CommonGame、Plugins/GameplayMessageRouter、Unreal Engine 源码及任何上游插件源码。
- 不要覆盖用户当前未提交修改：
  NewWorldOrder/Content/Assets/MilitaryWeapSilver/Weapons/Textures/Assault_Rifle_A_Diff.uasset
  NewWorldOrder/Scripts/Editor/Animation/__pycache__/
- 不要重复创建或迁移已有闭包：手雷、回血包、AudioModulation、CC Poses 之外的已完成动画清理成果。

INTERFACES
- 以项目现有 WeaponInstance、Equipment、ItemDefinition、QuickBar、GAS AbilitySet、GameplayCue 和正式 CC 动画层为准。
- C++ 不得硬编码 /Game/ 资产路径；具体网格、材质、AnimBP、Niagara、音效应由蓝图/DataAsset 配置。
- 资产操作必须使用 Unreal MCP/Python/Asset Registry；禁止直接读写 .uasset 二进制。
- 先建立迁移前清单和引用闭包，再做移动/复制/替换；每个批次可回溯。

CONSTRAINTS
- 任何删除前必须得到精确对象路径、确认路径在项目根目录内、确认无引用或已有替代引用，并记录删除原因。
- 禁止 PowerShell 递归删除、通配符删除、拼接不完整路径删除；多个文件或非空目录需要停下来列清单，不要猜删。
- 不要把“资产能加载”当成“运行时完成”；至少验证依赖、蓝图编译、引用方和必要的 PIE 表现。
- 迁移目标必须遵循项目现有目录约定，不要为了看起来像 Lyra 而破坏当前项目已经验证的 /Game/Effects/Blueprints、HUD 插槽或 GameplayCue 扫描路径。
- 每个实质批次都要更新任务文档，并使用精确文件列表提交和推送，不能裸 git commit 把用户暂存区全部卷入。

VERIFICATION
1. 输出迁移前后资产清单和引用差异。
2. 输出所有 Cascade 资产的处理结果：已转换、保留、阻塞、可删除候选。
3. 重启编辑器或等价地重新加载包后，复核蓝图/data asset 的资产引用仍然存在。
4. 编译项目 Editor/Runtime 目标；对每个涉及的武器表现做最小 PIE 验证。
5. 检查 git diff、git status，确认没有把用户未提交修改、Recovery 目录、插件源码或无关资产带入提交。
6. 提交并推送自己的分支，报告 commit hash、远程分支、未完成项和需要人工验收的项目。

IMPLEMENTATION REPORT
STATUS: COMPLETE | PARTIAL | BLOCKED
SUMMARY:
CHANGED_FILES:
MIGRATION_MANIFEST:
CASCADE_NIAGARA_AUDIT:
REFERENCE_VERIFICATION:
BUILD_AND_PIE:
COMMIT_AND_PUSH:
OPEN_RISKS:
NEXT_ACTIONS:
```

## 七、给 Sol High 的独立复核提示词

适用场景：Luna Max 或 Terra High 完成商城资产迁移后，打开全新的 Sol High 会话，要求它只做审计、修正确定的问题和最终验收，不要从头重做。

```text
你是 NewWorldOrder 商城武器迁移的最终 Sol High 审核者。

先读：
1. NewWorldOrder/AGENTS.md 全文。
2. Docs/Tasks/TaskAudit/NextWork_代办与提示词.md。
3. Terra High 报告中列出的迁移清单、Cascade/Niagara 清单、commit 和未完成项。
4. WeaponSystem、AnimationMigration、GEDamageRegression 的相关任务文档。

你的范围只包括：
- 检查 MilitaryWeapSilver/FPS_Weapon_Bundle 的迁移目录是否符合项目现有约定。
- 检查迁移资产的真实引用闭包、蓝图父类、GameplayCue 扫描路径、AudioModulation 和 Niagara 引用。
- 检查 Cascade→Niagara 的替换是否有证据，不能因文件名相似就判定等价。
- 检查是否误删、漏迁移、跨项目引用、修改了 Lyra/插件源码或卷入用户未提交文件。
- 对确定的错误做最小修正；不扩展到四类特殊武器完整实现，不重做已验收的手雷/回血包。

硬约束：
- 不能直接读取或写入 .uasset 二进制。
- 资产操作使用 MCP/Python/Asset Registry。
- 删除必须逐项确认精确项目内路径和引用结果；不能用递归 PowerShell 删除。
- 不修改用户工作树中已有的 Assault_Rifle_A_Diff.uasset 和 Python __pycache__。
- 不修改上游 Lyra、第三方插件或 Unreal Engine 源码。

验收：
1. 重载/重启后复核迁移资产、蓝图和材质引用。
2. 编译相关项目目标。
3. 对至少一个迁移武器做 PIE 表现验证；若没有足够数据，明确标为人工阻塞。
4. 输出精确审计报告、修正文件、未完成项、commit hash 和远程分支。

报告格式：
IMPLEMENTATION REPORT
STATUS:
AUDIT_SCOPE:
CONFIRMED_OK:
CORRECTIONS:
CASCADE_NIAGARA_FINDINGS:
DELETION_SAFETY:
BUILD_AND_PIE:
COMMIT_AND_PUSH:
BLOCKERS:
```

## 八、给当前 Codex 的窄任务提示词

适用场景：不启动商城包迁移时，当前会话可继续接手的最小代码任务。

```text
只实现 NewWorldOrder 的手雷距离衰减伤害，不处理商城武器包迁移，不修改回血包链。

先读 AGENTS.md、Docs/Tasks/TaskAudit/NextWork_代办与提示词.md、GEDamageRegression 任务文档，搜索当前手雷 Damage GE、Damage Execution、Curve_GrenadeDamage、SetByCaller 和 Lyra 对应实现。先给出当前链路和曲线行的证据，再做最小修改。

要求：
- 不在 C++ 中硬编码 /Game/ 资产路径或曲线数据。
- 不绕过 IncomingDamage、Shield-first 和现有团队/伤害规则。
- 近距离、远距离、曲线边界和无曲线兜底都要有可验证行为。
- 编译、PIE 或日志验证后更新任务文档。
- 只提交本任务精确文件，不卷入用户未提交的 Assault_Rifle_A_Diff.uasset 和 Python __pycache__。
- 最终报告必须写明 STATUS、调用链、改动文件、验证结果、commit hash、push 结果和剩余风险。
```

## 九、人工验收清单

以下项目不能仅凭静态代码或单机自动化标记完成：

- 手雷在实际视角中的投掷动画、弹体大小、轨迹遮挡、爆炸音量和爆炸视觉。
- 双端或 Listen Server 中爆炸 Niagara、音效、伤害和 Cue 是否每端只触发一次。
- 男女主切换后血量/护盾 UI 是否保持默认属性且不归零。
- 离开副本后技能是否被 Experience 生命周期取回，重新进入后是否重新授予。
- `InitializeRoundForAll` 是否已在目标 GameMode 蓝图接线，并能同时重置玩家和敌人。
- 商城武器迁移后每种武器的实际装备、开火、换弹、动画和弹药体验。

## 十、当前工作树注意事项

商城资产迁移、审计和源重定向器清理已经分别由 `273e4db1`、`a3536cbc`、`f9ecbd76` 提交并推送。2026-08-25 重启后的 AssetRegistry 复核中，两个源根目录资产数均为 0；旧的源侧 `Assault_Rifle_A_Diff` 路径已经不存在，目标 `/Game/Weapons/Assault_Rifle_A/Textures/Assault_Rifle_A_Diff` 仍存在。不要按旧交接说明重新创建源重定向器。

当前工作树仍有下列接手前或编辑器外部修改，后续任务必须继续排除：

- `NewWorldOrder/Content/Blueprints/Actor/Potion/BP_Negative_HealthPotion.uasset`
- `NewWorldOrder/Content/Blueprints/GameplayCueNotifies/GCN_Weapon_Impact.uasset`
- `NewWorldOrder/Content/Maps/HomeMap.umap`
- `NewWorldOrder/Scripts/Editor/Animation/__pycache__/`

它们不属于 `Shotgun_A` 逐发装填提交，不得恢复、覆盖或混入暂存区。

本轮迁移的完整资产数量和目标目录见 `Docs/Tasks/WeaponSystem/MarketplaceAssetMigration_资产迁移记录.md`；25 个 Cascade 逐项结论和动画时间轴见 `Docs/Tasks/WeaponSystem/CascadeNiagaraAudit_审计报告.md`；`Shotgun_A` 第一纵切见 `Docs/Tasks/WeaponSystem/ShotgunPerShell_逐发装填接入.md`。
