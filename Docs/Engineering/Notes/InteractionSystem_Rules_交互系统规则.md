# Interaction System Rules / 交互系统规则

更新时间：2026-03-20
负责人：Codex

## 1. 交互主体（Interaction Actors）
- PlayerControlled  
  - Avatar：`AShootCharacter`  
  - Controller：`AShootPlayerController`  
  - ASC：挂在 `AShootPlayerState`（`UShootAbilitySystemComponent`），通过 `AShootCharacterBase::GetAbilitySystemComponent` 暴露。  
  - 拥有账号仓库（`UShootInventoryManagerComponent`）、`UResourceInventoryComponent`、SaveGame、QuickBar。
- AIControlled  
  - Avatar：`AEnemyBotCharacter`（派生自 `AShootCharacterBase`）。  
  - Controller：`AAIController`。  
  - ASC：直接挂在 Pawn 上，同样是 `UShootAbilitySystemComponent`。  
  - 无账号仓库/SaveGame，仅限临时数值调整。

结论：
- 若项目启用通用玩家交互 GA，则 `UShootGA_Interact` 只授予 PlayerControlled 角色。
- AI 如需交互须使用独立 GA 或 BT Task，直接在服务器执行逻辑。
- 当前项目已经把玩家交互能力授予链收口到 C++：
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
  - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
  - 默认只兜底授予 `UShootGA_Interact`
  - `Collect`、`Revive`、`WorldCharacterSwitchRequest` 等执行能力由附近交互目标通过 `UAbilityTask_GrantNearbyInteraction` 动态授予
  - 不再依赖蓝图 `StartupAbilities` 或旧存档分支偷偷授予交互能力

## 2. 触发模式（Trigger Modes）
交互物/拾取物统一暴露以下配置：

```cpp
UENUM(BlueprintType)
enum class EShootInteractionTriggerMode : uint8
{
    None,
    AutoOverlap,
    PressToInteract,
    AIScripted
};

UENUM(BlueprintType)
enum class EShootInteractionUserFilter : uint8
{
    PlayerOnly,
    AIOnly,
    PlayerAndAI
};
```

- `PressToInteract` + `PlayerOnly/PlayerAndAI` → 若启用通用玩家交互 GA，则进入 `UShootGA_Interact` 的视野列表，使用输入 Tag `InputTag.Ability.Interact`。
- `AutoOverlap` + Player → 继续沿用 `AShootResourcePickup` 的自动拾取逻辑。  
- `AutoOverlap` + AI → 适用于敌人经过时自动生效（回血区、陷阱等）。  
- `AIScripted` → 行为树或脚本显式调用，不依赖输入/交互 UI。

## 3. 效果配置（Faction / Effect Table）
为避免在 C++ 中写 `if (IsPlayer)` 分支，交互物提供阵营驱动的效果表：

```cpp
USTRUCT(BlueprintType)
struct FShootFactionEffectEntry
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere)
    FGameplayTag FactionTag; // 例：Faction.Player、Faction.Enemy

    UPROPERTY(EditAnywhere)
    TSubclassOf<UGameplayEffect> EffectClass;
};

UPROPERTY(EditAnywhere, Category="Interaction")
TArray<FShootFactionEffectEntry> FactionEffects;
```

拾取或交互能力流程：
1. 从 Instigator 的 ASC 上读取阵营 Tag（`Faction.Player`、`Faction.Enemy` 等）。  
2. 查表获取对应 GE（玩家扣血、AI 回血等差异在数据中配置）。  
3. 使用 `UShootAbilitySystemComponent` 应用该 GE。

## 4. 能力职责
- `UShootGA_Interact`  
  - 玩家专用，职责：扫描 `IInteractableTarget`、维护聚焦选项、监听交互输入、向 `UShootGA_Interaction_Collect` 或其他交互 GA 发送事件。  
  - 必须通过 `ActorInfo->AbilitySystemComponent`、`ActorInfo->PlayerController`、`ActorInfo->AvatarActor` 获取上下文，禁止假设 ASC 挂在 Pawn 上。
  - 若项目最终启用这套通用交互 GA，推荐作为玩家预设常驻能力存在，不由交互物临时授予。
  - 当前 `StartupInputTag` 已在 C++ 构造函数中补齐为 `InputTag.Ability.Interact`，保证 `IA_Interact -> InputTag.Ability.Interact -> ASC` 输入链闭合。
  - `ActivationPolicy = OnSpawn`。能力授予时或 ASC 重新绑定 Avatar 后自动激活；输入只唤醒能力内部的 `WaitInputPress`，不会重新激活能力。
- `UShootGA_Interaction_Collect`  
  - 可被玩家或 AI 触发。  
  - 根据拾取物配置判断是账号资源（写入 `UResourceInventoryComponent`）、Persistent 物品（`UShootInventoryManagerComponent::AddPersistentItem`）、还是 RuntimeOnly 临时武器（`AddRuntimeItem`/`EquipTemporaryPickupWeapon`）。  
  - 服务器负责修改库存/资源与广播 UI 消息；客户端仅播放表现。
- AI 专用交互  
  - 当 AI 需要操控世界物件时，使用脚本/行为树任务或独立 GA（不依赖输入 Task），直接在服务器执行同样的 GE/库存接口。

## 5. ResourceInventory vs InventoryManager
- ResourceInventory（数量型仓库）  
  - 存储 Persistent 材料、货币、徽章、设计图。  
  - AutoOverlap/PressToInteract 皆可调用 `AddResource/ConsumeResource`，由 SaveGame 管理持久化。
- InventoryManager（有身份物品）  
  - 管理由 `EShootItemLifetime` 标识的 ItemInstance。  
  - `AddPersistentItem` → 账号武器/装备；`AddRuntimeItem` 或 `EquipTemporaryPickupWeapon` → 副本临时武器。  
  - QuickBar、EquipmentManager 只能引用此组件提供的实例。

## 6. UI 与输入
- 交互提示使用 `ULyraActionWidget` / `UCommonActionWidget`，信息来源于 `FInteractionOption`。  
- `UShootGA_Interact` 在聚焦变化时广播消息或调用委托更新 UI。  
- Enhanced Input → `InputTag.Ability.Interact` → Ability Input Binding → `UAbilityTask_WaitInputPress`。

补充规则：
- 主交互能力和执行交互能力分层：
  - 主交互能力是玩家侧基础设施，负责扫描、聚焦、输入和 UI
  - 执行交互能力负责具体业务，如拾取、救援、角色切换
- 对项目主线而言：
  - 如果启用通用交互系统，主交互能力建议默认常驻
  - 交互物只提供 `IInteractableTarget` 选项和事件数据
- `GrantNearbyInteraction` 负责按附近目标动态授予具体执行能力；目标离开范围后回收对应 Spec，避免玩家常驻所有世界交互能力。

输入事件约束：
- `IA_Interact` 保持普通 Boolean Action，不在 IA 资产上叠加 `Pressed + Released` Trigger。
- `UShootAbilitySystemComponent` 按 `OnInputTriggered / WhileInputActive / OnSpawn` 分流激活。
- 一次物理 Pressed/Released 只发送一次 GAS ReplicatedEvent；`Held` 不得每帧伪造 Pressed，否则 `WaitInputPress` 会重复触发。

## 7. 实装注意事项
- 所有交互/拾取相关 C++ 必须有中文注释说明“玩家 vs AI、TriggerMode、Inventory/Resource 交互”以便 AI 同事阅读。  
- 任何会修改账号数据的逻辑（ResourceInventory/InventoryManager/QuickBar/SaveGame）都需确认 `ActorInfo->PlayerController` 非空并在服务器端执行。  
- RuntimeOnly 物品在副本结束或返回 Hub 时统一清空，不写入 SaveGame，也不更新 QuickBar 配置。  
- AI 不得授予 `UShootGA_Interact`，也不得依赖 WaitInputPress/交互 UI，避免无意义的输入等待。
