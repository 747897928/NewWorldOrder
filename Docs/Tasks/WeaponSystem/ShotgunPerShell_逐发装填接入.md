# Shotgun A 逐发装填第一纵切

状态：功能验证版已提供玩家拾取入口；2026-09-11 修正空仓逐发换弹的首发开火门禁，并修正 Shotgun Ability CDO 的 GameplayTag 初始化，待本次 Live Coding 后完成编译与双端 PIE 复验；远端动画视觉和动画修型待后续验收

原始日期：2026-08-25
最近更新：2026-09-11

# 1. 范围和结论

本阶段把商城泵动式霰弹枪 `Shotgun_A` 接入现有 Lyra 派生武器链，完成以下最小闭环：

- RuntimeOnly `ItemDefinition -> EquipmentDefinition -> WeaponInstance -> WeaponActor`。
- `AbilitySet` 授予项目霰弹枪开火 GA、逐发装填 GA、共享按住 ADS GA 和共享 AutoReload GA。
- 弹药仍只使用 `UShootInventoryItemInstance` 的弹匣、备弹 StatTagStack，没有建立第二份 Actor 弹药状态。
- 角色 Reload Montage 使用 `ShotgunStart -> ShotgunLoop -> ShotgunEnd` 三段结构。
- 每个 `UShootAnimNotify_InsertShell` 只在弹壳实际送入弹仓的提交点发送 `GameplayEvent.Reload.InsertShell`；服务器每次只调用 `UShootRangedWeaponInstance::ReloadAmmo(1)`。
- 角色 Montage 上的项目 `UShootAnimNotify_PlayWeaponMontage` 子类先播放武器 Montage，再通过 `MontageSync_Follow` 让武器跟随角色时间轴。
- 非空弹匣换弹可被开火立即取消；Notify 前尚未提交的那一发不会增加。空仓开始换弹时只在首个 `InsertShell` 提交前临时加入 `Ability.Weapon.NoFiring`；第一发进入弹匣后即可由开火取消逐发换弹。
- Reload 是一次按下后持续逐发装填，不要求一直按住 R；装满、备弹耗尽或显式结束事件时进入 `ShotgunEnd`。非空弹匣仍可由 Fire 立即取消，未经过提交点的弹药不会增加。

本阶段没有实现独立的膛内弹药。当前数据模型只有“弹匣/管式弹仓数量 + 备弹数量”，符合用户要求的较简单游戏化规则，也避免在没有设计需求时把泵动状态、膛内状态和网络预测扩成第三套弹药状态。

# 2. 常见游戏规则依据

没有找到可核验的《穿越火线》或《反恐精英 Online》公开源码，因此不能把它们的精确行为写成已确认事实。可核验的 Valve 开源实现提供了相近规则参考：

- GoldSrc `weapon_shotgun.cpp` 使用弹仓数量与备弹数量，换弹分 Start/Insert/End 状态，每轮插入一发；开火会离开换弹状态。
- Source SDK 2013 `weapon_shotgun.cpp` 同样逐发转移弹药并在结束阶段执行泵动；玩家在换弹期间按开火会进入打断或排队开火流程。

参考：

- `https://github.com/ValveSoftware/halflife/blob/master/dlls/shotgun.cpp`
- `https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/hl2/weapon_shotgun.cpp`

项目第一纵切采用的规则是：已经经过 InsertShell Notify 的弹药永久保留；尚未经过提交点的本轮装填在取消时不计入；非空弹匣允许立刻打断并开火；空仓阶段在首发提交前禁止开火，首发提交后允许立刻打断并开火。当前仍不实现独立的 Source 风格“输入排队”状态，不能隐式塞进动画或 Actor 蓝图。

# 3. 资产闭环

## 3.1 武器配置

- `/Game/Weapons/Shotgun_A/ID_Shotgun_A`
- `/Game/Weapons/Shotgun_A/BP_Equipment_Shotgun_A`
- `/Game/Weapons/Shotgun_A/B_WeaponInstance_Shotgun_A`
- `/Game/Weapons/Shotgun_A/B_Shotgun_A`
- `/Game/Weapons/Shotgun_A/BP_WeaponPickup_Shotgun_A`
- `/Game/Weapons/Shotgun_A/AS_Weapon_Shotgun_A`

`ID_Shotgun_A` 当前保留现有参考 Shotgun 的平衡参数：弹仓 8、备弹 16、每次开火消耗 1、每壳 9 个弹丸。商城包没有提供可证明应覆盖这些数值的设计数据，因此没有凭网格外观猜测新平衡。

`ID_Shotgun_A` 没有加入账号永久出战目录。两张测试地图只通过 `BP_WeaponPickup_Shotgun_A` 产生 RuntimeOnly 物品，不修改 Persistent QuickBar 或 SaveGame：

- `/Game/Maps/TestMap_ListenServer`：`DungeonPickup_Shotgun_A`，位置 `(350, 500, 70)`。
- `/Game/Maps/TestMap_SplitScreen`：`DungeonPickup_Shotgun_A_P1` 和 `DungeonPickup_Shotgun_A_P2`，位置分别为 `(350, 500, 70)`、`(590, 500, 70)`。

拾取物世界标签为 `Shotgun A`，交互文本为“拾取霰弹枪”。

## 3.2 动画资产

- 女性角色：`/Game/Characters/Heroes/CC/MF/Animations/Weapons/Montages/AM_MF_Shotgun_A_Reload`、`AM_MF_Shotgun_A_Fire`。
- 男性角色：`/Game/Characters/Heroes/CC/MM/Animations/Weapons/Montages/AM_MM_Shotgun_A_Reload`、`AM_MM_Shotgun_A_Fire`。
- 男性离线重定向源：`/Game/Weapons/Shotgun_A/Animations/Character/MM/Source/MM_Reload_Shotgun_Ironsights`、`MM_Fire_Shotgun_Ironsights`。
- 武器 Montage：`/Game/Weapons/Shotgun_A/Animations/Montages/AM_Weap_Shotgun_A_Reload`、`AM_Weap_Shotgun_A_Fire`。
- 武器 AnimBP：`/Game/Weapons/Shotgun_A/Animations/ABP_Weap_Shotgun_A`。
- 同步 Notify 蓝图：`/Game/Weapons/Shotgun_A/Animations/Notifies/AN_PlayShotgunA_ReloadMontage`、`AN_PlayShotgunA_FireMontage`。
- 男性重定向器：`/Game/Weapons/Shotgun_A/Animations/Retargeting/RTG_ShenWanYun_To_ChenHaoYu_ShotgunA`。

男女 Reload Montage 均为 3.433317 秒，角色 Slot 为 `UpperBody`，武器 Slot 为 `DefaultSlot`：

| Section | 时间范围 | 下一段 | 作用 |
| --- | ---: | --- | --- |
| `ShotgunStart` | 0.000000–2.200000 | `ShotgunLoop` | 进入装填姿势并完成第一发送入动作 |
| `ShotgunLoop` | 2.200000–2.733333 | 自循环 | 每次循环提交一发 |
| `ShotgunEnd` | 2.733333–3.433317 | 无 | 护木收尾并返回持枪姿势 |

第一发提交 Notify 位于 2.166667 秒，Loop 提交 Notify 位于 2.700000 秒。收到提交事件后，GA 根据弹仓和备弹决定跳回 `ShotgunLoop` 或进入 `ShotgunEnd`。

武器源 Reload 动画保留商城包原有的 `Shotgun_Reload_In_Cue` 与 `Shotgun_Reload_Out_Cue`；Fire 动画保留开火声音、泵动声音和 `P_Shotgun_MuzzleFlash_01`。它们是角色动作、武器机械骨骼、音效和 Cascade 的表现闭包，本阶段没有删除 Notify 或把 Cascade 假装替换为 Niagara。

旧的 `/Game/Weapons/Shotgun_A/Animations/Character/MF|MM/AM_*` 包当前是本次受控移动产生的 `ObjectRedirector`。`ID_Shotgun_A` 已直接依赖新的正式 CC 路径，PIE 也从新路径播放。为避免在商城资产总重定向器复核完成前混入删除操作，本轮保留这四个重定向器。

# 4. C++ 调用链

```text
UShootGA_Weapon_AutoReload::CheckAutoReload
  -> 弹仓为 0、备弹大于 0、达到开火后等待时间
  -> 从同一 WeaponInstance SourceObject 查找 UShootGA_Reload_ShotgunPerShell Spec
  -> TryActivateAbility（Spec 已运行时不重复请求）

InputTag.R
  -> UShootGA_Reload_ShotgunPerShell
  -> 普通点击只负责启动，不监听 InputRelease 提前结束
  -> WeaponInstance::GetCharacterReloadMontage
  -> AbilityTask_PlayMontageAndWait(ShotgunStart)
  -> UShootAnimNotify_PlayWeaponMontage
       -> 武器 AnimInstance 播放 AM_Weap_Shotgun_A_Reload
       -> MontageSync_Follow(角色 AnimInstance)
  -> UShootAnimNotify_InsertShell
  -> GameplayEvent.Reload.InsertShell
  -> UShootGA_Reload_ShotgunPerShell::OnInsertShellEvent
  -> 服务器 WeaponInstance::ReloadAmmo(1)
  -> ShotgunLoop 或 ShotgunEnd
```

涉及项目类：

- `UShootGA_Reload_ShotgunPerShell`：负责可激活条件、Start/Loop/End、逐发弹药提交、取消和空仓射击门禁。
- `UShootGA_Weapon_AutoReload`：全部武器共享的 OnSpawn 被动；对 Shotgun_A 只负责空仓激活同一 SourceObject 的逐发 GA，不修改受保护的弹匣式 Reload，也不伪造输入。
- `UShootAnimNotify_InsertShell`：只发送动画提交事件，不直接改弹药。
- `UShootGA_Weapon_Fire_Shotgun`：激活时取消带 `Ability.Type.Action.Reload` 的能力，并受 `Ability.Weapon.NoFiring` 阻止。该子类与逐发 GA 的 CDO 初始化必须直接按稳定名字请求 GameplayTag，不能在 CDO 初始化阶段读取可能尚未填充的 `FShootGameplayTags` 单例；否则取消容器、阻挡容器和逐发事件 Tag 会固化为空。所有这些标签必须先登记在 `Config/DefaultGameplayTags.ini`，`FGameplayTag::RequestGameplayTag(FName(...), false)` 只查询已注册标签，不负责创建配置项。

# 5. 验证证据

## 5.1 编译和重载

- `Scripts/Build_Windows.ps1` 完成 UE 5.8 Editor 编译，UHT 与 6 个编译动作全部成功。
- 重启 NewWorldOrder 编辑器后，新 C++ Notify 与逐发 GA 可加载。
- 新建蓝图、AnimBP、Montage、ItemDefinition、Equipment 和 AbilitySet 均能从重新加载后的包解析；`ABP_Weap_Shotgun_A` 为 UpToDate。

## 5.2 单人 PIE

- RuntimeOnly 发放后，QuickBar 激活实例为 `B_WeaponInstance_Shotgun_A_C`，生成唯一 `B_Shotgun_A_C`。
- 武器 Actor 使用 `/Game/Weapons/Shotgun_A/Mesh/Shotgun_A` 和 `ABP_Weap_Shotgun_A_C`；初始弹药为 `8/16`。
- 从 `5/12` 开始换弹后，循环提交结果为 `8/9`，证明三次 Notify 各转移一发，没有整匣一次填满。
- 全局时间膨胀设为 `0.01` 后采样：角色与武器 Reload Montage 同时为 `0.16000` 秒；后续采样同时为 `1.58333` 与 `2.63333` 秒，证明 Montage Sync Follow 实际生效。
- 非空弹匣测试：Reload 激活时为 `7/6`；同帧激活 Fire 成功，角色 Montage 从 Reload 切换为 Fire，弹药变为 `6/6`，未经过 Notify 的装填没有增加。
- 空仓测试：旧验证曾确认 `0/6` 时整段换弹均禁止开火；2026-09-11 修正后，目标行为改为首个 `InsertShell` 前禁止开火、首个 `InsertShell` 后允许 Fire 取消 Reload。编辑器中静态回读还发现旧 CDO 的取消/阻挡/事件 Tag 曾为空，本轮已修正初始化路径；本次 Live Coding 后必须重新复验该时序。
- PIE 结束后已恢复时间膨胀为 1.0 并停止 PIE。

日志中的 Python `AttributeError` 来自测试脚本早期调用不存在的反射方法，未触发引擎断言、Ensure、蓝图编译错误或资产删除。正式运行链验证使用了实际暴露的 `GetCurrentAmmo`、`GetCurrentReserve` 和当前 Montage API。

## 5.3 玩家输入与地图入口复验

本轮不再用直接调用 GA 代替玩家输入。测试通过项目 `IMC_Default` 的正式 InputAction 或对应真实按键注入第一名 LocalPlayer：

- `F` 或 `IA_Interact`：在 `TestMap_ListenServer` 进入拾取碰撞后，拾取物消失，生成唯一 `B_Shotgun_A_C`，QuickBar 激活实例为 `B_WeaponInstance_Shotgun_A_C`。
- `LeftMouseButton`：初始 `8/16` 开火后为 `7/16`，角色播放新正式路径 `AM_MF_Shotgun_A_Fire`；HUD 同步显示 7 发。
- `IA_Reload`：它是键盘 `R` 的正式映射；`7/16` 完成一次逐发装填后为 `8/15`。
- 非空打断：测试设置为 `5/8` 后，用实际 `IA_Reload` 激活并把角色 Anim Rate 暂时降到 `0.01` 取样；随后输入 `IA_Attack`，结果为 `4/8`，角色 Montage 从 Reload 切到 Fire，证明未到 InsertShell 提交点的那一发没有增加。
- 空仓门禁：`0/8` 激活 Reload 后，在首个 `InsertShell` 前输入 `IA_Attack` 应保持 `0/8`；首个 `InsertShell` 后再次输入 `IA_Attack` 应取消 Reload 并消耗已装入的一发。
- `IA_Crouch`：持有 Shotgun_A 时 `is_crouched` 能在 `false -> true -> false` 间切换。
- `IA_Jump`：持有 Shotgun_A 时 CharacterMovement 进入 `MOVE_FALLING`。
- `W`：真实键盘事件由游戏输入链处理，角色位置发生变化。自动化长按受编辑器无焦点运行速度影响，只用于证明输入链，不作为移动手感或动画美术验收。
- 测试结束前恢复了角色和武器 `GlobalAnimRateScale=1.0`、世界时间膨胀 `1.0`，并停止 PIE。

2026-08-28 回归纠正了 `3737abd3` 引入的按住式语义：`UAbilityTask_WaitInputRelease` 会在玩家普通点击 R 后、首个 `InsertShell` 之前把 Montage 跳到 End，造成“动作播放但弹药不增加”。当前已删除该等待任务，男女 Reload Montage 各自仍保留 2 个 `UShootAnimNotify_InsertShell`。Listen Server Host 实际输入复验得到：`7/16 -> 8/15`；随后从 `3/12` 连续提交到 `5/10`，开火打断后为 `4/10`。这证明 Host 权威/本地统一实例链正确，不替代仍待完成的远端拥有客户端复制验收。

同日已把共享 `UShootGA_Weapon_AutoReload` 授予 `AS_Weapon_Shotgun_A`。Listen Server Host 将运行时弹药设为 `0/24` 后，被动激活同一 WeaponInstance SourceObject 的逐发 GA，完整动画结束为 `8/16`；设为 `0/0` 后连续观察 3 秒，Reload 保持未激活且弹药不变。逐发流程约 8 秒，短于首个提交点的采样不能用来判定 AutoReload 失败。Equipment 卸载时会通过 GrantedHandles 清除旧装备授予的 AutoReload 与 Reload Spec；远端拥有客户端仍需单独验证弹药复制和 Montage 同步。最终代码已通过 UE 5.8 Editor Target 冷编译；冷启动后 AbilitySet 复读为 4 个能力，其中包含逐发 Reload、ADS 和共享 AutoReload。

运行时 WeaponActor 的 SkeletalMesh 为 `/Game/Weapons/Shotgun_A/Mesh/Shotgun_A`，材质为 `/Game/Weapons/Shotgun_A/Materials/M_Shotgun_01`。这是用户修正后的材质配置，本轮只做读取和运行验证，没有覆盖该修改。

## 5.4 Split Screen 隔离复验

- `/Game/Maps/TestMap_SplitScreen` 启动后存在两个本地 `PlayerController`、两个角色和两个 Shotgun_A 拾取物。
- Player 0 通过正式 `IA_Interact` 拾取第一把后，Player 0 的 Active Weapon 为 `B_WeaponInstance_Shotgun_A_C`、槽位索引为 0。
- Player 1 仍为 `Active=-1`、无 Active Weapon，第二个 Shotgun_A 拾取物仍存在，证明本轮入口没有串到另一个 LocalPlayer。
- `capture_image` 在该双本地玩家 PIE 窗口返回 Slate 截图失败，因此本条只记运行对象和 LocalPlayer 隔离证据，不伪造视觉截图结论。

# 6. 玩家手工验收入口

## 6.1 Listen Server 测试地图

1. 打开 `/Game/Maps/TestMap_ListenServer` 并 Play。
2. 出生后前方一排为 Rifle、Pistol、Shotgun、Shotgun A；Shotgun A 位于 `(350, 500, 70)`，世界标签为 `Shotgun A`。
3. 走入拾取球体并按 `F`。
4. 用鼠标左键开火、`R` 逐发装填、`C` 蹲下、空格跳跃、WASD 移动。
5. 弹仓仍有弹时，按 `R` 后立即左键：换弹应被打断并开火，尚未送入的弹不增加。
6. 弹仓为 0 时按 `R` 后立即左键：当前规则应阻止开火，继续完成装填。
7. 弹仓为 0 且有备弹时不要按 `R`：等待开火后的保护间隔后，应自动开始逐发装填并持续到装满或备弹耗尽。
8. 弹仓和备弹均为 0 时，不应激活 Reload 或循环播放空换弹。

## 6.2 Split Screen 测试地图

1. 打开 `/Game/Maps/TestMap_SplitScreen` 并 Play。
2. 地图会创建两个 LocalPlayer；Shotgun A 在两列武器的最后一行，各有一个拾取物。
3. 每名玩家使用自己的交互输入拾取对应 Shotgun A，确认 QuickBar、弹药、角色 Montage 和武器表现互不串线。

本轮已把入口和自动化运行证据补齐，但用户手工视觉验收仍是最终标准。当前角色动作没有完成上半身与 Lyra/CC 下半身的离线合成，不能把手脚贴合、站姿或美术品质标记为完成。动画试水任务见 `ShotgunAnimationBlend_动画合成试水.md`。

# 7. 未完成项和验收边界

- Listen Server Host 已完成手动逐发、开火打断、空仓 AutoReload 和零备弹边界；尚未做远端拥有客户端的逐发入弹、取消预测、弹药复制和远端 Montage 同步验收，Host 结果不能替代双端完成声明。
- 两张测试地图已有 RuntimeOnly 固定拾取入口；`ID_Shotgun_A` 仍未加入账号永久出战目录，这是有意保留的副本边界。
- 2026-09-11 已修正空仓逐发换弹门禁：`UShootGA_Reload_ShotgunPerShell` 在首个服务器权威或本地预测 `InsertShell` 提交点后移除 `Ability.Weapon.NoFiring`；同时修正 Shotgun Fire/PerShell Reload 在 GameplayAbility CDO 初始化阶段读取空 Tag 单例的问题，恢复 `Ability.Type.Action.Reload` 的取消匹配、`Ability.Weapon.NoFiring` 的开火阻挡和 `GameplayEvent.Reload.InsertShell` 的事件监听。未修改通用弹匣式 Reload、AutoReload 或其它武器。本次 Live Coding 后仍需 Listen Server Host 与远端拥有客户端分别验收，尤其要确认客户端收到首发提交后不会因复制延迟导致 Fire 成本检查失败。
- 当前仍不实现输入排队：首发提交后允许 Fire GA 取消当前 Reload 并立即射击；尚未提交的动画装填不会被伪造为已入弹。
- 当前商城角色 Reload 动画没有完成项目骨架的 `weapon_r/weapon_l` 对齐，运行时可见枪体旋转或手部贴合问题。用户计划把 Rifle Reload 下半身与 Shotgun_A 上半身在 Blender 中融合后重新导入；因此本轮不对即将替换的正式动画做破坏性骨骼修型，只把问题记录为动画资产任务。
- 角色 Fire/Reload 目前使用商城动作的完整下半身。用户要求的最终动画应保留商城上半身的装弹或开火动作，同时融合现有 Lyra/CC 动画的脚距、pelvis 高度与重心、胸腔、肩肘姿势；MF 与 MM 各产出一份离线烘焙动画。该工作需要 Control Rig 或 Blender 视觉 A/B、骨骼朝向检查和用户验收，优先级晚于功能链，不能用 AnimBP 运行时分层冒充完成。
- Cascade 审计结论不变：`P_Shotgun_MuzzleFlash_01` 没有一对一 Niagara 等价证据，继续保留。
- 狙击枪、榴弹发射器和火箭筒已完成独立的 GAS/GE、弹药和表现资产迁移；它们的 Listen Server、真实输入和远端投射物验收不属于本纵切，详见 `SpecialWeapons_GE迁移与Fragment字段审计.md`。
