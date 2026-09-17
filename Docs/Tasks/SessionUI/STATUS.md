---
task_id: SessionUI
status: in_progress
assigned_to: Codex
progress: 98%
started: 2026-07-18
last_updated: 2026-09-01
---

# 任务状态

当前状态：in_progress
负责人：Codex
进度：98%

最后更新内容：

- 2026-09-01 玩家已把 Toggle Camera 从 V 改为 N 并确认 Apply 后实际生效，键位编辑主链验收通过。该验收发现设置页缺少全局恢复默认入口：项目已有 `DT_PMM_InputAction.Input_ResetDefaults`，但 `ULyraSettingScreen` 未注册。对照本机 LyraStarterGame 5.8 后确认，Lyra 原版 C++ 与 `W_LyraSettingScreen` 蓝图同样只有 Back、Apply、Cancel；只有单个键位 Entry 的恢复箭头。`UGameSettingPanel::GetSettingsWeCanResetToDefault()` 仅覆盖当前 Filter 且排除嵌套页，不能直接满足五类全局恢复。因此项目扩展为递归收集 `ULyraGameSettingRegistry` 完整树中的可重置 `UGameSettingValue`，先弹确认，再进入既有 ChangeTracker 事务；Apply 才保存，Cancel 可撤销。Reset Defaults 底部动作、确认弹窗、恢复和 Apply 流程已由玩家验收通过。
- 2026-09-01 修复 Mouse & Keyboard 列表的 `No Editor Found`：`GameSettingRegistryVisuals.EntryWidgetForClass` 过去把 `W_SettingsListEntry_KBMBinding` 错挂在空 Class 键，现已按 `/Script/NewWorldOrder.LyraSettingKeyboardInput` 注册。键位 Entry 同时补齐 `W_PressAnyKey` 与 `W_KeyAlreadyBoundWarning` 两个父类弹窗属性，资产编译为 UpToDate 并序列化复读通过。修复与审计脚本已加入同样的断言，避免再次迁移时回归。实际改键和 Apply 持久化已通过；重复键警告仍保留为后续人工回归项。
- 2026-09-01 全局恢复默认实现完成后执行 `NewWorldOrderEditor Win64 Development` 增量编译：首次因局部变量遮蔽告警失败，改名后 4 个 Action 编译成功，最终耗时 41.91 秒。重启编辑器后 `W_LyraSettingScreen` 编译状态为 UpToDate，四个动作行及键位 Entry 配置序列化复读通过。
- P0 玩家手工验收步骤已整理到 `Acceptance_P0手工验收.md`，覆盖 FrontEnd 五页设置、HomeMap 菜单与终端、Lobby、退出副本、重复 Travel 稳定性和 Dressing_Table_Set 衣柜入口。
- 2026-08-31 修正设置页与副本选择页的验收遗漏。设置页现由项目 `W_HorizontalTabList` 生成 Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五个可见 Tab；Back、Apply Changes、Cancel 使用项目 CommonUI Action 表。`W_BoundActionButton` 类默认 Style 已设为项目 `ButtonStyle-Clear`，消除动作按钮默认白色边框。
- 副本卡片点击现在只更新选择，独立的 Create/Enter 才执行 Local Travel 或 Online Host；页面同时提供 Search Rooms、Back、本地玩家数量与中途加入策略。Bot 继续明确显示 AI Teammates Coming Later。
- 已补齐 DungeonTest、SplitScreenTest、ExpeditionSandbox 三套完整 `UFE -> Map -> GameMode -> Shoot Experience` 配置和返回门。SplitScreenTest 明确只支持 Local，在线状态选择时自动回退 Local。
- 用户完成本轮 C++ 编译：`NewWorldOrderEditor Win64 Development`，6 个 Action，`Result: Succeeded`，总耗时 180.08 秒。随后 PIE 用 ExpeditionSandbox 验证真实 Host 按钮进入对应地图、加载对应 Experience，并通过 Portal 返回 HomeMap；设置页五 Tab、视频条目和三个动作按钮通过运行时截图与对象检查。
- 项目现有 Foundation 控件是唯一 UI 基础资产；早期迁入的同职责 Lyra 按钮由用户替换/重定向，不再作为实现或修复目标。
- 2026-08-30 根据新的产品方向，启动前端补入 Lyra 风格设置，HomeMap 的副本入口改为“终端选择 -> Local 直达或 Online 建房进入 LobbyMap -> 房主开始副本 -> 副本返回 HomeMap”的完整生命周期；旧的 HomeMap 直达测试图 Portal 实例已替换。
- 已迁入通用 GameSettings/GameSubtitles 依赖和设置 Widget，修复所有 `/Script/LyraGame` 父类与图表节点为当前项目类。Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 页面均编译 UpToDate；Performance Stats 延后，键位只读取项目 `IMC_Default`。
- 已恢复 `ULyraReplaySubsystem` 对 `ULyraSettingsLocal::GetNumberOfReplaysToKeep()` 的使用；副本 UFE 首版保持 `bRecordReplay=false`，录像浏览页列为后续 TODO。
- 已扩展项目现有 `UShootExperienceDefinition` 链而非整体照搬 Lyra GameFeature Experience；补齐 Primary Asset 扫描与 Travel URL `?Experience=` 解析。
- 已创建并审计 `DA_Experience_Lobby`、`BP_ExpeditionLobbyGameMode`、`W_ExpeditionLobbyScreen` 和 `BP_ExpeditionTerminal`；LobbyMap GameMode、HomeMap 唯一终端、TestMap 返回门及 UFE 地图/人数配置均通过 Unreal 资产级验证。
- Windows Editor Development 最终完整编译和 URL 修复后的增量编译均成功。批量 PIE 已覆盖 FrontEnd Options、HomeMap 唯一副本终端、Local 直达副本、Online 创建会话进入 LobbyMap、房主开始副本、返回门销毁会话并回到 HomeMap。
- Online PIE 首轮发现 Primary Asset Id 中的冒号会使 `CanServerTravel` 拒绝 URL；现只在 URL 传递资产名称，GameMode 按明确类型重建 `FPrimaryAssetId`。修复后 Lobby 与副本 Travel 均成功。
- 早期迁入的重复 `W_LyraMenuButton` 已由用户替换/重定向为项目 Foundation 按钮；当前 Host、Lobby 与 Settings 运行时使用项目按钮资产，不再维护重复的 Lyra 按钮实现。
- 当前增量已完成代码、资产、文档与 Windows PIE 验收。任务仍保持 in_progress，仅因为真实 Steam 双账号/跨平台 Find、macOS Metal 冻结、最终 UI 视觉打磨等外部或后续验收不能由本轮单机 PIE 替代。

- 2026-07-24 当前提交的 macOS 实机验收推翻了“Steam 生命周期待最终确认”的乐观假设：Steam Online Host 到 HomeMap、以及接受邀请 Join 到 HomeMap，均可看到 Pawn 出现但画面随后完全冻结；HUD 保持初始值。现有证据不能证明根因是 Steam SDK 同步调用、`CleanUpResidualSession` 或邀请链中的任一项，禁止据此直接加入强制 Idle、跳过清理或关闭 OnlineSubsystem 的补丁。
- `W_SessionBrowserScreen` 的 Find 闭环也未通过实机验收。`UShootSessionScreen::OpenSessionBrowser()` 只负责打开页面，当前浏览器刷新入口没有可靠地把一次搜索请求送到 Coordinator，页面可能永久显示加载状态。因此不能再称“搜索、列表 Join 已验证”。
- 本任务转为 macOS 运行时诊断交接；准确复现步骤、日志命令、待核对调用点和验收门槛见 `MacSteamFreezeAndFind_Handoff_交接.md`。在得到原始日志前不再继续修改会话代码。

- 已完成 13000 旧项目交互意图和 11000 Lyra/CommonUser 权威链路审计。
- 已实现 `UShootSessionCoordinatorSubsystem` 及 Host、Find、Join、离开/销毁、CleanUp 和 Steam 邀请入口。
- 已将游戏内页面定名为 `WBP_GameMenu`；它提供联机、衣柜、返回游戏和返回启动前端入口，所有页面均有可见返回或取消入口并启用 CommonUI Back Handler。
- `BP_ShootCharacter.ShowMenuWidget` 只在玩家输入后推入 `WBP_GameMenu`；进入 HomeMap 不再自动弹出菜单。
- 固定按钮点击事件位于 Widget Blueprint EventGraph，C++ 只暴露语义明确的业务方法；离开/销毁确认复用 `ShowConfirmationYesNo`，旧专用弹窗已删除。
- 已将 `UShootFrontendStateComponent` 更名为 `UShootHomeHubStateComponent`。`FrontEndMap` 专指启动标题前端，`HomeMap` 是玩家家园/联机大厅；HomeHub 组件只处理 Standalone 残留会话，不负责打开菜单。
- 已通过真实 CommonSession LAN Host 验证 `Map:/Game/Maps/HomeMap` 能创建会话并执行一次 Listen Server Travel。
- 已通过四个独立 `-game -nosteam` 进程角色验证 LAN Host/Find/Join、客户端主动离开、主机销毁及双方返回 HomeMap；每个客户端 Join 只有一次远端 Browse。
- 已在项目层 `UShootSessionCoordinatorSubsystem` 规避 CommonUser 在 `NoSession` 清理后遗留 pending destroy 的边界，并等待真实 Cleanup 完成；`Plugins/CommonUser` 保持 Lyra 官方原样。
- 已修复主机销毁时远端客户端被 Engine 默认送回 FrontEndMap 的问题，网络断开路径统一使用 `UShootGameInstance.SessionReturnMap`。
- Windows Editor Development 编译成功；PIE 确认 HomeMap 初始不会自动出现 `WBP_GameMenu`，相关蓝图均编译成功。
- 当前日志中 `SessionInterface.IsUnique()`、`Leader pose component skeleton doesn't match follower` 和 `UMultiplayerSessionsSubsystem` 均为 0 次。
- 平台邀请接受链已统一经过 Coordinator；FrontEndMap/冷启动邀请先进入可配置 HomeMap，等待 LocalPlayer、PlayerController 和旧会话清理完成后再 Join。
- Shipping 邀请按钮改为核对实际 `NAME_GameSession`，不再只信任可能因 Travel 丢失的内存 `InSession` 状态。
- 已按验收决定删除 `BuildIdOverride`，恢复 Lyra/引擎默认网络版本；SessionScreen 继续显示实际 BuildId，跨平台验收必须使用同一提交和相同构建配置。
- 已确认 CommonUser OSSv1 搜索硬编码最多 10 条；SteamDevAppId 480 的公共 Spacewar Lobby 会挤掉项目结果。项目 Coordinator 仅对 Steam Online 搜索扩大候选窗口为 1000，保持插件源码只读，并启用官方 SteamSockets NetDriver。
- 已把迁入的 `W_SessionBrowserScreen`、`W_SessionBrowserEntry` 改接项目父类和 Coordinator。搜索、列表 Join、状态结果及可见返回链已编译通过；固定按钮点击仍在 Widget Blueprint EventGraph。
- 2026-07-22 Windows Editor Development 完整编译成功；启用 SteamSockets 后重新执行 Null/LAN 两组双进程生命周期，客户端离开、主机销毁、双方返回 HomeMap、重复请求门禁全部通过，四份日志中目标 Ensure 和通用 Ensure 均为 0。
- 已通过 MCP 逐帧证明并修复出生首帧 Head/Body 同时显示完整兜底网格造成的“双角色”假象；修复后最终 LeaderPose 在第 3 tick 恢复。
- 已按“C++ 只供数据，文本控件、排版和视觉由 Widget Blueprint 管理”重构 Session UI：C++ 不再持有 `BindWidget` 文本/列表控件，不再执行 `SetText`、`ClearChildren`、`CreateWidget`、`AddChild` 或按钮点击绑定；`WBP_SessionScreen`、`WBP_SessionResultButton` 和新增 `WBP_SessionPlayerEntry` 的 EventGraph 负责会话文本、搜索结果、玩家列表与点击。
- 三个 Session Widget Blueprint 均为 UpToDate，Widget hierarchy validation 均通过；重构后的 Windows Editor Development 编译成功，总耗时 98.54 秒。
- Coordinator 的 Host、Find、Join 入口现以状态机和实际 `NAME_GameSession` 双重阻止重叠请求；Development 回归故意重复调用 Host/Find，均记录 `Accepted=0`。
- Join 通过 CommonSession 官方 `OnPreClientTravelEvent` 使用搜索结果广告的 `MAPNAME` 补全连接 URL；Null/LAN 首次远端 Browse 已从 FrontEndMap 修正为 HomeMap，普通 Join 与平台邀请共用该链。
- 平台邀请解析失败或等待 HomeMap、LocalPlayer、PlayerController、旧会话清理超时后，会清除缓存请求并把错误同步到 Session UI。
- 用户从编辑器菜单打包，项目已设置官方 `UProjectPackagingSettings.bUseZenStore=False`；最新 Shipping Build/Cook/Stage/IoStore/Archive 返回 `BUILD SUCCESSFUL`，最终 Pak/IoStore 格式不变。
- 已补齐 `W_SessionBrowserScreen` 的真实 component-bound 返回按钮事件；返回按钮在 2026-07-23 PIE 中通过蓝图委托退回 `WBP_SessionScreen`。搜索或 Join 进入 `Error` 时，蓝图会显示 Coordinator 的 `Status`、清空旧列表并复用原有解锁动画。
- 已删除 `WBP_SessionScreen` 中完全断开的旧 `FindOnlineSessions` 节点，避免 EventGraph 同时出现“打开浏览器”和“直接搜索”两种看似可用的入口。
- Coordinator 的扩展 Steam 搜索处理 OSSv1 同步失败回调：若 `FindSessions` 返回 `false` 前委托已经消费请求，不再手工发送第二次终态。
- 2026-07-23 `NewWorldOrderEditor Win64 Development` 编译成功，3 个 Action，总耗时 13.03 秒。HomeMap PIE 的联机浏览返回链和衣柜返回链均通过真实蓝图按钮委托；本轮日志中两个目标 Ensure、通用 Ensure、旧 Session 子系统和蓝图运行时错误均为 0。

下一步：

- Mac 开发端先以本任务交接文档的命令复现 Host 和邀请 Join，收集冻结前最后 200 行原始日志及主线程采样；在确认冻结发生于哪一个阶段前不改 Steam 插件、引擎源码或 OnlineSubsystem 关闭顺序。
- 修复浏览器后，必须用真实按钮确认每次打开页面仅发起一次 Find，能从 Searching 收敛到 Results/空结果/Error，Refresh 可重试，Join 结果由 Coordinator 驱动。
- 完成上述运行时验证后才能恢复 Steam 双账号、跨平台 Find、Overlay 邀请、离开/销毁与物理 Q/确认 Modal 的最终验收。

遇到问题：

- macOS Steam Host 和 Join 进入 HomeMap 后冻结，属于阻断性问题。Host 路径同样复现，故不能只检查邀请等待/`HandlePreClientTravel`。
- 浏览器页面的打开、刷新、Coordinator Find 调用之间未形成经实机验证的闭环；原先 PIE 资产级验证不足以代替一次真实搜索。

- HomeMap 是玩家家园/联机大厅，同时也是当前联机会话目标图，不能照抄 Lyra 在每次加载时无条件 CleanUp；已用 NetMode、Travel URL 和 Cleanup 状态保护。
- 早期 Development Cook 的 AttributeMenu/Reticle 阻塞已不再复现；最新 Shipping Loose Cook 处理 1512 个运行时包并完成 Pak/IoStore。
- Steam 双账号和 LAN 双机的真实 Find/Join/邀请结果需要最终打包后人工协同验收。
- 用户需要从本轮最终提交在 Windows 与 Mac 使用同一提交、相同构建配置重新打包，并记录两端实际 BuildId；旧包不能用于判断邀请与跨平台搜索修复结果。

相关 Commit：

- 设置五分类、三副本目录与 Host 交互修正：`2a6dffbc`；分支分叉已通过保留双方历史的 merge commit 合并后推送。
- Session + CommonUI 核心收尾提交：`70e0652`，已推送至 `origin/main`；Steam 双实例验收完成后追加最终验收提交。
- 并发门禁、邀请失败收口、Join URL 地图补全和编辑器打包配置：`7c53dd8`；真实 Steam 双账号验收完成后再把本任务改为 completed。
- 浏览器返回/失败恢复与扩展搜索同步回调防重：`20b01a8`，已随并行属性提交推送至 `origin/main`。
