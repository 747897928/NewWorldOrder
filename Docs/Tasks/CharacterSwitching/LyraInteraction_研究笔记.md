# Lyra Interaction 研究笔记

状态：当前实现依据

## 结论

- Lyra 是本项目玩家世界交互的唯一实现基准。
- `UShootGA_Interact` 是玩家常驻的交互扫描能力，只授予 PlayerControlled 角色。
- `Collect`、`Revive`、`WorldCharacterSwitchRequest` 等具体执行能力不常驻，由附近 `IInteractableTarget` 通过 `UAbilityTask_GrantNearbyInteraction` 动态授予和回收。
- 世界交互与 CommonUI 菜单是两种前端入口。角色切换菜单不经过 `UShootGA_Interact`，二者只复用 PlayerController / PlayerState 的切换后端。

## Lyra 对照资产与代码

参考 Lyra 资产：

- `/ShooterExplorer/Input/Abilities/GA_Interact`
- `/ShooterExplorer/Input/Abilities/AbilitySet_InventoryTest`
- `/ShooterExplorer/Input/Mappings/IMC_InventoryTest`
- `/ShooterExplorer/Input/Actions/IA_Interact`
- `/ShooterExplorer/Interact/GA_Interaction_Collect`
- `/ShooterExplorer/Interact/GA_Interaction_Sit`

已确认的 Lyra 行为：

1. AbilitySet 只常驻授予 `GA_Interact`，输入标签为 `InputTag.Ability.Interact`。
2. `GA_Interact` 使用 `ActivationPolicy = OnSpawn`，激活后同时启动视线扫描和 `WaitInputPress(false)`。
3. 目标提供的执行能力由 `GrantNearbyInteraction` 动态授予。
4. `IA_Interact` 是普通 Boolean Action，不叠加 Pressed / Released Trigger。
5. LocalPredicted / LocalOnly 的 OnSpawn 能力由本地控制端发起；ServerOnly / ServerInitiated 由权威端发起。

项目对应代码：

- `Source/NewWorldOrder/Private/Interaction/Abilities/ShootGA_Interact.cpp`
- `Source/NewWorldOrder/Private/Interaction/Abilities/ShootGameplayAbility_Interact.cpp`
- `Source/NewWorldOrder/Private/Interaction/Tasks/AbilityTask_GrantNearbyInteraction.cpp`
- `Source/NewWorldOrder/Private/Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SingleLineTrace.cpp`
- `Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGameplayAbility.cpp`
- `Source/NewWorldOrder/Private/AbilitySystem/ShootAbilitySystemComponent.cpp`

## 当前调用链

### 授予与激活

1. `AShootCharacter::PossessedBy`
2. `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
3. `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
4. 只授予 `UShootGA_Interact`
5. `UShootGameplayAbility::OnGiveAbility` 或 ASC 重新绑定 Avatar 后调用 `TryActivateAbilityOnSpawn`
6. `UShootGA_Interact::ActivateAbility` 启动扫描、动态执行能力授予和输入等待

### 输入

1. `IMC_Default` 把键盘和手柄键位映射到 `IA_Interact`
2. `DA_ShootInputConfig` 把 `IA_Interact` 映射到 `InputTag.Ability.Interact`
3. `UShootInputComponent` 把 Started / Triggered / Completed 分发到 ASC
4. ASC 按 `OnInputTriggered / WhileInputActive / OnSpawn` 区分激活策略
5. 已激活的 `UShootGA_Interact` 通过 `WaitInputPress` 接收一次 Pressed 事件并触发当前选项

输入约束：

- Pressed / Released 各只发送一次 GAS ReplicatedEvent。
- Held 只用于 `WhileInputActive` 的激活判断，不能每帧伪造 Pressed。
- 不为交互键增加特殊硬编码分支，不判断具体键盘或手柄按键。

### 执行能力

1. 目标实现 `IInteractableTarget::GatherInteractionOptions`
2. 选项声明 `InteractionAbilityToGrant`
3. `UAbilityTask_GrantNearbyInteraction` 在目标进入范围时授予执行能力
4. 视线扫描解析到已授予的 AbilitySpec
5. `UShootGA_Interact` 通过 GameplayEvent 激活执行能力
6. 目标离开范围后动态 AbilitySpec 被回收

## BP_ShootCharacter 启动能力

- `StartupAbilities` 只保留有效的 `GA_Hero_Jump`。
- `StartupPassiveAbilities` 保留 `GA_ListenForEvent`，它仍负责监听 GameplayEvent 并应用配置的 GameplayEffect。
- 男女主正式技能由 `AShootPlayerState::ApplyGenderAbilityKit` 授予，不再由角色蓝图启动数组重复授予。
- `GA_Skill1~4` 作为后续技能扩展模板保留，但不加入当前 `BP_ShootCharacter::StartupAbilities`，因此不会与正式性别技能套件重复授予。
- 旧蓝图 `GA_Interact` 是人工迁移的 Lyra 蓝图参考资产，继续保留用于对照；当前运行时主线仍只授予 C++ `UShootGA_Interact`。

Aura 式 Startup 数组暂时只承担跳跃和事件监听的既有配置职责。后续若全面迁移到 Experience / AbilitySet，应作为独立任务同时处理存档恢复与技能套件，不能再新增平行授予链。

## 运行时验证

PIE 的 AbilitySystem Inspector 已确认：

- `ShootGA_Interact` 只有一个 AbilitySpec，且处于 Active。
- 附近目标只动态授予一个 `ShootGA_Interaction_Collect`。
- `GA_Skill1~4` 不出现在当前玩家 ASC，但资产本身保留供后续扩展。
- `GA_Hero_Jump` 存在。
- `GA_ListenForEvent` 存在且处于 Active。

仍需玩家在 PIE 做一次手感验收：准星聚焦 PressToInteract 目标后，单次交互键应立即领取。自动化可以发送按键，但嵌入式 PIE 的 Slate 焦点不足以证明按键进入了游戏视口，因此不能替代该验收。

## 角色切换边界

- 世界入口可以继续通过 `UShootGA_WorldCharacterSwitchRequest` 桥接共享切换后端。
- CommonUI 菜单入口直接调用 `AShootPlayerController::RequestSwitchCharacter`，不经过世界交互 GA。
- 角色切换专属长按不得重新塞回 `UShootGA_Interact`。
