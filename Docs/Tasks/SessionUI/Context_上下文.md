# Session + CommonUI 生命周期上下文

# 背景

当前项目已全面采用 CommonUser 的 `UCommonSessionSubsystem` 作为唯一会话服务，旧 `UMultiplayerSessionsSubsystem` 已删除。会话底层主线已收敛，但当前项目没有可供玩家完成 Host、Find、Join、离开、销毁、CleanUp 和邀请处理的完整菜单 UI。

当前启动流程是：

```text
FrontEndMap
  -> 点击 StartGame
  -> HomeMap 大厅
  -> 玩家在 HomeMap 打开 MainMenu
```

本任务先以这个中间流程完成可测试闭环。未来 FrontEnd 与 HomeMap 的产品流程可以调整，但不能破坏会话与 UI 生命周期边界。

# 参考项目

- 13000 旧项目：`/Game/Blueprints/CommonUI/Menu`。
  - 目的：还原原有 Host、搜索列表、Join、玩家列表、离开和返回的交互意图。
  - 禁止迁回旧 `ShootUIBaseContainer`、自建 WidgetStack、FirstLocalPlayer 和 Widget 直接持有 OSSv1 interface 的架构。
- 11000 Lyra：FrontEnd、CommonUser、CommonSession 和 CommonUI 页面。
  - 目的：确认 Host、Find、Join、CleanUp、邀请、Travel、错误与加载状态的权威生命周期。
- 当前项目：CommonGame、CommonUser、PrimaryGameLayout、ULyraActivatableWidget、MVVM 与 GameplayMessage。

# 当前边界

- 会话网络与 Travel 由 `UCommonSessionSubsystem` 负责。
- MainMenu 和所有子页面属于目标 LocalPlayer，必须通过该玩家的 PrimaryGameLayout 管理。
- Widget 不接触 `IOnlineSessionPtr`，不创建第二套 Session Subsystem。
- 当前任务实现 UI、状态和生命周期，并扩展项目现有轻量 `UShootExperienceDefinition`；不整体迁入 Lyra 的 GameFeature Experience/PawnData 全套架构。
- 项目 Foundation 已有 Menu、Arrow、Modal、Tab、Close 和 TabList 控件，设置与副本页面以这些资产为准；旧迁入的同职责 Lyra 按钮已由用户替换/重定向，不应再次恢复。
