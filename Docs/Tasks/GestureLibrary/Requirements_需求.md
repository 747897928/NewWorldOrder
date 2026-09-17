# 姿势库需求

最后更新：2026-08-17

# 当前需求

- 姿势库是 GameMenu 的可选内容区，不是游戏内 HUD 浮层。
- 删除 `/Game/UI/Pose/W_PoseLibrary`；`/Game/UI/Menu/WBP_GameMenu` 直接在 `PoseLibraryLyraScrollBox` 内管理姿势网格。
- `/Game/UI/Hud/W_DefaultHUD` 删除 `ExtensionPoint_PoseLibrary`。
- 删除 `HUD.Slot.PoseLibrary`、UIExtension 注册句柄和仅为该插槽存在的 C++ 字段、函数、模块依赖。
- 删除 `W_PoseLibrary.CloseButton` 及其图表；关闭和返回由 `WBP_GameMenu` 的 CommonUI Back Action 统一处理。
- 删除已不再需要的 `/Game/Blueprints/Input/Actions/IA_PoseLibrary`，并从输入映射中移除它。姿势库不再拥有 P 或手柄 DPad Up 的独立打开动作。
- 复核 `/Game/Blueprints/Input/IMC_FrontEnd` 保持为 GameMenu 激活时使用的前端输入上下文，不为姿势库新增独立输入映射。

# 玩家体验目标

- 玩家打开 GameMenu 后，在右侧姿势库区域直接浏览、滚动并选择姿势。
- 选择姿势后立即请求服务器播放，菜单保持打开，玩家可以连续预览其它姿势。
- 玩家用 CommonUI 的真实返回动作关闭整个 GameMenu，输入和焦点恢复到游戏。
- 键盘、鼠标、手柄通过 CommonUI 与 Enhanced Input 的已有映射获得一致导航；C++ 不判断具体设备或硬编码键位。

# 保留的职责边界

- `DA_PoseLibrary` 继续提供展示名、图标、性别、动画、Slot 和播放参数。
- `WBP_GameMenu` 的蓝图在 `PoseLibraryUniformGrid` 生成三列姿势卡片；`W_PoseLibraryEntry` 点击后调用 `AShootPlayerController::RequestPlayPoseByIndex`。
- C++ 保留服务器校验和 NetMulticast 动态 Montage 播放，删除 UIExtension 装配与独立打开入口。
- 不在 C++ 写死姿势资源、菜单布局或键盘/手柄分支。

# 性别与兼容骨架边界

- MM/MF 正式骨架按兼容骨架契约维护相同的可播放层级。AnimBP、动态 Montage 播放函数和兼容骨架能力不得硬编码“某动作只能男性或女性播放”。若发现骨骼层级不兼容，应修复骨架或重定向数据，而不是在播放层增加性别补丁。
- Male/Female/通用只用于将来的玩家目录内容策略。例如某个可爱动作可以只向女性角色展示，但开发/调试入口仍应允许在兼容骨架上直接播放，用于动画制作和验收。
- 当前 26 个条目只有女性资源，`CompatibleGender` 与服务器拒绝是现阶段的目录保护，不是最终架构。加入男性动画前，需要把“目录是否展示/允许玩家选择”和“底层动画能否播放”拆成两个职责，避免服务器播放接口永久承担美术内容限制。
- 当前阶段不提前决定一个条目是否必须拆成男女两份，也不强制一个条目同时保存 Male/Female 动画。等男性姿势资源进入后，根据实际目录是否共享名称、图标与动作语义再确定 DataAsset 结构。
