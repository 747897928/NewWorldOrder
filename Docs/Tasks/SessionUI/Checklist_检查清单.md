# Session + CommonUI 生命周期检查清单

# 任务包与审计

- [x] 建立独立 SessionUI 任务包。
- [x] 完成 13000 旧项目菜单交互审计。
- [x] 完成 11000 Lyra/CommonUser 会话和 UI 审计。
- [x] 从 GameFrameworkMigration 移除本任务详细内容并保留链接。

# GameMenu

- [x] HomeMap 不自动弹菜单；玩家菜单输入打开 WBP_GameMenu，而不是直接打开 W_Cloth。
- [x] 左上角存在联机按钮。
- [x] 左上角存在衣柜按钮。
- [x] 右下角存在返回按钮。
- [x] WBP_GameMenu 可通过按钮和 CommonUI Back Action 关闭。
- [x] 衣柜可打开并逐级返回 WBP_GameMenu。

# Session UI

- [x] SessionScreen 有返回按钮。
- [x] SessionScreen 可 Host Online/LAN，并可返回。
- [ ] Online/LAN Find、刷新和返回经真实 UI 闭环验证。
- [ ] 搜索中、空结果、失败和结果列表状态由真实 Coordinator 回调验证。
- [ ] SessionResultButton 真实选择结果并 Join。
- [ ] Join 失败能恢复操作并再次搜索。
- [x] 客户端有离开会话入口。
- [x] 主机有销毁会话入口和确认。
- [x] CleanUp 可重复调用且协调层委托只注册一次。
- [x] 邀请请求有接受、失败和已有会话处理路径。
- [x] SessionScreen 显示实际平台会话、地图、BuildId、主客角色和当前玩家/延迟。

# 生命周期与网络

- [ ] Steam Host 只发生一次 ServerTravel，且进入 HomeMap 后持续 Tick，不冻结。
- [ ] Steam Join 只发生一次 ClientTravel，且进入 HomeMap 后持续 Tick，不冻结。
- [x] 客户端离开不结束主机。
- [x] 主机销毁后客户端可恢复到 HomeMap/大厅。
- [x] 所有调用使用目标 PlayerController/LocalPlayer。
- [x] 重复开关 UI 不重复注册委托或遗留输入焦点。
- [x] 不出现 `SessionInterface.IsUnique()`。

# 验证与交付

- [x] Widget Blueprint 全部编译保存。
- [x] Windows Editor 编译成功。
- [x] 编辑器等价 Windows Shipping Build/Cook/Stage/IoStore/Archive 成功，未使用 Zen cooked output store。
- [x] PIE 验证 HomeMap 初始不自动弹出 WBP_GameMenu；WBP_GameMenu、SessionScreen 和确认链蓝图均编译保存。
- [x] PIE 将 WBP_GameMenu 推入目标 LocalPlayer 的 GameMenu Stack 后，真实点击联机、Find、浏览器返回、SessionScreen 返回和 GameMenu 返回。
- [x] PIE 真实点击衣柜入口和 W_Cloth 可见返回按钮，正确恢复到 WBP_GameMenu。
- [ ] 人工物理按 Q 打开 WBP_GameMenu，并点击离开/销毁确认 Modal 的 Confirmed、Declined 和 Cancelled 路径。
- [ ] Steam 双独立实例 Host/Find/Join 验收记录，包含 macOS Host/Join 后至少 30 秒持续运行证据。
- [ ] 同一提交重新构建 Windows/Mac 包，确认 SessionScreen 的 BuildId 一致并完成跨平台 Find。
- [ ] Development/Shipping 分别确认 Steam Overlay 邀请，并确认受邀者最终进入 Host 的 HomeMap 会话。
- [x] 文档与 Lyra 操作笔记已更新。
- [ ] STATUS 更新为 completed（当前被 macOS 冻结和 Find 闭环阻断）。
- [x] Commit 和 Push 完成（核心提交 `70e0652` 已推送至 `origin/main`）。

# 2026-08-30 副本与设置增量

- [x] FrontEnd Options 连接 Lyra 风格设置页面。
- [x] 迁入 GameSettings/GameSubtitles 依赖和设置 Widget，并修复为当前项目 C++ 父类。
- [x] Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五个运行时 Tab 均可见、可切换，Video 页显示 Window Mode、Display、Resolution 与画质设置。
- [x] Back、Apply Changes、Cancel 通过项目 CommonUI Action 表注册；`W_BoundActionButton` 使用项目 `ButtonStyle-Clear`，无默认白色边框。
- [x] Mouse & Keyboard 只使用 `IMC_Default` 的 Player Mappable 映射。
- [x] Replay 保留数量清理链恢复，副本首版默认关闭录制。
- [x] Primary Asset 注册包含 UserFacing Experience 与项目 Experience。
- [x] GameMode 支持从 `?Experience=` 解析玩家选择。
- [x] HomeMap 旧直达 Portal 替换为 LocalPlayer 隔离的副本终端。
- [x] Online Host 先进入 LobbyMap，房主开始后再进入副本。
- [x] Lobby 等待页、玩家数、Join-in-progress 状态、Host Start 和 Leave/Destroy 入口完成。
- [x] Bot 定义为未来 AI 队友填位；当前显示 Coming Later 并禁用。
- [x] Host 卡片只选择副本；创建/进入、搜索房间、返回、本地玩家数量和中途加入策略均有独立可见控件。
- [x] DungeonTest、SplitScreenTest、ExpeditionSandbox 三套 UFE、Map、GameMode、Shoot Experience 完整配置及返回门审计通过。
- [x] 最终 Windows Editor C++ 编译通过。
- [x] `BP_DungeonPortal_ToHomeMap.bEndOnlineSessionBeforeTravel` 在新反射属性可用后写入并复核。
- [x] 批量 PIE 验证 FrontEnd Options、HomeMap 终端、Local、Online Lobby、Host Start 和返回 HomeMap。
- [x] 当前 2026-08-31 修正增量提交并推送远程分支。
