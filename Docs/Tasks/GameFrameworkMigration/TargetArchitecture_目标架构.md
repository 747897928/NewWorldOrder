# 目标架构

# 四层数据模型

```text
Experience
  全局玩法规则、默认 PawnData、模式 AbilitySet、HUD 插槽、队伍和胜负规则
        |
        +-- 每个 PlayerState / LocalPlayer 选择 PawnData
                Pawn 类、输入配置、相机配置、基础 AbilitySet、TagRelationshipMapping
                        |
                        +-- Equipment AbilitySet
                        +-- 临时交互执行能力
                        +-- 运行时模式附加 AbilitySet
```

# Experience

Experience 是一局游戏的全局玩法定义，不保存某个玩家的私有状态。

建议职责：

- 默认 PawnData。
- GameMode 或规则组件配置。
- 模式级 AbilitySet，例如感染规则、关卡通用交互、观战或复活规则。
- HUD Layout 和 UIExtension 注册配置。
- 默认队伍和阵营初始化策略。
- 允许的视角、角色选择和重生规则。
- 可选的 GameFeature Action 列表，但第一阶段不强制启用 GameFeatures。

建议首批资产：

- `DA_Experience_CampaignSolo`
- `DA_Experience_CampaignSplitScreen`
- `DA_Experience_CampaignOnline`
- `DA_Experience_Infection`

# PawnData

PawnData 是每个玩家当前可操控角色的配置。Experience 可以提供默认值，PlayerState 的选择可以覆盖默认值。

建议字段：

- PawnClass。
- `UShootInputConfig`。
- CameraMode 或相机配置。
- 基础 AbilitySet 数组。
- GameplayTagRelationshipMapping。
- 角色身份标签，例如 `Character.Protagonist.Male`、`Character.Protagonist.Female`、`Character.Infected`。
- 默认 Equipment 或 Loadout 定义引用。
- UI 展示元数据。

建议首批资产：

- `DA_PawnData_Protagonist_Male_TPS`
- `DA_PawnData_Protagonist_Female_TPS`
- `DA_PawnData_Protagonist_Male_FPS`
- `DA_PawnData_Protagonist_Female_FPS`
- `DA_PawnData_Infected`

如果 FPS/TPS 只差相机与表现，优先让相机配置可组合，避免复制全部技能和角色数据。

# AbilitySet

项目已经有 `UShootAbilitySet`，迁移应扩展和统一它，而不是新建第二套同义类型。

每次授予必须记录：

- AbilitySpecHandle。
- ActiveGameplayEffectHandle。
- 动态 AttributeSet 引用。
- 授予来源标识。
- 撤销时机。

建议来源层级：

- Experience：全局模式能力。
- PawnData：角色基础、移动、跳跃、交互扫描、主角技能和被动。
- Equipment：开火、瞄准、换弹和武器专属能力。
- Interaction Target：附近目标临时执行能力。
- Runtime Rule：感染、剧情或随机事件临时套件。

# GameplayTagRelationshipMapping

该数据资产负责按 Ability Tags 声明：

- 激活某能力时阻塞哪些能力。
- 激活某能力时取消哪些能力。
- 能力额外需要或阻止哪些 Owner Tags。

它不负责输入映射，也不负责授予能力。ASC 在能力激活查询中统一读取当前 PawnData 的映射。

# PlayerState、Pawn 与 LocalPlayer

- PlayerState 持有 ASC、Persistent Inventory、ResourceInventory、账号和角色快照。
- Pawn 持有移动、表现、Combat、Equipment 等与 Avatar 生命周期绑定的组件。
- PlayerController 负责服务器请求入口和玩家输入拥有权。
- LocalPlayer 持有本地设置、CommonUI 根布局和输入设备上下文。
- Experience 是世界级共享配置。
- PawnData 是玩家级选择，禁止放在全局单例里。

# 存档模型

建议分为三层：

- 账号或战役共享：关卡进度、解锁、Persistent Inventory、资源、衣柜拥有权。
- 角色独立：男女主 Loadout、QuickBar、外观、技能槽位选择。
- 运行时不保存：AbilitySpecHandle、活动输入状态、目标临时能力、RuntimeOnly 拾取和 Experience 授予句柄。

# 模式组合

- 单人战役：CampaignSolo Experience + 选定男女主 PawnData。
- 本地分屏：CampaignSplitScreen Experience + 两个 LocalPlayer 各自 PawnData；共享战役存档，不共享输入焦点和玩家私有 UI。
- 在线合作：CampaignOnline Experience + 每个连接玩家自己的 PawnData 和账号存档。
- 感染模式：Infection Experience 决定队伍、感染规则和 HUD；被感染玩家切换到 Infected PawnData 或叠加感染 AbilitySet。
- FPS/TPS：优先复用同一角色数据和能力，通过 PawnData 中相机与表现配置组合。

# GameFeatures 边界

第一阶段只实现可在主模块运行的 Experience、PawnData 和管理组件。

满足以下条件后才评估 GameFeatures：

- Experience 选择和加载时序稳定。
- AbilitySet 授予与撤销有完整自动化测试。
- 本地分屏和在线模式均能按玩家选择 PawnData。
- 模式内容具备独立打包、按需加载或团队隔离的真实需求。

届时可以把感染模式、特殊活动或独立 HUD 片段拆为 GameFeature，不应为了“像 Lyra”提前增加插件复杂度。
