# Mac Steam 冻结与 Find Session 诊断交接

状态：runtime_validation_required

更新时间：2026-07-24

# 本文目的

本文只交接已经观察到的运行时事实、代码入口和下一步诊断顺序。它不把猜测写成根因，也不授权修改 `Plugins/CommonUser`、Lyra 插件或引擎源码。

# 当前阻断现象

## 2026-07-24 原始采样：Windows Host，Mac Client Join 后渲染死锁

本次已取得可复核的原始证据，测试角色必须严格区分：**Windows 端创建 Steam 会话，Mac 端加入；冻结和后续掉线发生在 Mac 客户端。** Windows 主机能看到 Mac 玩家加入玩家列表；随后 Mac 因本地 Tick 停止而超时离开会话。此前将该现象描述为“Windows 客户端掉线”的说法不正确，特此更正。

- 截图：Mac 已进入 HomeMap，画面停在两个完整角色，HUD 血条仍为初始状态。
- 进程：`NewWorldOrder-Mac-Shipping`，ARM64，UE 5.8.0 CL 55116800，macOS 26.5.2（M1 Pro）。
- 原始采样：`~/Desktop/NewWorldOrder-host-freeze.sample.txt`（5 秒）和 `~/Desktop/NewWorldOrder-host-freeze-2.sample.txt`（10 秒）。
- 两份采样一致：`GameThread` 停在 `FEngineLoop::Tick -> FFrameEndSync::Sync -> FRenderCommandFence::Wait`；`RenderThread` 停在 `FDeferredShadingSceneRenderer::BeginInitViews -> FVisibilityTaskData::ProcessRenderThreadTasks`，等待可见性任务；对应 Background Worker 停在 `FMetalDynamicRHI::RHIGetRenderQueryResult -> FMetalRHIRenderQuery::GetResult`。
- `OnlineAsyncTaskThreadSteam` 仍在执行 `FOnlineAsyncTaskManagerSteam::OnlineTick -> SteamAPI_RunCallbacks`，而不是被同步 Steam API 卡住。

结论：本次冻结是 Metal 硬件遮挡查询/渲染任务同步链的阻塞，令 GameThread 停在帧尾围栏，进而停止 HUD、输入、网络心跳和 Mac 客户端的保活。它不是已证实的 Steam 会话状态机、邀请、Cleanup 或项目 HUD 逻辑死锁。项目当前 `Config/DefaultEngine.ini` 明确启用了 `r.AllowOcclusionQueries=True`，可作为下一次打包前的**项目级规避验证**候选；不得因此修改引擎、OnlineSubsystem 或 Steam 插件源码。

本证据只覆盖“Mac 客户端 Join”。历史上报告过的“Mac Host 冻结”仍需使用同样方式单独复现和采样，不能由本次结果代替。

## 2026-07-24 Mac 项目级规避配置已加入，待重新打包验证

已新增 `Config/Mac/MacEngine.ini`，仅对 macOS 包体设置：

```ini
[/Script/Engine.RendererSettings]
r.AllowOcclusionQueries=False
```

该设置不改动 Unreal Engine、Steam 或任何插件源码，并且不影响 Windows。它只作为对已采样的 Metal 硬件遮挡查询等待链的可回退规避验证，不是对上游根因的修复声明。需要重新打包 Mac Shipping；既有 App 不会自动获得此设置。

验收顺序：以相同提交和构建配置让 Windows 创建 Steam 会话，再由 Mac 加入；持续移动并观察 HUD/网络保活至少 60 秒，同时让 Windows 主机确认 Mac 不超时掉线。随后单独复测 Mac Host 与邀请 Join。若仍复现，必须先根据新采样选择下一项单变量诊断，并保留证据向 Epic 报告；不要改引擎或插件。

## 2026-07-24 第二次 Mac Client 冻结采样：Metal 呈现信号量等待

本次包体在 10:00 重新完成 Cook/Stage；`FinalCopyMac_UFSFiles.txt` 明确列出 `NewWorldOrder/Config/Mac/MacEngine.ini`，因此 `r.AllowOcclusionQueries=False` 已随包部署，不能再将本次采样视作未使用新配置的旧包。

- 原始采样：`~/Desktop/NewWorldOrder-host-freeze.sample.txt`，进程 `16885`，采样时间 10:01。
- `GameThread` 仍在 `FEngineLoop::Tick -> FFrameEndSync::Sync -> FRenderCommandFence::Wait` 等待。
- 此次没有出现 `FMetalDynamicRHI::RHIGetRenderQueryResult` 或可见性遮挡查询栈；取而代之的是 `RHISubmissionThread` 在 `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait` 永久等待。
- UE 5.8 的此等待由上一帧 Metal command buffer 的 completed handler 发信号；它没有完成会让 RHI、Render 和最终 GameThread 连锁停住。Steam 并非该等待调用链的一部分。

结论：禁用硬件遮挡查询不足以消除冻结，且本次证据将当前阻断点收敛为 UE 5.8 Metal 呈现/command-buffer 完成链。下一次只做一次互不混杂的启动参数诊断：`-norhithread`（禁用独立 RHI 线程），再次复现 Windows Host -> Mac Join 并采样。不要同时添加 `-gpulockstep` 或其他渲染参数，以保留因果性。该参数不修改代码、引擎或插件，也不应作为最终修复承诺。

## 2026-07-24 `-norhithread` 现象与下一轮包体配置

用户用 `-norhithread` 启动后，HUD 已能完整显示，并且 App 可由正常退出路径关闭；但画面仍在 Join 后冻结。它证明独立 RHI 线程会放大此前的全局卡死/无法退出表现，却还不能证明冻结根因已经消失。

下一轮包体曾在 `Config/Mac/MacEngine.ini` 加入仅 macOS 生效的：

```ini
[ConsoleVariables]
r.RHICmdBypass=1
```

它让 RHI 命令立即执行，跳过并行 RHI command-list 路径；代价是失去 CPU 并行度，通常会降低 CPU 受限场景的帧率。本轮重新打包后的采样（PID `2866`，10:19）仍显示 `RHISubmissionThread -> FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`，GameThread 仍在 `FFrameEndSync::Sync -> FRenderCommandFence::Wait`。因此该 CVar **未改变根链，已从 `MacEngine.ini` 移除**，不得作为长期配置保留。

## 2026-07-24 `-gpulockstep` 已否决；衣柜单机复现把 Steam 排除在外

- `-gpulockstep` 在尚未创建 Session 的单机阶段已使运行严重卡顿，不能作为实际修复或继续验证的启动参数，后续不得要求用户再次使用它。
- 用户未创建 Session、仅打开衣柜即稳定冻结。10:41 的 sample（PID `21886`）仍显示 `GameThread -> FFrameEndSync::Sync -> FRenderCommandFence::Wait`，以及 `RHIThread -> FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`；Metal 的命令队列线程停在 `IOGPUMetalCommandQueue` 的 kernel 调用中。
- 这将 Steam、网络 Travel 和远端复制 Pawn 从该冻结的必要条件中排除。衣柜入口会调用项目的 `UMutableAppearanceComponent::InitializePreviewComponents -> ApplyPreviewAppearanceTags -> UpdateSkeletalMeshAsync`，因此 Mutable/其生成资产是高优先级关联对象；但 sample 没有停在该函数，不能据此断言 Mutable 已被证明为根因。
- 不修改项目 C++、引擎或插件源码，也不关闭 Mutable 功能。下一步应使用衣柜作为最小复现，收集 Mutable 更新前后的运行日志和 Metal 调试信息，针对具体 CO、参数或输出网格建立证据。

当前仍保留的 `r.AllowOcclusionQueries=False` 已补充中文注释，包含原始证据、性能代价、不受影响的裁剪机制，以及回滚条件，便于后续维护者判断。

## Steam Host 与邀请 Join 都会冻结

macOS 上使用 Steam Online 会话时，以下两条路径均可复现：

1. 创建 Steam 会话，Travel 到 HomeMap。
2. 接受 Steam 平台邀请，Join 后 Travel 到 HomeMap。

两个路径的共同现象：

- 可以短暂看到 Pawn 已在场景中生成，说明网络连接/地图加载至少部分完成。
- 随即画面完全静止，不黑屏、不正常退出。
- 血条等 HUD 保持初始值，没有进入稳定更新。
- Development/编辑器与 Shipping 均报告过同类现象。

结论：邀请链不是唯一嫌疑，因为 Host 不经过邀请链也会冻结。现阶段不能声称是 Steam SDK 同步调用、Cleanup 轮询、Mutable、HUD 或某一项地图回调已经导致冻结。

## Session Browser Find 未闭环

`UShootSessionScreen::OpenSessionBrowser()` 只把 `W_SessionBrowserScreen` 推入 CommonUI 层，不调用 `UShootSessionCoordinatorSubsystem::FindSessions()`。

当前浏览器要依赖 Widget Blueprint 内的 Refresh/激活图表启动搜索。此前 PIE 的按钮委托检查没有出现以下 Coordinator 日志：

```text
Lifecycle ... -> Searching
Starting expanded Steam lobby search ...
Expanded Steam lobby search completed ...
```

因此“打开浏览器后一直 Loading、没有结果”是未完成的功能闭环，不能把结果归因于 Steam 结果为空，更不能标记 Find 为已验收。

# 已知边界与不应做的事情

- `BuildIdOverride` 已删除，恢复引擎默认网络版本。跨平台测试必须使用同一提交、相同构建配置；不要把 Development 与 Shipping 的结果混为一组。
- 项目层可直接使用 OSSv1 做扩展 Steam 搜索，但上游 Plugins 保持只读。当前实现仅在 Steam Online 时将候选上限扩大到 1000，目的是避免 AppId 480 公共 Lobby 挤掉项目结果。
- `HomeMap` 是玩家家园/联机大厅，也是当前 Host/Join 目标图；不能照搬 Lyra 的专用 FrontEndMap 逻辑，在每次 HomeMap BeginPlay 都无条件 CleanUp。
- 不要根据“可能是 Destroy 回调丢失”直接把 Coordinator 强制置 Idle，或跳过 `CommonSessionSubsystem->CleanUpSessions()`。这会把状态机与真实 `GameSession` 分叉，可能重新引入重复 Host/Join 与 `SessionInterface.IsUnique()`。
- 不要在项目模块 `ShutdownModule` 或 `GameInstance::Shutdown` 猜测性关闭 Steam OnlineSubsystem。退出期 Steam/ConsoleManager 崩溃需要原始 `.ips`、退出日志和官方兼容性证据后单独处理。

# 优先收集的原始证据

## macOS Development 或 Shipping 日志

Shipping 默认不一定写可用日志。用终端启动已打包 App，复现一次 Host 冻结，再保存冻结前最后 200 行：

```bash
open "/Users/zhaoyijie/Documents/Unreal Projects/MyFirstGame/NewWorldOrder/Build/Mac/NewWorldOrder-Mac-Shipping.app" --args -log -stdout -LogCmds="LogShootGameInstance Verbose,LogShootSessionCoordinator Verbose,LogShootHomeHubState Verbose,LogNet Verbose,LogLoad Verbose"
```

实际 App 名或路径不同可替换，但不要删除 `-log -stdout` 与 `-LogCmds`。若编辑器复现，直接保存 Output Log 同一时间段。

每一份日志必须标出测试类型：Host、手动 Find/Join、接受邀请、客户端离开、主机销毁，以及是否 Development/Shipping。

## 冻结时主线程采样

画面冻结后，不要立刻强制退出。先用 Xcode、Activity Monitor Sample Process 或 `sample <pid> 5` 获取主线程调用栈。目标是确认它停在：

- Steam/SteamSockets 调用。
- 在线会话回调或 Cleanup 轮询。
- 地图加载或网络 Travel。
- Pawn、Mutable、ASC、HUD 或 UI 初始化。

没有该采样前，不能把“HUD 为 0”解释为 HUD 本身死锁；它仅说明后续更新没有发生。

# 代码调用点

| 位置 | 作用 | 诊断问题 |
| --- | --- | --- |
| `Source/NewWorldOrder/Private/System/ShootGameInstance.cpp` `HandlePostLoadMap` | 返回地图加载后启动缓存邀请 Join | Host 同样冻结，故该函数不是唯一对象；先用日志确认 Host 是否到达此处。 |
| 同文件 `TryJoinRequestedSessionAfterTravel` | 等 HomeMap、目标 LocalPlayer、PlayerController、旧会话清理后再 Join | 仅邀请 Join 路径；检查是否发生重复 Join 或重试循环。 |
| 同文件 `HandleNetworkFailure` | 断开后下一帧回到可配置 SessionReturnMap | 不是正常 Host 后立即冻结的首要嫌疑，但需查看是否误报 NetworkFailure。 |
| `Source/NewWorldOrder/Private/UI/Frontend/ShootHomeHubStateComponent.cpp` `StartHomeHubFlow` | HomeMap Standalone 入口才清理残留会话 | Host/Join 到 HomeMap 都会执行 BeginPlay；日志会显示 NetMode、listen、LanMatch、是否发起 Cleanup。 |
| `Source/NewWorldOrder/Private/Online/ShootSessionCoordinatorSubsystem.cpp` `HostSession`、`JoinSession` | CommonSession 请求与生命周期状态机入口 | 检查一次操作是否只进入一次 Hosting/Joining。 |
| 同文件 `HandlePreClientTravel` | 把广告 `MAPNAME` 补入客户端连接 URL | 仅 Client Join；Host 冻结不应由它单独解释。 |
| 同文件 `CleanUpResidualSession`、`PollCleanupCompletion`、`HasPlatformSession` | 真实 `NAME_GameSession` 清理与异步完成轮询 | 只在日志显示异常 Cleanup 或轮询持续时再调查；不要先猜 macOS 回调丢失。 |
| `Source/NewWorldOrder/Private/UI/Menu/ShootSessionScreen.cpp` `OpenSessionBrowser` | 仅打开 Browser | 浏览器入口没有搜索副作用，需在 Widget Blueprint 侧建立明确的首次查询/刷新调用。 |

# 推荐诊断顺序

1. 用同一 Mac 包分别复现 Steam Host 与邀请 Join，取日志和主线程采样。
2. 在两份日志中标出 `Lifecycle`、`HomeHub loaded`、`ProcessServerTravel`/`Browse`、`PostLoadMap`、NetworkFailure 的先后顺序。
3. 若冻结前出现 Cleanup，先确认 `HomeHub loaded` 的 `CleanupStandalone=0/1` 是否符合当次 NetMode 与 URL。只有该证据出现后，才考虑针对性修复 HomeHub 清理边界。
4. 若没有 Cleanup，而主线程采样停在 Steam/SteamSockets，保留最小复现包和引擎版本信息，查 UE 5.8/Steamworks macOS ARM64 的官方问题或升级路径；不要自行改插件。
5. 若主线程采样停在项目 Pawn/Mutable/ASC/HUD 初始化，单独建立对应任务包。会话状态机不承担这些系统的修复。
6. 修复前后均执行 Steam Host 后至少 30 秒持续移动/属性更新验证，再测试邀请 Join；“能看到 Pawn 一帧”不算通过。

# Find 修复的目标形态

浏览器必须提供一个明确、可见且只有一条业务入口的搜索流程：

```text
打开 Browser
  -> Widget Blueprint 的首次查询或用户点击 Refresh
  -> UShootSessionScreen 的 BlueprintCallable FindOnline/FindLAN
  -> UShootSessionCoordinatorSubsystem::FindSessions
  -> Searching
  -> Results / 空结果 / Error 三种终态之一
```

固定按钮的 EventGraph、文本、Spinner 和列表视觉保留在 Widget Blueprint。C++ 只提供原始会话数据、状态与语义化业务方法。不要把控件绑定、文字设置或布局控制重新塞回 C++。

验收最小项：

1. 打开 Online Browser 仅发起一次 Online Find，并在日志看到一次 Searching。
2. Refresh 再发起一次，不产生重叠请求。
3. 无结果时停止 Loading 且给出空状态。
4. 错误时停止 Loading 且给出可重试状态。
5. 有结果时行按钮进入 Coordinator Join，失败后可再次搜索。

# 当前任务状态

- 不允许标记 completed。
- 不允许宣称 Steam 跨平台 Find、邀请、Host/Join 已通过。
- 不修改任何上游插件或引擎。
- 在得到上面两类原始证据后，再建立最小代码修复提交和新的验收记录。

# 2026-07-24 衣柜冻结：1024 生成贴图上限无效

- 为排查 Mutable 生成资源体积，macOS 包曾临时设置 `Mutable.MaxTextureSizeToGenerate=1024`。该值不关闭 Mutable，也不跳过网格更新。
- 本次衣柜界面与基础预览角色已经绘制出来后仍永久冻结；11:01 +0800 的 `sample` 显示与此前完全同类的链：GameThread 在 `FFrameEndSync::Sync -> FRenderCommandFence::Wait` 等待，RHIThread 在 `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait` 等待，Metal command-queue 线程停在 `IOGPUMetalCommandQueue` 提交。
- 采样发生在渲染已停止之后，未保留 Mutable 调用帧；它不能证明 Mutable 无关，但已经排除“只要压低生成贴图尺寸即可恢复”的假设。
- 已从 `Config/Mac/MacEngine.ini` 删除该临时值，避免后续包在画质下降的同时继续冻结。保留遮挡查询配置仅作为先前 query-result 等待的独立诊断，不把它宣称为本次 present 死等的解决方案。

## 2026-07-24 Development 包：衣柜第二次 Mutable 更新与冻结的时间关联

- Development 包也复现。运行日志在进入 HomeMap 时记录玩家本体的 `LogMutable: Started Update Skeletal Mesh Async`（frame 313）；打开衣柜时，在 frame 513 记录另一 `CustomizableObjectInstance` 的同一开始事件，随后衣柜 UI 完成焦点设置，frame 514 记录预览 LUT 格式，之后不再有游戏帧日志。
- 同一轮的 11:25 sample 仍为 GameThread 等待渲染围栏、RHI 提交线程等待 `PresentDrawLayers` 的帧可用信号量、Metal queue 在 `IOGPUMetalCommandQueue` 提交。它把“第二次预览 COI 更新开始”和故障时间窗口关联起来，但不把相关性误写成 Mutable 内部已被证明的根因。
- 此轮从 Finder 启动，日志中的命令行为仅 `-installed`；未真正带上 `-MetalRuntimeDebugLevel=2` 和 `LogCmds`，因此还需要一次不重打包、直接从终端运行当前 Development `.app` 的受控采集。

## 2026-07-24 Metal 验证层：首次衣柜成功，角色切换仍触发同一呈现死等

- 当前 Development `.app` 已直接使用 `-MetalRuntimeDebugLevel=2 -LogCmds="LogMutable VeryVerbose,LogMetal VeryVerbose,LogRHI Verbose"` 启动。验证层和首次管线状态创建造成明显卡顿，属该诊断的预期代价。
- 与默认运行不同，首次打开衣柜的男性预览完成：`CO_Character_M` 的 preview COI 更新在约 1.10 秒内完成，画面可显示。切换角色/性别后再次永久冻结。
- 已在仍运行的冻结进程上取得独立 10 秒采样：`~/Desktop/Wardrobe-switch-freeze.sample.txt`（11:34）。GameThread、RHI 提交线程和 Metal queue 分别仍为 `FFrameEndSync::Sync -> FRenderCommandFence::Wait`、`FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`、`IOGPUMetalCommandQueue` 提交；所以验证层改变的是触发时机，未消除根链。
- verbose 日志在约 1.3 秒内记录了三次明确的 `SwitchToCharacter`（男→女、女→男、男→女）。用户已确认是因 UI 已无响应而连续补点，不是业务层自动重入。每次按设计会分别为玩家 COI 和衣柜预览 COI 提交一次更新；这解释了该次交错更新负载，但不构成“项目代码自动重复切换”的证据。验证层没有在卡死前报告 Metal 或 Mutable 校验错误。
- 随后的干净启动使用同一验证命令，在首次打开衣柜、尚未能点击切换前再次冻结。男性预览 COI 的更新已经记录为完成（`UpdateTime=0.047291 s`），但 11:40 的 `~/Desktop/Wardrobe-open-freeze-metaldebug.sample.txt` 仍是同一 GameThread/RHI/Metal queue 呈现等待链。故验证层仅改变触发时机/概率；“切换角色”及连续点击都不是冻结的必要条件。

## 2026-07-24 正常 Steam Host：会话成功后仍在同一 Metal 呈现点冻结

- 用户清理桌面旧采样后，正常经 Steam 启动当前包，创建一次会话即冻结；无需恢复旧 txt。本轮已从现场进程取得新的 `~/Desktop/Steam-host-freeze.sample.txt`（11:46）。
- 运行日志严格显示：Steam Lobby 创建成功（真实 lobby id）→ `OnCreateSessionComplete(... bWasSuccessful: 1)` → `/Game/Maps/HomeMap?listen` server travel 成功 → `SteamSocketsNetDriver` 在 7777 监听 → `OnStartSessionComplete(... bWasSuccessful: 1)` → Listen Server HomeMap 进入 InProgress。故会话创建、Travel、网络驱动和 Steam 启动均在冻结前完成。
- 紧随其后玩家本体 COI 记录 `Started Update Skeletal Mesh Async`；新的 sample 仍为 GameThread `FFrameEndSync::Sync -> FRenderCommandFence::Wait`、RHI `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`、Metal queue `IOGPUMetalCommandQueue` 提交。`OnlineAsyncTaskThreadSteam` 继续在 `SteamAPI_RunCallbacks`，并非阻塞调用者。
- 这将 Steam Host 归类为与离线衣柜不同的触发入口，而不是根栈；二者的共享终点是 UE 5.8/macOS Metal command-buffer 完成/呈现等待。Mutable 仍是两条路径中紧邻冻结的共同动态资源工作，但尚不能因此归咎插件或项目 C++。

## 2026-07-24 Xcode GPU Capture：停止继续尝试；macOS Watchdog 证据已保留

- Xcode Scheme 已确认 GPU Frame Capture 为 Metal。执行 `Debug -> Capture GPU Workload` 后未得到可恢复的 `.gputrace`，反而导致整机无响应并需要重启；后续不应把 GPU Capture 当作本机的常规复现手段。
- 重启后系统生成 `/Library/Logs/DiagnosticReports/WindowServer-2026-07-24-121803.ips`：WindowServer 被 watchdog 记录为 40 秒无响应。关联 watchdog spin / Xcode resource 诊断时间为 12:18。
- 系统诊断中的 GPU replay 栈包含 `GPUToolsReplayService -> AGXMetalG13X -> IOGPUMetalCommandQueue submitCommandBuffers -> IOGPUCommandQueueSubmitCommandBuffers`。这表明 Xcode replay 也到达 Apple GPU/Metal queue 卡点，但无法单独反推出具体项目资源或证明驱动根因。
- 本条提高 Epic/Apple 诊断的优先级，不把本机反复卡死风险转嫁给用户。下一步以现有样本、日志和系统 watchdog 诊断提交上游；任何临时规避只在取得上游明确建议或项目层可逆最小改动后进行。
