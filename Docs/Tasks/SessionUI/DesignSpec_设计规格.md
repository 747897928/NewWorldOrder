# Session + CommonUI 生命周期设计规格

# 当前设计状态

状态：[已实现，待打包联机验收]

13000 旧项目与 11000 Lyra 审计已经完成。当前实现采用单一 SessionScreen，加上动态结果条目，并复用项目现有 `ShowConfirmationYesNo` 处理离开/销毁确认；没有迁入旧项目的页面容器与 OSSv1 服务。

# 职责分层

```text
UCommonSessionSubsystem
  Host / Find / Join / CleanUp / 邀请 / Travel

UShootSessionCoordinatorSubsystem
  监听 CommonSession 事件
  管理搜索请求与结果快照
  区分 Idle / Hosting / Searching / Joining / InSession / Leaving / Error
  不持有 OSSv1 interface

CommonUI 页面
  WBP_GameMenu
    -> WBP_SessionScreen
       -> WBP_SessionResultButton
       -> ShowConfirmationYesNo
    -> W_Cloth
  所有页面都有返回或关闭入口
```

# UI 生命周期

- WBP_GameMenu 仅在玩家触发 `BP_ShootCharacter.ShowMenuWidget` 后，由目标 LocalPlayer 的 PrimaryGameLayout 推入 `UI.Layer.GameMenu`；HomeMap 加载不自动打开它。
- WBP_GameMenu 自身只负责入口导航和关闭，不承担 Session 服务实现。
- 子页面优先使用 CommonUI ActivatableWidget 栈；返回行为只回退一级并恢复焦点。
- 页面激活时注册事件，停用时解除事件；重复激活不产生重复绑定。
- 异步操作期间禁用会导致重复请求的按钮，并提供取消或返回策略。
- 衣柜作为现有独立页面继续由 CommonUI 生命周期管理，从衣柜返回 WBP_GameMenu 时恢复原页面而不是重新创建全局 UI。

# 会话状态机

```text
Idle
  -> Hosting -> InSession | Error
  -> Searching -> Results | Error
Results
  -> Joining -> InSession | Results/Error
InSession
  -> Leaving/CleaningUp -> Idle + HomeMap
  -> HostDestroying -> Idle + HomeMap
AnyState
  -> InviteRequested -> CleaningUpIfNeeded -> Joining
```

# 已确认的实现选择

- Lyra 没有一个适合直接复制为本项目产品菜单的完整 Find/Join Widget；采用其 CommonSession 和前端生命周期，不照搬视觉资产。
- 页面父类使用项目现有 `ULyraActivatableWidget`；动态结果按钮直接继承 `UCommonButtonBase`，避免 Lyra 菜单按钮用 InputAction 文案覆盖服务器名称。
- 搜索结果当前由 SessionScreen 动态生成结果按钮；后续大列表可替换为 ListView，但不改变 Coordinator API。
- Host 地图通过 WBP Class Default 的 `HostMapId` 配置，UE 5.8 必须写完整 PrimaryAssetId，例如 `Map:/Game/Maps/HomeMap`，不能硬编码裸地图名。
- 返回地图由 `UShootGameInstance::SessionReturnMap` 配置，当前为 HomeMap，不在 Session Widget 中硬编码。

# 2026-08-30 副本生命周期设计

```text
FrontEndMap
  -> W_FrontEnd.Start -> HomeMap
  -> W_FrontEnd.Options -> W_LyraSettingScreen

HomeMap
  -> BP_ExpeditionTerminal（目标 LocalPlayer 的 UI.Layer.GameMenu）
  -> W_HostSessionScreen
     -> 选择副本卡片（只更新选择，不立即 Travel）
     -> Create/Enter + Local -> 所选副本地图
     -> Create/Enter + Online -> CommonSession Host -> LobbyMap
     -> Find -> 既有 W_SessionBrowserScreen -> Join LobbyMap

LobbyMap
  -> BP_ExpeditionLobbyGameMode
  -> DA_Experience_Lobby
  -> W_ExpeditionLobbyScreen
     -> Host Start -> 所选副本地图
     -> Leave/Destroy -> HomeMap

Dungeon
  -> BP_DungeonPortal_ToHomeMap
  -> Leave/Destroy Session -> HomeMap
```

- `ULyraUserFacingExperienceDefinition` 负责面向玩家的地图、Lobby、Experience、人数、Join-in-progress 和 Replay 策略。
- `AShootGameModeBase` 解析 Travel URL 的 `Experience` 参数；不存在参数时才使用蓝图 `ExperienceDefinition`。
- `UShootExpeditionLobbyComponent` 放在 `AShootGameStateBase`，复制等待大厅的副本、人数、加入策略和开始状态。
- `UShootSessionCoordinatorSubsystem` 是 Host/Join/Leave/Destroy 的唯一项目协调层；页面和 Portal 不直接建立第二套 Session 状态机。
- `AShootExpeditionTerminal` 只提供交互数据并把页面推入发起交互者的 PrimaryGameLayout，符合本地分屏的 LocalPlayer 隔离要求。
- `UShootExperienceDefinition` 继续作为项目副本内容入口，后续 PVE、生化、PVP 模式通过 Data Asset 扩展，不复制当前不需要的 Lyra GameFeature Action 全套。
- 当前目录用 DungeonTest、SplitScreenTest、ExpeditionSandbox 三套完整配置验证选择链。SplitScreenTest 明确标记为只支持 Local；在线模式下选择它会回退 Local，不能创建错误的 Listen Server 请求。
- Bot 的长期语义是“AI 队友填补空位”；没有 AI 队友实现前保持禁用，不复刻 Lyra 的敌对 Bot 开关语义。

# 2026-08-30 设置设计

- 项目迁入 Lyra 通用 GameSettings 与 GameSubtitles 插件，并在 `Source/NewWorldOrder` 提供项目适配类；上游插件源码保持只读。
- `ULyraSettingsLocal` 保存设备级视频、音频、画质、语言和 Replay 设置，`ULyraSettingsShared` 保存 LocalPlayer 级控制与辅助设置。
- `ULyraGameSettingRegistry` 生成 Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五个页面；键位页从 `IMC_Default` 的 Player Mappable 配置动态生成。
- `W_LyraSettingScreen`、`W_SettingsPanel`、`W_GameSettingsDetailView` 和编辑器条目 Widget 只负责布局、描述与按钮交互；Apply/Cancel/Back 走 GameSettings Registry 生命周期。
- 五个分类由 `W_HorizontalTabList` 动态创建项目 `W_LyraButtonTab`；CommonBoundActionBar 动态创建项目 `W_BoundActionButton`，该按钮类默认使用 `ButtonStyle-Clear`。不要为设置页再迁入重复的 Lyra Menu/Arrow/Modal 按钮资产。
