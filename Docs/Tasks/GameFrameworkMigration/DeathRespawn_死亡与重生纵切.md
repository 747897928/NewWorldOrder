# 死亡与重生纵切

## 当前产品边界

- 默认战斗模式先实现终结死亡与 GameMode 定时重生。
- “倒地等待救起”只属于未来 PVE 丧尸挑战模式，不是所有 GameMode 的默认规则。
- 生化感染模式属于未来独立规则，本纵切不实现。
- 玩家 ASC、属性与有身份背包在 PlayerState；RuntimeOnly QuickBar 会话在 PlayerController；Pawn 是可替换的表现与输入 Avatar。

## 旧实现的问题

- `AShootCharacterBase::Die` 只有未复制的 `bDead`，服务器关闭移动与碰撞后，客户端没有可靠的死亡状态转换。
- `Die` 直接取消能力、关闭碰撞，却同时让 `AShootCharacter::OnHealthChanged` 打开救援交互；终结死亡与倒地语义互相覆盖。
- `UShootGA_Female_RescueCloak` 虽能把 Health 恢复到 50%，但不会恢复 `bDead`、移动、碰撞或能力，现有救援不是闭环。
- ASC 位于 PlayerState。直接 `RestartPlayer` 会让新 Pawn 继续绑定死亡时 Health 为 0 的同一 AttributeSet。
- 玩家复活若未经过 PlayerController 的 `OnUnPossess` / `OnPossess`，RuntimeOnly QuickBar 会话可能丢失。

## 本纵切实现

- `AShootCharacterBase` 使用复制的 `EShootDeathState`：`NotDead -> DeathStarted -> DeathFinished`，状态转换方式对齐 Lyra `ULyraHealthComponent`。
- `AShootGameModeBase::bRespawnPlayers`、`PlayerRespawnDelay`、`PlayerCorpseLifeSpan` 由具体 GameMode 蓝图配置；Hub 默认不启用。
- 玩家终结死亡后立即 UnPossess，使 `AShootPlayerController::OnUnPossess` 暂存 RuntimeOnly QuickBar；定时器调用引擎 `RestartPlayer`，新 Pawn 的 `OnPossess` 恢复会话。
- 新 Pawn 完成 PlayerState ASC 绑定后，GameMode 只补满 Health 与 Shield，并清零 IncomingDamage；不会重读或写回账号存档属性。
- 敌人既有“原类 + 初始 Transform”重生规则保留，只补齐 `DeathFinished` 状态。
- 默认角色不再监听 `Health <= 0` 并自动开放救援交互。终结死亡与未来 Downed 状态的职责已经分开，救援组件只允许由启用该玩法的模式 AbilitySet/GA 驱动。

## 后续 PVE 倒地设计

- PVE Experience 通过模式 AbilitySet 授予一个死亡拦截/倒地状态能力和一个救援执行能力。
- 致死伤害在进入终结 `Die()` 前先询问模式允许的倒地规则；允许时进入独立 Downed 状态，不进入 `DeathStarted`。
- Downed 状态负责限制移动、武器与部分能力，同时保留可发现的救援碰撞；救起后由同一状态能力恢复移动、碰撞和允许能力。
- 流血倒计时、全队倒地判负、救援次数和无敌时间属于 PVE GameMode/Experience 配置，不写进角色基类。
- 不允许把现有 `Health <= 0` 的 UI 回调当作完整倒地状态机。

## 验收清单

- [x] 分屏中只死亡目标 Pawn 进入终结状态，另一位 LocalPlayer 不受影响。
- [x] 启用玩家重生的测试 GameMode 中，死亡后按配置延迟生成新 Pawn。
- [x] 新 Pawn Health 与 Shield 恢复，PlayerState 与现有角色数据保持。
- [x] RuntimeOnly QuickBar 槽位、激活槽、Rifle 实例和 30/60 弹药在重生后恢复。
- [ ] 实际玩家击杀时，肉眼确认只有击杀者收到 `Message.UI.Reticle.Elimination`。
- [ ] Listen Server 已确认远端玩家由服务端真实 GAS 致死并立即 UnPossess；仍需补一次延迟后客户端新 Pawn 占有的完整观察。

## 2026-08-22 运行证据

- `TestMap_SplitScreen`：P0 的 PlayerState ASC 接收真实 `ShootEffect_DamageSetByCaller` 致死伤害后，旧 Pawn 为 `DeathFinished`、Health 为 0，P1 保持 `NotDead` 且满血。
- 5 秒后 P0 获得新 Pawn，Health 为 276/276，Shield 为 102.12/102.12；P1 未变化。
- 致死前通过服务器权威拾取 RuntimeOnly Rifle，激活槽为 0、弹匣 30、备弹 60；重生后四项均保持。
- `TestMap_ListenServer`：服务端世界包含 host 本地权威 Controller 与远端权威 Controller，客户端世界只包含自己的 autonomous Controller；真实 GAS 致死已确认只由服务端执行并解绑远端 Pawn。
- 临时 PIE 设置已经恢复为 Standalone、单客户端；HomeMap 为编辑器最终停留地图。
- 冷编译命令使用 `Scripts/Build_Windows.ps1 -NoHotReloadFromIDE`，最终结果为 `Succeeded`。

## 2026-08-25 Lyra 风格消息与 HUD 接入

- 从 Lyra 迁移 `FLyraVerbMessage` 到 `Source/NewWorldOrder/Public/Messages/LyraVerbMessage.h` 与对应 `Private` 实现，保留类名、字段和 `ToString()`；没有直接复制依赖 Lyra `HealthComponent`、`GameState` 的整套 `GA_AutoRespawn`。
- 当前项目仍以 `AShootGameModeBase::PlayerDied` 作为唯一最终死亡入口：服务器发送 `Ability.Respawn.Duration.Message`，延迟结束后通过下一帧重启入口生成新 Pawn，再发送 `Ability.Respawn.Completed.Message`。这样不会让 GameMode 计时器和未来 AutoRespawn Ability 重复重生。
- GameplayMessageSubsystem 不负责网络复制，因此重生消息通过拥有者 `AShootPlayerController` 的 Client Reliable RPC 到达对应客户端，再在本地 World 广播；消息 `Instigator` 使用跨 Pawn 稳定的 `AShootPlayerState`。
- `/Game/UI/Weapon/W_RespawnTimer` 已迁移到 `/Game/UI/Hud/Respawn/W_RespawnTimer`，并通过 `DA_Experience_DungeonTest` 的 `HUD.Slot.RespawnTimer` 注入 `W_DefaultHUD` 的扩展点。旧路径仅可能在编辑器打开期间保留 ObjectRedirector，不能用磁盘命令强删。
- 当前没有实现死亡动画或溶解特效；它们属于后续表现任务，不影响终结死亡、消息和重生主链。PVE 倒地/救援仍未授予技能，不能作为本次重生验收项。
- 2026-08-25 HUD 条修复：`W_Healthbar` 与 `W_Shieldbar` 的血量/护盾材质写入统一使用各自的动态材质实例和 `Health_Current`/`Health_Updated` 参数，并在初始化、重生和材质未就绪时保护所有 MID 更新。数值文本以 ViewModel 当前值初始化，并在目标值与传入值不一致时补齐插值，避免重生期间进度条满而文本仍为 0。
- `W_Shieldbar` 是独立 `UserWidget`，不是 `W_Healthbar` 子类；其 EventGraph 必须调用本资产自己的函数，并通过带 `OldPawn`/`NewPawn` 参数的 `OnPosessedPawnChanged` 委托处理重新 Possess。两个条资产最终蓝图编译通过；仍需在编辑器重启后补一次真实伤害与玩家重生的 PIE 肉眼验收。
- `Scripts/Build_Windows.ps1 -Target NewWorldOrder -NoHotReloadFromIDE` 已成功，C++ 可执行文件链接完成；编辑器目标此前曾因正在运行的 `UnrealEditor.exe` 锁定 DLL 而失败，本次使用无热重载参数后已完成编译验证。
- 2026-08-25 HUD 动画与死亡时序修复：两个条的 `SetHealthValue`/`SetShieldValue` 先保存上一次 ViewModel 值，再写入新值；数值文本先跳到旧值，再插值到目标值，下降播放 `OnDamaged`，上升播放 `OnHealed`，从正值过渡到 0 播放 `OnEliminated`。`OnPosessedPawnChanged` 的无效 Pawn 分支只清理初始化状态，不中断正在播放的终结动画；有效 Possess 通过 `OnSpawned` 重置并恢复视觉。
- 2026-08-25 重生肉身清理修复：`AShootGameModeBase::PlayerCorpseLifeSpan` 默认改为 0 秒，`BP_ShootGameMode`、`BP_TestGameMode`、`BP_TestListenGameMode` 的 CDO 配置也统一为 0；玩家死亡完成 UnPossess 后立即销毁旧 Pawn，死亡期间只保留 HUD 的 `OnEliminated` 与 Respawn UI，避免旧角色模型额外停留约一秒。
- 2026-08-25 PIE 直接函数验收：健康值 100→60 触发 `OnDamaged`，60→80 触发 `OnHealed`，80→0 触发 `OnEliminated`；护盾 100→50 触发 `OnDamaged`，100→0 触发 `OnEliminated`。护盾归零不再依赖 Health ViewModel 已先完成归零，避免属性通知顺序导致护盾条漏播终结动画。两个蓝图编译通过，实际受影响资产仍需在编辑器重启后补一次真实伤害与玩家重生的肉眼验收。
- 2026-08-25 HUD 函数图表可读性重构：`W_Healthbar` 的 `SetHealthValue`、`SetBarMaterialRatios` 与 `W_Shieldbar` 的 `SetShieldValue`、`SetBarMaterialRatios` 已清理重复节点和重复材质写入分支。两个 `SetBarMaterialRatios` 统一为入口 `Sequence` 后按 Border、Fill、Glow 各执行一次 `IsValid`，每个动态材质只写入 `Health_Current` 与 `Health_Updated` 一次；每个数值函数复用同一个 Max 值、数字控件引用和零常量，保留旧值/新值、文本插值以及 `OnDamaged`、`OnHealed`、`OnEliminated` 分支。四个函数均无节点重叠、无反向执行线，关键输入 pin 已复读确认连接；本轮没有改动已由用户整理的 `InitializeBarVisuals` 与 `ResetAnimatedState`。
- 2026-08-25 HUD 初始化时序保护：`EventGraph` 的 `OnPosessedPawnChanged` 在 `NewPawn` 无效时仍可能调用 `ResetAnimatedState`，此时 `SetDynamicMaterials` 尚未完成，三个 MID 为空。两个条的 `ResetAnimatedState` 入口现在依次检查 `BarBorderMID`、`BarFillMID`、`BarGlowMID`，任一材质未就绪就跳过材质重置，避免初始化/重生窗口的 `Accessed None`；PIE 启动到关闭未再产生该运行时错误。
