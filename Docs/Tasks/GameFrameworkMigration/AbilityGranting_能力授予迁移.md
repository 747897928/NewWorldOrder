# 能力授予迁移

# 核心规则

每个能力必须只有一个当前有效的授予来源。相同 GA 不得同时由 Character 启动数组、SaveGame、PawnData 和 C++ 兜底重复授予。

# 2026-09-02 P0 落地状态

- 新建 `/Game/GameFramework/Abilities/DA_AbilitySet_PlayerCore`，当前包含相机切换、Jump、Interact、QuickBar Next、QuickBar Previous 和 DropWeapon。
- 三个可玩 Experience 的 `UShootExperienceDefinition::CommonAbilitySet` 已改为该资产；Lobby Experience 保持空套件。
- `BP_ShootCharacter` 继承自 `AShootCharacter`，原先配置在父类属性 `AShootCharacterBase::StartupAbilities`、`AShootCharacterBase::StartupPassiveAbilities` 以及旧 `AShootCharacter::CoreInteractionAbilities/CoreCombatAbilities` 中的玩家直授数据已清空。
- `AShootCharacter::LoadProgress` 只初始化副本属性，不再给 PlayerState ASC 授予玩家能力。`AShootCharacterBase` 的 Startup 数组仍只保留给 ASC 归属于 Pawn 的 AI 路径。
- 空配置的 Aura 遗留 `GA_ListenForEvent` 没有迁入 PlayerCore；它当前只有 `BP_ShootCharacter` 一个引用，且其 EventBasedEffectClass/EventTags 默认值为空。资产暂不删除，等待单独清理旧 Aura 内容。
- 公共 AbilitySet 与性别 AbilitySet 在 `AShootPlayerState` 中分别持有 `FShootAbilitySet_GrantedHandles`。性别切换只刷新性别来源，Experience 卸载按逆序全部撤销。
- 动态 AttributeSet 撤销已调用 `UAbilitySystemComponent::RemoveSpawnedAttribute`，不再只清空本地数组。
- 四个主角被动改为 `OnSpawn`。三个长期 Buff 被动保存各自的 ActiveGE 句柄并在 Ability 结束时撤销；MarkHunter 保持激活并在撤销时解绑击杀委托。
- `InitializeRoundForAll` 只重置回合属性，不重新授予 Match 技能。迟到玩家由 `PostLogin` 幂等补授予。
- 主动技能与性别被动仍都由 Experience 管理，但授予时机不同：性别被动在 Experience 入场时立即授予；
  主动技能由 Experience 的 `SkillLoadoutConfig` 声明本局目录，玩家局内获得后才按槽授予。
- `UShootSkillLoadoutComponent` 为每个主动槽保存独立 AbilitySet 句柄。Experience 卸载先清主动槽，再撤销性别与公共套件。

当前 PlayerCore 暂由 Experience 的 CommonAbilitySet 承载，这是在没有完整 `UShootPawnData` 前消除平行授予链的最小安全步骤。后续若引入 PawnData，应迁移该资产的归属而不是再复制一份能力数组。

# 2026-09-03 Home Hub 最小能力落地

- 新建 `/Game/GameFramework/Abilities/DA_AbilitySet_HomeCore`，只包含相机切换、Jump 和 Interact。
- 新建 `/Game/GameFramework/Experiences/DA_Experience_Home`，其 `CommonAbilitySet` 指向 HomeCore，不配置
  `SkillLoadoutConfig`、QuickBar/DropWeapon 能力或战斗 HUD 注册。
- `/Game/Blueprints/GameMode/BP_ShootGameMode` 的父类 `AShootGameModeBase::ExperienceDefinition` 已显式指向
  Home Experience；HomeMap 的 WorldSettings 使用该 GameMode。
- 这修复 HomeMap 无法跳跃、无法与副本传送门和梳妆台交互的问题，同时没有恢复 Character 直授链。
- 当前战斗 Experience 选择 PlayerCore，Home Hub 选择 HomeCore；两套最小集按 Experience 互斥授予并分别持有可撤销句柄。

# 当前桥接授予矩阵

| 能力类型 | 授予来源 | SourceObject | 撤销时机 |
| --- | --- | --- | --- |
| Jump、相机切换 | Experience CommonAbilitySet：战斗用 PlayerCore，Home 用 HomeCore | PlayerState / 当前套件 | Experience 卸载 |
| 交互扫描主能力 | Experience CommonAbilitySet：PlayerCore 或 HomeCore | PlayerState / 当前套件 | Experience 卸载 |
| 男女主角色固有被动 | 当前性别 AbilitySet，由 Experience 管理 | PlayerState / 性别套件 | 切换角色或 Experience 卸载 |
| 四槽局内主动技能（含手雷） | Experience SkillLoadoutConfig 中的 Definition；获得后按槽 AbilitySet 授予 | SkillLoadoutComponent | 槽位替换、清空或 Experience 卸载 |
| 武器开火、瞄准、换弹 | Equipment AbilitySet | EquipmentInstance 或 WeaponInstance | 卸下装备 |
| Collect、Revive、世界切换请求 | Interaction Target 动态授予 | 交互目标 | 目标离开扫描范围 |
| 感染或模式能力 | Experience 或运行时规则 AbilitySet | Experience/规则对象 | 模式结束或阵营转换 |
| Round 三选一、商店、神秘商人技能 | 只调用 SkillLoadoutComponent 获得/升级入口 | 对应局内规则对象 | Match 或 Experience 结束 |

未来引入 `UShootPawnData` 时，Jump、Interact 等基础能力可迁到 `PawnData::AbilitySets`，由 PlayerState 按来源授予和撤销。
这是归属迁移，不是保留 CommonAbilitySet 后再复制一份。Lyra 对照证明的是这一职责方向，不要求现在全抄 GameFeature Experience。

# 现有能力的迁移去向

- `GA_Hero_Jump`
  - 已从 `AShootCharacterBase::StartupAbilities` 迁入可撤销 AbilitySet；当前战斗 PlayerCore 与 HomeCore 都按需包含它。
- `GA_ListenForEvent`
  - 已确认是父类为原生 `GameplayAbility` 的 Aura 遗留 Event 到 GE 桥接蓝图。
  - 当前 EffectClass 与 EventTags 均为空，不进入正式 PlayerCore AbilitySet。
  - 暂保留资产，等待确认历史内容无外部依赖后删除。
- `UShootGA_Interact`
  - 从 `AShootCharacter::CoreInteractionAbilities` 迁入玩家基础 AbilitySet。
  - 保持 OnSpawn 激活策略。
- 男女主角色固有被动
  - 当前男女主 AbilitySet 各只保留两个被动，并由 Experience 入场时授予。
  - 被动使用 OnSpawn 激活；长期 GE 和事件委托必须随各自 AbilitySet 句柄撤销。
- 男女主原有主动技能与手雷
  - 不再随性别套件整包入场授予。正式候选已拆成 Skill Definition 与独立 AbilitySet。
  - 玩家从 Experience 的随机池、Round 三选一或商店获得后，才由四槽组件按槽 InputTag 授予。
  - 手雷固定 T 映射已移除，作为普通主动技能进入四槽；槽位 IA 仍通过 IMC 支持改键与多输入设备。
- 旧蓝图 `GA_Interact`
  - 继续作为 Lyra 对照资产保留，当前正式 AbilitySet 只引用 C++ `UShootGA_Interact`。

# 输入与激活策略

- Enhanced Input 只产生 InputTag，不直接决定能力来源。
- AbilitySet 把 InputTag 写入 AbilitySpec 的动态来源标签。
- ASC 按 `OnInputTriggered`、`WhileInputActive`、`OnSpawn` 分发。
- OnSpawn 能力在 Avatar 绑定完成且 AbilitySpec 可用时激活。
- `AbilitySpecInputPressed/Released` 继续使用 GAS ReplicatedEvent，满足 WaitInputPress/WaitInputRelease。
- 禁止恢复 Held 每帧伪造 Pressed 的旧逻辑。

# 存档边界

- 当前产品决策是不保存局内主动技能、技能等级、槽位、属性等级或 Match Gold。
- `UShootSkillLoadoutComponent` 只存在于当前 Match/Experience；它的复制数据不得进入 SaveGame。
- SaveGame 继续只保存账号级外观、Persistent 物品、资源与 QuickBar 配置。
- 如果未来增加账号级“技能图鉴/解锁”，必须与局内四槽实例分成两个数据结构；图鉴解锁也不能直接恢复 AbilitySpec。

# 去重与撤销

- `UShootAbilitySet::GiveToAbilitySystem` 必须由服务器调用。
- 每个来源持有自己的 `FShootAbilitySet_GrantedHandles`，不能把不同来源混在一个全局数组。
- PawnData 切换先撤销旧句柄，再授予新句柄。
- 被动能力撤销时必须同时清理由该来源产生的长期 GameplayEffect；当前主角四个被动已按此规则修正。
- Equipment 的 SourceObject 约束保持不变。
- 不使用按 AbilityClass 全局扫描作为正常撤销手段；按类扫描只允许用于一次性迁移诊断。

# 验收用例

- 首次创建角色仅存在一份 Jump、一份 Interact 和当前主角被动；未局内获得的主动技能不存在 AbilitySpec。
- HomeMap 可跳跃，并可与梳妆台及副本传送门交互；只有一份 Jump、Interact、Camera，且无 QuickBar/Drop/四槽能力污染。
- 死亡重生后 AbilitySpec 数量不增长。
- 男女主往返切换十次后没有另一性别技能或被动效果残留。
- 装备和卸下武器后，武器能力与 SourceObject 正确增减。
- 走近两个交互目标再离开，执行能力按目标句柄正确回收。
- 退出并重新进入 Experience 后四槽为空，SaveGame 中不出现本局技能 Definition、等级或 AbilitySpec。
- `Shoot.Skill.AcquireRandom` 在有空槽时先填满四个不同技能，满槽后才升级；`Shoot.Skill.Clear` 后能力与 UI 同步清空。
- Listen Server 的主机和远程客户端均能正确预测并由服务器校正。
