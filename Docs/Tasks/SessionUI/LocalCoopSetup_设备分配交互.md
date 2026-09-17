# 本地双人设备分配

- 日期：2026-09-14。
- 状态：用户已确定 Split Fiction / It Takes Two 式核心交互；尚未完成页面、设备路由或 PIE 验证。
- 本文属于 SessionUI 任务，不替代角色切换、存档或 GameFramework 的既有架构。

## 已确定需求

- 单人和在线：同一玩家当前操控男主或女主，沿用已有主角选择；在线每台一人。
- 本地双人：同一台机器两个 LocalPlayer，一人男主、一人女主，玩法继续沿用现有多人实现。
- 允许连接键鼠、手柄 1、手柄 2 等多个设备，从中选择两套输入；没有第三名本地玩家。
- 玩家可以决定哪套设备控制哪位主角；界面显示实际键鼠或手柄类型。
- 此处是设备与双主角的分配，不是账号登录，也不是调用单人切换入口改存档。

## 已确定交互

1. 进入 Local Co-op 后同时展示男主角和女主角两张角色卡，以及参与选择的设备操作卡。布局与反馈参考 Split Fiction 和 It Takes Two 的角色选择页，但使用本项目角色、材质、图标和视觉语言。
2. 打开菜单的设备默认指向当前主角，属于“已选择但未确认”的初始状态，不能自动 Ready。其他候选设备保持中间等待位置，产生有效方向输入后才参与角色选择。
3. 每套设备独立左右选择角色。键盘默认支持 A / D 与 Left / Right Arrow，手柄默认支持左摇杆与 D-pad；具体键位必须由 InputAction 和 InputMappingContext 配置，C++ 不判断具体按键。
4. 设备向左或向右选择后，对应设备操作卡在视觉上移动到男主或女主角色卡一侧，让玩家直接看出“哪套设备正在选择哪个角色”。
5. 选择不等于确认。键盘默认用 Enter、手柄默认用 Face Button Bottom / A 执行确认；提示使用 CommonUI 动作图标，从实际映射解析，不把 Enter 或 A 画死在 Widget 中。
6. 两套设备可以暂时指向同一个角色，但一个角色只能被一套设备确认锁定。若该角色已被另一设备确认，第二套设备的确认动作不可执行，并显示清楚的占用反馈；玩家需要左右切换到另一角色。
7. 两套设备必须分别确认两个不同主角。双方确认完成后才进入所选本地双人副本，并继续走现有离线 Travel、第二 LocalPlayer 创建与 SplitProtagonists 链。
8. 本地双人没有第三个角色，也不允许同一设备控制两个角色。即使连接设备超过两套，同一时刻也只有两套参与设备能够成为 Player 1 和 Player 2 的输入来源。

## 已否决的旧提案

- 不增加独立的 Swap Characters 按钮；角色归属由每套设备自己左右选择。
- 不把第二设备的核心流程设计成传统大厅式 Join 按钮后固定占据“剩余角色”。
- 不由 Player 1 在双方 Ready 后再按一个全局 Start Expedition；最终门槛是两套设备分别确认不同角色。
- 不在打开页面时把当前设备直接标为 Ready，也不允许一套设备替另一套设备确认。
- 返回、取消确认、设备断开重连和第三设备候选切换的最终细节尚未由用户逐项确认，实施时应延续上述状态机并用 PIE 实测后记录，不能恢复本节已经否决的方案。

## 状态与设备行为

- 键盘和鼠标作为一套 Keyboard & Mouse，不能拆成两个玩家。
- 设备只是连接时不自动确认角色；方向选择与确认是两个独立状态，避免第三个手柄误触即锁定角色。
- 两个角色都已确认时，其他设备不能改变角色归属或抢走已确认设备的输入焦点。
- 玩家离开后释放其设备和主角占位，并取消开始资格；第三台设备随后可以加入空位。
- 设备断开时显示 Controller disconnected，取消该槽准备并禁用开始；同一设备恢复连接后仍需重新 Ready。
- 已确认设备若要换角色，必须先取消自己的确认；另一设备不能替它取消或改选。
- 更换所选副本后取消双方准备，确保确认针对当前任务。
- 当前方案不加入设备拖拽、全局交换按钮、多级设置弹窗、玩家账号选择等与两人开局无关的操作。

## UMG 展示

- 两张角色卡展示主角视觉、Player 1/2、设备图标/名称、状态和所属设备的动作提示。
- 角色预览来自项目已有男女主外观/预览能力；不使用附件截图中的其他游戏角色作为项目资源。
- 设备状态区只呈现真实检测结果，区分已分配、可加入、断开；不把当前系统枚举下标永久保存为设备身份。
- 固定文案使用英文 FText，例如 Local Co-op、Player 1、Player 2、Keyboard & Mouse、Controller、Choose a character、Confirm、Confirmed、Character taken、Waiting for input、Cancel、Back、Controller disconnected。最终源文案必须保持稳定 namespace/key 并进入项目本地化流程。
- 同屏两张卡的按键图标必须分别按所属设备解析；不能使用 Player 1 最近一次输入类型刷新整页所有提示。
- CommonUI/Enhanced Input 定义横向选择、确认和取消动作；默认映射覆盖 A/D、左右方向键、手柄左摇杆、D-pad、Enter 和手柄 Face Button Bottom，但 C++ 不硬编码这些键位。
- 具体控件排版、颜色、动画和文案留在 Widget Blueprint；数据状态可通过 MVVM 绑定，不给页面增加固定名称的强制 BindWidget 清单。

## 已核实的复用链

```text
TestMap_SplitScreen
  WorldSettings_1.DefaultGameMode
  BP_TestGameMode / AShootGameMode
  LocalPlayerMapPolicy = SplitProtagonists

UShootGameInstance::ApplyLocalPlayerMapPolicy
  SetDisableSplitScreen(false)
  UGameplayStatics::CreatePlayer
  引擎 Login / RestartPlayer

AShootGameMode::ResolveInitialCharacterGender
  Player01 默认性别或未来选择结果
  Player02 相反性别

AShootGameMode::HandleStartingNewPlayer_Implementation
  RestorePlayerInventoryState
  Pawn 生成前完成主角快照恢复
```

- BP_TestGameMode 继承自 AShootGameMode，使用的是 AShootGameModeBase 上的 LocalPlayerMapPolicy 和 AShootGameMode 上的 SplitPlayer01DefaultGender。目前 CDO 为 SplitProtagonists、MALE，ExperienceDefinition 指向 DA_Experience_SplitScreenTest。
- AShootGameMode::ShouldPersistLastActiveGender 已禁止本机两位分屏玩家覆盖单人 LastActiveGender，必须保留。
- GameFrameworkMigration/STATUS 已明确后续选择页应让 ResolveInitialCharacterGender 读取互斥选择结果，不应在 Pawn 生成后再切性别，以免与 Mutable 异步恢复冲突。
- DefaultEngine.ini 当前为竖向分屏、bUseSplitscreen=True、bOffsetPlayerGamepadIds=True；默认键鼠/手柄偏移不是完整的任意设备映射方案。
- UCustomGameViewportClient::RemapControllerInput 目前只执行 Super；历史自定义映射注释未启用，HandleInputDeviceConnectionChange 为空。后续应替换这些相关遗留片段，不恢复硬编码 ControllerId 分支。

## 实施边界与验收

- 设备发现、连接状态、设备归属与 CommonUI 输入类型是不同的数据：CommonInput 可提供类型/提示，但不能单靠最近输入类型判断哪个物理手柄加入哪个玩家。
- 选择页阶段不为每个检测到的设备创建 LocalPlayer。先保存本地配置，按现有生命周期在合适时间创建第二位玩家，防止庭院 PrimaryOnly 策略提前清理或不必要地切开选择界面。
- 对数据跨地图的传递和输入映射，只在核对引擎/项目真实接口后实现；本提案不声称任意设备重映射已经可用。
- 保留 UI 的目标 LocalPlayer 所有权；两设备参与选择由局部配置协调，不能通过 AddToViewport 或 GetPlayerController(0) 绕过 CommonUI 规则。
- 验证键鼠+一手柄、两手柄、三设备任选两台、交换主角、各自准备、第三设备误触、断开重连、返回重进和进入副本后的操控归属。
- 验证本地双人退出后，单人/在线仍恢复原先主角；设备分配不能污染 LastActiveGender。
- 本次审计未执行 PIE、真实设备枚举/输入验证或 C++ 编译，不作为功能通过记录。
