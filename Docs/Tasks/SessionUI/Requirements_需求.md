# Session + CommonUI 生命周期需求

# 产品入口

- 在 HomeMap 按现有 `IA_OpenMenu` 打开新的 MainMenu，不再直接打开 `W_Cloth`。
- MainMenu 左上角至少提供：
  - 联机。
  - 衣柜。
- MainMenu 右下角必须提供明确的返回按钮。
- 衣柜按钮打开现有 `/Game/UI/Mutable/W_Cloth`。
- 联机按钮打开 Session 面板。
- MainMenu、Session 面板、创建面板、搜索面板、结果详情和衣柜都必须存在可见的退出或返回路径。
- 关闭页面必须使用 CommonUI 的 `DeactivateWidget()` 或层级栈返回，不使用 `RemoveFromParent()`。

# 会话能力

- Host：
  - 创建 Listen Server 会话。
  - 支持 Online/Steam 与 LAN 模式配置。
  - 可设置公开人数等必要参数。
  - Host 成功后只允许 CommonSession 执行一次 ServerTravel。
- Find：
  - 有明确的搜索中、空结果、失败和成功状态。
  - 搜索结果可刷新，不产生重叠请求和重复列表项。
- Join：
  - 玩家选择结果后加入，不默认静默加入第一项。
  - Join 成功后只允许 CommonSession 执行一次 ClientTravel。
  - Join 失败返回可操作界面并显示原因。
- 客户端离开：
  - 只让当前客户端退出，不销毁主机仍在运行的会话。
  - 清理本地 Session 状态后回到 HomeMap 或明确的大厅入口。
- 主机销毁：
  - 主机主动结束会话时销毁服务端会话。
  - 远程客户端收到断开并进入可恢复的 HomeMap/大厅流程。
- CleanUp：
  - 返回主菜单、网络失败、退出会话和 GameInstance 重置都必须收敛到 CommonSession 清理链。
  - 清理必须可重复调用且不会产生双重 Destroy 或悬挂回调。
- 邀请：
  - 接受 Steam Overlay 邀请后走 CommonSession 请求链。
  - 若当前已有会话，先完成清理或展示确认 UI，再加入受邀会话。
  - 邀请失败必须恢复 UI 和输入状态。

# 玩家与网络生命周期

- Listen Server 主机同时是服务器玩家和本地玩家，两种职责不能混为一条 UI 状态。
- 远程客户端只能操作自己的 LocalPlayer UI。
- 所有 Host、Find、Join 调用必须传入发起操作的目标 PlayerController。
- 禁止 `GetFirstPlayerController`、`GetPlayerController(0)` 和无 OwningPlayer 的 CreateWidget。
- 为未来本地分屏保留每个 LocalPlayer 独立 PrimaryGameLayout、焦点与 UI 状态。
- UI 不保存权威会话状态；它只展示 CommonSession 请求与事件形成的本地视图状态。

# 验收要求

- PIE 可完整操作 MainMenu、Session 子页面和所有返回路径。
- HomeMap 可触发 Host、Find、Join 入口；Steam Join 由两个打包实例或虚拟机协同验收。
- 客户端离开不会结束主机；主机销毁后客户端能恢复。
- 重复打开、关闭和返回页面不会重复注册委托、重复叠加 InputMapping 或遗留鼠标/输入焦点。
- 日志不得出现 `SessionInterface.IsUnique()`、重复 Create/Join/Destroy、悬挂 Widget 回调或重复 Travel。

# 2026-08-30 副本入口与设置范围补充

- 启动流程为 `FrontEndMap -> HomeMap`；`HomeMap` 不再用旧直达 Portal 进入测试图，而是通过每个 LocalPlayer 可独立交互的副本终端打开选择页面。
- `W_HostSessionScreen` 是副本选择入口，提供：
  - Local：直接进入所选副本；本地玩家数量沿用 CommonSession 请求并受副本最大人数限制。
  - Online：创建 Listen Server 后先进入 `LobbyMap`，不立即进入副本。
  - 搜索与加入继续复用现有 Session Browser，不建立第二套 OnlineSubsystem 服务。
- 副本卡片只负责选择，不得在点击卡片时立即 Host；页面必须提供独立的“创建/进入”“搜索房间”“返回”按钮，以及本地玩家数量和中途加入策略的可见状态。
- POC 至少提供三套可辨认的 `UserFacing Experience -> Map -> GameMode -> Shoot Experience` 配置，用于验证选择项确实改变地图与 Experience，而不是只换卡片文本。
- `LobbyMap` 必须显示等待大厅页面；只有房主可以点击开始，客户端只能等待或离开。房主可配置是否允许中途加入，默认允许。
- 副本返回门必须把玩家送回 `HomeMap`。在线房主返回时销毁会话，在线客户端返回时只离开自己的会话；本地模式直接 Travel。
- Bot 的产品含义固定为“AI 队友填补空余玩家位”，首版没有 AI 队友实现，UI 必须显示 Coming Later 且不可操作，不能伪装成已支持。
- Experience 保留并扩展 `UShootExperienceDefinition`，不整体迁入 Lyra GameFeature Experience 架构；Travel URL 的 `?Experience=` 必须能覆盖 GameMode 蓝图默认值。
- `DefaultGame.ini` 必须注册 `ShootExperienceDefinition` 与 `LyraUserFacingExperienceDefinition` 的 Primary Asset 扫描目录。
- `W_FrontEnd.OptionsButton` 必须打开 `/Game/UI/Settings/W_LyraSettingScreen`。设置首版包含语言、视频与画质、音频、鼠标键盘、项目手柄设置。
- 设置页必须使用 Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五个可见 Tab，并提供可见 Back；发生未应用更改时同时显示 Apply Changes 与 Cancel。Video 页必须能访问 Window Mode、Display、Resolution 与画质项。
- 设置页必须常驻提供全局 Reset Defaults。触发后先显示确认框；确认只把五个分类中允许重置的设置写入当前 ChangeTracker 事务，不立即保存。玩家选择 Apply Changes 后持久化，选择 Cancel 则恢复打开页面时的值。键位条目仍保留单项 Reset，不用全局入口替代。
- 设置页和副本页优先复用项目现有 Foundation 控件。底部 CommonBoundActionBar 动态按钮统一使用 `/Game/UI/Foundation/Buttons/ButtonStyle-Clear`，不得重新迁入同职责的 Lyra 按钮资产。
- 键位重绑定只读取 `/Game/Blueprints/Input/IMC_Default` 的 Player Mappable 映射，不迁入 Lyra 的玩法键位资产，也不在 C++ 硬编码键盘或手柄分支。
- Replay 基础设置和本地录像保留数量清理链恢复，但副本首版默认不录制；Replay Browser 作为后续 TODO。
- Performance Stats 是面向开发/性能诊断的屏显选项，不属于本轮玩家设置首版，暂不暴露。
