# Experience 与 PawnData 实施规格

# 建议新增类型

命名可在编码前按项目规范复核，但职责不能混合。

- `UShootExperienceDefinition`
  - PrimaryDataAsset，描述一局玩法组合。
- `UShootExperienceManagerComponent`
  - 挂在 GameState，服务器选择 Experience，客户端等待复制并完成加载。
- `UShootPawnData`
  - PrimaryDataAsset，描述某个玩家的 Pawn 配置。
- `UShootGameplayTagRelationshipMapping`
  - DataAsset，描述 Ability Tag 关系。
- `UShootPawnExtensionComponent`
  - 可选。只有当前 Pawn 初始化顺序无法可靠协调 ASC、PawnData、输入和组件就绪时才引入。

# Experience 加载状态

建议最小状态：

```text
Unloaded
  -> Loading
  -> Loaded
  -> Activating
  -> Ready
  -> Deactivating
```

约束：

- Experience 只由服务器选择并复制标识。
- 客户端不得自行选择不同 Experience。
- Pawn 生成、输入启用和 HUD 注册必须等待 Experience Ready。
- 加载失败必须有明确错误，不允许静默回退到硬编码默认玩法。
- 第一阶段使用同步或简单异步 PrimaryAsset 加载即可，不复制 Lyra 全部 ActionSet 和 GameFeature 状态机。

# PawnData 选择优先级

推荐优先级：

1. 当前模式强制的 PawnData，例如感染者。
2. PlayerState 已验证的角色选择。
3. Experience 默认 PawnData。
4. 开发期测试覆盖，仅在非 Shipping 构建允许。

选择结果由服务器权威确定，并保证每个 PlayerState 独立。

# 初始化调用链

```text
服务器选择 Experience
  -> GameState ExperienceManager 复制并加载
  -> Experience Ready
  -> GameMode 为每个 Controller 解析 PawnData
  -> 生成或重启 Pawn
  -> PlayerState ASC 绑定 Owner=PlayerState、Avatar=Pawn
  -> 授予 Experience AbilitySet
  -> 授予 PawnData AbilitySet
  -> 初始化输入、相机和 HUD
  -> 玩家可操作
```

卸载顺序反向执行，先停输入和 UI，再撤销 PawnData/Experience 句柄，最后解绑 Avatar 或销毁 Pawn。

# 双主角与分屏

- 单人切换角色时，保存当前角色快照，解析目标 PawnData，切换或重建 Avatar，再恢复目标角色 Loadout 和外观。
- 本地分屏创建两个 LocalPlayer，每个 LocalPlayer 有自己的 PlayerController、PlayerState、PrimaryGameLayout 和 PawnData。
- 共享战役存档不等于共享 PlayerState，也不等于共享 UI。
- 男主和女主可共享解锁池，但 QuickBar、外观和 PawnData 选择独立。
- 禁止使用 `GetFirstPlayerController`、`GetPlayerController(0)` 或无 OwningPlayer Widget。

# 在线合作

- 主机 Experience 决定本局规则。
- 每个远程 PlayerState 保存自己的 PawnData 选择和账号数据。
- 主机验证角色、阵营和模式限制后生成 Pawn。
- 无缝旅行时 Experience 与 PlayerState 数据的保留边界必须有自动化测试。

# 感染模式

推荐首版：

- Experience 配置人类和感染阵营规则。
- 人类使用男女主 PawnData。
- 感染后切换到 Infected PawnData，并撤销原主角 PawnData AbilitySet。
- 账号永久解锁和背包不因临时感染写坏。
- 模式临时能力和 RuntimeOnly 物品在对局结束时统一清理。
- 阵营判断继续使用 `ILyraTeamAgentInterface`，不写 `IsPlayer` 或具体 Character Cast 分支。

# 蓝图与 DataAsset 分工

- C++ 定义类型、加载状态、复制、授予和撤销生命周期。
- DataAsset 配置 Experience、PawnData、AbilitySet 和标签关系。
- 蓝图子类配置 Pawn 资产、相机、动画、UI 和可视化参数。
- GameMode 蓝图只选择默认 Experience 或提供开发期覆盖，不硬编码完整模式数组。
- 新增玩法、角色或技能组合应主要通过资产完成。
