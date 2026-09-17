# Session + CommonUI 验收记录

# 2026-07-24 实机验收更正与诊断交接

## 原始 Mac Client 冻结采样（Windows Host -> Mac Join）

- 角色更正：本次是 **Windows 端创建 Steam 会话，Mac 端加入**。Windows 主机短暂看到 Mac 玩家加入；冻结、超时和掉线发生在 Mac 客户端。不得写成“Windows 客户端加入后掉线”。
- Mac Shipping 进程 `13359`（ARM64，UE 5.8.0 CL 55116800，macOS 26.5.2 / M1 Pro）在冻结状态下取得 5 秒和 10 秒 `sample`。两份均显示 `GameThread` 长时间等待 `FFrameEndSync::Sync -> FRenderCommandFence::Wait`。
- `RenderThread` 正在 `FDeferredShadingSceneRenderer::BeginInitViews -> FVisibilityTaskData::ProcessRenderThreadTasks` 等待任务；其可见性工作线程在 `FMetalDynamicRHI::RHIGetRenderQueryResult -> FMetalRHIRenderQuery::GetResult` 等待。故 HUD 动画和网络保活停止是 GameThread 被渲染围栏阻塞的结果。
- Steam 线程仍持续执行 `FOnlineAsyncTaskManagerSteam::OnlineTick -> SteamAPI_RunCallbacks`。本次证据排除了“GameThread 卡在同步 Steam 调用”作为此冻结的直接解释。
- 项目 `Config/DefaultEngine.ini` 当前设置 `r.AllowOcclusionQueries=True`。下次可仅以项目配置或启动参数禁用硬件遮挡查询，作为受控规避验证；不修改引擎、Steam、SteamSockets 或 CommonUser 插件源码。

## Mac 遮挡查询规避待验收

- 已加入项目级 `Config/Mac/MacEngine.ini`：`r.AllowOcclusionQueries=False`。它只作用于 macOS，保留 Windows 的默认设置，并不改动引擎或插件源码。
- 未重新打包、未进行实机验证，不能据此标记冻结已修复。
- 通过条件：同一提交、相同构建配置下，Windows Host -> Mac Join 后连续 60 秒保持 HUD 更新、可移动且未从 Windows 主机超时掉线；随后分别复测 Mac Host 与邀请 Join。

## 第二次 Mac Client 冻结采样（遮挡查询关闭后）

- 10:00 的重新 Cook/Stage 已将 `Config/Mac/MacEngine.ini` 列入 Mac UFS 清单，故该次测试的 `r.AllowOcclusionQueries=False` 配置已进入包体。
- 10:01 的桌面采样（PID `16885`）仍显示 `GameThread -> FFrameEndSync::Sync -> FRenderCommandFence::Wait`，但没有出现首次采样中的 `RHIGetRenderQueryResult`。
- 该次的直接阻塞线程是 `RHISubmissionThread`：`FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`。UE 5.8 代码表明此信号量依赖 Metal command buffer 完成回调；因此当前是 Metal 呈现/完成回调未返回，而非 Steam 调用、会话状态机或 HUD 逻辑阻塞。
- 下一次受控诊断仅使用启动参数 `-norhithread`，验证移除独立 RHI 线程后 Windows Host -> Mac Join 是否仍冻结；不要把该参数与其他渲染参数混用。

## `-norhithread` 观察与下一轮 Mac 包体诊断

- 使用 `-norhithread` 后，HUD 已显示且 App 可正常退出，但 Join 后画面仍冻结；因此它改善了独立 RHI 线程造成的整体挂死表现，不构成问题已修复的证据。
- `r.RHICmdBypass=1` 的包体复测仍冻结：10:19 的 sample（PID `2866`）不但保留 `GameThread -> FFrameEndSync::Sync -> FRenderCommandFence::Wait`，`RHISubmissionThread` 仍在 `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`。该项未改变根因，已从项目配置移除。
- `-gpulockstep` 在创建 Session 前已使单机运行严重卡顿，判定为不可用，未写入项目配置。
- 用户未创建 Session、仅打开衣柜也复现冻结（PID `21886`，10:41）。线程链仍为 `GameThread -> FFrameEndSync::Sync -> FRenderCommandFence::Wait` 和 `RHIThread -> FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`；Steam/网络不是必要条件。衣柜的 Mutable 预览异步更新是高优先级关联对象，但此 sample 没有显示其函数正在执行，尚不能认定为已证实根因。
- 不再继续叠加渲染 CVar；下一步应将可复现样本、`-norhithread` 与默认线程模式的栈、UE 5.8/M1 Pro/macOS 版本信息整理为 Epic 报告，或在另一台 M2+/不同 macOS 版本的 Mac 上做对照复现。

- 本节覆盖此前把 Steam 浏览和 Steam 生命周期描述为“待最终人工确认”的记录。当前 macOS 实机已证明该表述不足：Steam Online Host 到 HomeMap，以及接受平台邀请后的 Join 到 HomeMap，均可短暂看到 Pawn 后完全冻结；HUD 保持初始值，说明游戏线程未能进入稳定 Tick 或在首轮初始化中被阻塞。
- 该现象同时出现在 Host 和 Join，不能仅归因于平台邀请链。`UShootGameInstance::HandlePostLoadMap`、`UShootHomeHubStateComponent::StartHomeHubFlow`、Coordinator 清理与 Steam 网络初始化都是待排查候选，不是已经确认的根因。
- `W_SessionBrowserScreen` 也没有完成真实搜索验收：打开浏览器本身不调用 Find，刷新入口未在 PIE 证实能使 Coordinator 记录 `Lifecycle -> Searching` 或 Steam 搜索开始日志。此前的资产编译和按钮返回链测试不构成 Find 功能验收。
- 后续排查入口、精确日志命令和验收矩阵见同目录 `MacSteamFreezeAndFind_Handoff_交接.md`。在该文档的原始日志要求满足前，本任务不得改为 completed。

日期：2026-07-19

# Windows 编译

- 命令：`powershell -ExecutionPolicy Bypass -File .\Scripts\Build_Windows.ps1`
- Target：NewWorldOrderEditor Win64 Development
- 结果：Succeeded
- 最终重编译结果：Succeeded，总耗时 387.22 秒（清理停滞的旧编译/编辑器进程后，从干净状态完成 6 个 Action）。

# 资产与 PIE

- `WBP_GameMenu`、`WBP_SessionScreen`、`WBP_SessionResultButton`、`BP_ShootGameState`、`BP_ShootCharacter` 均已编译保存并处于 UpToDate 状态。
- HomeMap PIE 创建了 `W_OverallUILayout`，初始 Widget 列表中没有 `WBP_GameMenu`，证明地图加载不会再自动弹出游戏菜单。
- `BP_ShootCharacter.ShowMenuWidget` 图表已复核为向目标玩家的 `UI.Layer.GameMenu` 推入 `WBP_GameMenu`。自动化无法可靠触发该 BlueprintImplementableEvent，因此按 Q 打开和完整点击路径保留为人工 PIE 验收项。
- WBP_GameMenu 固定按钮点击事件已放在蓝图 EventGraph；C++ 不再绑定或覆盖其文案、布局和对齐。
- SessionScreen 运行时 `HostMapId` 为 `Map:/Game/Maps/HomeMap`，八个动作和可见返回按钮均存在。
- 离开/销毁按钮调用项目现有 `ShowConfirmationYesNo`；仅 Confirmed 分支进入 Coordinator，取消路径保持 SessionScreen 激活。
- SessionScreen 和 WBP_GameMenu 通过 `DeactivateWidget()` 正确退栈，不使用 `RemoveFromParent()`。

# LAN Host

真实 CommonSession Host 请求记录：

```text
LogCommonSession: OnCreateSessionComplete(SessionName: GameSession, bWasSuccessful: 1)
LogGameMode: ProcessServerTravel: /Game/Maps/HomeMap?bIsLanMatch?listen
LogNet: GameNetDriver ... listening on port 7777
```

结论：UE 5.8 的完整 `Map:/Game/Maps/HomeMap` 可以被 AssetManager 解析，Host 成功后由 CommonSession 执行一次 ServerTravel，项目 UI 没有重复 Travel。

# Ensure 扫描

当前 PIE 日志精确计数：

- `SessionInterface.IsUnique()`：0
- `Leader pose component skeleton doesn't match follower`：0
- `UMultiplayerSessionsSubsystem`：0

日志仍有一次与本任务无关的 Engine ConsoleManager `Ensure condition failed: false`。它不是 SessionInterface 或 LeaderPose 目标 Ensure，本任务不扩散修改该引擎控制台问题。

# Null/LAN 双进程生命周期

使用 Editor Development 模块启动独立 `-game -nosteam` 进程，角色参数为：

```text
-ShootSessionAutomation=HostHold|ClientLeave|HostDestroy|ClientHold
-ShootSessionAutomationMap=Map:/Game/Maps/HomeMap
```

场景 A 的最终日志为 `SessionAuto_A3_HostHold.log` 和 `SessionAuto_A3_ClientLeave.log`：

- Host 通过 CommonSession 创建 LAN 会话并监听 7777。
- Client Find 得到 1 个结果，Join 后进入 `NetMode=3`。
- Client 只有 1 次远端 Browse，主动 Leave 后返回 `/Game/Maps/HomeMap` 并完成。
- Client 退出后 Host 进程仍存活，证明客户端离开没有结束主机。

场景 B 的最终日志为 `SessionAuto_B4_HostDestroy.log` 和 `SessionAuto_B4_ClientHold.log`：

- Client Find/Join 后稳定进入 `NetMode=3`。
- Host 检测到 2 名玩家后执行销毁，Host 返回 `/Game/Maps/HomeMap`。
- Client 收到 HostClosedConnection 后由可配置 `SessionReturnMap` 覆盖 Engine 默认 FrontEndMap，实际返回 `/Game/Maps/HomeMap`。
- Host 和 Client 均记录 `Complete ... ReturnedToLobby` 并自动退出。

四份最终日志精确计数：

```text
SessionInterface.IsUnique()                              0
Leader pose component skeleton doesn't match follower   0
Client remote Browse                                     1 / client
```

# Development Cook

- 删除了已废弃且会触发 UE 5.8 ConsoleManager Ensure 的 `r.Mobile.VirtualTextures=False`；同一配置段已有 `r.VirtualTextures=False`，渲染意图不变。
- 全量 Cook 处理完 1522 个包后失败，阻塞来自既有 `WBP_AttributeMenu` 的 MVVM `AttributeViewModel.None` 绑定和 ShootReticle CDO 缺失材质引用。
- SessionUI、HomeMap 和本轮新增资产没有 Cook Error。该问题属于属性菜单/准星系统，修复前必须先按对应需求包审计，本任务不跨域修改。

# 尚需打包人工验收

- 两个不同 Steam 账号：Host、Find、选择结果 Join、Overlay 邀请和接受邀请。
- LAN 双机或两个独立打包实例：复核 Find 与 Join；本机两个独立 `-game` 进程已通过。
- 完成上述步骤后再把 SessionUI 状态从 98% 改为 completed；PIE 和 `-nosteam` 双进程不替代真实 Steam 验收。

# 2026-07-20 邀请、跨平台搜索与运行时状态补验

- Windows Editor Development C++ 重编译成功，8 个 Action，总耗时 390.67 秒；依赖缓存版本变化导致本次 Unity 单元重编译，非编译死锁。
- `WBP_SessionScreen` 新增蓝图视觉控件 `SessionDetailsText` 与 `PlayerListText`，资产编译状态 UpToDate，Widget hierarchy validation 通过。C++ 只刷新动态数据，未接管固定按钮、样式和 Slot 对齐。
- HomeMap PIE 初始 `WBP_GameMenu` 与 `WBP_SessionScreen` 实例列表为空，仍由玩家菜单输入打开。
- 当前 PIE 日志中 `SessionInterface.IsUnique()`、`Leader pose component skeleton doesn't match follower`、`Ensure condition failed` 均为 0 次。
- 平台邀请现在通过 `UShootSessionCoordinatorSubsystem::JoinSession`，并在 HomeMap、目标 LocalPlayer、PlayerController 与旧会话清理全部准备好后才发起；此项仍需两个 Steam 账号验证 Overlay 冷启动与跨地图 Join。
- UE 5.8 Steam 会删除 BuildUniqueId 不兼容的 Lobby 搜索结果。本项目已删除覆盖值并恢复引擎默认；下一轮 Windows/Mac 必须使用同一提交和相同构建配置重新打包，并在 SessionScreen 对照实际 BuildId。

# 2026-07-20 Mutable 首帧逐帧采样

```text
Tick 1: Pawn=1, Head=ChenHaoYu visible=true, Body=ChenHaoYu visible=false, Leader=None
Tick 3: Pawn=1, Head=head_2 visible=true, Body=Body_2 visible=true, Leader=CharacterMesh0
```

结论：截图中的重叠不是两个 Pawn，而是两个组件同时显示完整兜底人物。Body 现在等待最终 Mutable 输出和完整 LeaderPose 后才显示；普通换衣不进入隐藏/解绑路径。

# 2026-07-20 Session UI 表现职责复核

- `WBP_SessionScreen`、`WBP_SessionResultButton`、`WBP_SessionPlayerEntry` 均通过 MCP 编译，状态为 UpToDate，Widget hierarchy validation 为 valid。
- `WBP_SessionScreen` 的 Blueprint EventGraph 负责状态文本写入、搜索结果清空/创建、会话详情格式化、未加入/已加入 WidgetSwitcher 和玩家列表创建。
- `WBP_SessionResultButton` 的 Blueprint EventGraph 负责结果文案、满员可用态和 `OnClicked -> JoinStoredSession`。
- `WBP_SessionPlayerEntry` 的 Blueprint EventGraph 负责玩家序号、名称和 Ping 三列文本。
- Session UI C++ 范围检查未发现 `SetText`、`ClearChildren`、`CreateWidget`、`AddChild`、控件显隐、Slot 对齐或强制 `BindWidget`；插件目录无改动。
- 重构后 `NewWorldOrderEditor Win64 Development` 编译成功，UnrealBuildTool 结果 `Succeeded`，总耗时 98.54 秒。
- 重构后重新启动 HomeMap PIE，运行中 `WBP_GameMenu` 与 `WBP_SessionScreen` 实例均为空，证明地图加载仍不会自动弹菜单。该次运行新增日志中目标两个 Ensure、旧 Session 子系统和通用 `Ensure condition failed` 均为 0；日志文件中较早的 Widget GUID Ensure 来自资产删除控件时的首次编译，后续重复编译已 UpToDate，不属于本次 PIE 新增错误。

# 七项反馈关闭状态

1. 邀请接受停在 FrontEndMap：代码链已改为先进入可配置 `UShootGameInstance.SessionReturnMap`，等待 HomeMap、LocalPlayer、PlayerController 和清理完成，再通过 Coordinator Join；尚需当前提交双账号冷启动邀请实测。
2. Shipping 无法邀请：邀请入口已改为核对实际 `NAME_GameSession`；尚需当前提交 Shipping 包验证 Steam Overlay。
3. Windows/macOS 互相 Find 不到：已恢复引擎默认 BuildId，并确认 CommonUser OSSv1 的 10 条结果上限会被 SteamDevAppId 480 公共 Lobby 池挤占；项目层现扩大 Steam 候选窗口并显示实际 BuildId。仍须同一提交、相同配置的两端包实测，Development 与 Shipping 不混测。
4. macOS 退出崩溃：已审阅现有诊断报告，但它不是原始崩溃证据；若新包仍复现，需要 `.ips`、项目 `Saved/Logs` 和退出时序才能关闭。
5. 出生瞬间双角色：已完成，逐帧采样证明只有一个 Pawn，修复了 Head/Body 同时显示完整兜底网格。
6. 无法查看会话与玩家信息：数据链和蓝图玩家列表已实现并通过资产验证；真实多人名称、Ping 和主客机状态仍随第 1 至 3 项一起做打包视觉验收。
7. 创建会话后旧文案残留：蓝图状态回调会先清空并重建搜索列表，WidgetSwitcher 区分空闲与已加入状态；资产已验证，真实 Host 后页面视觉仍需当前包人工点击确认。

# 2026-07-20 Windows Shipping 打包恢复

- 用户禁用 MCP/Toolset 后，Cook 首先暴露 `/Game/UI/Mutable/Animation/ABP_WardrobePreview_Female` 对运行时 `MovieSceneAnimMixer` 的真实依赖。项目保留 `SequencerAnimMixerToolset=false`，只显式启用 `MovieSceneAnimMixer`；随后全量 Shipping Cook 通过，未再出现 `AnimBlueprintExtension_SequencerMixerTarget` Unknown structure。
- Cook 通过后，Stage 仍误报 `Failed sending oplog request to Zen at [::1]:8558`。进程、端口、`zen status`、UnrealPak 和 `/health/ready` 均证明 Zen 正常，失败点收敛到 AutomationTool 随附 .NET 10 对 IPv6 回环的健康检查。
- 将本次 Cook 生成的 `ue.projectstore` 主机改为 `127.0.0.1` 后，同一份数据成功读取 1514 条 oplog，Stage、IoStore、Package 和 Archive 全部完成，UAT 返回 `BUILD SUCCESSFUL`。
- 手工把本次 Cook 生成的 `ue.projectstore` 改为 IPv4 后，Build、Cook、Stage、IoStore、Package 和 Archive 曾返回 `BUILD SUCCESSFUL`，因此 `[::1]` 已被证明是故障点而不是素材或 Session 代码。该手工改动只用于诊断，未提交 `Saved`。
- 用户只使用编辑器 Package Project，不采用命令行包装脚本。最终项目方案改为官方 `UProjectPackagingSettings.bUseZenStore=False`：编辑器不再传 `-zenstore`，Cook 写本地文件，Stage 仍生成 Pak/IoStore。
- 已用读取同一 Project Packaging Settings 的完整 BuildCookRun 验证编辑器等价路径：Shipping Build、1520 余个资产 Cook、Stage、IoStore、Package 和 Archive 全部成功，总耗时 295.39 秒。IoStore 统计为 `Zen: 0`、`Loose File: 1,009,211,080`，UAT 最终返回 `BUILD SUCCESSFUL`，产物包含 `.pak/.ucas/.utoc`。
- 这次仅证明 Windows Shipping 构建链恢复。Steam 双账号、Windows/macOS 同提交同配置互相 Find、邀请接受和 UI 视觉验收仍是未关闭项目。

# 2026-07-20 完成审计补强

- 完成性审计发现旧实现只依赖 Widget 交互节奏，没有在 Coordinator 的 Host、Find、Join 入口阻止重叠异步请求。现在稳定状态和实际 `NAME_GameSession` 共同构成 C++ 门禁，重复点击或邀请重入不会第二次调用 CommonSession。
- `IsInSessionOrTransitioning()` 现在同时核对平台实际会话，避免 UI 状态曾进入 Error 时错误地认为可以直接接受另一个邀请。
- 邀请解析失败或等待 HomeMap、PlayerController、旧会话清理超时后，会清除缓存邀请并向 Session UI 广播 Error，不再只写一条日志后留下陈旧请求。
- 上述改动需要重新执行 Windows 编译和 Null/LAN 双进程回归；真实 Steam 邀请仍保留为双账号门禁。

# 2026-07-20 Join 初始 FrontEndMap 根因

- 最新 Null/LAN 回归中，Client Join 首次 Browse 为 `198.18.0.1/Game/Maps/FrontEndMap`，服务器欢迎后才加载 `/Game/Maps/HomeMap`。这证明搜索与网络连接成功，但纯主机连接地址被 UE 自动补成了 `GameDefaultMap`。
- 该现象与真实 Steam 邀请“先进入 FrontEndMap、随后没有留在好友地图”的反馈同源风险很高：握手成功时会被服务器纠正，握手失败或延迟时则可能停在错误地图。
- 项目现通过 CommonSession 的 `OnPreClientTravelEvent` 使用搜索结果广告 `MAPNAME` 补全连接 URL。修复后的回归必须证明首次远端 Browse 已直接包含 HomeMap，且仍只有一次 ClientTravel。

# 2026-07-20 URL 与退出生命周期回归

- 场景 A：HostHold + ClientLeave。Client 的连接地址从 `198.18.0.1:7777` 补全为 `198.18.0.1:7777/Game/Maps/HomeMap`，首次远端 Browse 直接进入 HomeMap；握手成功后客户端主动离开并返回 HomeMap，主机仍存活。
- 场景 B：HostDestroy + ClientHold。首次远端 Browse 同样直接进入 HomeMap；主机检测到两名玩家后销毁，Host 和 Client 都记录 `Complete ... ReturnedToLobby`，客户端没有落到 FrontEndMap。
- 两个场景中 `SessionInterface.IsUnique()` 和 `Leader pose component skeleton doesn't match follower` 均为 0。
- Development 自动化在首次 Host 和 Find 后故意立即重复调用一次；最终场景 G 记录 `DuplicateHostExpectedRejected Accepted=0` 与 `DuplicateFindExpectedRejected Accepted=0`，证明两次调用均在进入 CommonSession 前被协调层拒绝。客户端仍完成 Join、Leave 和返回 HomeMap，主机保持存活。

# 2026-07-20 最新 Shipping 交付包

- URL 补全、邀请失败收口和并发门禁完成后，再次执行不带 `-zenstore` 的完整 Shipping BuildCookRun。
- Shipping Game 模块重新编译，Cook、Stage、IoStore、Package 和 Archive 全部完成，总耗时 237.13 秒，最终 `BUILD SUCCESSFUL`。
- IoStore 再次确认 `Zen: 0`、`Loose File: 1,009,211,080`，本地最新产物位于 `Build/SessionLatestShipping/Windows`；Build 目录不纳入 Git。
- 该包可用于当前 Windows 端人工 Steam 验收。Mac 仍必须从同一最终提交重新构建，不能与旧 Mac Shipping 包混测。
- 最后将 Pending Join 地图清理由误放的 Host 失败分支移到 Join 失败、Cleanup 和邀请失败分支；随后 `NewWorldOrderEditor Win64 Development` 再次编译成功，总耗时 120.69 秒。

# 2026-07-22 跨平台搜索与 Lyra 浏览器改造

- 用户实测旧包仍是 Windows Host、macOS Find 为零；反向由 macOS Host 时，在创建成功并 Travel 到 HomeMap 后出现画面冻结。当前没有该次运行的原始 Mac `Saved/Logs` 或采样栈，不能把二次报告中的“DestroySession 回调丢失”当成 Host Travel 冻结根因。
- 项目删除 `BuildIdOverride=1001`，恢复引擎默认网络版本。此改动不会绕过 BuildUniqueId 兼容性过滤；两端仍必须同提交、同配置，并记录实际 BuildId。
- 代码审计确认 `Plugins/CommonUser` 的 OSSv1 搜索固定 `MaxSearchResults=10`，同时 SteamDevAppId 480 被所有 Spacewar 项目共享。Coordinator 仅在 Steam Online Find 时用同样的 `OSSv1`、Lobby、`GameSession` 条件扩大到 1000，再把结果交回 CommonSession 数据模型；插件未修改。
- 项目启用 `SteamSockets`，GameNetDriver 使用 `SteamSocketsNetDriver`，保留 IpNetDriver fallback。
- `W_SessionBrowserScreen` 已继承 `UShootSessionScreen`，`W_SessionBrowserEntry` 已继承 `UShootSessionBrowserEntry`。旧 Lyra 直接 Find/Join、Experience Cast 和 JoiningProgressWidget 链已清除；条目 OnClicked 在蓝图调用 `JoinStoredSession`，浏览页可见 BackButton 在 EventGraph 调用 `CloseSessionScreen`。三个相关 Widget Blueprint 编译 UpToDate 且 hierarchy validation 通过。
- 新增 Coordinator 状态迁移日志和 HomeHub 加载日志。若 Mac 再冻结，必须保留冻结前最后一条 `LogShootSessionCoordinator`、`LogShootHomeHubState`、`LogNet` 与 `LogLoad`；不要先加“3 秒强制 Idle”、手工 Shutdown Steam 模块或盲目重试掩盖真实停点。
- C++ 完整编译：`NewWorldOrderEditor Win64 Development`，7 个 Action，结果 `Succeeded`，总耗时 99.69 秒。
- SteamSockets 配置完成后执行场景 H：`HostHold + ClientLeave`。客户端找到 1 个 LAN 会话、Join 后为 `NM_Client`、主动离开后返回 HomeMap；主机保持 `NM_ListenServer`。重复 Host/Find 仍被拒绝。
- 执行场景 I：`HostDestroy + ClientHold`。主机检测到 2 名玩家后销毁；主机和客户端均完成清理并返回 HomeMap。HomeHub 日志在 Listen Server 上记录 `Listen=1 CleanupStandalone=0`，在 Client 上记录 `NetMode=3 CleanupStandalone=0`。
- `SessionAuto_H_*` 与 `SessionAuto_I_*` 四份日志中 `SessionInterface.IsUnique()`、`Leader pose component skeleton doesn't match follower`、`Ensure condition failed` 和 `UMultiplayerSessionsSubsystem` 计数均为 0。

# 2026-07-23 浏览器返回、失败恢复与最终 PIE 导航

- 重启编辑器后重新读取持久化 EventGraph，确认 `W_SessionBrowserScreen.BackButton` 是 `K2Node_ComponentBoundEvent`，并连接到项目父类 `UShootSessionScreen::CloseSessionScreen`。此前仅调用 `WidgetService.bind_event` 的瞬时结果没有持久化，不能作为完成证据。
- `BP_OnSessionStateDataChanged` 现在先把 Coordinator 的 `Status` 写入蓝图 `NoGamesMessage`，再进入状态 Switch。`Error` 分支显示 `NoGamesBorder`、用空的 `SearchResults` 清空 `LyraListView`，最后进入既有 `UnlockAfterSearchFinish`，不会把刷新按钮和转圈永久锁住。
- 删除 `WBP_SessionScreen` 中无任何执行连接的旧 `FindOnlineSessions` 节点；FindOnlineButton 只负责打开浏览器，浏览器 RefreshButton 才负责发起搜索。
- `UShootSessionCoordinatorSubsystem::StartExpandedSteamSearch` 捕获本次 Search 指针。若 OSSv1 在 `FindSessions` 返回 `false` 之前已经同步执行完成委托并 Reset 请求，返回值分支不会再次 `NotifySearchFinished`。
- `NewWorldOrderEditor Win64 Development` 编译成功：3 个 Action，`Result: Succeeded`，总耗时 13.03 秒。
- HomeMap PIE 初始 `WBP_GameMenu`、`WBP_SessionScreen` 和 `W_SessionBrowserScreen` 实例为空。把 GameMenu 推入目标玩家 Stack 后，以下真实蓝图按钮链通过：

```text
WBP_GameMenu.OnlineButton
  -> WBP_SessionScreen.FindOnlineButton
  -> W_SessionBrowserScreen.BackButton
  -> WBP_SessionScreen.BackButton
  -> WBP_GameMenu.ReturnButton
```

- 衣柜链通过真实按钮验证：`WBP_GameMenu.WardrobeButton -> W_Cloth.BackButton -> WBP_GameMenu`。
- PIE 日志计数：`SessionInterface.IsUnique()` 0、`Leader pose component skeleton doesn't match follower` 0、通用 `Ensure condition failed` 0、`UMultiplayerSessionsSubsystem` 0、Blueprint Runtime Error 0。
- MCP 不能可靠伪造 Coordinator 的真实异步失败回调，因此 `Error` 分支以重启后的连接级持久化、蓝图编译和 hierarchy validation 为证据；真实 Steam 搜索/Join 失败后的可重试视觉仍随双账号打包验收确认。

# 2026-07-24 macOS 衣柜 Metal 冻结诊断

- macOS ARM64 Shipping 包在未创建、查找或加入 Session 的前提下，进入游戏后打开 `W_Cloth` 可稳定冻结；因此 Steam、Session 与网络 Travel 不是该离线复现的必要条件。
- `Mutable.MaxTextureSizeToGenerate=1024` 的 macOS 临时试验未通过：衣柜 UI 与基础预览角色已出现，随后画面冻结。11:01 +0800 的进程采样仍为 `FFrameEndSync::Sync -> FRenderCommandFence::Wait`，以及 `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`。
- 结论：1024 贴图上限不是可用缓解措施，已删除。此项不是 Mutable 已被排除的证据；采样时间点已在提交到 Metal 的命令缓冲区无法完成之后。

# 2026-08-30 Lyra 设置与副本生命周期验收

## Windows 编译

- 首次最终编译：`NewWorldOrderEditor Win64 Development`，5 个 Action，`Result: Succeeded`，总耗时 126.85 秒。
- Online PIE 暴露 Travel URL 冒号问题后修复并增量重编译：4 个 Action，`Result: Succeeded`，总耗时 153.74 秒。
- 修复方式：URL 中只传 `DA_Experience_DungeonTest` 这类资产名称；`AShootGameModeBase` 分别按 `ShootExperienceDefinition` 与 `LyraUserFacingExperienceDefinition` 类型重建 Primary Asset Id，避免把 `Type:Name` 中的冒号写进 Travel URL。

## 设置页面

- FrontEndMap PIE 中存在并激活 `W_FrontEnd`；通过该运行时实例调用与 Options 按钮相同的 `OpenSettingsScreen` 路径后，`W_LyraSettingScreen` 被推入目标 LocalPlayer 的 CommonUI 栈，实例可见且处于 Activated。
- Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五页及全部迁入设置条目通过最终资产审计；相关 Widget Blueprint 状态均为 UpToDate。
- 键位注册只扫描 `/Game/Blueprints/Input/IMC_Default`，最终映射为 51 条，其中 26 条键盘/鼠标映射具有 Player Mappable 元数据；没有迁入 Lyra Gameplay 键位资产。
- Replay 保留数量清理已重新接到 `ULyraSettingsLocal`；当前副本 UFE 的 `bRecordReplay=false`，录像浏览 UI 留作后续功能。
- Performance Stats 调试页未纳入首版设置验收。

## HomeMap 与 Local 生命周期

- HomeMap PIE 中只存在 1 个 `BP_ExpeditionTerminal`，旧的 `DungeonPortal_ToTestMap` 实例为 0；终端使用可见 Cube 占位表现并为目标 LocalPlayer 打开 `W_HostSessionScreen`。
- Host 页面默认状态为 Offline、最多 4 人、本地玩家 1、允许中途加入、Bot 不可用。切到 Online 时本地玩家数被约束为 1；关闭 Join-in-progress 可正确写入请求；尝试启用 Bot 仍保持关闭。
- Local 请求成功进入 `/Game/Maps/TestMap_ListenServer`。地图中存在 1 个返回门，调用返回门后成功回到 HomeMap。

## Online Lobby 生命周期

- Online 请求通过 CommonSession 创建会话并以 Listen Server 进入 LobbyMap；运行时存在 1 个 `W_ExpeditionLobbyScreen` 和 1 个 `UShootExpeditionLobbyComponent`。
- Lobby 运行状态为 Waiting，携带目标地图 `/Game/Maps/TestMap_ListenServer`、目标 Experience `DA_Experience_DungeonTest`、允许中途加入和 Bot 关闭策略；房主具有开始副本权限。
- 房主调用 Start 后成功 ServerTravel 到 TestMap；日志中的最终 URL 为 `?Experience=DA_Experience_DungeonTest`，不再触发 `CanServerTravel` 非法 URL。
- 在线副本返回门的 `bEndOnlineSessionBeforeTravel=true` 与 `bClearAllRuntimeSessionsBeforeTravel=true` 已落盘。返回时 Coordinator 成功完成 DestroySession，随后进入 HomeMap，运行时会话状态为未加入。
- 本项是同一台 Windows 编辑器中的 Online/Listen Server PIE 验收，不替代两个 Steam 账号的真实 Find、Join、中途加入和跨平台验证。

## 资产与运行时错误回归

- 最终生命周期审计确认 Lobby Widget、Lobby Experience、Lobby GameMode、终端表现、HomeMap 唯一终端、TestMap 返回门和 UFE 地图/Experience/人数配置全部通过。
- 本节原先把重复迁入的 `W_LyraMenuButton` 当成应继续维护的资产，该结论已被 2026-08-31 用户复核纠正：项目 Foundation 已有同职责按钮，当前页面以项目按钮和重定向结果为准。后续不得恢复重复资产。
- HomeMap PIE 中依次激活并退栈 `W_HostSessionScreen` 与 `W_ExpeditionLobbyScreen`；运行时新增日志段中 `Blueprint Runtime Error`、`Accessed None`、`SessionInterface.IsUnique()`、`Ensure condition failed`、`CanServerTravel`、`Fatal error` 均为 0。

## 保留验收项

- 使用两个真实 Steam 账号验证创建、Find、Join、房主开始、中途加入关闭策略和成员离开。
- 使用同一提交与相同构建配置复测 Windows/macOS，并继续处理既有 macOS Metal 冻结。
- 对设置页、Host 页和 Lobby 页做最终分辨率适配、样式与文案打磨；当前验收聚焦功能闭环和 CommonUI 生命周期。
- 后续实现 AI 队友填补空位、录像浏览页和可选 Performance Stats 调试页。

# 2026-08-31 设置导航与三副本目录修正

## 编译与资产

- 用户执行 `Scripts/Build_Windows.ps1`：`NewWorldOrderEditor Win64 Development` 共 6 个 Action，`Result: Succeeded`，总耗时 180.08 秒。
- Unreal 资产审计确认三套完整目录均存在且互不混用：
  - `DA_UFE_DungeonTest -> TestMap_ListenServer -> BP_TestListenGameMode -> DA_Experience_DungeonTest`。
  - `DA_UFE_SplitScreenTest -> TestMap_SplitScreen -> BP_TestGameMode -> DA_Experience_SplitScreenTest`，`bSupportsOnline=false`。
  - `DA_UFE_ExpeditionSandbox -> TestMap_ExpeditionSandbox -> BP_ExpeditionSandboxGameMode -> DA_Experience_ExpeditionSandbox`。
- 每张副本地图均有返回 HomeMap 的 Portal；Host Widget 和 Tile Widget 编译状态为 UpToDate。选择 Tile 的图表不再直接调用 Host。

## Host 交互与生命周期

- Host 页面显示三张可选择副本卡片；独立显示 Create/Enter、Search Rooms、Back、本地玩家数量和 Mid-Join 策略。Add/Remove 本地玩家受 UFE 最大人数限制；切到 Online 时本地人数回到 1。
- SplitScreenTest 在 Online 状态下被选择时自动回退 Local，且不能构造 Online Hosting Request。DungeonTest 与 ExpeditionSandbox 支持 Listen Server。
- PIE 通过真实 HostButton 委托选择 ExpeditionSandbox，进入 `/Game/Maps/TestMap_ExpeditionSandbox`；运行时 GameMode 为 `BP_ExpeditionSandboxGameMode_C`，Experience Manager 当前资产为 `DA_Experience_ExpeditionSandbox`。调用地图 Portal 后成功回到 HomeMap。
- Online DungeonTest 的 `Host -> LobbyMap -> 房主 Start -> TestMap_ListenServer -> 销毁 Session -> HomeMap` 已在上一节完成，不因本次纯目录/交互修正重复执行。

## 设置交互与样式

- FrontEndMap 通过 `W_FrontEnd.OpenSettingsScreen` 打开真实设置页。运行时 TabList 注册 5 个可见 Tab：Gameplay、Video、Audio、Mouse & Keyboard、Gamepad，逐个选择均成功。
- Video 页运行时显示 Window Mode、Display、Resolution、Graphics Quality 等条目；Windows 平台配置具有 `Platform.Trait.SupportsWindowedMode`。
- 设置产生未应用更改后，底部动作栏运行时生成 3 个 `W_BoundActionButton`：Back、Apply Changes、Cancel。三个实例的 Style 均为 `/Game/UI/Foundation/Buttons/ButtonStyle-Clear`，截图 `Saved/VibeUE/Captures/capture-game-20260831-092329.png` 确认不再出现默认白色按钮边框。
- `W_BoundActionButton` 资产在编辑器保存缩略图时会以无 LocalPlayer 的预览实例执行旧的 CommonInput 绑定图表，日志出现一次 `GetLocalPlayerSubsystem` 为空的编辑器缩略图 Warning；随后真实 FrontEndMap PIE 段没有 Blueprint Runtime Error 或 Accessed None。该编辑器预览警告不等同于玩家运行时错误。
