# 游戏框架迁移需求

# 产品目标

- 核心类型为第三人称合作生存射击。
- 射击约占玩法权重 80%，技能约占 20%；技能服务于战术和 Build，不替代枪法与走位。
- 网络统一使用 Listen Server，在线合作最多 4 人。
- 支持单人、本地双人竖直分屏和在线合作。
- 单人模式可在 Hub 切换男女主，副本战斗中不切换。
- 本地分屏由两个 LocalPlayer 分别操控男女主，共享战役进度、奖励和解锁；每名角色保留独立 Loadout、QuickBar 和外观。
- 在线模式允许玩家自由选择男女角色组合，不把“角色性别”硬编码成唯一玩家身份。
- 后续需要容纳第一人称、第三人称、生化感染等玩法变体，不通过复制一套 Character 或 GameMode 代码实现。

# 架构目标

- Experience 决定当前世界的全局玩法规则和默认配置。
- PawnData 决定某个玩家当前操控角色的 Pawn 类、输入、相机、基础 AbilitySet 和标签关系。
- AbilitySet 是技能、效果和 AttributeSet 的数据驱动授予单元，并能按来源完整撤销。
- GameplayTagRelationshipMapping 统一表达 Ability Tag 之间的阻塞、取消和附加激活要求。
- 装备继续通过 EquipmentDefinition 的 AbilitySet 授予武器能力，SourceObject 必须是 EquipmentInstance 或 WeaponInstance。
- 交互扫描主能力来自玩家基础配置；具体交互执行能力由附近目标动态授予和回收。
- 存档保存长期进度、解锁、技能选择、角色 Loadout 和外观，不保存可由配置重建的运行时 AbilitySpec 列表作为最终权威。
- UI 使用 CommonUI、MVVM 和 GameplayMessage，并严格绑定目标 LocalPlayer。

# 迁移原则

- Lyra 是框架生命周期和数据分层的首要参考，不是要求复制所有示例内容。
- 保留已经验证的项目业务能力，不因来源是 Aura 就直接删除。
- 每次只替换一条可验证的授予或初始化链路，旧链路在替代完成前不得提前删除。
- 替代完成后必须删除旧入口、兼容兜底和重复数据源，不能长期双轨运行。
- C++ 负责网络权威、生命周期和稳定接口；蓝图和 DataAsset 负责玩法组合、资产引用、UI 表现与调优。
- 禁止在 C++ 构造函数硬编码技能数组、模式资产路径、UI 样式和可配置目录。

# 完成标准

- 单人、本地分屏、在线 Listen Server 均通过同一套 Experience 启动流程。
- 同一局内每个玩家可拥有独立 PawnData，不通过全局单例保存玩家私有选择。
- 每个 GA 都能说明唯一授予来源、撤销时机、输入标签和 SourceObject。
- 首次进入、重生、无缝旅行、断线重连、角色切换时不会重复授予 AbilitySpec 或遗留被动效果。
- 男女主技能由 PawnData 引用的 AbilitySet 配置，不再由 PlayerState 构造函数硬编码数组。
- 新玩法模式可以通过新增或组合 Experience、PawnData、AbilitySet 和 GameplayTag 配置接入，基础 C++ 不需要为每个模式增加分支。
- 旧存档能迁移到新结构，或明确提供一次性版本升级；不得静默丢失解锁、库存、Loadout 和外观。
- 删除旧 Aura 路径前有引用证据、替代链路和自动化或 PIE 验收记录。

# 当前需要在实现阶段最终确认的产品细节

- 本地分屏“共享进度”的持久化所有者：推荐由主 LocalPlayer 的存档保存战役和共享解锁，第二 LocalPlayer 仅保存个人输入、角色偏好和可选个人配置。
- 在线合作奖励的归属和回写策略：推荐每名在线玩家写自己的账号存档，关卡规则仍由主机 Experience 权威决定。
- 感染模式中阵营切换后是否更换 PawnData、仅叠加模式 AbilitySet，或两者组合；首版推荐更换 PawnData 并由 Experience 追加模式套件。
