# 副本选择与小队大厅重构需求

- 更新日期：2026-09-14。
- 来源：用户本次明确要求与附件布局参考。
- 本文是副本选择和小队大厅的新需求入口；旧 `Requirements_需求.md` 中与本入口冲突的 Online 菜单、Session 测试面板和 POC 展示要求由本文替代。设置页、会话清理、LocalPlayer 归属、输入恢复等既有要求继续有效。
- 实际依赖审计与待决问题见 `RefactorAudit_重构审计.md`。本文描述目标，不代表功能已经实现。

## 玩家流程

```text
                    HomeMap_Courtyard
                       │
                 与副本门交互
                       │
                       ▼
               Expedition Selection
                       │
          ┌────────────┼────────────┐
          │            │            │
        单人         本地双人      在线合作
          │            │            │
          │            │       Create / Find
          │            │            │
          │            │        OSS Session
          │            │            │
          │            │            ▼
          │            │      SquadLobbyMap
          │            │       Listen Server
          │            │
          │            │      等待 1~4 玩家
          │            │      Ready / Invite
          │            │            │
          │            │      Host Start
          │            │            │
          └────────────┴────────────┤
                                    │
                              ServerTravel
                                    │
                                    ▼
                              DungeonMap
```

- 唯一副本选择入口是庭院的 `HM_Expedition_Gate` 交互；保留现有交互能力和目标 LocalPlayer 路由，切换它打开的页面。
- `WBP_GameMenu` 移除 Online 按钮及其废弃导航代码。衣柜、设置、退出副本、返回等其他有效入口保持各自职责。
- 单人：选择副本后显式确认开始，直接进入副本。
- 本地双人：选择副本，两套设备分别左右选择男女主角并各自确认；只有确认不同角色后才进入副本。已有 TestMap_SplitScreen 的 LocalPlayer 创建、分屏和互斥主角恢复链应直接复用；缺口是选择页的设备分配和确认，不是重新实现本地多人。
- 在线合作：选择副本后 Create Squad 或 Find Squads；创建或加入真实服务器大厅，显示真实成员，准备后由房主开始。
- 用户已确认：在线每台机器仅一名玩家，不支持同机双人联网；在线角色沿用单人模式当前选择的主角。
- 用户已确认：本地双人仅离线，允许连接超过两个设备，但最多选两个设备分别控制男女主；设备编号、Player01/02 和主角性别不是同一个概念。页面必须显示设备类型并允许调整设备与主角归属。
- 本地双人选择属于本次功能范围，不能以“角色切换后续接入”为由省略设备到男女主的分配。具体交互提案与代码接点见 `LocalCoopSetup_设备分配交互.md`。
- 打开菜单的设备默认指向当前主角但不自动确认；其他设备通过方向输入参与选择。设备操作卡应随左右选择移动到对应角色卡一侧。
- 两套设备可以暂时指向同一角色，但一个角色只允许一套设备确认；第二套设备对已锁定角色的确认不可执行，并须显示占用反馈。
- 本地双人不使用 Swap Characters、传统大厅式 Join 或双方确认后的全局 Start 按钮替代独立左右选择与各自确认。
- `SquadLobbyMap` 是目标流程中的大厅名称；当前真实资产仍是 `/Game/Maps/LobbyMap`，不能将文档命名误当作已经完成地图迁移。
- 在线人数范围为 1–4；准备规则拟为所有已加入成员准备后允许房主开始，1 人房主允许独自开始。

## 第一阶段交付与视觉方向

- 先做可在 Unreal 中查看和操作的正式 UMG 布局与可复用组件，再接业务缺口，最后替换入口并清理旧实现。每阶段写清已实现和未接入项。
- 参考附件中的深色半透明面板、细青蓝边框、明确的选中态、左侧副本列表和右侧详情区；不要求复制复杂光效或草图中的场景。
- `W_ExpeditionSelection` 共用副本列表、模式 Tab 和详情。`W_LocalCoopSetup`、`W_OnlineCoopEntry` 优先作为其内部模式区域，避免三份重复页面和三套选择状态。
- `W_FindSquad` 是独立搜索页面；`W_SquadLobby` 是真实大厅 World 上的 CommonUI 页面；`W_ExpeditionTransition` 对接现有加载生命周期。
- 复用组件按实际需要拆出模式按钮、副本列表条目、任务详情、玩家槽位、小队结果条目。主次按钮首先基于项目 Foundation；不因参考文档列出名字就另造同职责基础按钮体系。
- 页面主布局用 HorizontalBox / VerticalBox 的 Fill、Spacer、ScrollBox / ListView。Overlay 用于背景、边框和状态叠层；CanvasPanel 只用于顶层定位。
- 使用 `/Game/UI/Textures/Session` 现有图标，优先白色透明版本。边框优先验证 `/Game/UI/Hud/Art/MI_UI_QuickBar_Border_Square`，Brush Draw As 为 Box，Margin 为 0.5。
- 新按钮继承 `ULyraButtonBase`，Style 使用 `/Game/UI/Foundation/Buttons/ButtonStyle-Clear`，避免默认白色背景。
- 普通按钮使用内容驱动的 Desired Size；模式 Tab 由父容器 Fill，只有主操作区、列表卡、缩略图和角色卡等具有明确布局语义的控件才约束最小尺寸或 Override。
- 使用 `/Game/UI/Textures/T_ArrowRight`、`T_ArrowDown`、`T_ArrowDown_Simple`、`T_ArrowUp` 等现有方向图标，不用 `>` 或 `v` 字符拼箭头。
- 固定文案、图标、大小、布局、导航和视觉状态在 Widget Blueprint 配置，选中态复用项目已有材质与动画做法。
- 最终视觉需有 Designer 或 PIE 截图，交互需 PIE 验证；离屏预览不能代替最终验收。

## 数据和代码边界

- `ULyraUserFacingExperienceDefinition` 继续作为副本目录的唯一来源，保留地图、Experience、人数和策略配置；预计时长、难度、敌人、奖励说明在需要时扩展该类。不得建立第二套目录表。
- Widget 可持有当前展示对象或 ViewModel 引用，但不复制维护一套副本配置，也不承担权威网络状态。
- C++ 管核心业务、异步生命周期、权威准备状态及数据出口；本地可观察状态用 MVVM FieldNotify；有多个跨系统消费者的状态变更才用 GameplayMessage。
- Widget Blueprint 配置绑定、负责视觉转场，并通过语义明确的业务函数处理固定按钮点击。不得用强制 BindWidget 名单规定整张页面结构。
- 可复用组件允许有明确的内部结构契约，但页面装饰、排版和固定文案不应成为 C++ 必须存在的控件。
- CommonUI 页面绑定目标 OwningPlayer，经该 LocalPlayer 的 PrimaryGameLayout 入栈，关闭调用 DeactivateWidget。
- 保留现有 CommonSession 与 `UShootSessionCoordinatorSubsystem` 请求、邀请、退出和 Travel 链，重构页面不新建网络服务。
- 大厅玩家 Ready 必须由服务器校验并复制，不能仅改本地按钮颜色；本地双人准备必须核对实际 LocalPlayer 和输入归属。
- 本地设备分配沿用现有 SplitProtagonists 启动链；不调用单人角色切换来模拟双人分配，不覆盖单人/在线使用的 LastActiveGender。选择结果须在恢复玩家快照与生成 Pawn 前生效。

## 英文源文案与真实性

- 开发源文案使用英文可 Gather 的 FText，保持稳定 namespace/key，并按项目流程更新 11 个 Culture 的 PO 和 locres。
- 玩家用词：Single Player、Local Co-op、Online Co-op、Create Squad、Find Squads、Join Squad、Invite Friends、Ready、Start Expedition、Leave Squad。
- Session、OSS、Listen Server、Local Players、Network Local 不出现在玩家界面和错误提示中。
- 草图中的 Research Facility、推荐等级、敌人、奖励、账号等级和加载百分比不是已确认游戏数据；不得据此新增虚假目录、恢复旧等级体系或伪造加载进度。
- 当前目录没有缩略图。实施时优先使用实际地图截图或用户指定图片；蓝图可配置无图展示，不硬编码其他关卡图片冒充实际内容。
- 搜索页面不伪造尚未获取到的远端成员、难度或房间信息；缺字段须补数据协议或合理省略。

## 替换和清理顺序

1. 完成 UMG 视觉及组件编辑体验，记录截图和结构检查。
2. 接入目录和状态；补真实本地加入与在线准备逻辑；验证加载、错误、返回和重复请求。
3. 从 `UShootSessionScreen` 迁出搜索浏览器仍使用的能力，再替换浏览器父类和图表引用。
4. 副本门切到新入口，Lobby Experience 切到新大厅 UI；验证旧入口已无运行依赖。
5. 移除 GameMenu Online 导航；核对蓝图、C++、配置、脚本引用后清理旧 Session 页面、专用条目和后端，不能用 Hidden、Collapsed 或空函数留壳。
6. 多文件删除须遵守项目现有安全约束，先提供精确清单和替代链证据，再由用户手动处理；其余重构工作继续完成。
7. 更新任务包，完成适当编译和运行验证，提交并推送，报告 commit hash。
