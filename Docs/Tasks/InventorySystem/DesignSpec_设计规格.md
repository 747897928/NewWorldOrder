# 方案选型
1. **直接扩张 UCombatComponent**  
   - 优点：少量文件，沿用现有武器槽逻辑。  
   - 缺点：需要手写 FastArray/预测/复制，无法复用 Lyra 的 ItemDefinition/Fragment/Cost 流水线，后续消耗品与制作逻辑难维护。
2. **移植 Lyra 库存管线（推荐）**  
   - 模块：`FLyraInventoryList` + `ULyraInventoryManagerComponent` + `ULyraInventoryItemInstance/Definition` + `FGameplayTagStackContainer`。  
   - 优点：原生支持 TagStack、AbilityCost、Replication、装备/碎片拓展，实现团队也熟悉；与 GameDesign 需求高度匹配。  
   - 成本：需新建 Inventory 目录、注册子对象复制、整合到角色与 UI，但比重新造轮子风险低。

=> 结论：采用方案 2，并根据项目命名规范包装为 `ShootInventory*` 系列类，同时保留 `UCombatComponent` 作为 Quickbar（用于可装备武器/Consumable）。背包系统负责所有 Item 数据，Quickbar 只指向背包中的可装备条目。

# 目标架构
```
ShootCharacter
├─ UCombatComponent（现有：武器槽/Quickbar）
└─ AShootPlayerState（账号数据）
    ├─ UShootInventoryManagerComponent（战斗背包：武器/消耗品，Lyra 风格，ItemInstance+Equipment）
    ├─ UResourceInventoryComponent（新增：材料/货币/徽章/设计图等账号资源，非战斗背包）
    └─ SaveGame 映射（Resources / OwnedWeapons / UnlockFlags）
```

## 核心组件
1. **UShootInventoryManagerComponent**  
   - 从 Lyra 复制 `ULyraInventoryManagerComponent`，移除对 Lyra AbilitySet/Equipment 的硬依赖。  
   - 挂在 `AShootPlayerState`（账号级），经由 `UShootInventoryFunctionLibrary::GetInventoryManager` 在角色/控制器/组件侧访问。  
   - 暴露 `AddItemDefinition/ConsumeItemsByTag` 等接口供拾取、奖励、制作调用。  
   - 负责子对象复制（`ReplicateSubobjects`）以同步 ItemInstance；支持物品生命周期（Persistent/RuntimeOnly），SaveGame 只序列化 Persistent，副本结束清理 RuntimeOnly。

2. **UShootInventoryItemDefinition / Instance**  
   - Definition：静态数据（名称、类型、Icon、默认 StatTags、GrantedAbilities/Effects、CraftingRecipe）。  
   - Instance：运行时状态（耐久、随机属性、当前堆栈），内含 `FGameplayTagStackContainer StatTags`（Lyra System/GameplayTagStack.h 直接复用）。  
   - 对材料：使用单独 ItemDef（如 `Item.Material.MilitaryAlloy`），堆叠逻辑由 InventoryManager 控制；StatTags 中记录“可作为 AbilityCost 使用的标签数值”。  
   - 新增 `UShootInventoryFragment_StackRules`：提供 MaxStackCount / bUnique 规则，ResourceInventory 与 InventoryManager 统一读取。  
   - 新增 `UShootInventoryFragment_QuickbarSlotRules`：限制物品可放入的槽位类型，QuickBar 在服务器侧校验。  

3. **FShootInventoryList / Entry**  
   - 直接复制 `FLyraInventoryList/Entry`，重命名并放入 `Source/NewWorldOrder/Public/Inventory`。  
   - 用于在组件内部管理 Items，并通过 `PostReplicatedAdd/Change` 触发 GameplayMessage（驱动 UI、Tab 面板、制作界面）。

4. **FGameplayTagStackContainer**  
   - 复制 Lyra 版本（Docs/ExampleProjectCode/.../System/GameplayTagStack.h/.cpp），命名为 `ShootGameplayTagStack` 以避免冲突。  
   - 用作材料/货币统计，支持 Add/Remove/Query + 快速映射 + FastArray 复制。  
   - 将军用合金、碎片、金币等定义为 GameplayTag（例：`Inventory.Material.MilitaryAlloy`）。  
   - GAS AbilityCost（如弹药、制作）可直接读写 TagStack，与 Lyra 的 `ULyraAbilityCost_ItemTagStack` 接口兼容。

5. **UI / 消息桥接**  
   - 继续使用 GameplayMessageSubsystem，新增 `Msg_Inventory_ItemChanged`、`Msg_Inventory_StackChanged`。  
   - Tab 背包界面与 Hub 制作界面订阅消息或直接查询 InventoryManager（LocallyControlled 客户端读取复制数据即可）。

6. **UResourceInventoryComponent（新增账号资源仓库）**  
   - 轻量结构：`TArray<FResourceEntry{ItemDef, Count}>`，对材料/货币/徽章/设计图等做 Add/Consume/Query；复制可选，持久化必需。  
   - 挂在 PlayerState，与 SaveGame 对应，避免战斗背包混用；拾取/制作/商店接口直接调用资源仓库。  
   - 配方/制作系统从资源仓库扣减，再通过 InventoryManager 产出武器/装备类 Item。

### 物品生命周期与拾取规则（2025-11-15）
- `UShootInventoryItemInstance` 必须内置 `EShootItemLifetime` 字段：`Persistent` 代表账号资产（会持久化到 SaveGame，可配置 QuickBar），`RuntimeOnly` 代表副本临时物品（不写存档，副本结束清理）。
- `UShootInventoryManagerComponent` 需要提供 `AddPersistentItem` 与 `AddRuntimeItem` 两条入口，拾取 Actor 必须根据物品类型调用对应接口：Hub/奖励/制作产生的武器→Persistent，关卡内临时武器拾取→RuntimeOnly。
- SaveGame 与 QuickBar 配置仅包含 `Persistent` 实例；副本内武器替换时 QuickBar 只更新当前装备引用，不修改账号槽位配置。
- `UResourceInventoryComponent` 始终存储 Persistent 资源（材料/货币/徽章/设计图），副本内临时增益请直接由能力或拾取效果处理，避免引入 RuntimeOnly 资源形态。
- 代码层面所有库存/武器类需加入中文注释，显式说明 ResourceInventory=数量仓库、InventoryManager=有身份背包、QuickBar/Equipment 只读 InventoryManager，防止未来协作出现理解偏差。

## 现阶段落地结构（2025-11-18）
- `UResourceInventoryComponent`（`Source/NewWorldOrder/Public/Inventory/ResourceInventoryComponent.h`）  
  - 挂载在 `AShootPlayerState`，负责材料/货币/徽章/设计图等“数量型账号仓库”。  
  - 所有拾取/制作/奖励若仅影响数值资源，必须经由 `AddResource/ConsumeResource` 调用，内部广播 `UI.Toast.ResourcePickup`。  
  - 不与 QuickBar 交互，SaveGame 永远写入 Persistent 数值。
- `UShootInventoryManagerComponent`（PlayerState）  
  - 承载所有“有身份”的 Item（武器/装备/消耗品），保存 `UShootInventoryItemInstance` 子对象并同步到客户端。  
  - 提供 `AddPersistentItem/AddRuntimeWeaponItem/RemoveRuntimeItems/FindItemByInstanceId`，是 QuickBar、拾取、制作唯一的数据入口。  
  - SaveGame 只序列化 `EShootItemLifetime::Persistent`；`RemoveRuntimeItems` 在副本结束时由 CombatComponent 调用，保证 RuntimeOnly 不进存档。
- `UCombatComponent`（Character）  
  - 仅负责 QuickBar 槽位状态、当前激活槽、装备/卸装流程。  
  - 从 PlayerState.InventoryManager 读取 ItemInstance Guid，并通过 `AssignSlotFromInventory`/`RemoveItemFromSlot` 驱动 `UShootWeaponInstance` 与 Legacy `AShootWeaponActor`。  
  - `DropActiveRuntimeWeapon`、`ClearRuntimeSlots` 等方法只在服务器调用，日志 `LogCombatComponent` 会标明 `[Server]`/`[Client]`。
- 拾取 Actor：`AShootWeaponPickupActor` 与 `AShootResourcePickup`  
  - WeaponPickup：服务器在 `HandlePickup` 中调用 `InventoryManager->AddRuntimeWeaponItem`，随后转给 `UCombatComponent::AssignSlotFromInventory` 自动装填；`DropActiveRuntimeWeapon` 生成的拾取 Actor 仅携带 ItemDefinition + StatTags。  
  - ResourcePickup：所有资源型拾取强制通过 `UResourceInventoryComponent` 操作，避免 RuntimeOnly 资源形态。  
  - 两者都实现 `IInteractableTarget`，可被 `UShootGA_Interact` 或自动 Overlap 触发，权威逻辑全部在服务器执行。

### 交互/武器能力原则（Phase2-B）
- GA 只认 `UShootRangedWeaponInstance` / `UShootWeaponInstance`：  
  - `UShootGameplayAbility_Weapon_Fire`、`UShootGameplayAbility_Reload*`、`UShootAbilityCost_Ammo` 的 `SourceObject` 均为 WeaponInstance。  
  - 若需要 Legacy Actor（散布恢复、Mesh 操作），在能力内部通过 `WeaponInstance->GetSpawnedActors()` Cast，例如 `Cast<AShootWeaponActor>`，绝不在对外接口暴露 Actor。
- QuickBar 与能力之间的桥接：  
  - `UCombatComponent::GetOrCreateWeaponInstanceFromInventory` 确保槽位绑定 WeaponInstance，`PopulateQuickbarSlotData`/HUD 读取数据时优先走 WeaponInstance，`AShootWeaponActor` 仅作为表现兜底。  
  - WeaponInstance ←→ Legacy Actor 的关系写成文本架构：
    ```
    PlayerState.InventoryManager --FGuid--> UCombatComponent.QuickBar
        ↘ UShootWeaponInstance (UObject 逻辑层)
            ↘ [可选] AShootWeaponActor / AShootWeaponActor (Visual shell)
    ```
- HUD/Reticle 约束：UI 只能通过 QuickBar 消息或 `UShootWeaponInstance/UShootRangedWeaponInstance` 获取展示数据；禁止直接依赖 `AShootWeaponActor/AShootWeaponActor` 状态。若确需 Actor（挂特效/定位枪口等），仅在内部通过 `WeaponInstance->GetSpawnedActors()` 做 Cast，不向外暴露 Actor 指针。
- GA_Interact 责任划分：  
  - `UShootGA_Interact` 扫描 IInteractableTarget 并监听 `InputTag.Ability.Interact`。  
  - `UShootGA_Interaction_Collect` 统一处理按键拾取（武器/资源），在服务器调用 `AShootWeaponPickupActor::HandlePickup` 或 `AShootResourcePickup::ProcessPickupFromAbility`。  
  - 只有本地玩家 Pawn 授予 GA_Interact，AI/敌人仅通过 AutoOverlap、脚本或专用 GA 修改数据（避免 WaitInputPress）。  
  - 拾取 Actor 默认为 PressToInteract 时授予 `UShootGA_Interaction_Collect`，AutoOverlap/AI 则继续直接调用组件接口。  
- Legacy Actor 触点（Phase2-C 后）：
  - HUD/Reticle 已移除 Actor 回退，仅绑定 `UShootRangedWeaponInstance`。
  - AnimNotify/个别射击 GA 只在内部通过 `WeaponInstance->GetSpawnedActors()` 获取表现挂点，外部 API 不再暴露 Actor。

### Definition / Instance / Actor 职责（Phase4 定稿）
- InventoryItemDefinition：物品元数据（名称、Icon、品质、价格、默认弹药 TagStack、账号侧展示），仅通过 Fragment（WeaponBasic/Ranged/Projectile 等）提供静态配置。
- WeaponInstance（`UShootWeaponInstance/UShootRangedWeaponInstance`）：运行时逻辑核心，连接 ItemDefinition+Fragments，维护 StatTags 弹药、预测状态、开火/换弹/补给接口，Expose 给 GA/HUD/Reticle/AnimNotify。
- WeaponActor（`AShootWeaponActor`）：纯表现壳，仅在内部通过 SpawnedActors 访问，用于 Mesh/Socket/可见性/FX；不对外暴露公共 API。
- 架构流水线（简图）：
```
ItemDefinition + Fragments(WeaponBasic/Ranged/Projectile/Equippable)
        ↓
ItemInstance + StatTags (Ammo_Magazine/Reserve/耐久等)
        ↓
UShootWeaponInstance / UShootRangedWeaponInstance (GA/HUD/Reticle 入口)
        ↓
AShootWeaponActor (SpawnedActors 视觉壳：Mesh/挂点/FX)
```

### Instance/Actor 命名与 Phase4 完成情况
- “Instance” 在本项目默认指 UObject（`UShootInventoryItemInstance/UShootWeaponInstance/UShootRangedWeaponInstance` 等）；Actor 仅作为视觉壳。
- Phase4 已完成武器 Actor 重命名：`ARangedWeaponInstance` → `AShootWeaponActor`，`AHitscanWeaponInstance` → `AShootWeaponActor`（保留独立命名以承载散布/命中表现），不做 Class Redirect，蓝图需手动重绑。
- Actor 的使用范围被锁定为内部表现：仅允许在 `WeaponInstance->GetSpawnedActors()`、`CombatComponent::RefreshSlotActorFromEquipment` 等内部函数中 Cast/控制可见性或 FX；禁止新增任何对外暴露这些 Actor 的公共 API（含 BlueprintCallable/Assignable）。
- 如后续完全不再需要单独的 Hitscan Actor，可在 Phase5 合并为单一 `AShootWeaponActor`（需重绑蓝图），不影响当前接口。

### Phase3 结果对外暴露规则
- 所有对外接口（GA/HUD/AnimNotify/Projectile/QuickBar）统一使用 `UShootWeaponInstance/UShootRangedWeaponInstance`，禁止新增 Actor 指针参数/返回值。
- 拾取/补给：武器实例的弹药/资源全部通过 StatTags 写入（`Inventory_Ammo_Magazine/Reserve`），ResourcePickup 如需补弹仅调用 `UCombatComponent::GrantAmmoToActiveWeapon`，Runtime 弹药不入存档。
- SaveGame/BP API：保持现有接口不变；新增内部功能仅在 C++ 私有层生效，不改对外签名。

## 与现有系统的衔接
- **QuickBar 继承关系**：当前 `UCombatComponent` 是从 Lyra `ULyraQuickBarComponent` 精简而来（路径：`Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Equipment`）。回归 Lyra 式库存后，QuickBar 继续只负责热键槽与装备切换，真正的 Item 数据与持久状态放在 InventoryManager 侧。负责 Phase 2 的成员在实现前需复核 Lyra QuickBar 的接口，以便最小代价地对齐。  
- **武器实例形态**：Lyra 的 WeaponInstance 是 `UObject`（非 AActor），可见的 SkeletalMesh/特效由单独的 AActor（或 Blueprint Attachment）驱动。我们短期仍保留 `AShootWeaponActor` 作为 AActor，但在长远规划中，可考虑拆分为 `UShootWeaponInstance`（UObject）+ `AShootWeaponActor`（表现）。实现团队需要调研 Lyra 的装备蓝图（`LyraCharacter` 的 EquipmentSet/EquipmentInstances）来决定如何在本项目中生成/附着 Mesh Actor，以及 QuickBar 如何引用这些实例。  
- **武器 GA 影响**：当武器实例/弹药来源迁移到 InventoryManager 后，所有射击与换弹 Gameplay Ability（`ShootGA_Weapon_Fire_*`、`ShootGA_Reload_*`）都必须改用新的 SourceObject/Cost 接口，确保客户端预测继续依赖 PredictionKey，服务器扣减来自 Inventory TagStack。Phase 2+ 的开发者在实装前需列出受影响的 GA 清单与改造计划。
- **制作系统**：Hub UI 根据 ResourceInventory 和 InventoryManager 的 TagStack 判断条件，执行制作时调用资源仓库扣减，产出物品通过 InventoryManager 添加。制作结果（武器/服装）以 ItemDefinition 形式添加，再驱动 `UCombatComponent` 或外观系统。  
- **掉落/奖励**：战斗物品拾取调用 InventoryManager 的 `AddItemDefinition/AddItemInstance`；材料/金币掉落调用 ResourceInventory Add/Consume（自动堆叠）。  
- **预测与回滚**：高价值操作通过 GAS Ability 或 GameplayMessage + PredictionKey（开火/消耗），材料拾取默认权威同步即可（自动拾取 + UI 提示可在客户端播放）。  
- **物品生命周期**：账号武器/装备/可持久消耗品标记为 Persistent（参与 SaveGame、Hub 配置 QuickBar）；副本内临时拾取武器标记为 RuntimeOnly（仅本局有效，不写存档、不改账号 QuickBar 配置）。QuickBar/Equipment 只引用 InventoryManager 中的实例，Persistent 由存档加载，RuntimeOnly 由副本拾取产生。

## 迁移阶段
1. **Phase 0（基础设施）**  
   - 移植 `GameplayTagStack`、`InventoryItemInstance`、`InventoryManagerComponent`、`InventoryList`。  
   - 在 `ShootCharacter` 中创建组件实例，确认复制、子对象复制与保存工作。  
2. **Phase 1（材料/货币）**  
   - 定义材料/货币 ItemDef + GameplayTag；实现拾取、副本奖励接口；Tab UI 读取数据。  
   - 替换 GameDesign 需求中的材料提示（UI + 消息）。  
3. **Phase 2（武器/装备接入）**  
   - 为武器创建 ItemDef + Fragment（WeaponBasicConfig/RangedConfig/ProjectileConfig 等），InventoryManager 生成 WeaponInstance 并交给 CombatComponent。  
   - 弹药 Reserve 迁移到 TagStack，以支持商店/补给。  
4. **Phase 3（制作/消耗品）**  
   - 搭建 Hub 制作界面、NPC 对话接口；根据 TagStack 校验条件并消耗资源。  
   - 支持服装/消耗品/任务物品的 ItemDef/Fragment。  
5. **Phase 4（持久化与高级特性）**  
   - 与存档/章节系统对接，处理“已拾取物品不重置”。  
   - 扩展到多人同步、共享仓库、任务追踪等。

## 影响范围
- 新增目录：`Source/NewWorldOrder/Public|Private/Inventory/`、`.../Items/`、`.../UI/Inventory`.  
- 修改类：`ShootCharacter`（添加组件）、`UCombatComponent`（与背包协调）、`AShootWeaponActor`（获取弹药来源）、拾取/奖励相关 Actor、UI Widget、Hub NPC 交互、GAS Costs。  
- 文档：GameDesign 任务包、Engineering Notes（库存、TagStack、Ability Cost）、QuickReference Checklist（新增“InventoryManager 预测/复制检查”）。*** End Patch
