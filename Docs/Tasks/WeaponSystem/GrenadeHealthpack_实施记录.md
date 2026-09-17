# 手雷与治疗包实施记录

# 目标与边界

本任务参考 Lyra 的投掷物、冷却消息和拾取物思路，但不迁入或依赖 LyraGame 的 GA、GE、UI、输入或 `/ShooterCore` 资产。

- 手雷是男女主共有的玩法能力，输入由 `/Game/Blueprints/Input/IMC_Default` 的 `IA_Grenade` 提供；键盘为 `T`，手柄为 `Gamepad_RightShoulder`。
- 手雷 UI 使用项目 `W_DefaultHUD` 内的 `GrenadeExtensionPoint`，其标签为 `HUD.Slot.ExtraEquipment`；`DA_Experience_DungeonTest` 将 `W_GrenadeCooldown` 注册到该插槽。
- 治疗包是战斗内可配置消耗的 `IInteractableTarget`，不进入 QuickBar、`UResourceInventoryComponent`、Persistent 背包或 SaveGame；默认 `RespawnDelay=30` 秒重生，设为 `0` 时才是一次性消耗。
- 治疗包沿项目交互链动态授予治疗能力，不创建武器实例，不另建绕过 GAS/AttributeSet 的加血旁路。

# 已实现的原生逻辑

## 手雷

- `UShootGA_ThrowGrenade` 在客户端本地预测激活，在服务器生成现有 `AShootProjectileGrenade`。
- 投射物使用项目 `UShootEffect_DamageSetByCaller`、现有伤害半径配置和 `GameplayCue.Weapon.Grenade.Detonate`；不会连接 Lyra 伤害类。
- `UShootEffect_GrenadeCooldown` 是项目原生 10 秒冷却 GE，授予 `Cooldown.Weapon.Grenade`，能力激活时阻断重复投掷。
- `Ability.Grenade.Duration.Message` 通过项目 `UGameplayMessageSubsystem` 广播给 `W_GrenadeCooldown`；该 Widget 的 `W_ActionTouchButton` 已配置 `IA_Grenade` 与该 Duration Message Tag。
- 当前 `AShootPlayerState` 会随男女两套主动技能一起授予该通用能力，因此角色切换后不会遗留旧 Avatar 的 AbilitySpec。

## 治疗包

- `AShootHealthpackPickup` 继承 `AActor` 并实现 `IInteractableTarget`，只在角色未满血且进入交互碰撞范围时提供选项。
- `UShootGA_Interaction_HealthPack` 沿项目交互链动态授予，服务器应用 `UShootEffect_HealInstant`，通过 `SetByCaller.Heal` 默认治疗 50 点；不创建武器实例、不占 QuickBar、不改存档。
- 实际治疗量大于零时才执行 `GameplayCue.Character.Heal` 并消耗拾取物；默认消耗后隐藏并关闭碰撞，30 秒后由服务器恢复，`RespawnDelay=0` 时才销毁。满血、无效事件或治疗没有产生正向变化时不会白白消耗拾取物。
- `/Game/Weapons/Healthpack/Blueprint/BP_HealthpackPickup` 继续使用现有健康包网格和材质，世界中的三个地图实例均已引用该蓝图。

# 当前资产与旧实现状态

- `/Game/Weapons/Grenade/GA_Grenade`
- `/Game/Weapons/Healthpack/Blueprint/BP_HealthpackPickup`
- `/Game/GameFramework/Experiences/DA_Experience_DungeonTest`
- `/Game/UI/Weapon/W_GrenadeCooldown`
- `/Game/Blueprints/GameplayCues/Weapons/Grenade/GCN_Weapon_Grenade_Detonate`
- `/Game/Blueprints/GameplayCues/Character/GCN_Character_Heal`

旧的武器化治疗包资产 `GA_ConsumeHealthPack`、`AS_Healthpack`、`AS_Healthpack_Use`、
`B_WeaponInstance_Healthpack`、`BP_Equipment_Healthpack`、`ID_Healthpack`、
`BP_WeaponPickup_Healthpack` 当前均不存在；新的 `BP_HealthpackPickup` 仍被
`HomeMap`、`TestMap_ListenServer`、`TestMap_SplitScreen` 引用，因此保留。

# 本轮已完成的资产与运行时验收

1. `/Game/Blueprints/GameplayCues/Weapons/Grenade/GCN_Weapon_Grenade_Detonate` 已继承项目原生 `UShootGameplayCueNotify_Presentation`，引用 `NS_Grenade_Explosion`、爆炸 MetaSound 和 `ATT_Grenade`。
2. `/Game/Blueprints/GameplayCues/Character/GCN_Character_Heal` 已引用项目内可用的 `NS_Pickup_Success`，由治疗能力在实际产生正向治疗后执行。
3. TestMap Listen Server 已通过真实 `IA_Interact` 交互验证：角色先在可见火焰区自动受到周期伤害，当前夹具已改为向 `IncomingDamage` 写入伤害，正式后处理顺序为先扣 `Shield`，护盾耗尽后才扣 `Health`；随后使用治疗包恢复生命并进入重生计时。
4. 手雷蓝图子类 `GA_Grenade_C` 已通过 GAS 激活，服务器生成 `ShootProjectileGrenade`，引信后执行爆炸 Cue 并加载 `NS_Grenade_Explosion`；投掷音效由 `ThrowSound` 在本地控制端播放。

## GameplayCue 表现与 Lyra 对齐 (2026-08-23)

- Lyra 的一次性爆炸、治疗等表现由非 Actor GameplayCue Notify 统一触发，在 Cue 参数的位置播放 Niagara 和音频；本项目新增原生适配类 `UShootGameplayCueNotify_Presentation`，只负责可配置的瞬时表现，不修改 Lyra 或引擎源码，也不硬编码 `/Game/` 资产路径。
- `AShootProjectileBase::Explode` 现在只处理伤害、销毁和 `GameplayCue.Weapon.Grenade.Detonate`；爆炸 Niagara 不再同时由投射物 C++ 直接生成，避免 Listen Server 上服务器和本地表现重复播放。爆炸 Cue 蓝图配置为 `NS_Grenade_Explosion`、`sfx_Weapon_GrenadeExplosion_nl_meta` 和 `ATT_Grenade`。
- `UShootGA_ThrowGrenade` 增加了可由 `GA_Grenade` 蓝图配置的 `ThrowSound` 与 `ThrowSoundAttenuation`，只在本地控制端播放一次。本轮暂用现有迁移资产 `Explosions_Grenade_Ricochet_01` 作为投掷释放声；它是可替换配置，不代表最终音色。
- GameplayCue 管理器延后 Runtime Object Library 扫描，等待编辑器 Asset Registry 完成初始 gather；否则编辑器刚启动时 CueSet 为空，手雷爆炸和治疗 Cue 都不会找到 Notify。
- Lyra 迁移包中本项目当前没有 `NS_Heal` 与 `sfx_Heal_nl_metaPreset`，因此 `GCN_Character_Heal` 暂用现有 `NS_Pickup_Success` 验证表现链。后续获得正式治疗特效/音频后，只需替换该 Cue 蓝图的配置，不改治疗能力逻辑。

## PIE 测试方法与结果 (2026-08-23)

- TestMap 已放置 `/Game/Blueprints/Testing/BP_TestDamageHazard`，地图实例标签为 `TestDamageHazard_Fire`，位置为 `(260, 0, 0)`；蓝图使用 `NS_Fire`，Box 范围为 `100 x 100 x 60`。玩家踩入后服务器自动施加周期 GE，离开后自动移除，不需要按 `F` 或任何交互键。
- `UShootEffect_TestHazardDamagePeriodic` 仅是 PIE 验收夹具：Infinite、每 0.5 秒向 `IncomingDamage` 写入正向 `5`，通过正式 `UShootAttributeSet::HandleIncomingDamage` 验证可见火焰、护盾优先和持续掉血；它不冒充正式火焰伤害来源。离开火焰后活动效果为 0，等待后属性保持稳定。
- 本轮通过 `/Game/Blueprints/Input/Actions/IA_Interact` 对应的实际交互键 `F`，验证交互能力、Heal GE、AttributeSet 和正向治疗 Cue 的完整链路，不是直接调用加血函数：受伤后 `Health=228.12`，按 `F` 后恢复到 `276.0`；回血包随即 `bAvailable=false`、隐藏，等待 30 秒后恢复为可用且显示。
- 手雷测试重新启动 PIE 后激活实际蓝图类 `GA_Grenade_C`，日志确认服务器生成 `ShootProjectileGrenade`，引信后加载 `NS_Grenade_Explosion`。投掷声由 `ThrowSound` 播放，爆炸声由爆炸 GameplayCue 播放；两者均有项目资产引用和运行时配置。

## 暂存 audio/effect 资产审计结果 (2026-08-23)

审计以手雷蓝图、爆炸 Cue、治疗 Cue 为根，沿 Unreal Asset Registry 的实际依赖闭包判断，不以目录名猜测。

保留并作为本轮手雷表现闭包的资产：

- `Audio/AttenuationPresets/ATT_Grenade`。
- `Audio/MetaSounds/MS_Graph_TriggerDelayPitchShift_Mono`、`sfx_Weapon_GrenadeExplosion_nl_meta`。
- `Audio/Sounds/Explosions/Explosions_Grenade_Ricochet_01`。
- `Audio/Sounds/Grenade/grenade_explode_body_01..03`、`sizzle_01..02`、`thump_01..03`、`whack_01..05`。
- `Effects` 下当前手雷爆炸、轨迹、碎片、材质、网格和纹理依赖闭包中的全部新增资产。
- 已有 Audio Modulation 的 `CB_*` 与 `PP_Default*` 修改保留：Asset Registry 显示它们仍被项目现有 Audio Classes/ParameterPatches 引用，且本轮不改变其归属。

已删除并从暂存新增集合中清理的无引用资产：

- `Audio/Sounds/Explosions/MSS_Explosions_Grenade`。
- `Explosions_Grenade_Noise-Exterior-Distant_01..08`、`Noise-Exterior_01..04`、`Noise-Interior-Distant_01..04`、`Noise-Interior_01..04`。
- `Explosions_Grenade_Punch-Close_01..06`、`Punch-Distant_01..04`、`Ricochet_02..08`、`SFX_01..06`。
- `Audio/Sounds/Weapons/MS_StereoGain`、`MS_StereoHighShelf`、`MS_WaveArrayCrossfader`。

这些删除均先经过 Asset Registry 依赖检查；最后残留的 `MSS_Explosions_Grenade.uasset` 又通过 `Resolve-Path` 验证为项目目录内的单个文件后删除，没有使用通配符、递归参数或未解析路径。

# 已发现的迁移资产风险

编辑器启动日志在加载已暂存的 Audio Modulation 资产时持续报告：`CB_*` 与 `PP_Default*` 依赖 `/Script/AudioModulation`，但 `AudioModulation` 插件未挂载。这不是手雷 C++ 的编译错误；本轮通过 Asset Registry 确认这些修改中的资产仍被项目现有 `Audio/Classes/*` 和 `ParameterPatches` 互相引用，因此不删除、不把它们混入手雷清理。

本轮其余新增 audio/effect 资产均按实际依赖闭包审计，不按目录名猜测；无引用的旧爆炸 MetaSound 和其独占音频已删除，完整清单见文末。

## 更新 (2026-08-23 接管后)

- 手雷 UI 已改为独立插槽: W_DefaultHUD 新增 GrenadeExtensionPoint (ExtensionPointTag=HUD.Slot.ExtraEquipment,
  默认位置右下 QuickBar 上方, ZOrder 6); DA_Experience_DungeonTest 中 W_GrenadeCooldown 的 SlotID 已从
  HUD.Slot.Quickbar 改为 HUD.Slot.ExtraEquipment。
- HUD.Slot.ExtraEquipment 已通过 VibeUE GameplayTagService.add_tags 写入 DefaultGameplayTags.ini 并运行时注册,
  重启编辑器不失效, 无需编译。
- 手雷结构参考 Lyra (GA_Grenade + B_Grenade 投射物 + GE_Damage_Grenade 曲线 + GE_Grenade_Cooldown + GCN,
  W_GrenadeCooldown 注册到 HUD.Slot.ExtraEquipment), 详见外部恢复包 09 号文档。
## 治疗包重写 (2026-08-23, 拾取即用方案, 替代 Codex 的武器化实现)

- 旧实现(错误, 已移除): 治疗包 = Equippable/WeaponInstance 占 QuickBar 槽、按 InputTag.Weapon.Fire 消耗, 改动 UCombatComponent::ConsumeActiveRuntimeItem。
- 新实现: AShootHealthpackPickup(战斗内可配置拾取物, IInteractableTarget) -> 交互链动态授予
  UShootGA_Interaction_HealthPack -> 服务器应用 UShootEffect_HealInstant(SetByCaller.Heal, 默认 50)
  -> 默认隐藏并在 RespawnDelay(30 秒)后恢复；RespawnDelay=0 时销毁拾取物。满血时不提供交互选项。
- 资产规划: BP_HealthpackPickup(父 AShootHealthpackPickup, 引用 SM_healthpackFull/MI_Item_HealthPackFull)
  放 /Game/Weapons/Healthpack/Blueprint/; 网格/材质/贴图留在 /Game/Weapons/Healthpack/{Mesh,Material,Texture}。
- 旧的武器化资产已按 Asset Registry 逐项检查，`GA_ConsumeHealthPack` / `AS_Healthpack` /
  `AS_Healthpack_Use` / `BP_Equipment_Healthpack` / `ID_Healthpack` /
  `B_WeaponInstance_Healthpack` / `BP_WeaponPickup_Healthpack` 均不存在；地图中的有效放置体为
  `BP_HealthpackPickup`，继续保留。
- 后续: 治疗垫(恢复站)复用 AShootSkillMedicalStation(女主 X 技能 MedicalStation 的现有骨架), 需补技能冷却与视觉。

## 手雷冷却按钮初始进度修复 (2026-08-23)

- `W_GrenadeCooldown` 的 `ActionTouchButton` 子控件使用的是 `W_ActionTouchButton` 资产上定义的 `UpdateCoolDownPercent` 函数；真实冷却开始后仍由 `Ability.Grenade.Duration.Message` 和 Widget Tick 更新进度。
- 冷启动灰态的实际根因是：`UpdateCoolDownPercent(1.0)` 只写入 `Animate_CoolDown`，而 `W_ActionTouchButton` 的 `ResetMaterials` 仍让 `ItemGlow` 和 `WeaponCard` 上的 `Animate_ActiveToInactive` 保持 `1.0`。因此进度虽然是 100%，不可用态遮罩仍在。
- 修复放在 `/Game/UI/Weapon/W_ActionTouchButton` 的 `Event Construct`：子控件初始化后分别取得 `ItemGlow`、`WeaponCard` 的动态材质，并把 `Animate_ActiveToInactive` 写为 `0.0`；共享冷却材质默认值不改。原有 `UpdateCoolDownPercent` 继续负责真实冷却进度。
- 冷启动 TestMap Listen Server PIE 已实测：手雷按钮亮蓝色可用，实时动态材质值为 `Animate_ActiveToInactive=0.0`、`Animate_CoolDown=1.0`；按钮不再依赖先收到冷却消息才能亮起。

## 护盾优先与回血包重生 (2026-08-24)

- `UShootAttributeSet::HandleIncomingDamage` 现在严格按 `Shield -> Health` 结算：先从当前 `Shield` 扣除伤害，剩余部分才写入 `Health`；`ShieldCapacity` 仍表示上限，不作为当前护盾资源使用。
- `UShootEffect_TestHazardDamagePeriodic` 已改为写入 `IncomingDamage`，因此 TestMap 可见火焰不需要按 `F`，踩入范围即可进入正式伤害后处理并验证护盾先掉、护盾耗尽后血量再掉。
- `AShootHealthpackPickup` 新增复制的可用状态和服务器重生计时器。`RespawnDelay` 默认值为 `30.0` 秒；设置为 `0.0` 时保持一次性行为并销毁 Actor。消耗期间会隐藏视觉、关闭查询碰撞和 overlap，重生时恢复。
- 本轮还修正了环境伤害上下文：测试火焰以自身作为 Instigator，避免把环境伤害误判为玩家对自己的友伤并被友伤系数清零。
- Windows `NewWorldOrder` 与 `NewWorldOrderEditor` 均使用 `-DisableUnity -NoUBA` 冷编译成功；本轮 PIE 实测初始 `Health=276.0 / Shield=102.12`，踩火约 2 秒后为 `276.0 / 32.12`，再过约 2 秒为 `248.12 / 0.0`，证明护盾耗尽前不会扣血；治疗包默认 `RespawnDelay=30.0`，消费后隐藏，30 秒后恢复可用和显示。

## 手雷冷却消息监听修复 (2026-08-25)

- `W_GrenadeCooldown` 的 `ActionTouchButton` 子控件沿用 `/Game/UI/Weapon/W_ActionTouchButton`；后者原本已经有 `ListenForDuration`，会订阅 `Ability.Grenade.Duration.Message`，校验消息的 Instigator 后写入 `Duration`、`StartTime`，再由 Tick 更新 `Animate_CoolDown`。
- 实际故障是 `W_ActionTouchButton` 的 `EventOnInitialized` 执行线断开，虽然 `DurationMessageTag` 有效性判断和 `ListenForDuration` 节点都存在，却从未执行。手雷的冷却 Gameplay Effect 已经生效，但按钮只执行初始化时的 `UpdateCoolDownPercent(1.0)`，所以视觉一直显示为可用。
- 已将 `EventOnInitialized` 接回现有的 `DurationMessageTag` 有效性分支，保留原有消息过滤和材质动画逻辑，不在 `W_GrenadeCooldown` 里重复实现冷却状态。
- PIE 验证：实际运行中的按钮进入 `Charging=true`，`Duration=10.0`；冷却约进行到 2.68 秒时，动态材质 `Animate_CoolDown` 已更新到约 `0.268`，并进入 `Animate_ActiveToInactive` 状态，证明 UI 已重新接收到真实冷却消息并随时间推进。
