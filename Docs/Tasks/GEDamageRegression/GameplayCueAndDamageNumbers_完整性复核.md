# 伤害数字与 GameplayCue 完整性复核

更新：2026-08-22

## 复核范围

- 项目服务器权威扣血到 Niagara 伤害数字的完整链。
- Lyra 的 `GameplayCue.Character.DamageTaken`、通用武器 Impact Cue 和三把枪 Fire Cue。
- 项目 Rifle、Pistol、Shotgun 的 Fire/Impact Tag、Cue 资产和 `B_Weapon` 表现入口。
- 本轮只判断表现闭包是否完整，不把 GE 伤害公式、距离衰减和弱点倍率的迁移混进同一个结论。
- 2026-08-22 GE 数值链已经单独落地；本文件列出的 Cue 缺口仍然有效，后续必须保持 `B_Weapon` 为 Impact Niagara/Decal 唯一主入口。

## 结论矩阵

| 子系统 | 状态 | 结论 |
|---|---|---|
| Niagara 伤害数字 | 功能闭环，视觉隔离待最终截图 | 服务器按实际掉血量生成请求，只发给伤害来源 LocalPlayer；真实射击已证明实际扣血与 NiagaraActor 生成，代码复读证明每 Controller 实例与 `HiddenActors` 隔离。分屏/Listen Server 的逐帧视觉归属仍需可稳定截获的最终证据。 |
| WeakSpot 数字样式 | 已闭环 | `Gameplay.Zone.WeakSpot` 被编码为负 W，Niagara 使用暴击样式。 |
| Fire Cue 基础武器表现 | 已闭环 | 三把枪 Cue 能找到当前 `UShootWeaponInstance` 的表现 Actor，并调用 `B_Weapon.Fire`。 |
| Fire Cue 本地反馈层 | 最小闭环 | Pistol/Rifle/Shotgun 已按 Lyra 配置接入各自 CameraShake 与 ForceFeedback，并以 `InstigatorActor` 本地控制条件隔离 LocalPlayer；设备反馈和完整音频仍未迁入。 |
| Impact 视觉 | 已有基础链 | `B_Weapon.Fire` 会进入 `B_WeaponImpacts`、`B_WeaponDecals`，处理 Tracer、Impact 与 Decal。 |
| Impact Cue 反馈层 | 首期已闭环 | 三把枪已与 Lyra 一致共用 `GameplayCue.Weapon.Rifle.Impact`；项目 GCN 按角色/环境表面播放命中 SoundCue，不重复 Niagara 与贴花。 |
| Character DamageTaken Cue | 最小闭环 | 服务器最终发生实际 Health 损失后执行 Cue；GCN 只播放 Lyra 的 CameraShake 与 ForceFeedback，不重复 NumberPop。 |

## Listen Server 真实输入验收

2026-08-22 已使用真实 PIE 输入而不是 Python 直接激活 Ability 完成远端客户端闭环：

- Client 1 聚焦自己的 Slate PIE 视口后按 `F`，经 Enhanced Input → InputTag → ASC 拾取 Rifle；服务端远端 Pawn 与客户端本地 Pawn 均得到 30/60，Host 不持有该武器实例。
- Client 1 真实左键开火后，服务端与客户端弹匣同步 `30 → 29 → 28`。
- 第二枪命中敌人弱点后，服务器和客户端 Health 同步 `276 → 258`，实际损失 18 与 Rifle 近距弱点公式一致。
- 直接从客户端 Python 调用预测型 `try_activate_ability` 会绕开项目输入入口并造成预测键递归，已明确禁止用于后续网络验收。

这组证据关闭了“远端客户端能否经正常输入触发服务器权威 GE 并复制结果”的缺口；Niagara 伤害数字的逐帧视觉归属仍按独立验收项保留，不能仅凭血量复制推断完成。

## 分屏伤害数字运行时复核

2026-08-22 在 `TestMap_SplitScreen` 使用 P0 的真实 `F` 与真实左键完成一次 Rifle 命中：

- 弹匣 `30 -> 29`，证明不是 Python 直接调用 Ability 的旁路。
- 敌人 `Health 276 -> 258`，`IncomingDamage` 在结算后归零，实际 Health 损失为 18；显示值来源是 `PreviousHealth - FinalHealth`，不是旧固定 10。
- 命中后只生成一个临时 `NiagaraActor`，其 System 为 `/Game/Effects/Particles/Impacts/NS_DamageNumbers`。
- 该帧之后 Niagara 已消费并清空 `DamageInfo` 数组，因此事后查询空数组不能反推“没有生成数字”。

当前 MCP 没有暴露 `APlayerController::HiddenActors` 的运行时读取接口，瞬时 NumberPop 也未在自动截图中稳定截获。因此这里只关闭“真实伤害值进入项目 NumberPop Niagara”的运行时缺口；P0 可见、P1/Host 不可见的逐帧视觉隔离仍保留为人工/高速录屏验收项，不能由代码结构冒充视觉验收。

## 项目伤害数字事实

当前链路为：

```text
UShootAttributeSet::HandleIncomingDamage
-> 计算 PreviousHealth - FinalHealth
-> SendDamageNumberFeedback
-> 只接受伤害来源 AShootPlayerController
-> ClientAddDamageNumber
-> UShootNumberPopComponent_NiagaraText::AddNumberPop
-> 每个 LocalPlayer 复用自己的临时 NiagaraActor 和 DamageInfo 数组
```

项目刻意没有照搬 Lyra 的 `GCNL_Character_DamageTaken -> AddNumberPop` 蓝图入口，而是在最终 Health 扣减后发送实际伤害。这保证：

- 友伤系数为零、免疫或没有实际掉血时不显示数字。
- 过量伤害显示实际 Health 损失，不显示尚未落地的理论伤害。
- 伤害数字只属于伤害来源 Controller；AI、环境伤害或没有玩家来源的伤害不会创建玩家私有数字。
- 每个本地 Controller 只维护自己的 Niagara 实例；其他本地 Controller 通过 `HiddenActors` 隔离，不会在分屏中串屏。

已有验收证据位于：

- `Docs/Tasks/LyraShooterCoreAdaptation/Status_状态.md`
- `Docs/Tasks/LyraShooterCoreAdaptation/Verification_验收.md`

因此伤害数字不需要再引入第二套 GameplayCue 路由。以后若迁入 `GCNL_Character_DamageTaken`，必须关闭其中的 `Add Number Pop` 分支，避免一次命中生成两份数字。

## Lyra Weapon Impact Cue 的真实职责

11000 只读实例确认：

- Lyra `GA_Weapon_Fire` 先向武器拥有者执行一次 Fire Cue，再遍历 TargetData 对每个命中执行其 `GameplayCue_Impact` 变量。
- 通用资产为 `/Game/GameplayCueNotifies/GCN_Weapon_Impact`，不是 Pistol、Rifle、Shotgun 各复制一份。
- 11000 CDO 复读确认一个容易误判的事实：手枪基类、Rifle Auto 与 Shotgun 的 `GameplayCue_Impact` 均为 `GameplayCue.Weapon.Rifle.Impact`；GCN 自身也绑定该标签。资产名通用，但标签名沿用了 Rifle。
- 该 GCN 的 EventGraph 为空，表现由 `GameplayCueNotify_Burst` 的 `BurstEffects` 配置完成。
- BurstEffects 按 `SurfaceType_Default`、`SurfaceType2`、`SurfaceType3` 分流命中声音，并配置 `CS_Weapon_Fire` 与 `FFE_Weapon_Fire`。
- 其三个 Particle 条目只有 Surface 条件，没有实际 Niagara 资产；主要粒子、Tracer 和 Decal 仍由 `B_Weapon.Fire -> B_WeaponImpacts/B_WeaponDecals` 负责。

所以迁入通用 Impact Cue 不会取代 `B_Weapon`，也不应再生成第二套相同 Impact Niagara。项目首期只补逐命中的表面音频；Lyra 的 CameraShake 与 ForceFeedback 依赖在项目中不存在，不能用无资源空配置冒充完成。

## Lyra Fire Cue 与项目适配器差异

Lyra 三个 `GCN_Weapon_*_Fire` 都继承 `GameplayCueNotify_Burst`，除调用 `B_Weapon.Fire` 外还包含：

- 每把枪不同强度的 `Send Weapon Fire`：Pistol `0.1`、Rifle `0.15`、Shotgun `0.2`。
- `Set Weapon Sound Params`。
- Early Reflections。
- Whiz By。
- CameraShake 与 ForceFeedback；Rifle 还有单独的 Fire Audio 调用。

项目三个 GCN 使用 `UShootGameplayCueNotify_WeaponFire` 作为父类。该 C++ 类只负责从 Cue 的 `SourceObject` 找到 `UShootWeaponInstance` 对应表现 Actor，再反射调用 `B_Weapon.Fire`；它不等价于 Lyra 完整的 Burst 配置与蓝图节点。

## Fire Cue 依赖闭包决策

2026-08-22 通过 Lyra 11000 只读 MCP 递归审计得到：

- Pistol Fire GCN：370 个包。
- Rifle Fire GCN：408 个包。
- Shotgun Fire GCN：363 个包。
- Character DamageTaken GCN：259 个包。
- 通用 Weapon Impact GCN：81 个包。

直接迁移任一完整 GCN 都会把大型音频、MetaSound、设备反馈和其他 ShooterCore 依赖一起带入，因此本项目没有把“能迁移”误当成“应该全量复制”。本阶段只取零递归 `/Game` 依赖的反馈资产：

- `/Game/Feedback/CameraShakes/CS_Weapon_Fire_Pistol`
- `/Game/Feedback/CameraShakes/CS_Weapon_Fire_Rifle`
- `/Game/Feedback/CameraShakes/CS_Weapon_Fire_Shotgun`
- `/Game/Feedback/CameraShakes/CS_Weapon_Fire`
- `/Game/Feedback/Haptics/FFE_Weapon_Fire`
- `/Game/Feedback/Haptics/Weapon_Fire_Auto/FFE_Weapon_Fire_Auto`
- `/Game/Feedback/CameraShakes/CS_Character_DamageTaken`
- `/Game/Feedback/Haptics/FFE_Character_Damage`

后两项已接入 Character DamageTaken 的项目化最小闭包；完整的 Lyra 受击粒子、音频、Lens、Montage 和 GameplayMessage 仍未迁入。

## 2026-08-22 已实施

1. Pistol、Rifle、Shotgun 的 `ImpactGameplayCueTag` 统一为 Lyra 实际使用的 `GameplayCue.Weapon.Rifle.Impact`，不再注册两条不存在于 Lyra 参考链的武器专用标签。
2. 新增 `/Game/Blueprints/GameplayCueNotifies/GCN_Weapon_Impact`，父类为 `GameplayCueNotify_Burst`，位于 GameplayCueManager 已扫描路径。
3. `SurfaceType1 (Character)` 使用 `Rifle_ImpactBody_Cue`；`Default/Concrete/Glass` 使用 `Rifle_ImpactSurface_Cue`。
4. GCN 的 `BurstParticles` 与 `BurstDecal` 保持为空；`B_WeaponImpacts` 继续作为 Impact Niagara/Decal 的唯一入口。
5. 冷编译成功；重启后复读三把枪 CDO 均为同一有效 Impact Tag，GCN 为 `BS_UP_TO_DATE`；PIE 启停无本链 GameplayCue 错误。
6. `UShootGameplayCueNotify_WeaponFire` 改为继承 `UGameplayCueNotify_Burst`：父类播放蓝图可配置的本地反馈，项目子类继续只做 `WeaponInstance -> B_Weapon.Fire` 适配。
7. 三把枪 Fire GCN 已配置：
   - Pistol：`CS_Weapon_Fire_Pistol`、`FFE_Weapon_Fire`。
   - Rifle：`CS_Weapon_Fire_Rifle`、`FFE_Weapon_Fire_Auto`。
   - Shotgun：`CS_Weapon_Fire_Shotgun`、`FFE_Weapon_Fire`。
8. 三组 SpawnCondition 均保持 Lyra 的 `LocallyControlledSource=InstigatorActor`、`ChanceToPlay=1`；CameraShake 为 CameraSpace、Scale 1，ForceFeedback 非循环且不按世界播放。
9. UCLASS 父类变化已冷编译成功；三个 GCN 编译为 UpToDate，序列化 CDO 复读引用正确。TestMap_SplitScreen 中 P0 真实输入拾取 Rifle 并开火后弹匣 `30 -> 29`，日志未出现新增 GameplayCue 或 Blueprint Runtime Error。
10. 注册原生标签 `GameplayCue.Character.DamageTaken`，并由 `UShootAttributeSet` 在服务器最终扣除 Health 后执行。传入的 RawMagnitude 是 `PreviousHealth - FinalHealth`，因此友伤归零、免疫、只损失护盾和零实际掉血不会误播，过量伤害也不会使用理论伤害。
11. 新增 `/Game/Blueprints/GameplayCueNotifies/GCN_Character_DamageTaken`，父类为 `GameplayCueNotify_Burst`；只配置 `CS_Character_DamageTaken` 与 `FFE_Character_Damage`，SpawnCondition 使用 Lyra 的 `LocallyControlledSource=InstigatorActor`。
12. 该 GCN 不含 NumberPop、粒子、声音、Lens、Decal、Montage 或 GameplayMessage。现有服务器实际掉血 NumberPop RPC 保持唯一入口。
13. C++ 冷编译成功，GCN 编译与序列化 CDO 复读通过。TestMap_SplitScreen 中 P0 经真实 `F` 和真实左键完成 Rifle 命中，敌人 Health `276 -> 264`；日志无 GameplayCue 查找失败、Blueprint Runtime Error 或 Accessed None。

## Character DamageTaken 的项目化执行边界

Lyra 把 `GameplayCue.Character.DamageTaken` 配在基础 Damage GameplayEffect 上；项目没有原样照搬这个执行位置。项目的友伤系数、免疫、护盾和最终 Health 扣减在 `UShootAttributeSet::HandleIncomingDamage` 才全部确定，因此 Cue 放在最终 Health 损失之后执行：

```text
UShootDamageExecution
-> UShootAttributeSet::IncomingDamage
-> HandleIncomingDamage 结算护盾、友伤和最终 Health
-> ActualHealthDamage > 0
-> SendDamageNumberFeedback（唯一 Niagara NumberPop）
-> SendDamageTakenCue（CameraShake / ForceFeedback）
```

Cue 在目标 ASC 上执行，但参数 `Instigator` 使用伤害来源 Actor。GCN 的 `InstigatorActor` 本地控制条件因此只把本地反馈交给实际伤害来源对应的 LocalPlayer，不把分屏另一位玩家或 Listen Server Host 当成反馈拥有者。这个边界是项目适配差异，不应在后续“对齐 Lyra”时无条件移回 GE 顶层。

## 当前明确缺口

1. Fire Cue 的 CameraShake / ForceFeedback 已最小闭环；Lyra 的设备反馈、`Send Weapon Fire`、声音参数、Early Reflections、Whiz By 与 Rifle 专用 Fire Audio 仍未迁入。
2. Character DamageTaken 的 CameraShake / ForceFeedback 已最小闭环；Lyra 的受击粒子、DamageTaken/Dealt 音频、Lens、命中 Montage 与 DamageTaken GameplayMessage 仍未迁入。
3. 项目当前 `AShootCharacterBase::GetHitReactMontage` 只有接口与返回值，未发现武器伤害主链调用，因此不能把该接口视为尚缺受击 Montage 的替代闭环。

## 最小实施边界

第一阶段武器 Impact Cue 闭包已经完成：

1. 共用 Lyra 实际标签 `GameplayCue.Weapon.Rifle.Impact`。
2. 项目化 GCN 只引用项目现有 SoundCue。
3. 保留 `B_Weapon.Fire` 为 Impact 粒子、Tracer 和 Decal 的唯一主入口。

Fire GCN 已选择项目化子集：保留三枪真实 CameraShake / ForceFeedback，不迁入数百包的大型声音闭包。后续若补设备反馈或音频，仍须先做依赖闭包审计并按可独立验证的子集迁入。

第二阶段 Character DamageTaken Cue 的最小闭包已经完成：保留服务器实际掉血后的 NumberPop RPC，只加入本地 CameraShake / ForceFeedback。后续若扩展受击粒子、声音、Lens、Montage 或 GameplayMessage，必须继续按独立依赖闭包迁入，并维持当前实际 Health 损失与 LocalPlayer 归属边界。

## 验收标准

- Rifle、Pistol、Shotgun 共用的 Impact Tag 有效，并能由 GameplayCueManager 找到通用 Impact GCN。
- 每个 TargetData 只播放一次表面音频/反馈；Shotgun 多 pellet 不因 Fire Cue 与 Impact Cue 产生重复 Impact Niagara。
- Fire Cue 仍只负责表现，不进入服务器伤害数值计算。
- 分屏中 CameraShake、ForceFeedback 和伤害数字只作用于正确 LocalPlayer；其中伤害数字需用高速录屏或稳定截帧补齐最终视觉证据。
- Listen Server 的远端客户端能看到自己的 Fire/Impact/伤害数字，Host 不收到远端玩家私有反馈；当前网络伤害与复制已验收，NumberPop 的逐帧视觉归属仍单列验收。
- 迁入 Character DamageTaken Cue 时，单次命中仍只有一份伤害数字。
