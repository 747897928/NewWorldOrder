# 旧项目与 Lyra 审计

# 13000 旧项目

状态：[已完成]

已知资产：

- `/Game/Blueprints/CommonUI/Menu/WBP_MainMenu`
- `/Game/Blueprints/CommonUI/Menu/WBP_CreateSessionPanel`
- `/Game/Blueprints/CommonUI/Menu/WBP_FindSession_Panel`
- `/Game/Blueprints/CommonUI/Menu/WBP_Session_Info_Item`
- `/Game/Blueprints/CommonUI/Menu/WBP_Session_Panel`
- `/Game/Blueprints/CommonUI/Menu/WBP_PlayerEntry`

已有结论：

- 视觉、文案、按钮分组和交互流程可参考。
- Host、Find、Join、Destroy、Kick 和状态查询分散在 Widget EventGraph 与旧父类。
- 旧父类、OwnerContainer、FirstLocalPlayer、自建 WidgetStack 和直接 OSSv1 interface 不迁移。

审计结论：

- MainMenu 打开 SessionPanel；Create、Find、结果项和 PlayerList 分页承担创建、查找、加入、离开与销毁。
- Create 页面包含 LAN 与人数选项，Find 页面包含刷新和结果列表，所有子页面都有返回入口。
- 主机显示销毁，客户端显示离开；旧流程的交互意图保留到新 SessionScreen 与确认弹窗。
- 旧实现把 Session 服务、Travel 和 Widget 栈放在 UI 父类并使用 FirstLocalPlayer，不符合当前 CommonUI 与本地分屏约束，因此只迁移交互意图。

# 11000 Lyra

状态：[已完成]

源码确认：

- `ULyraFrontendStateComponent` 在 Experience 加载后运行 ControlFlow：等待用户初始化、可选 Press Start、尝试加入缓存邀请，最后才推入 MainScreen。
- 前端流程始终调用 `UCommonSessionSubsystem::CleanUpSessions()`；该前提是 Lyra 的前端地图与比赛地图分离。
- `UCommonGameInstance` 在 `Init()` 绑定邀请和平台销毁请求。邀请先缓存到 `RequestedSession`；若当前状态不允许立即 Join，则返回前端，前端流程再调用 `JoinRequestedSession()`。
- Host 成功后的 ServerTravel、Join 成功后的连接地址解析与 ClientTravel 都由 CommonSession 负责，项目 UI 不重复 Travel。
- 页面通过 `UPrimaryGameLayout::PushWidgetToLayerStackAsync` 进入 GameplayTag 层级，并在异步 Push 完成前保持 Loading 状态。
- 可直接采用的是 CommonUser、CommonGame、PrimaryGameLayout 和上述生命周期边界；Lyra Experience、Press Start、ControlFlow、加密示例属于可选业务层，不为当前项目强行搬入。

# 当前项目与 Lyra 的差异

- 当前 `HomeMap` 暂时兼任前端地图和 Listen Server/Client 的会话目标地图。
- 因此 `UShootFrontendStateComponent` 只在 `NM_Standalone` 进入 HomeMap 时清理残留 Session；Listen Server 和 Client Travel 到 HomeMap 时跳过清理。
- 将来拆出独立 FrontEndMap 和 Lobby/GameplayMap 后，应恢复 Lyra 的最终形式：每次进入专用前端地图都无条件 CleanUp，再初始化用户、处理邀请、推 MainMenu。

# 当前项目采用原则

- 交互意图从旧项目恢复。
- 生命周期与技术实现以 Lyra/CommonUser 为权威。
- 当前项目业务入口通过 MainMenu 组合联机与衣柜。
- 所有玩家私有 UI 绑定目标 LocalPlayer。
