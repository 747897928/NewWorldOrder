# HomeMap 环境交互配置

## 用户要求与边界

- 2026-09-05：昼夜切换改为实物控制面板，复用现有 IA_Interact，不额外占用键盘 N、手柄 View 或新的 IMC。
- 循环为白天、黄昏、夜晚。2026-09-06 用户最新要求：首次进入白天，第一次操作进入黄昏。
- 衣柜和副本入口继续使用各自既有业务，不修改角色、相机、Experience、库存或菜单流程。

## 必要桥接

- 项目 IInteractableTarget 标记 CannotImplementInterfaceInBlueprint。AShootEnvironmentControl 只创建碰撞和模型组件，并通过现有附近交互能力授予链提供 UShootGA_Interaction_Environment。
- 已验收实现由 UShootGA_Interaction_Environment 调用 AShootEnvironmentControl::AdvanceTimeOfDay，再广播 OnEnvironmentInteraction。AShootEnvironmentControl 根据实例的 DayNightSequenceActor、DayFrame、DuskFrame、NightFrame 调用 SequencePlayer；关卡蓝图不再重复控制序列。
- `BP_HomeMap_EnvironmentControl` 继承 AShootEnvironmentControl；关卡实例使用父类的 InitialTimeOfDay=Day。已删除旧 Level Blueprint 内固定跳到 60 帧的启动路径，防止默认白天被覆盖。
- `HM_DayNight_Control` 位于 `(170,-690,0)`，Visual 使用 `Props/Hub/SM_HM_EnvironmentConsole`，模型以地面中心为 pivot，组件相对变换为单位变换。模型约 4,649 三角面，不新增贴图。提示位置 Z=145，显示范围保留试点的 160 厘米。
- 当前环境切换在操作客户端执行；本轮没有实现跨网络同步天气，不能把它当作联机天气系统。

## 提示组件

- UShootLocalInteractionPrompt 继承 UWidgetComponent。WidgetClass、DrawSize、Space、位置等使用父类属性，在 HomeMap 蓝图子类中设置；显示距离使用本类 DisplayDistance。
- 编辑器里的组件是样式模板。运行时每个 LocalPlayer 使用独立的 WidgetComponent，并显式设置 OwnerPlayer 和 Widget 的 OwningLocalPlayer，避免分屏玩家共享屏幕提示。
- 每 0.2 秒检查一次玩家到物件根部的水平距离。提示的视觉 Z 偏移不参与距离计算；地图卸载清理计时器和临时组件。
- 衣柜原来的 InputActionWidget 默认可见，Sphere 的重叠图表未筛选对象；HomeMap 子类需禁用旧提示和旧提示球体的重叠事件，保留 WardrobeInteraction 业务组件与交互查询碰撞。
- 试点已经给 AShootExpeditionTerminal 增加提示组件。主线沿用该实现和备份关卡的实例配置，不再次添加组件，不修改 OpenExpeditionScreenForPawn。门和衣柜的 DisplayDistance=256 为用户验收值，保留。

## Sequence 来源

- LS_HomeMap_DayNight 复制自 NiagaraExamples/Gallery/LevelSequences/LS_DayNight，重新绑定 HomeMap 的日光、月光、天空光和雾；原示例序列不修改。
- 0 帧白天、60 帧黄昏、120 帧夜间，30 fps。PlayTo 在三个标记间平滑移动，保持端点状态。
- 不再注册演示 IA_DayNight，也不复制 Gallery 角色/HUD/曝光按键图表。

## 验证入口

- 当前在 HomeMap_Courtyard 中靠近控制台，使用现有交互操作；复查三种状态与循环顺序。该正式关卡由已验收 BackUp 重命名而来。
- 靠近、远离衣柜和副本入口，检查提示范围与实际交互能力。
- 结束 PIE 后检查没有 RemoveMappingContext 的 Accessed None；本地图已移除该注册/移除路径。
