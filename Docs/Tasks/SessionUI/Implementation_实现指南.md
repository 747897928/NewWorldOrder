# Session + CommonUI 生命周期实现指南

# 实施顺序

1. 审计旧项目 UI。
2. 审计 Lyra/CommonUser 权威链路。
3. 确定当前项目父类、UI Layer、状态适配层和资产清单。
4. 先实现 C++ 会话状态与稳定 Blueprint API。
5. 创建 MainMenu 与 Session Widget 资产。
6. 接入 `BP_ShootCharacter` 的 ShowMenuWidget。
7. PIE 操作所有页面、返回路径和错误状态。
8. 编译、文档、提交和推送。

# 实现原则

- C++ 负责异步请求、委托解绑、状态转换、LocalPlayer 归属，并向蓝图发送原始会话、搜索结果和玩家数据。
- Widget Blueprint 负责文本控件、文案组合、按钮点击、列表创建、排版、样式和视觉转场。Session UI 的 C++ 父类不得持有 `BindWidget` 文本/列表控件，也不得调用 `SetText`、`CreateWidget` 或 `AddChild` 接管页面表现。
- MVVM 负责搜索状态、错误、会话结果快照等本地 UI 数据。
- GameplayMessage 只用于确有多个松耦合消费者的跨系统事件，不替代页面内部直接状态绑定。
- 不迁移旧项目 Widget 对 `IOnlineSessionPtr`、OwnerContainer 或自建 WidgetStack 的依赖。

# 当前数据出口

- `UShootSessionScreen::BP_OnSessionStateDataChanged`：发送状态文本数据和原始 `UCommonSession_SearchResult` 数组；`WBP_SessionScreen` 清空并重建搜索结果视觉列表。
- `UShootSessionScreen::BP_OnRuntimeSessionDataChanged`：发送 `FShootSessionRuntimeInfo` 与 `FShootSessionPlayerInfo`；`WBP_SessionScreen` 组合会话文案并创建玩家行。
- `UShootSessionResultButton::BP_OnSearchResultDataChanged`：发送模式、地图、人数、Ping 和是否可加入；`WBP_SessionResultButton` 自己格式化文本、设置可用态，并在蓝图 `OnClicked` 调用 `JoinStoredSession`。
- `UShootSessionPlayerEntry::BP_OnPlayerDataChanged`：发送序号、玩家名和 Ping；`WBP_SessionPlayerEntry` 决定三列文本与布局。

# 并发请求门禁

- Widget Blueprint 可以根据状态禁用按钮，但它只能作为交互提示，不能承担会话并发正确性。
- `UShootSessionCoordinatorSubsystem` 只允许在 `Idle`、`Results` 或 `Error` 且平台不存在 `NAME_GameSession` 时启动 Host、Find 或 Join。
- `Hosting`、`Searching`、`Joining`、`CleaningUp`、`Leaving` 和实际平台会话存在期间，重复按钮事件、快速双击或平台邀请重入会直接返回 false，不会再次进入 CommonSession。
- 邀请解析失败或等待 HomeMap、PlayerController、旧会话清理超时后，`UShootGameInstance` 必须清除缓存邀请，并通过 Coordinator 把原因写回 Session UI 状态；不能只写日志后留下不可重试的 RequestedSession。

# Join 连接 URL 地图补全

- OSSv1 的 `GetResolvedConnectString()` 可能只返回主机地址。UE 从纯地址构造 FURL 时会补上项目 `GameDefaultMap`，本项目因此会先 Browse 到 FrontEndMap，直到服务器握手后才纠正为会话地图。
- Coordinator 在 Join 前从 `UCommonSession_SearchResult` 读取广告的 `MAPNAME`，再通过 CommonSession 官方 `OnPreClientTravelEvent` 把地图包名补入连接 URL。
- 普通搜索结果和 Steam 平台邀请都走同一处理；地图来自会话广告，不硬编码 HomeMap。没有有效 `MAPNAME` 时保持 OSS 原始 URL。
- 该项目层钩子不重复调用 ClientTravel，也不修改 CommonUser 插件；最终 ClientTravel 仍由 CommonSession 唯一执行。

# 修改前检查

- 用 MCP `search_subclasses` 或 Python 资产查询确认当前项目已有 Widget 父类。
- 查看 MainMenu、W_Cloth 和 PrimaryGameLayout 的现有 InputMapping 与 UI Layer。
- 确认 CommonSession 请求对象和所有事件的真实 UE 5.8 签名。
- 确认所有 Session 页面都有可见返回按钮和 CommonUI Back Action。

# 编译与资产验证

- 每次 C++ 阶段执行 `Scripts/Build_Windows.ps1`。
- 每 5 个 UMG 控件修改后编译 Widget Blueprint。
- 保存并重新读取 Widget 控件树、父类、编译状态和关键属性。
- 最终 PIE 实际点击，不以离屏预览代替交互验收。

# 2026-08-30 实现清单

- 设置 C++ 适配位于：
  - `Source/NewWorldOrder/Settings`：Local/Shared Settings、Registry 及各设置页。
  - `Source/NewWorldOrder/UI/Settings`：设置 Screen、Panel、详情与条目父类。
  - `Source/NewWorldOrder/Input`：灵敏度数据和 Enhanced Input Modifier。
  - `Source/NewWorldOrder/Audio`、`Performance`、`Development`：Lyra 设置依赖的最小项目实现。
- 设置资产入口：
  - `/Game/UI/FrontEnd/W_FrontEnd` 的 Options 绑定 `UShootFrontEndScreen::OpenSettingsScreen`。
  - `/Game/UI/Settings/W_LyraSettingScreen` 继承项目 `ULyraSettingScreen`。
  - `/Game/Blueprints/Input/IMC_Default` 的 Player Mappable 映射是键位目录的单一来源。
  - `/Game/UI/Foundation/TabbedView/W_HorizontalTabList` 与 `/Game/UI/Foundation/Buttons/W_LyraButtonTab` 提供五分类导航。
  - `/Game/UI/Foundation/Widgets/BottomBar/W_BoundActionButton` 类继承自项目 `ULyraBoundActionButton`，使用该父类上的 `Style` 属性；该类默认值必须指向 `/Game/UI/Foundation/Buttons/ButtonStyle-Clear`。动作栏按钮是运行时生成的，因此样式不能只改某个设置页实例。
- 副本资产入口：
  - `/Game/GameFramework/Experiences/UserFacing/DA_UFE_DungeonTest`、`DA_UFE_SplitScreenTest`、`DA_UFE_ExpeditionSandbox`。
  - `/Game/GameFramework/Experiences/DA_Experience_DungeonTest`、`DA_Experience_SplitScreenTest`、`DA_Experience_ExpeditionSandbox`。
  - `/Game/GameFramework/Experiences/DA_Experience_Lobby`。
  - `/Game/Blueprints/GameMode/BP_ExpeditionLobbyGameMode`。
  - `/Game/Blueprints/Interaction/BP_ExpeditionTerminal`。
  - `/Game/UI/Menu/Experiences/W_ExpeditionLobbyScreen`。
- `LobbyMap` 的 World Settings 使用 `BP_ExpeditionLobbyGameMode`；`HomeMap` 只有一个 `BP_ExpeditionTerminal`，旧 `DungeonPortal_ToTestMap` 实例已移除。
- `TestMap_ListenServer` 的 `BP_DungeonPortal_ToHomeMap` 必须启用 `bEndOnlineSessionBeforeTravel`，让在线返回收敛到 Coordinator；`bClearAllRuntimeSessionsBeforeTravel` 继续负责清空 RuntimeOnly 副本物品。
- 三套测试地图分别使用不同 GameMode 蓝图，并由 GameMode 默认值指向对应 Shoot Experience；每张图均保留回 HomeMap 的 Portal。新增目录项时必须同时配置 UFE、Map、GameMode、Shoot Experience，不得只复制 UFE 卡片。
- Replay 首版由 UFE 的 `bRecordReplay=false` 控制；`ULyraReplaySubsystem` 已重新使用 `GetNumberOfReplaysToKeep()` 清理本地录像。
- 复现和审计脚本统一放在 `Scripts/SessionUI_*.py`，必须在 Unreal Python 环境执行，不读取或二进制修改 `.uasset`。
- `SessionUI_RepairLyraMenuButtonMaterials.py` 仅保留为早期错误修复记录，并有显式停用门禁；当前项目存在 Foundation 按钮时不得执行或改写重定向后的 `W_LyraMenuButton`。
