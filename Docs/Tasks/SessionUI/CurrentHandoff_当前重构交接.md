# 副本选择 UMG 重构当前交接

- 日期：2026-09-16。
- 状态：已完成副本模式过滤、正式小队搜索条目迁移和无引用临时资产清理；编辑器和 PIE 已停止，后续仍需完成本地设备分配、在线大厅 Ready 和最终视觉验收。
- 当前分支：`codex/session-ui-refactor`。
- 权威需求入口：`Requirements_需求（新）.md`。
- 本地双人最终交互：`LocalCoopSetup_设备分配交互.md`。
- 调用链和待清理边界：`RefactorAudit_重构审计.md`。
- 本文记录本轮聊天中已经确定的产品目标、资产事实、进行中的实现和恢复顺序，防止模型切换或上下文压缩后要求用户重复解释。

## 2026-09-16 接手后的当前结论

- `W_ExpeditionSelection` 的模式 Tab 是副本列表的筛选入口，不是装饰性 Tab。`UShootHostSessionScreen` 保留完整 UFE 目录，在 `SelectedExpeditionMode` 变化后生成当前模式的过滤视图，并在当前副本不属于新模式时重新选择该模式的默认项或首项。
- UFE 使用 `SupportedModes` 作为唯一目录归属配置。它是 `ULyraUserFacingExperienceDefinition` 上的 `FGameplayTagContainer`，不是从 `bSupportsOnline` 或本地玩家数量推断；因此一个副本可以同时出现在多个模式 Tab。
- `W_FindSquad` 的 `SquadEntries` 已直接配置为 `/Game/UI/Menu/Expedition/W_SquadBrowserEntry`。`W_SquadBrowserEntry` 现在继承项目的 `UShootSquadListItem`，保留原来 `W_LyraSessionButton` 子控件提供的视觉样式；其图表只负责把 `BP_OnSquadResultChanged` 的结果数据写入按钮文本和可用状态。选中、Join 和会话状态仍由 `UShootFindSquadScreen` 与外层条目基类处理。
- 已删除且不再允许作为实现入口的资产：`W_SquadListItem`、`W_ExpeditionModeTab`、`W_ExpeditionListItem_ScratchBackup`。旧 `W_ExpeditionListItem` 只剩失效重定向器，也已清除。删除前已核对资产注册表、硬依赖以及加载资产后的实时引用。
- `W_FindSquad_LegacyLayoutBackup` 的历史未解析编译提示已清除。编辑器重启后 `W_ExpeditionSelection`、`W_FindSquad`、`W_SquadBrowserEntry` 均为 UpToDate，Widget hierarchy validation 均通过；PIE 能正常启动且未再弹出该蓝图编译阻塞。资产注册表曾短暂保留软依赖缓存，但加载确认的实时引用为空，不能把它当作现行调用链。
- 本轮遵循用户要求，没有重做 `W_ExpeditionButton` 或 `W_ExpeditionSelection` 的配色、布局和撑满 viewport 处理。用户后续草图到位前，不继续做视觉方向重构。
- 当前只完成结构和 PIE 启动级验证，没有把一次未成功打开搜索页的手动交互误记为完整流程通过；后续应在本地设备分配、在线大厅状态完成后统一做真实门链路和导航回归。

## 最终产品目标

将早期联机测试菜单和后续 Lyra POC 重构为正式的副本选择与小队流程。玩家不再从 `WBP_GameMenu` 的 Online 按钮进入旧 Session 面板，而是在 `HomeMap_Courtyard` 与 `HM_Expedition_Gate` 交互，进入统一的 Expedition Selection。

```text
HomeMap_Courtyard
  -> HM_Expedition_Gate
  -> W_ExpeditionSelection
     -> Single Player
        -> 选择当前操控主角与副本
        -> 确认
        -> 直接进入副本
     -> Local Co-op
        -> 两套设备分别左右选择男女主
        -> 两套设备分别确认不同角色
        -> 本地分屏进入副本
     -> Online Co-op
        -> 当前机器只有一名玩家，沿用单人选择的主角
        -> Create Squad / Find Squads
        -> 真实 CommonSession Listen Server 大厅
        -> 成员 Ready、邀请、房主开始
        -> ServerTravel 到副本
```

- 在线模式明确不支持同机双人联网；一台机器一名在线玩家。
- 本地双人明确只存在于本机，玩法类比 It Takes Two 和 Split Fiction，并复用项目现有分屏玩法链。
- `LobbyMap` 是当前真实大厅资产；设计稿中的 `SquadLobbyMap` 是产品流程名称，不能假装资产已重命名。
- 现有 CommonSession、CommonUser、OSSv1、`UShootSessionCoordinatorSubsystem`、Listen Server 和 Travel 主线优先复用。先检查可用轮子，只替换不适合正式产品的部分。

## 本地双人最终交互

- 页面同时展示男主和女主两张角色卡，以及每套参与设备的操作卡。
- 打开页面的设备默认指向玩家当前主角，但仍处于未确认状态。
- 每套设备独立左右选择。键盘默认 A / D 或 Left / Right Arrow；手柄默认左摇杆或 D-pad。
- 设备选择角色后，其操作卡移动到对应角色卡一侧，形成明确的位置反馈。
- 选择后还必须由该设备确认。键盘默认 Enter；手柄默认 Face Button Bottom / A。
- 两套设备可以暂时选择同一角色，但一个角色只能被一套设备确认。第二套设备不能确认已锁定角色，必须明确显示占用反馈并切换到另一角色。
- 两套设备分别确认两个不同主角后才允许进入游戏。
- 不增加 Swap Characters 按钮，不用传统大厅式 Join 锁定剩余角色，不让 Player 1 在双方确认后再按全局 Start，不允许一套设备替另一套设备确认。
- 键位只描述默认交互。实现必须使用 InputAction、InputMappingContext、CommonUI Action 与实际设备图标，不在 C++ 写键盘或手柄分支。
- 设备编号、LocalPlayer 编号、Player 1/2 和角色性别是四个不同概念，不能互相硬编码。
- 允许机器连接超过两套设备，但最多两套成为本轮操作来源；具体第三设备候选与断开重连策略需要在真实设备 PIE 测试后记录。

## 正式 UMG 范围

主要页面目标：

- `/Game/UI/Menu/Expedition/W_ExpeditionSelection`
- `/Game/UI/Menu/Expedition/W_LocalCoopSetup`
- `/Game/UI/Menu/Expedition/W_OnlineCoopEntry`
- `/Game/UI/Menu/Expedition/W_FindSquad`
- `/Game/UI/Menu/Expedition/W_SquadLobby`
- `/Game/UI/Menu/Expedition/W_ExpeditionTransition`

按实际复用需要拆分组件，不为清单机械创建重复层：

- 模式 Tab。
- 副本列表条目。
- 任务详情面板。
- 本地设备操作卡与角色卡。
- 在线成员槽位与搜索结果条目。
- 主次操作按钮。

视觉和编辑体验要求：

- 参考图只决定总体气质与信息层级，不要求一比一复刻，也不能照搬 AI 生成的占位内容。
- 用户允许基于真实项目上下文自由调整草图，验收目标是玩家容易理解、容易操作且达到正式游戏页面观感；仅有可编译控件树、明显占位排版或需要用户重画整页都不算完成。
- 采用深色半透明面板、细青蓝边框、明确的 hover / focus / selected / disabled 状态、左侧副本列表和右侧详情区。
- 使用 `/Game/UI/Menu/Art`、`/Game/UI/Hud/Art` 与 `/Game/UI/Textures/Session` 现有资源。
- `/Game/UI/Hud/Art/MI_UI_QuickBar_Border_Square` 使用 Box Brush，Margin 0.5，作为细边框候选。
- `/Game/UI/Textures/T_ArrowRight` 用于向右进入动作；`T_ArrowDown`、`T_ArrowDown_Simple` 和 `T_ArrowUp` 用于展开或排序等匹配方向的控件，不再用字符 `>` 冒充图标。
- 新按钮继承 `ULyraButtonBase`，Style 使用 `/Game/UI/Foundation/Buttons/ButtonStyle-Clear`，避免默认白色背景。
- `W_MenuButton` 和 `W_MenuButton_Modal` 可以作为代码或状态参考，但用户不推荐沿用它们的视觉；只有实际画面符合新方向时才复用。
- 普通按钮由内容决定 Desired Size，不统一写死宽高。模式 Tab 由父容器 Fill；主操作区、缩略图、角色卡和列表行只有在布局语义要求稳定尺寸时才设置最小值或 Override。
- 页面布局优先 VerticalBox、HorizontalBox、Overlay、GridPanel、ScrollBox、DynamicEntryBox 或 ListView；CanvasPanel 只用于真正需要自由定位的顶层。
- Widget Blueprint 负责固定文案、图标、颜色、尺寸、排版、导航与视觉动画。不得用 C++ `BindWidget` 名单锁死整张页面，让设计者删不掉控件。
- 玩家可见源文案全部使用英文 FText，并进入 `/Game/Localization` 的既有 11 Culture 流程。
- 玩家界面不得出现 Session、OSS、Listen Server、Local Players、Network Local 等实现术语。
- 草图里的 Research Facility、等级、预计时间、难度、敌人、奖励和加载百分比没有项目数据依据，不得伪造。

参考图现位于：

- `Docs/Tasks/PVEExperience/单人模式选择副本.png`
- `Docs/Tasks/PVEExperience/本地双人选择副本.png`
- `Docs/Tasks/PVEExperience/在线合作选择副本.png`
- `Docs/Tasks/PVEExperience/查找小队会话.png`
- `Docs/Tasks/PVEExperience/小队大厅.png`
- `Docs/Tasks/PVEExperience/进入副本中（可要可不要）.png`

## 已核实的现有实现

- `HM_Expedition_Gate` 的链路是 `BP_ExpeditionTerminal -> AShootExpeditionTerminal -> UShootGA_Interaction_OpenExpeditionScreen -> 发起者 LocalPlayer 的 PrimaryGameLayout / UI.Layer.GameMenu`，LocalPlayer 归属正确，只需替换页面类。
- `BP_ExpeditionTerminal` 继承 `AShootExpeditionTerminal`，当前使用父类上的 `ExpeditionScreenClass=W_HostSessionScreen`。
- `UShootHostSessionScreen` 没有页面 `BindWidget`；它已有目录枚举、选择、Local / Online 请求参数、Host、Find、打开浏览器和关闭能力。
- `ULyraUserFacingExperienceDefinition` 是副本目录单一数据源，已有标题、副标题、描述、图标、地图、Lobby、Experience、人数和在线支持字段。当前三份 UFE 的 `TileIcon` 均为空，玩家文案仍需改为英文。
- 现有目录为 `DA_UFE_DungeonTest`、`DA_UFE_ExpeditionSandbox` 和 `DA_UFE_SplitScreenTest`。设计稿中的其他任务名只是视觉示例。
- `W_SessionBrowserScreen` 仍继承旧 `UShootSessionScreen`，所以删除旧 Session C++ 前必须迁出搜索页仍使用的 Find、状态和关闭能力。
- `W_SessionBrowserScreen` 和 `W_LyraExperienceTileButton` 的现有视觉与交互可作为参考；已经被改丑的 `W_HostSessionScreen` 和 `W_ExpeditionLobbyScreen` 不作为最终画面基础。
- 现有 `UShootExpeditionLobbyComponent` 复制目标地图、Experience、人数和加入策略，但没有服务器权威 Ready 状态；旧大厅 Widget 也没有真实四成员列表。
- `TestMap_SplitScreen` 使用 `BP_TestGameMode`，继承 `AShootGameMode`；`LocalPlayerMapPolicy=SplitProtagonists`，已有第二 LocalPlayer 创建、分屏、互斥男女主和存档隔离链。
- `UCustomGameViewportClient::RemapControllerInput` 当前只调用父类；`bOffsetPlayerGamepadIds=True` 不能等同于任意设备分配已经完成。
- 项目已有 `UShootObjectEntryButtonBase`，用于运行时对象注入和设备无关的点击、焦点上报，应复用而不是再造 List 条目桥接基类。

## 本轮已产生的实现

### 资产

- `/Game/UI/Menu/Expedition/W_ExpeditionButton`
  - 父类为 `ULyraButtonBase`。
  - Style 已设为 `ButtonStyle-Clear`。
  - 根 SizeBox 保留为服务限制下的透传容器，`MinDesiredHeight` Override 已关闭，实际尺寸按内容 Desired Size。
  - 内部使用 `T_ArrowRight` Image，不再使用 `>` TextBlock。
  - 使用深色底、QuickBar 方框材质边框与青色焦点 / 悬停 / 选中高亮。
- `/Game/UI/Menu/Expedition/W_ExpeditionListItem`
  - 已改为复用 `UShootObjectEntryButtonBase`。
  - 使用 `ButtonStyle-Clear`、缩略图区域、标题和副标题。
  - `On Entry Object Set` 已接 `ULyraUserFacingExperienceDefinition.TileTitle` 和 `TileSubTitle`，蓝图编译状态在编辑器关闭前为 UpToDate。
  - 列表卡属于有稳定节奏的内容行，目前保留 136 的最小行高；这不代表通用按钮也写死高度。

### 脚本

- `Scripts/SessionUI_BuildExpeditionWidgets.py`
  - 只在 Unreal Python 环境运行。
  - 当前负责以上两个基础资产，包含 Desired Size 修正、真实箭头纹理、透明按钮 Style、数据条目和高亮图表。
  - 已补充幂等检查，识别父类事件显示名 `On Entry Object Set`，避免恢复工作时重复创建图表。

### C++

- `UShootHostSessionScreen::PopulateExperienceEntries` 是进行中的运行时桥接。
- 蓝图主动传入自己选择的 `UDynamicEntryBox` 和条目类；C++ 只创建条目、注入已有 UFE 对象、处理 hover / click 选择并同步 selected 状态。
- 页面没有新增 `BindWidget`，也没有在 C++ 中创建固定页面结构、颜色、文案或尺寸。
- UHT 已成功处理新增声明；完整 C++ 编译在 Unity 编译阶段按用户要求中止，因此该实现尚未获得编译通过结论。

## 暂停原因与构建状态

- 用户说明 G 盘存在问题，构建时磁盘活动时间达到 100%，表现为 Unity 单元长时间编译。
- 本轮错误地在只完成一个小 C++ 桥接后启动了冷编译。恢复工作时应先集中完成同一阶段所需 C++ API，再只执行一次冷编译。
- 标准构建已经完成 UHT，进入 5 个 NewWorldOrder 编译动作后由用户要求停止；不能记录为成功，也不能据此记录代码错误。
- 尝试诊断 UBA 时临时修改过 `Saved/UnrealBuildTool/BuildConfiguration.xml`，暂停前已经恢复原内容，没有留下构建配置变更。
- 当前未启动 Unreal Editor，没有运行 PIE，也没有主页面或运行截图。两个基础控件的离屏预览不能作为最终视觉验收。
- 用户考虑将来迁移到 macOS，但当前仓库约 15 GB，尚未进行 Git 瘦身，Mac 磁盘容量也有限。迁移和 Git 瘦身不是本轮 UI 重构的默认范围，除非用户另行要求。

## 恢复工作顺序

1. 先读本文、`Requirements_需求（新）.md`、`LocalCoopSetup_设备分配交互.md`、`RefactorAudit_重构审计.md` 和 `STATUS.md`，不要要求用户重新解释流程。
2. 检查当前分支与远端、工作区状态、编辑器进程和 G 盘状态。用户明确通知恢复前不要自行启动冷编译。
3. 先完成本阶段全部 C++ 数据出口：目录动态条目、选择详情数据、模式状态、本地设备选择状态和大厅 Ready / 成员状态设计。不要为页面布局增加 BindWidget。
4. 再执行一次 Windows Editor Development 冷编译。若 G 盘仍卡顿，先报告实际进程与磁盘证据，再由用户决定 Windows 或 macOS 构建环境。
5. 编译成功并重启编辑器后，批量创建模式 Tab、任务详情、角色 / 设备卡和 `W_ExpeditionSelection`，每五个新增 Widget 编译一次。
6. 先在真实副本门链路中打开主页面，获取 PIE 截图并修正视觉、焦点、键鼠和手柄导航，再扩展 `W_FindSquad` 与 `W_SquadLobby`。
7. 完成本地设备路由和服务器权威 Ready 后，迁移搜索页父类、切换 Terminal / Lobby 入口，重新核对引用。
8. 最后移除 GameMenu Online 按钮和废弃导航。旧 Session 资产与多份 C++ 文件属于多文件删除，按项目规则给用户精确清单和替代证据，由用户手动删除。
9. 更新任务包、编译与运行验收记录，提交并推送，报告 commit hash。

## 不得恢复的错误理解

- 不把在线模式做成同机双人联网。
- 不把本地双人简化为 LocalPlayerCount 加减按钮。
- 不使用默认 Player 1 男、Player 2 女作为不可修改的最终分配。
- 不增加 Swap Characters 与全局 Start 来替代每套设备的左右选择和各自确认。
- 不用 CommonInput 最近输入类型推断物理设备归属。
- 不把 UI 做成纯视觉虚拟大厅；在线玩家已经真实连接到同一个 Listen Server World。
- 不因旧页面难看就重写已经可用的 CommonSession / Coordinator 网络体系。
- 不在 C++ 写固定控件名、颜色、Padding、具体资产路径或键盘 / 手柄分支。
- 不把设计稿中的占位关卡、等级、奖励或加载百分比写入正式数据。
- 不在完成替代链和引用核对前删除 `UShootSessionScreen`，因为搜索浏览器仍依赖它。
- 不保留 `WBP_GameMenu` 的 Online 按钮或旧 Session 测试入口作为“兼容路径”；替代链验证完成后应彻底移除，避免误导同事和后续 AI。
