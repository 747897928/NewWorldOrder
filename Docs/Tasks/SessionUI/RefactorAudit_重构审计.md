# 副本选择与小队大厅重构审计

- 日期：2026-09-14。
- 阶段：需求与依赖审计完成，两个基础 UMG 控件和无 BindWidget 的目录条目桥接已开始；主页面、入口切换和旧代码清理尚未完成。
- 依据：用户本轮指令、SessionUI 现有需求/Context/DesignSpec/Implementation/STATUS、MCP 踩坑记录、项目 C++、目标编辑器中的 Widget 控件树/图表/资产注册表/CDO 与关卡实例。
- 暂停前 MCP 编辑器 World：`/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`；当前编辑器已按用户要求关闭。
- 已创建 `/Game/UI/Menu/Expedition/W_ExpeditionButton` 与 `W_ExpeditionListItem`，并新增目录动态填充桥接；没有运行 PIE。UHT 成功后，完整 C++ 编译因 G 盘 100% 活动和用户要求在 Unity 编译阶段中止，因此不宣称编译或运行验收通过。
- 当前恢复入口与详细状态见 `CurrentHandoff_当前重构交接.md`。

## 已核实的调用链

```text
HM_Expedition_Gate
  BP_ExpeditionTerminal（/Game/Gameplay/Interactables/Stations）
  AShootExpeditionTerminal
  UShootGA_Interaction_OpenExpeditionScreen
  OpenExpeditionScreenForPawn
  发起者 LocalPlayer 的 PrimaryGameLayout / UI.Layer.GameMenu
  W_HostSessionScreen / UShootHostSessionScreen
  ULyraUserFacingExperienceDefinition
  UShootSessionCoordinatorSubsystem
  UCommonSessionSubsystem

WBP_GameMenu / UShootMainMenuScreen
  OnlineButton.OnClicked
  OpenSessionScreen / SessionScreenClass
  WBP_SessionScreen / UShootSessionScreen
  W_SessionBrowserScreen / UShootSessionScreen
  W_SessionBrowserEntry / UShootSessionBrowserEntry

DA_Experience_Lobby
  W_ExpeditionLobbyScreen / UShootExpeditionLobbyScreen
  AShootGameStateBase 上的 UShootExpeditionLobbyComponent
  StartSelectedExpedition
  ServerTravel
```

- `BP_ExpeditionTerminal` 继承自 `AShootExpeditionTerminal`，使用该父类上的 `ExpeditionScreenClass`，当前值是 `W_HostSessionScreen`。庭院实例的实际读取与此一致；新入口应改该配置。
- 实际在线大厅是 `/Game/Maps/LobbyMap`。三份 UFE 的 `LobbyMapID` 都指向它，不能先改文档地图名就认为引用已迁移。

## 主要发现

1. `W_SessionBrowserScreen` 和旧 `WBP_SessionScreen` 共用 `UShootSessionScreen` 父类。浏览器 EventGraph 仍调用该父类的 FindOnlineSessions、CloseSessionScreen，并接收 On Session State Data Changed。直接删除这个类会破坏保留的搜索浏览器。
2. `UShootHostSessionScreen` 没有 BindWidget，它通过三个 BlueprintImplementableEvent 通知目录、选项和异步状态。该页面难编辑不能全部归因于 C++ 绑定，还需处理蓝图自身的控件变量、事件和动画引用。
3. `UShootExpeditionLobbyScreen` 仍通过 BindWidgetOptional 访问 StartButton 和 InviteButton，并直接设置可用态。可迁为可观察业务状态，由新蓝图绑定，释放固定控件名称约束。
4. MCP 查询 Host 和 Lobby Widget 的 ViewModel 注册数量均为 0。当前主要是数据事件模式，还没有接成用户期望的 MVVM 页面。
5. LobbyComponent 复制了目标地图、Experience、等待与加入策略，但没有 Ready 字段。StartSelectedExpedition 只检查服务器、发起者本地控制器、等待状态和目标地图，不检查成员准备状态。
6. Host 的 AddLocalPlayer / RemoveLocalPlayer 只调整 LocalPlayerCount 请求值。在线时强制为 1，本地上限 2；这不是设备加入与两人各自准备流程。后续核对确认 TestMap_SplitScreen 的真实分屏链已经存在：UShootGameInstance::ApplyLocalPlayerMapPolicy 创建第二名 LocalPlayer，AShootGameMode 解析互斥主角并复用存档恢复；不能把页面缺口误写成项目没有本地多人能力。
7. `W_ExpeditionLobbyScreen` 实际仅有数量和文字，没有真实成员槽位列表。网络已经是真实大厅，UI 成员展示需要补齐。
8. UFE 已统一维护 TileTitle、TileSubTitle、TileDescription、TileIcon、MapID、LobbyMapID、ExperienceID、MaxPlayerCount、bSupportsOnline；时长、难度、敌人、奖励说明尚不存在。
9. 三份 UFE 的 TileIcon 均为空，TileTitle/描述仍是中文，其中副标题直接出现 Listen Server。英文源文案修改必须覆盖数据资产和状态/错误提示，不能只改按钮。
10. 搜索页可以复用现有列表和请求链，但旧 STATUS 同时包含历史通过记录与后续 Mac/Steam 未验收项；本次没有实机验证，不能把它们写成已修复或必然不可用。

## 现有目录

- `DA_UFE_DungeonTest`：`/Game/Maps/TestMap_ListenServer`，Experience 为 `DA_Experience_DungeonTest`，最大 4 人，支持在线。
- `DA_UFE_ExpeditionSandbox`：`/Game/Maps/TestMap_ExpeditionSandbox`，Experience 为 `DA_Experience_ExpeditionSandbox`，最大 4 人，支持在线。
- `DA_UFE_SplitScreenTest`：`/Game/Maps/TestMap_SplitScreen`，Experience 为 `DA_Experience_SplitScreenTest`，最大 2 人，不支持在线。
- 附件中的 Research Facility 等只是视觉示例，不是现有目录项。

## 复用与待清理边界

- 保留主线：CommonSession、Coordinator、门交互 GA/Actor、UFE 目录、Shoot Experience、Lobby GameState 组件、PrimaryGameLayout。
- 重构或迁移：Host 页面数据后端、Lobby 页面数据后端、搜索页所需的旧父类能力、准备状态与成员展示。
- 复用视觉参考：`W_SessionBrowserScreen`、`W_SessionBrowserEntry`、`W_ExperienceList`、`W_ExperienceTile`、项目 Foundation 按钮和 Menu/Hud Art。
- 当前 Experiences 目录实际卡片资产名是 `W_ExperienceTile`；用户提及的 W_LyraExperienceTileButton 不能未经定位就当成待编辑资产路径。
- 待清理候选：`WBP_SessionScreen`、`WBP_SessionResultButton`、`WBP_SessionPlayerEntry`；配套 `ShootSessionScreen`、`ShootSessionResultButton`、`ShootSessionPlayerEntry` 的头文件和实现；GameMenu Online 控件及 OpenSessionScreen/SessionScreenClass。
- 资产注册表当前返回：旧 SessionScreen 由 GameMenu 引用；旧 ResultButton/PlayerEntry 仅由旧 SessionScreen 引用；浏览器由旧 SessionScreen 和 HostScreen 引用。该结果是候选依据，删除前还必须重新核对迁移后的类引用、软引用和替代入口。
- C++ `ShootSessionCoordinatorSubsystem.h` 仍有指向旧 WBP_SessionScreen 的注释，清理时同步更新，不能留下错误交接信息。
- 删除候选涉及多文件。项目 AGENTS 要求此类删除由用户手动处理；本轮不执行删除，等替代链完成后再提交具体清单。

## 第一阶段 UMG 设计决定

- 新选择页采用同一外壳：页头、左侧任务列表、右侧模式 Tab、任务图片/描述/信息、底部动作和 CommonUI 输入提示。
- LocalCoopSetup、OnlineCoopEntry 优先做嵌入式区域，复用任务详情和选择状态，不复制三份选择页。
- 搜索页保留列表/详情双栏，明确搜索中、空结果、失败、不可加入和加入中状态。
- 小队大厅在真实 Lobby World 上显示任务详情、4 个成员槽位、准备状态、邀请/离开/开始动作。
- 过渡页遵循真实加载信号；没有可信进度接口时使用不定进度，不显示虚构百分比。
- 页面外壳和所有视觉参数放蓝图；数据来自 ViewModel/业务事件。可复用条目内部保持简短可读的更新与视觉转场图表。

## 2026-09-14 用户澄清与已确定边界

- 在线每台一人，使用单人模式已选的当前主角；不支持同机双人联网。
- 本地双人参考 Split Fiction / It Takes Two 的角色选择：打开页面的设备默认指向当前主角但不确认；每套设备独立左右选择，设备卡随选择移动到男女主一侧，再由本设备确认。可以连接第三个设备，但只有两套设备控制两名玩家。
- 两套设备可暂时指向同一角色，但同一角色不能被两套设备确认。后确认者的确认动作不可执行，必须切到另一角色。双方分别确认不同角色后进入游戏。
- 旧提案中的 Swap Characters、传统 Join 占据剩余角色、Player 1 全局 Start 已被用户否决，不得恢复。
- 先前“角色选择可以后续接入”的建议混淆了单人主角切换与本地设备分配，已撤回。本次本地选择页必须包含设备到男女主的分配。
- 已实际读取 TestMap_SplitScreen 的 WorldSettings_1，DefaultGameMode 为 `/Game/GameFramework/GameModes/BP_TestGameMode`；地图有两个 PlayerStart。
- BP_TestGameMode 继承 AShootGameMode，使用 AShootGameModeBase 上的 LocalPlayerMapPolicy=SplitProtagonists，以及 AShootGameMode 上的 SplitPlayer01DefaultGender=MALE；Player02 取相反性别。
- GameInstance 已通过标准 CreatePlayer 创建第二个 LocalPlayer 并启用分屏；AShootGameMode::ResolveInitialCharacterGender 已有选择页接入位置，双人配置不覆盖 LastActiveGender。
- CustomGameViewportClient 当前仅调用父类 RemapControllerInput；自定义映射片段被注释，连接回调为空。DefaultEngine.ini 启用了 bOffsetPlayerGamepadIds，不能把默认偏移当成任意设备选择已经完成。
- 最终交互与明确的未实现项见 `LocalCoopSetup_设备分配交互.md`。当前仍未进行设备实操与 PIE 验收。

## 当前实现检查点

- `W_ExpeditionButton` 继承 `ULyraButtonBase`，使用 `ButtonStyle-Clear`、QuickBar 方框边框材质和 `T_ArrowRight`。普通按钮的 MinDesiredHeight Override 已关闭，尺寸由内容 Desired Size 决定。
- `W_ExpeditionListItem` 复用现有 `UShootObjectEntryButtonBase`，从 `ULyraUserFacingExperienceDefinition` 刷新标题与副标题，编辑器关闭前蓝图为 UpToDate。
- `UShootHostSessionScreen::PopulateExperienceEntries` 由蓝图传入 DynamicEntryBox 和条目类，C++ 负责动态条目与选择回调，没有新增页面 BindWidget；该 C++ 尚未完成完整编译。
- `Scripts/SessionUI_BuildExpeditionWidgets.py` 保存以上资产的可重复构建步骤。主页面和其余组件尚未创建。
- 构建和 Unreal Editor 已停止。恢复时先批量完成同阶段 C++，再做一次冷编译，避免在 G 盘问题下频繁重编。

## 交接与验证门槛

- 新代码前通过 MCP 查询实际蓝图子类/CDO；新字段的默认值和覆盖位置写中文注释及配置步骤。
- 视觉验证覆盖英文源文案、长文本、选择态/焦点态/禁用态和分屏页面归属。输入提示通过 CommonUI/Enhanced Input，不写死 Q/E/Esc 图标。
- 功能验证覆盖单人、本地双人、真实 Host/Join Lobby、准备同步、房主开始、离开/销毁、返回和重复打开。
- 蓝图编译/属性复读、单机 PIE、双实例网络验证分开记录，不能互相替代。
- 当前工作区已有用户暂存的 `Content/Maps/HomeMap.umap` 删除；不纳入本轮文档提交。新需求草稿原有 HomeMap_Courtyard 修正已保留。
