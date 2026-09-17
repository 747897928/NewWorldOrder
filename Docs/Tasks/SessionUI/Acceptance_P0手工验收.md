# Session UI P0 手工验收

# 验收环境

- 使用本轮最终提交启动 NewWorldOrder Editor。
- 先用 Standalone 单客户端覆盖主流程，再按需要用 Listen Server 两客户端复验联机房间。
- 每项失败时记录当前地图、NetMode、屏幕截图和 `Saved/Logs/NewWorldOrder.log` 最后 200 行。

# 1. FrontEnd 设置页

- 从 FrontEnd 点击 Options。
- 顶部能看到 Gameplay、Video、Audio、Mouse & Keyboard、Gamepad 五个页签。
- Back、Apply Changes、Cancel 可见且使用项目 Foundation 样式，没有白色默认边框。
- Mouse & Keyboard 中 Jump、Move、Interact、Skill 等条目显示实际按键，不再显示 `No Editor Found`。
- 点击 Jump 的键位按钮会弹出 `W_PressAnyKey`，输入新键后返回设置页。
- 把 Jump 改到一个已被其他动作占用的键，确认弹出 `W_KeyAlreadyBoundWarning`。
- Apply Changes 后关闭并重新打开设置页，新键仍存在；Cancel 不保存本次未应用修改。
- 回到 PIE 后用实际新键触发 Jump，证明不是只改了显示文本。
- 修改任意一个键位后，该条目出现单项恢复默认按钮；点击后只恢复这一项。
- 底部常驻显示 Reset Defaults。触发后先出现确认框，Cancel 不修改设置。
- 确认 Reset Defaults 后，五个分类中允许重置的项目恢复默认并进入未应用状态；选择 Cancel Changes 可撤销整次恢复，重新确认并选择 Apply Changes 后重开设置页仍为默认值。

# 2. HomeMap 游戏内菜单

- 进入 HomeMap 后菜单不会自动弹出。
- 按 M 打开 `WBP_GameMenu`。
- Settings 与 Wardrobe 均可打开，子页面返回后仍回到正确 LocalPlayer 的菜单层。
- HomeMap 不显示“退出副本”动作；返回游戏能关闭菜单并恢复角色输入。
- 重复打开和关闭菜单、设置、衣柜至少三轮，无重复页面、焦点丢失或崩溃。

# 3. HomeMap 副本终端

- 靠近 ExpeditionTerminal，使用项目当前 `IA_Interact` 实际映射键打开副本选择页。
- DungeonTest、SplitScreenTest、ExpeditionSandbox 卡片均可选择。
- 单击卡片只改变选择，不立即 Travel。
- Local 入口按卡片配置进入目标图；SplitScreenTest 在 Online 状态下自动回退 Local。
- Online 入口先创建会话并进入 LobbyMap，不直接进入副本。
- Search Rooms、Back、本地玩家数量和中途加入策略可见；Bot 明确显示尚未支持。

# 4. LobbyMap

- 房主进入后能看到等待状态、目标地图和 Experience。
- 房主 Start 进入所选副本；非房主没有房主专属启动权限。
- Leave 会先显示确认，确认后离开会话并返回 HomeMap；Cancel 关闭确认框且不离开。
- 重复点击 Host、Start 或 Leave 不产生重叠请求或多次 Travel。

# 5. 副本内菜单与返回

- 在 TestMap_ListenServer、TestMap_SplitScreen 或 ExpeditionSandbox 按 M。
- Settings 可正常打开；HomeMap 专属 Wardrobe/Online 入口按上下文隐藏。
- “退出副本”可见，触发后先显示确认框。
- Cancel 留在当前副本；确认后清理 RuntimeOnly 会话数据并返回 HomeMap。
- Online Host 返回时销毁 Session；远端客户端离开时不伪造服务端权威清理。

# 6. 稳定性

- 连续执行 FrontEnd -> HomeMap -> Lobby -> Dungeon -> HomeMap 至少两轮。
- 日志中没有 Blueprint Runtime Error、Accessed None、重复 UI Layer、GC 崩溃或 Session 重入 Ensure。
- 本地分屏时每个 LocalPlayer 只打开和关闭自己的页面，不抢另一视口的焦点。

# 7. HomeMap 衣柜家具

- 进入 `Dressing_Table_Set` 的交互范围，提示可见。
- 使用 `IA_Interact` 当前映射键打开 `/Game/UI/Mutable/W_Cloth`；不能在家具逻辑中依赖硬编码 F。
- 打开的页面与 M 菜单 Wardrobe 是同一个 W_Cloth 类。
- 退出范围后提示隐藏；关闭衣柜后角色输入恢复。
- 分屏复验时只为发起交互的 LocalPlayer 打开衣柜。

# 2026-09-01 玩家验收记录

- 梳妆台交互范围内按当前 `IA_Interact` 默认键 F，成功打开与 M 菜单相同的 `W_Cloth`：通过。
- 设置页把 Toggle Camera 从 V 改为 N，Apply 后实际改键生效：通过。
- 全局 Reset Defaults 底部动作、确认弹窗、恢复默认和 Apply 流程：玩家验收通过。
