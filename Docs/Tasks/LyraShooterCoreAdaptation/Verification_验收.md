# Lyra ShooterCore 玩家验收

## 验收方式

- 使用编辑器真实 PIE 和 Enhanced Input，不用直接调用业务函数伪造成功。
- 每个用例记录模式、玩家编号、角色性别、武器、操作前后状态和截图。
- 动画用例至少观察正面、背面和侧面；分屏问题必须同时观察两个玩家。
- 随机性问题连续重复至少 20 次方向反转或 10 次装备操作。

## 单人基础移动

- 空手待机自然，无默认 Pose 或悬空。
- 连续前、后、左、右移动，步幅和速度匹配，无滑步、双脚绑腿和膝盖突跳。
- 连续执行左到右、右到左、前到后、后到前各 20 次，不卡在 Start、Stop、Pivot 或蹲伏姿势。
- 跳跃能够离地，并依次显示 JumpStart、FallLoop、Land。
- 蹲伏同时改变碰撞体和角色动画；蹲伏移动与起身无卡帧。

## 武器与动画

- 固定点交互分别拾取 Rifle、Pistol、Shotgun。
- 每把武器装备后右手、左手与武器位置合理；左手不穿模、不悬空离开护木。
- 每把武器连续开火、移动开火、蹲伏开火，手臂不拉伸，角色与武器蒙太奇同步。
- 每把武器从部分弹匣换弹；换弹过程中镜头和角色比例不突变，结束后回到可移动、可射击状态。
- 多次切换三把武器后槽位、模型、动画层和弹药均属于当前武器。
- 丢弃当前武器后进入空手；丢弃全部武器后快捷栏为空且仍能正常移动、跳跃和蹲伏。

## 弹药

- 开火时本地 HUD 立即预测扣除一发。
- 服务器状态返回后 HUD 与 `UShootWeaponInstance` 权威弹匣一致，不出现幽灵子弹。
- 整弹匣换弹只补到弹匣容量，备弹减少相同数量。
- 弹匣为零且备弹足够时能够换弹并恢复开火。
- 备弹不足时只装入剩余数量；备弹为零时换弹不会产生子弹。
- 快速切枪再切回，显示原武器自己的弹匣和备弹。

## 分屏

- HomeMap 选择男性后进入 `TestMap_SplitScreen`，两个本地玩家仍为一男一女；选择女性时同样互斥。
- P1/P2 的 QuickBar、当前武器、弹药、准星、后坐力、姓名牌和输入互不串线。
- 玩家名显示实际名称；仅在名称冲突或需要区分本地玩家时追加 P1/P2。
- 两人同时移动、跳跃、蹲伏、开火、换弹和切枪，不影响另一人的镜头或动画。
- 重复换弹时不会出现摄像机进入角色身体、角色突然变大或视口显示错误玩家。

## Listen Server

- 主机和客户端分别拾取、射击、换弹、换枪和丢枪。
- 服务器权威弹药与两个端的 HUD 最终一致。
- 友方玩家不能互相造成伤害；敌方目标能够受到伤害并死亡。
- 远端观察到的角色、武器、开火和换弹动画与权威状态一致。
- 断线或退出副本后 RuntimeOnly 武器不进入 Persistent 仓库或 SaveGame。

## 资产与构建

- 冷编译 `NewWorldOrderEditor Win64 Development` 成功。
- 正式运行时动画引用全部位于 `/Game/Characters/Heroes/CC/MM|MF`。
- 对旧男女 Animations、Mannequin 动画和 `AnimPoseProbe` 的引用扫描为零后才执行删除。
- 资源保存后无意外重定向到旧目录，无新增未说明的插件、Build 产物或工具配置进入 Git。

## 2026-08-06 本地分屏实测记录

- 模式：TestMap，两名本地玩家，P0 男性、P1 女性；两者分别使用自己的 Enhanced Input LocalPlayerSubsystem。
- P0 用真实 `IA_Interact` 拾取 Shotgun、Pistol、Rifle，三个 RuntimeOnly 槽位分别建立；`IA_WeaponNext` 依次切换到 Pistol、Rifle，模型、正式动画层和槽位同步。
- P0 Shotgun：`8/16 -> 7/16 -> 8/15`；P0 Rifle：`30/60 -> 29/60 -> 30/59`；权威弹药、预测弹药和 HUD 一致。
- P0 Pistol：`12/48 -> 11/48`；快速切枪后 Shotgun、Pistol、Rifle 仍分别显示自己的 `8/15`、`11/48`、`30/59`。
- P0 连续执行三次 `IA_WeaponDrop` 后三个槽位均为空，`ActiveIndex=-1`，角色回到空手动画，仍可进入正式蹲伏动画。
- P1 用自己的 `IA_Interact` 拾取 Pistol，运行时链接层为 MF 的 `ABP_PistolAnimLayers_Feminine`；`12/48 -> 11/48 -> 12/47`，没有修改 P0 槽位。
- P1 换弹曾因 Mutable 主 Mesh 的瞬态 Skeleton 无法匹配 ItemDefinition 而失败；修正蒙太奇选择逻辑、Live Coding 成功后，在同一瞬态 Skeleton 条件下复测通过。
- 未通过项：TestMap 的 EnemyBot 占用 `PlayerStart_0`，P0/P1 会共同选择 `PlayerStart_1` 并仅靠碰撞分离。另一名角色进入相机射线会造成近景遮挡，须先调整地图出生点后再判定“换弹时角色突然变大”用例通过。

## 2026-08-06 队伍与敌人重生实测记录

- 两名 PlayerState 均为 Team 1。相同位置、相同瞄准与真实 `IA_Attack` 下，P0 Rifle 对 P1 射击消耗一发但 P1 Health 保持 `276`；目标换成敌人后 Health `276 -> 266`。
- 敌人被真实 Rifle 射击降到 0 后进入 `IsDead=true`；TestMap 的 `BP_TestGameMode` 清理旧尸体，并按敌人 BeginPlay 记录的初始 Transform 生成相同蓝图类。
- 重生实例由 `BP_EnemyBotCharacter_C_0` 变为 `C_1`，位置恢复 `(0,0,89.65)`，Health/MaxHealth 为 `276/276`、`IsDead=false`，并获得新的 `BP_EnemyBotController_C_1`。
- HomeMap 两个直接 GrantActor 与两个 GrantSpawner 已迁移到蓝图配置的 Cylinder 和 `/Game/UI/FrontEnd/W_Icon_Interact`；C++ 不再加载旧 `/Game/UI/Widgets/W_Icon_Interact` 路径。

## 2026-08-07 Listen Server 与冷启动实测记录

- 模式：TestMap2，进程内 Listen Server 加一个远端客户端。`BP_TestListenGameMode.LocalPlayerMapPolicy` 已从错误的 `PRIMARY_ONLY` 改为 `KEEP_CURRENT`；权威世界有两名 PlayerController，客户端进入同一 TestMap2，不再因 `?MaxPlayers=1` 停留在 FrontEndMap。
- 主机通过自己的 Enhanced Input 子系统拾取 Rifle，远端客户端通过自己的 Enhanced Input 子系统拾取 Pistol；服务器分别建立独立 RuntimeOnly QuickBar 和 WeaponInstance。
- 主机 Rifle 实际 `IA_Attack` 后服务器 `Current/Predicted Ammo` 为 `29/29`，实际 `IA_Reload` 完成后为 `30/30`。
- 客户端 Pistol 实际 `IA_Attack` 后，服务器侧远端实例与客户端本地实例均为 `11/11`；实际 `IA_Reload` 完成后两端均为 `12/12`。
- 主机与客户端分别执行实际 `IA_WeaponDrop` 后，服务器两名玩家和客户端本地玩家均为 `ActiveIndex=-1`、无 Active Weapon，未串改对方状态。
- 冷编译 `NewWorldOrderEditor Win64 Development` 6/6 成功。全新编辑器进程中 18 个正式 CC AnimBP/动画接口资产全部 `BS_UP_TO_DATE`，Standalone PIE 只有一个 HomeMap 世界。
- 冷启动发现并修复男性 `AO_MM_Pistol_Idle_ADS` 的 15 个空样本；全新进程重载后为 15/15 有效，正式 CC 的 8 个 BlendSpace/AimOffset 空样本扫描为零。
- 新日志对 `ABP_ItemAnimLayersBase` 同名类型不兼容、循环自动过渡、男性 Pistol AimOffset 空样本和旧 `/Game/UI/Widgets/W_Icon_Interact` 路径均为零匹配。

## 2026-08-08 散布、准星与弹药 UI 实测记录

- 项目 Rifle、Pistol、Shotgun 的 Heat 曲线、准星 Widget 尺寸及半径映射与 Lyra 11000 逐项一致。当时观察到的区间中点初始值后来证明会被项目 QuickBar 的重复 Equip 生命周期再次写入，已由 2026-08-10 的修复取代。
- Rifle 开火后实测姿态倍率为移动 `1.0`、停止 `0.8`、蹲伏约 `0.4801`、腾空约 `1.4544`、瞄准约 `0.5216`。Fragment Heat 曲线枪支已正确进入站立、蹲伏、腾空和瞄准倍率计算。
- Listen Server 远端客户端仅拾取一把 Shotgun，不依赖第二次拾取触发刷新：真实 Slate HUD 初始为 `8/16`，打空为 `0/16`，整弹匣换弹后为 `8/8`；再拾 Rifle 后切换显示 `30/60`，五次快速切换回 Shotgun 仍显示 `8/8`。
- 复制乱序根因为 QuickBar Slot Guid 可先于 PlayerState Inventory Item UObject 到达，WeaponInstance 又可先于 Equipment Instigator 到达。修复只在客户端未解析槽位的复制窗口重试，并优先从同槽 Inventory Item 填充弹药，不引入 Timer、固定延时或第二套状态源。
- 本地双人分屏中，P0 Shotgun 从 `0/16` 换弹到 `8/8` 时 P1 Rifle 保持 `27/60`。两人各自再拾 Pistol 并快速切换后，P0 最终显示 Shotgun `8/8`，P1 最终显示 Rifle `27/60`，两侧快捷栏选中态和弹药独立。
- 修复后的 `NewWorldOrderEditor Win64 Development` 冷编译成功；本轮弹药 UI 仍使用既有 QuickBar 消息、Inventory Item StatTag 和每个 LocalPlayer 的 HUD，不新增全局 UI 或延时轮询。

## 2026-08-09 出生点与橡皮手复测记录

- TestMap 的 `PlayerStart_0` 原与 EnemyBot 重合；仅将其从 `(0,0,92)` 移到 `(0,-330,92)`，`PlayerStart_1` 保持 `(0,330,92)`，敌人及重生 Transform 保持 `(0,0,89.65)`。
- 新 PIE 中 P0 男性生成在 `(0,330,89.65)`，P1 女性生成在 `(0,-330,89.65)`。两个角色保持男女互斥，截图中两个 SpringArm/相机均未被另一名角色贴脸遮挡。
- 旧复测只证明骨长没有拉伸，却把用户不可接受的双肘外翻误判为无需处理。重新读取 Lyra 11000 的 `WID_Rifle/Pistol/Shotgun` 后确认，三者都使用 `weapon_r` 与 Z 轴 `-90` 度相对旋转；项目三份 Equipment 原为 `weapon_socket_hand_r + Identity`。正式资产现已逐一对齐为 Lyra 配置。
- 对齐挂点后，Equip 的 `0.0-0.6s` 仍是 CC 最终表现中的异常过渡段。该轮审计时按零 Notify 处理；2026-08-12 复读已发现 MM `AM_Shotgun_Equip` 当前在 `0.0001s` 有一枚 `AN_ShootPlayWeaponMontage`，旧的“六个全部为零”结论作废。三把 `B_WeaponInstance_*` 配置 `EquipMontageStartPosition=0.6s`，不改变装备权威状态、Actor 显隐或 GAS 生命周期。
- `0.4s` 中间值下，男性 Shotgun 和女性 Pistol 的真实输入已不再出现旧式骨骼拉伸，但干净女性 Shotgun 截图仍显示单臂横伸，因此没有把该值写成通过。女性 Shotgun 在 `0.6/0.8/1.0s` 逐帧采样中从 `0.6s` 起自然双手持枪，正式值据此调整为 `0.6s`；后续真实切换回归使用该最终值。
- 男性 Shotgun 使用持续逐帧 `IA_Move` 后，开火前速度约 `240cm/s`，捕获时稳定为 `300cm/s`；真实 `IA_Attack` 令 Shotgun Fire Montage 位于约 `0.067s`。截图 `HighresScreenshot00056-00058.png` 连续三帧未见手臂拉伸。此前只注入一帧 Move、日志速度为 `0` 的 `00053-00055` 不计入移动开火验收。
- 正式 `0.6s` 配置下，女性真实拾取 Rifle 的首个捕获帧约为 `0.669s`，截图 `HighresScreenshot00071-00073.png` 为自然过渡持枪。拾齐三枪后连续十次 `IA_WeaponNext` 的实际顺序为 Pistol、Shotgun、Rifle 循环，各次 Equip 捕获位置约 `0.69-0.71s`，唯一附着的 B_* Actor 始终位于 `weapon_r`；最终截图为 `HighresScreenshot00074.png`。
- 女性 Shotgun 使用逐帧持续 `IA_Move` 后速度稳定为 `300cm/s`，真实 `IA_Attack` 激活 Fire Montage，捕获位置约 `0.028s`。截图 `HighresScreenshot00075-00076.png` 中左手保持护木跟随、右臂未横向拉长，未复现用户验收图中的 Equip 或横移开火橡皮手。
- PIE 结束前再次读取正式资产：三把 `B_WeaponInstance_*` 均为 `BS_UP_TO_DATE` 且 `EquipMontageStartPosition=0.6`；三份 `BP_Equipment_*` 也均为 `BS_UP_TO_DATE`。本轮 PIE 已正常结束。
- 用户已从编辑器手工删除零引用临时目录 `/Game/Developers/Codex/AnimPoseProbe`、旧男女 Animations 目录和两个 `NoAddCandidate`；本任务不恢复这些资产。
- 基于用户提交 `4caef3b` 的最终冷编译再次返回 `Result: Succeeded`、Target up to date。正式 CC 目录的 18 个 AnimBP 全部为 `BS_UP_TO_DATE`，男女六个 Fire Montage 全部使用对应 CC Skeleton 和 `FullBodyAdditivePreAim`；正式链没有出现未知 Blend Space、Item Base 类型不兼容或错误 Slot。

## 2026-08-09 Foot Controls 最终玩家视角回归

- 男女性别分别通过独立 LocalPlayer 的真实 `IA_Move`、`IA_Jump`、`IA_Crouch` 输入完成前进、后退、左右移动、连续反向、腾空、落地和蹲伏。所有移动阶段都逐帧持续注入输入，不把单帧零速度状态计入验收。
- 男性四向速度稳定为 `300cm/s`，采样脚距约 `33.84-70.12cm`；Jump 采样垂直速度约 `445.62cm/s`，落地后双脚 Z 约 `9.39/7.42cm`；Crouch 胶囊半高由 `88` 降至 `40`。截图为 `HighresScreenshot00077-00083.png`。
- 女性四向速度稳定为 `300cm/s`，采样脚距约 `49.11-73.37cm`；Jump 采样垂直速度约 `432.74cm/s`，落地后双脚 Z 约 `9.02/7.64cm`；Crouch 胶囊半高由 `88` 降至 `40`。截图为 `HighresScreenshot00084-00090.png`。
- 两组截图均未见双脚固定在同一点、膝盖逆向折叠或落地后持续悬空。左右移动出现的是有分离脚距和支撑脚切换的交叉步，不应误判为“两脚绑住”。FootPlacement、LegIK、GroundDistance 与 Stride Warping 的单变量诊断开关保持恢复状态。
- 项目编辑器正常退出后再次执行 `Scripts/Build_Windows.ps1`，`NewWorldOrderEditor Win64 Development` 返回 `Result: Succeeded`、`Target is up to date`；本轮 C++ 终态已通过无编辑器占用的冷编译检查。

## 2026-08-10 准星生命周期与双模式验收

- 两张测试地图已按用途重命名：`/Game/Maps/TestMap_SplitScreen` 用于两名本地玩家分屏；
  `/Game/Maps/TestMap_ListenServer` 使用 `BP_TestListenGameMode`，用于一台机器一个本地玩家的
  Listen Server。HomeMap 的副本入口已指向 `TestMap_SplitScreen`。
- 本地双人快速切换 `[1,2,0,2,1,0,2]` 后，两名玩家当前准星都绑定各自当前 WeaponInstance。
  Shotgun 装备后立即为 `6.0°`，稳定站立倍率为 `0.9`、最终角度为 `5.4°`；没有再次出现旧
  Pistol/Shotgun 大准星覆盖当前 Rifle 的现象。
- P0 Shotgun 真实开火后弹药 `8 -> 7 -> 6`，散布 `6.0° -> 9.5°`，恢复延迟结束后回到
  `6.0°`。真实 Jump 中倍率从地面 `0.9` 平滑上升到约 `1.6915`，落地回到 `0.9`；真实 Crouch
  为 `0.72`、角度 `4.32°`；移动到 `300cm/s` 时倍率收敛到约 `0.98`；Reload `6 -> 8` 期间
  散布保持稳定。准星表现与实际 WeaponInstance 数值使用同一条链。
- Listen Server 进程内有两个 World：服务器 World 包含本地权威与远端权威 Controller，客户端
  World 包含本地非权威 Controller。Host 只属于服务器本地 Controller 和客户端本地 Controller；
  服务器远端 Controller 没有玩家私有 UI。CommonUI 旧/当前 HUD 栈会保留两个 Host UObject，
  但只有当前 HUD 构造子准星，这不是可见叠加。

## 2026-08-12 补给站长按与玩家私有 HUD 验收

- 正式资产链为 `BP_AmmoSupplyStation.InteractionPrompt -> W_Icon_Interact` 与 `BP_ShootPlayerController.InteractionHUDComponent -> HUD.Slot.Interaction -> Interactive_Progress_Bar`。进度条不再使用世界共享 `WidgetComponent`，Station 临时组件和临时 EventGraph 已清空。
- 本地分屏中把 P0 移到站点有效扫描距离并开始长按后，P0 Controller 的 `InteractionProgressVisible=true`、进度约 `0.307`；P0 当前 HUD 中真实 `InteractionProgressBar` 为 `SelfHitTestInvisible` 且 Percent 约 `0.307`。P1 Controller 与当前 HUD 保持 `false/Collapsed/0`。
- P0 提前释放后，同一帧恢复 `InteractionProgressVisible=false`、进度 `0`，真实进度条与提示文本均为 `Collapsed`；P1 全程没有收到可见状态。
- Listen Server 中，远端客户端长按时客户端进度约 `0.367`，Host 的 Controller 和当前 HUD 均保持 `false/Collapsed/0`；远端提前释放后客户端立即隐藏并清零，Host 全程不串线。
- 功能结算沿用此前真实验收：本地分屏 P0/P1 初始 `1/12`，提前释放不补弹；P0 完成长按后为 `12/48` 且 P1 不变。Listen Server 客户端完成后，服务器权威与客户端复制值均为 `12/48`。
- 最终恢复检查：Station CDO `HoldDuration=3.0`；`WaitInputRelease(this, true)` 与仓库基线一致；PIE World 数为 `0`；编辑器 Play 设置为 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`。

## 2026-08-13 补给站玩家复验修正

- 范围验收不再使用 C++ 独立距离。Station CDO 的 `InteractionCollision` 半径为 `50cm`；P0 位于球体内时 `IsOverlappingActor=true` 且存在交互选项，P1 位于球体外时选项数为 `0`。服务器结算调用同一个 `CanSupplyPawn -> InteractionCollision.IsOverlappingActor`，调整蓝图球体不会再被隐藏的 `550cm` 放宽。
- `BP_AmmoSupplyStation` 的继承组件 `InteractionPrompt` 为 Screen、`60x60`、相对位置 `(0,0,70)`、初始隐藏；Visual 仍为用户选择的 `/Game/Assets/Furniture/AmmoBox/StaticMeshes/Mesh_0009`，比例 `(1.2,1.2,1.6)`。
- 分屏提示复验：P0 在范围内时模板隐藏，只有 OwnerPlayer=P0 的运行组件显示；P1 进入且 P0 离开后，P0 组件变为隐藏，OwnerPlayer=P1 的第二个运行组件显示。两份实例均从蓝图模板复制 Screen、尺寸与位置，模拟代理没有本地提示实例。
- `W_DefaultHUD.InteractionExtensionPoint` 的 Blueprint Canvas Slot 为锚点 `(0.5,0.82)`、Alignment `(0.5,0.5)`、零 Offset、AutoSize；`Interactive_Progress_Bar.SizeBox_0` 为 `100x20`。因此进度条位于每个 LocalPlayer HUD 的居中偏下区域，不再被 Fill 拉成全屏高度。
- `Interactive_Progress_Bar` 默认 Percent 为 `0`、文案为“弹药补充中”；空闲时 `InteractionProgressBar` 与 `MessageCommonText` 均由所属 Controller 状态折叠。长按、提前释放、完成补弹与 Listen Server 数据结果继续使用上一节同一实现链的真实输入证据。
- C++父类不再设置提示的 WidgetSpace、DrawSize 或 RelativeLocation，只负责创建组件、服务器权威与 LocalPlayer 生命周期。Station、Controller、HUD、Progress WBP 均为 `BS_UP_TO_DATE`，C++修正通过 Live Coding；最终 PIE 已停止并回到 HomeMap。

## 2026-08-12 Lyra Niagara 伤害数字验收

- Lyra 11000 基线：`Damage_BasicNiagaraStyle.NiagaraArrayName=DamageInfo`，系统为 `/Game/Effects/Particles/Impacts/NS_DamageNumbers`；`GCNL_Character_DamageTaken` 只路由到伤害来源 Pawn 的本地 Controller。项目保留逐 TargetData、来源玩家私有表现和每 Controller 单组件生命周期，不引入另一套伤害聚合。
- 本地分屏使用 P0 自己的 `EnhancedInputLocalPlayerSubsystem` 注入真实 `IA_Attack`。Rifle Current/Predicted Ammo 从 `30/30` 变为 `29/29`、再到 `28/28`；敌人 Health 从 `276` 变为 `266`，实际伤害数字被视口捕获。
- 同轮 P0 的 `DamageNumberComponent` 只懒创建一个活动 `NS_DamageNumbers` NiagaraComponent，后续命中继续复用；P1 的 NiagaraComponent 数保持 `0`。组件为 `OnlyOwnerSee=true`，并把 P0 当前 ViewTarget/Pawn 登记为 VisibilityOwner，避免同一世界的另一分屏视口渲染该世界粒子。
- Listen Server 使用 `TestMap_ListenServer`，远端客户端真实 `IA_Attack` 后，客户端本地 Rifle 与服务器远端实例均为 `29/29`，目标 Health 在服务器和客户端均为 `266`。客户端本地 Controller 有一个活动 NiagaraComponent；服务器远端 Controller 与 Host 本地 Controller 都为 `0`，证明 Client RPC 未串到 Host。
- 伤害值取服务器权威的实际 Health 差值；友伤过滤、免疫或零实际掉血不会生成数字。命中带 `Gameplay.Zone.WeakSpot` 的 Physical Material 时以负 W 交给 Niagara 暴击样式。
- 构建与恢复：完整反射改动已通过 `NewWorldOrderEditor Win64 Development` 冷编译；VisibilityOwner 的纯 `.cpp` 收尾补丁通过 Live Coding。最终 PIE World 为 `0`，Play 设置恢复 `PIE_Standalone`、`1 Client`、`RunUnderOneProcess=true`，编辑器回到 HomeMap。

## 2026-08-14 动画与伤害数字玩家复验纠错

- MF `ABP_Mannequin_Base` 的五条 GameplayTag 映射已从 `PropertyName=None/Guid=0` 恢复到正式 Bool 属性；重新编译、保存、复读均为 `BS_UP_TO_DATE`，最新 PIE 日志没有再次出现对应 `[None]` 错误。
- MM `MM_Shotgun_Fire.weapon_r` 的 21 帧已从 Lyra 同名源逐帧恢复；关键时间点局部 Transform 差均为零。此前男性轨道在 0.1 至 0.2 秒把武器参考骨抬到约 `33.5cm`，与女性和 Lyra 都不一致，不能再归类为正常后坐或 Actor Transform 问题。
- 男女 3 个 Equip 与 Generic Unequip 均已补齐 Lyra 的 `ScaleDownWeaponR`、`DisableLHandIK` 曲线；三把 `B_WeaponInstance_*` 的 `EquipMontageStartPosition` 已恢复为 `0.0`。资产复读确认 Rifle/Shotgun、Pistol、Generic Unequip 分别持有匹配来源和裁剪窗口的曲线数据。
- Niagara 可见性做了运行 A/B：旧 Controller Owner 下组件 Active 且数组被系统消费，但 `WasRecentlyRendered=false`；临时关闭 `OnlyOwnerSee` 后立即变为 true。改成当前 ViewTarget/Pawn Owner 并重新 Live Coding 后，保持 `OnlyOwnerSee=true` 仍为 `WasRecentlyRendered=true`，Standalone 实际窗口捕获到橙色暴击数字 `789`。
- 分屏窗口已启动为同一 World 的两个 LocalPlayer，左右玩家各自拥有独立 Controller 组件；测试结束后已停止 PIE、回到 HomeMap，并恢复 `Standalone + 1 Player + RunUnderOneProcess`。真实武器拾取/开火与 Listen Server 的最终视觉复验仍列为待补，不把运行注入写成完整玩家验收。

## 2026-08-12 Equip/Unequip 终验

- Lyra 11000 重新取证覆盖原生 C++、`B_WeaponInstance_Base` EventGraph/宏、四把 WeaponInstance CDO、三份 `WID_*`、三份 Equip Montage 与四个源 Sequence。项目不是只按旧文档自证：装备/卸下动画层选择与播放顺序、`weapon_r/-90` 挂点、男女层规则和 Generic Unequip 参数均重新与官方事实对照。
- 正式 CC Equip 的基础轨为非 Additive，配对轨为 Local Space Additive；Pistol/Rifle/Shotgun 使用同侧目标 Skeleton。男女 Equip 的动态 `weapon_r` 在 `0.0/0.3/0.6/1.0s` 与 Manny 源逐点一致，排除本轮由辅助骨轨道丢失造成橡皮手。
- `TestMap_SplitScreen` 使用两名真实 LocalPlayer。P1 女性真实拾取 Shotgun，MF `AM_Shotgun_Equip` 冻结在 `0.6001s`；P0 男性真实拾取 Rifle 与 Pistol，MM Montage 同样冻结在 `0.6001s`；女性 Pistol 也以真实交互复测。各截图均为自然持枪，手臂未被骨长拉伸，另一玩家状态保持独立。
- 对比采样中把女性 Shotgun 强制查看到源动作 `0.3s`，可见双肘外翻；回到正式 `0.6001s` 后进入自然持枪。这证明当前裁剪针对的是 Lyra 源动作在 CC 比例上的不可接受过渡，不是用延时掩盖错误 Socket、缺失 `weapon_r` 或跨性别 Montage。
- 分屏女性真实丢弃后，MF `AM_Generic_Unequip` 冻结在 `0.25s`，手臂比例正常且快捷栏进入空槽；P0 没有被误播。当前 MM Shotgun Equip 的 `0.0001s` Notify 已单独记录，未在本阶段修改用户动画资产。
- `TestMap_ListenServer` 远端客户端真实拾取 Rifle：服务器模拟代理与客户端本地 Pawn 都播放 MF `AM_Rifle_Equip`，冻结位置均为 `0.6001s`；Host 无活动 Montage。远端客户端真实丢弃后，两端都播放 MF `AM_Generic_Unequip` 并冻结在 `0.25s`，Host 仍无误播，掉落 Actor 两端一致。
- 恢复检查：PIE 已停止，Play 设置为 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`，编辑器回到 HomeMap。本阶段没有修改 C++、Blueprint 或动画资产。

## 2026-08-12 Shotgun 开火旋转与武器 Montage 审计

- Lyra 11000 对照确认：`GCN_Weapon_Shotgun_Fire -> B_Weapon.Fire` 只处理枪口、弹壳、Tracer、Impact 和 Decal；没有 Actor Rotation 节点。角色 `AM_MM_Shotgun_Fire` 的 `AN_PlayWeaponMontage` 明确播放 `AM_Weap_Shotgun_Fire`，武器 AnimInstance 随后以角色 Montage 为 Leader 执行 `MontageSync_Follow`。
- `TestMap_SplitScreen` 中 P0 男性和 P1 女性分别通过真实 `IA_Interact` 拾取 Shotgun，再由各自 LocalPlayer 的真实 `IA_Attack` 激活同侧正式 `AM_Shotgun_Fire`。两把 B_Shotgun 全程只有一个实例，均附着 `weapon_r`，根组件相对旋转始终为 `Yaw=-90`。
- 男性将 Fire Montage 冻结在 `0.000s` 与 `0.07645s` 时，武器 Actor 四元数为约 `(-0.0214,0.0044,-0.6468,0.7624)` 与 `(-0.0151,-0.0032,-0.6221,0.7828)`，对应约 `7` 度连续变化；女性同区间四元数几乎不变。Rotator 的单轴 Yaw 数值会因欧拉角表示产生更大的表面变化，不能作为 Actor 被代码旋转的证据。
- 11000 源 `weapon_r` 同区间约变化 `4-5` 度，属于 Shotgun Fire 后坐。项目没有相对 Transform 改写、socket 突跳或第二把表现 Actor，因此本项不修改 Equipment、B_Shotgun、角色 Fire 轨道或挂点。
- 当时另行发现：男女三枪六个正式 Fire Montage 都带空配置 BP `AN_ShootPlayWeaponMontage`，Reload 也没有武器 Montage 同步入口。该审计结论已由 2026-08-13 的正式修复取代，历史证据保留用于说明缺口来源。
- 恢复检查：PIE 已停止，Play 设置仍为 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`，编辑器回到 HomeMap。本阶段只更新文档，没有修改受保护的正式动画、`AN_*`、Blueprint 或 C++。

## 2026-08-12 三环境终态冒烟回归

- HomeMap 干净启动只有一个 PIE World、一个本地 `BP_ShootPlayerController`、一个 Pawn 和一个当前 `EnhancedInputLocalPlayerSubsystem`。Controller 的 RuntimeOnly QuickBar 为三个空槽、`ActiveIndex=-1`，Pawn 没有副本武器 Equipment；未把上一轮副本物品带回 Persistent 环境。
- `TestMap_SplitScreen` 干净启动生成两个本地 Controller 与两个 Pawn。P0 的正式 Skeleton 为 `ChenHaoYu_Skeleton`，P1 身体组件为 `ShenWanYun_Skeleton`，男女互斥且出生点分离。
- 两人分别通过各自真实 `IA_Interact` 拾取 Rifle、Pistol、Shotgun；三槽均为 RuntimeOnly，且每个 Pawn 始终只有一个活动 Weapon Equipment。两人 Rifle 同步完成 `30/60 -> 29/60 -> 30/59`，Pistol 完成 `12/48 -> 11/48 -> 12/47`，Shotgun 完成 `8/16 -> 7/16 -> 8/15`；`IA_WeaponNext` 与弹药快照没有跨玩家串线。
- 两名玩家真实 `IA_Crouch` 均令 Capsule HalfHeight 从 `88` 变为 `40`，再次输入恢复。该轮持续 Move 的新注入 API 没有形成速度，明确不计为新移动证据；移动、Jump 与连续反向仍引用 2026-08-09 已记录的逐帧真实输入证据，不伪造本轮通过项。
- 两人从 Shotgun 开始连续执行三次真实 `IA_WeaponDrop`：先自动选 Rifle，再自动选 Pistol，最后均为 `ActiveIndex=-1`、三个槽位全空、Weapon Equipment 数为 0。掉落和另一名玩家的 QuickBar 没有互相覆盖。
- `TestMap_ListenServer` 干净启动为两个 World：服务器包含 Host 本地 Controller 与远端权威 Controller，客户端包含一个本地非权威 Controller。Host 真实拾取 Rifle，远端客户端真实拾取 Pistol；服务器快照分别为 `30/60` 与 `12/48`，客户端 Pistol 初始同步为 `12/48`。
- 两端真实 `IA_Attack` 后，Host Rifle 为 `29/60`，远端 Pistol 在服务器与客户端均为 `11/48`；真实 `IA_Reload` 后分别同步为 `30/59` 与 `12/47`。两端真实丢枪后服务器立即为空，客户端在正常短复制窗口后也变为 `ActiveIndex=-1`、三个槽位全空、Weapon Equipment 数为 0。
- Widget 运行态扫描确认：服务器远端 Controller 没有 QuickBar、ReticleHost 或 InteractionProgress 的玩家私有 Widget；服务器 Host 本地和客户端本地各自拥有其所属 Controller 的 Widget。CommonUI 历史/当前 HUD 栈仍可保留同 OwningPlayer 的旧 UObject，但只有当前 Host 构造当前武器准星，该现象与 2026-08-10 结论一致。
- 本轮没有重复破坏性测试已完成的友伤、敌人重生、补给站完成结算和 Niagara 数字；这些链在当前提交基线上均有同版本分屏/Listen Server 实测记录。终态冒烟重点验证阶段提交后未破坏地图模式、三枪权威弹药、丢弃空手与玩家私有 UI 边界。
- 恢复检查：PIE 已停止，编辑器回到 HomeMap；Play 设置为 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`。

## 2026-08-12 Mannequin 依赖终点审计

- AssetRegistry 对男女正式 18 个 CC AnimBP/动画接口的直接硬依赖扫描得到 9 个 Mannequin 包；它们属于枚举、结构、接口、父 AnimBP、Transition Notify、Skeleton、ControlRig 和 IK Retargeter。沿这些包递归展开为 37 个 Mannequin 包，其中包含父类默认值带入的 21 个 Sequence 和 1 个 AimOffset，因此不能把“正式 AnimBP 编译成功”解释成“Mannequin 已可删除”。
- 同时逐项读取 `target_skeleton`：MM 九项全部指向 `ChenHaoYu_Skeleton`，MF 九项全部指向 `ShenWanYun_Skeleton`。正式运行骨架没有退回 `SK_Mannequin`；AssetRegistry 中出现的 `SK_Mannequin` 属于另一条待定位的节点、父类或工具链依赖。
- 扩展扫描 `/Game/Characters/Heroes/CC/MM` 与 `/Game/Characters/Heroes/CC/MF` 的 742 个资产，共得到 49 个 Mannequin 依赖包和 1157 条直接硬依赖边。大量边来自 AnimationModifier、曲线压缩设置和重定向源/基姿势引用；这些依赖必须逐类迁移，不能用目录批量替换。
- 运行数据复读确认两个明确未闭环项：MF `AO_MM_Rifle_Idle_Hipfire` 的 15 个样本和 Preview Base Pose 指向 Mannequin Rifle 数据；MF `AO_MM_Unarmed_Idle_Ready` 的 15 个样本和 Preview Base Pose 指向 Mannequin Unarmed 数据。另有 31 个 MM Additive Sequence 的 `ref_pose_seq` 仍指向 Mannequin：Rifle ADS 14 个、Rifle Crouch 14 个、Pistol 45 度样本 2 个、HitReact 1 个。
- Lyra 11000 对照给出不可丢失的源契约：三个 AimOffset 均为 15 点网格；方向 Sequence 为 Mesh Space Rotation Offset Additive，Base Pose 类型为 Anim Frame，使用同组中心样本第 0 帧。MF 目录已有 `MF_Rifle_Idle_ADS` 与正式 Unarmed Idle 可作为 Preview Base，30 个 Rifle/Unarmed 方向样本尚无 MF 对应资产，不能只改指针而跳过正式重定向。
- 本轮为只读审计，没有改动受保护的动画资产。修复验收必须包含：目标 Sequence 绑定同侧正式 CC Skeleton、AimOffset 15 个坐标保持不变、Additive 类型和 Base Pose 指向同侧 CC 资产、18 个正式 AnimBP 冷启动仍为 `BS_UP_TO_DATE`，以及男女空手/Rifle/Pistol/Shotgun 的真实输入视觉 A/B。

## 2026-08-13 正式 CC 姿势数据与武器 Montage 修复验收

- MM 31 个正式 Additive Sequence 已改用同侧 ChenHaoYu Base Pose：Rifle ADS 14、Rifle Crouch 14、Pistol 45 度 2、HitReact 1；复读剩余 Manny Base 为 0。
- MF Rifle Hipfire 与 Unarmed AimOffset 已接入 30 个同侧 ShenWanYun 正式样本，两个 Preview Base 也指向同侧正式 Idle。两个 AimOffset 均为 15/15 有效样本，采样坐标和 Mesh Space Additive 契约保持。
- 男女三枪 6 个 Fire 与 6 个 Reload 角色 Montage 已统一为原生 `UShootAnimNotify_PlayWeaponMontage`。Fire 不再保留空配置 BP Notify；Reload 继续保留 `AN_Reload` 结算 Notify。每个角色 Montage 都有且只有一个正确的武器 Montage 同步入口。
- 18 个正式 CC AnimBP/动画接口全部重新编译为 `BS_UP_TO_DATE`，零失败。
- 男性 Shotgun Fire 的目标 Sequence 曾为 `AAT_NONE/ABPT_NONE`，而 Lyra 源与女性正式资产均为 Mesh Space Additive、自身第 28 帧 Base Pose。恢复该元数据后，同一 `0.12s` 冻结帧从枪体位于头顶恢复为肩部持枪。
- 分屏真实 `IA_Attack`：男性 Shotgun 弹药 `7 -> 6`，角色与武器 Montage 同时 Active/Playing，位置均为 `0.178007s`，视口未再出现飞枪。Actor 始终附着 `weapon_r`，相对旋转 `Yaw=-90`。
- Listen Server 远端客户端真实 Shotgun 开火 `8 -> 7`：客户端本地 Pawn 与服务器模拟代理均播放 MF 角色 Fire 与武器 Fire Montage，时间约 `0.342s`；Host 未成为同步目标，网络归属正确。
- 该轮只确认基础/Additive 类型与冻结帧骨长，旧的 `EquipMontageStartPosition=0.6s` 结论已由 2026-08-14 曲线修复取代；当前三把武器均从 `0.0s` 起播。
- 恢复检查：PIE 已停止，Play 设置恢复 `PIE_Standalone`、`PlayNumberOfClients=1`、`RunUnderOneProcess=true`。

## 2026-08-13 冷编译与剩余依赖分类验收

- 先关闭 NewWorldOrder 编辑器与其 Live Coding Console，Lyra 11000 保持运行；`Scripts/Build_Windows.ps1` 使用 `-NoHotReloadFromIDE` 完成 `NewWorldOrderEditor Win64 Development` 冷编译，结果为 `Succeeded`。
- 冷启动的新编辑器确认连接项目为 `NewWorldOrder.uproject`，当前地图为 `/Game/Maps/HomeMap`。18 个正式 CC AnimBP/动画接口全部为 `BS_UP_TO_DATE`；MM 九项目标 Skeleton 为 `ChenHaoYu_Skeleton`，MF 九项为 `ShenWanYun_Skeleton`。
- 男女正式 CC 目录当前有 409 个资产直接依赖 Mannequin 包、1097 条边。按源/目标类型统计，AnimSequence 的 1065 条边来自 268 条曲线压缩设置引用，以及 797 条 AnimationModifier/Transition Notify 引用；它们不携带角色运行姿势。
- 六个正式 AnimBP 的剩余直接依赖逐项定位为 Lyra 枚举、结构、Transition Notify、动画层接口/类型、`CR_Mannequin_FootPlant`、`RTG_Mannequin` 和编辑器 Skeleton 契约。父类复读为 `ShootMannequinAnimInstance` 或引擎 `AnimInstance`，没有源 Mannequin Blueprint 父类继续控制正式运行链。
- MF `SplashPose_Quinn_1/10/11` 仍绑定 Manny Skeleton 和 Quinn Retarget Source，但三者直接引用者均为零。它们是未使用的迁移残留，不是当前 Gameplay/AnimBP 姿势来源；按多文件删除红线留给用户在 Content Browser 手工清理。
- 结论：正式运行姿势检查仍为 `MANNY_REF_POSE=0`、`MANNY_AIM_REFS=0`。剩余包依赖是可解释的共享类型与编辑器工具契约，不应通过目录级替换或复制资产强行清零。

## 2026-08-17 男性 Shotgun Fire Blend A/B

- 分屏中先使用原配置冻结正式 MM Shotgun Fire 于 `0.15s/0.30s`，再与正式 MF 同时播放对照。角色与武器 Montage 均能推进到相同位置，武器 Actor 全程附着 `weapon_r`；`AM_Generic_Unequip` 没有活动实例。
- 静态复读显示 Manny 源和正式 MM 的 Blend In 都为 `0.0s`，正式 MF 为 `0.25s/HermiteCubic`。这是“只在男性明显”的唯一 Montage 混入差异；Fire Action 的长度、Additive 类型、Base Pose 和 `weapon_r -> hand_r` 关系没有性别侧脱节。
- 将正式 MM Blend In 改为 `0.25s/HermiteCubic` 后，同一 `0.15s` 帧的男性枪体/双手不再瞬间顶到脸部；`0.30s` 恢复段保持附着和角色/武器同步。随后直接通过 P0 ASC 激活原生 `ShootGA_Weapon_Fire_Shotgun`，确认实际选择的仍是该正式 MM Montage，而非备用 Montage。
- Fire Montage 通知链为：`0.0001s` 原生 `UShootAnimNotify_PlayWeaponMontage`，末帧 Skeleton Notify `ResetCombo` 与 `SaveAttack`。`sfx_WeaponSwap_nl_meta_Preset` 是 Equip 的声音 Notify，不应复制进 Fire；用户看到的第二条 Timing 正是这枚声音 Notify，而不是遗漏的第二条动画轨。
- PIE 前后 MM、MF 正式 Base AnimBP 均为 `BS_UP_TO_DATE`。日志按 2026-08-17 时间过滤，未出现新的 `property [None]` 或五个 `Event.Movement.*` 映射错误。
