# 实现步骤指南（交付给实现团队前的草案）

## 架构约束补充（2025-11-15）
1. PlayerState 上的 `UResourceInventoryComponent` 只处理可堆叠资源（材料/货币/徽章/设计图），所有接口默认保存到存档；战斗内一次性掉落若不需要持久化，直接通过能力或即时逻辑处理，禁止进入该组件。
2. PlayerState 还必须包含 `UShootInventoryManagerComponent`，用于管理有身份的物品/武器实例；`UCombatComponent`、QuickBar、EquipmentManager 与各类武器 GA 只能与该组件交互，不得绕过创建孤立实例。
3. `UShootInventoryItemInstance` 需要 `EShootItemLifetime` 字段，区分 `Persistent`（账号资产）与 `RuntimeOnly`（副本内临时物品）；`UShootInventoryManagerComponent` 同时提供 `AddPersistentItem` 与 `AddRuntimeItem`，副本拾取必须走 RuntimeOnly。
4. SaveGame 与 QuickBar 配置仅记录 `Persistent` 实例；所有 `RuntimeOnly` 物品在副本结束或返回 Hub 时统一清空，不写存档、不改 QuickBar。
5. 新增拾取 Actor：材料/货币拾取只能调用 `UResourceInventoryComponent::AddResource`，武器拾取只能调用 `UShootInventoryManagerComponent::AddRuntimeItem` 并在成功后触发装备动画。
6. 编写库存/武器相关 C++ 代码时务必加入中文注释，明确“ResourceInventory = 数量型仓库 / InventoryManager = 有身份背包 / QuickBar-EQ 只引用 InventoryManager”，避免未来 AI/同事误解架构。

## Phase 0：移植与骨架
1. 从 `Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/System/GameplayTagStack.*` 复制为 `ShootGameplayTagStack.*`（放在 `Source/NewWorldOrder/Public/Inventory`），保持 API，命名空间替换为项目前缀。
2. 复制 `LyraInventoryItemInstance.*`、`LyraInventoryManagerComponent.*`、`LyraInventoryList.*` → 对应 `ShootInventoryItemInstance/ManagerComponent/InventoryList`，剔除与 Lyra 装备/AbilitySet 的耦合。
3. 将 `UShootInventoryManagerComponent` 挂到 `AShootPlayerState`（账号级），通过 `UShootInventoryFunctionLibrary::GetInventoryManager` 提供统一访问；确认 `ReplicateSubobjects` 调用成功。
4. 为 `UShootInventoryManagerComponent` 添加 GameplayMessage 广播（Add/Remove/Change），消息类型可放在 `Character/QuickbarMessageTypes.h` 同级目录；准备资源仓库组件接口。

## Phase 1：材料/货币 MVP
1. 定义 GameplayTag 常量（`Inventory.Material.MilitaryAlloy` 等），可放入 `ShootGameplayTags.h`。  
2. 创建 DataAsset：`UShootInventoryItemDefinition`（继承 Lyra 版本），每个材料/货币一个 Definition，填入 Icon、描述、默认堆叠、TagStack（示例：Alloy 默认 0，MaxStack 999）。  
3. 扩展 `UShootInventoryItemInstance`：增加 `FText DisplayName`、`FSlateBrush Icon` 缓存供 UI 使用，并实现 `EShootItemLifetime` 字段与初始化接口；在 `OnRep_StatTags` 时发送 UI 更新。  
4. 资源仓库组件（`UResourceInventoryComponent` 已挂 PlayerState）：`AddResource/ConsumeResource/GetCount` + 资源变更消息 Tag；拾取/掉落调用资源仓库并触发 UI 提示；资源数据从 SaveGame 加载，禁止混入副本内临时武器。  
5. 实现 Tab → 背包 → 材料标签 Widget，读取 ResourceInventory/InventoryManager，按 `EItemCategory` 过滤渲染；HUD/制作界面订阅 StackChanged/ResourceChanged 消息更新。
6. 资源系统接口清单（执行前请逐项确认）：
   - **消息**：`Inventory.Resource.Message.Changed`（由 `UResourceInventoryComponent` 广播），UI ViewModel 需要在 BeginPlay 时注册监听，区分 PlayerState（本地玩家）与其他 Actor。
   - **拾取逻辑**：`AShootResourcePickup`（AutoOverlap）直接调用 `UResourceInventoryBlueprintLibrary::AddResource`；交互拾取（PressToInteract）由 `UShootGA_Interaction_Collect` 转交给 Resource/InventoryManager，根据拾取配置写入 Persistent/RuntimeOnly。
   - **蓝图访问**：`UResourceInventoryBlueprintLibrary::GetResourceInventory` 给 BP UI/GC 使用；新增 `GetAllResources` 或 `GetResourcesByCategory` 以便材料面板整批读取（计划添加）。
   - **制作/验证**：
     1. `UCraftingRecipe` DataAsset（武器/服装）列出资源需求。
     2. 新增 `UResourceInventoryComponent::HasEnoughResource(FResourceCost& Cost)` 辅助函数，制作 UI 可实时检查。
     3. 制作按钮 → 服务器 Ability/Action：`HasEnoughResource → ConsumeResource → InventoryManager.AddPersistentItem`，成功后广播 `Msg_Crafting_Completed`。
   - **UI 提示**：HUD 需要一个 Lightweight Widget 来显示 `+10 军用合金`，可直接在 `UResourceInventoryComponent::BroadcastChange()` 内发送另一个 GameplayMessage（`UI.Toast.ResourcePickup`），后续实现。
   - **中文注解**：在 `UResourceInventoryComponent` / BlueprintLibrary / 拾取 Ability 中增加简明中文注释，说明责任边界（Resource=数量型仓库 vs InventoryManager=有身份背包），避免阅读代码时迷失。

### 资源接口落地记录（2025-11-16）
- `UResourceInventoryComponent` 新增 `GetAllResources` 与 `HasEnoughResource`，供 UI/制作检查批量数据；`UResourceInventoryBlueprintLibrary` 提供配套 Blueprint 入口。
- HUD Toast Tag `UI.Toast.ResourcePickup` 已注册；`BroadcastChange` 中在资源增加时广播该消息（载荷沿用 `FResourceChangedMessage`），UI 订阅即可显示 `+10` 提示。
- 所有资源组件/蓝图函数加入“ResourceInventory=数量仓库 / InventoryManager=有身份背包”中文注解，文档对齐 AGENTS 约定。
- `AShootPlayerController` 默认挂载 `UShootHUDResourceToastComponent`：只在本地 PlayerController 上运行，监听 `UI.Toast.ResourcePickup` 并转发给 `UShootResourceToastWidgetBase`。设计师可基于该 Widget 的 `HandleResourceToast` 蓝图事件完成 UI 视觉，实现快速堆叠/展示。默认属性暴露 ZOrder、WidgetClass，未配置时会静默忽略消息。
- 材料/货币面板基类 `UShootResourceListWidgetBase` 负责监听 `Inventory.Resource.Message.Changed` 并缓存 `FResourceEntry`；客户端 Tab 或 HUD 只需在蓝图子类中实现 `HandleResourceEntriesUpdated`，通过 `ResourceEntries` 数组渲染内容。打开界面时调用 `ActivateWidget` → `RefreshResourceEntries` 会立即读取 PlayerState 上的 ResourceInventory，保证和拾取/制作消息保持一致。

### CommonUI 使用约定补充（2025-11-16）
- **ULyraActivatableWidget 生命周期**：打开界面时调用 `ActivateWidget()`，不要手动调用 `NativeOnActivated`；关闭界面时统一调用 `DeactivateWidget()`，由引擎触发 `NativeOnDeactivated/BP_OnDeactivated` 并自动回收输入映射与可见性；禁止直接 `RemoveFromParent`。
- **Layer/Stack 管理**：所有可切换主界面（背包、制作、副本准备等）通过 `AsyncAction_PushContentToLayerForPlayer(PC, UI.Layer.GameMenu, WidgetClass)` 打开，Widget 内部在返回按钮/Esc 中调用 `DeactivateWidget()` 关闭；不要手写 Pop/RemoveFromParent。
- **HUD 常驻控件**（准星、资源 Toast、QuickBar、小提示等）沿用 “PlayerController 上的 HUD ActorComponent + ULyraActivatableWidget” 模式：组件负责 CreateWidget/AddToViewport/Activate 与最终 Deactivate + RemoveFromParent，Widget 子类在 `BP_OnActivated/BP_OnDeactivated` 里做动画。
- **交互提示**：交互 GA 扫描交互体后通过 `UGameplayMessageSubsystem` 发送 `UI.Interaction.Prompt`（待建立 Tag），PlayerController 上的 `UShootHUDInteractionPromptComponent` 监听并驱动 `ULyraActivatableWidget` 子类，这样 GA/数据层与 UI 解耦。
- **命名约定**：C++ GA 使用 `UShootGA_*`，蓝图扩展使用 `GA_`/`WGA_`；Widget 基类使用 `UShootXXXWidgetBase`，蓝图资产命名 `W_XXX`。

## Phase 2：武器与 Quickbar 对接
1. `UShootInventoryItemDefinition_Weapon`：账号层“武器类型”定义，仅挂 Fragment（WeaponBasicConfig / RangedWeaponConfig / ProjectileWeaponConfig 等），不再引用 `UWeaponDefinition`。  
2. `UShootEquipmentDefinition_Weapon`：封装 Equip/Unequip Montage、ActorsToSpawn、AbilitySets，默认 `UShootRangedWeaponInstance`，不再注入 Legacy Definition；表现切换继续交给 AnimNotify/WeaponActor，状态与数值由 WeaponInstance+Fragment 提供。  
3. 扩展 `UShootInventoryItemInstance`：当 ItemDef 带有 `WeaponFragment` 时，自动生成/缓存 `UShootWeaponInstance`（UObject 逻辑层）并在装备流程中传给 CombatComponent；`AShootWeaponActor` 仅作为表现 Actor，由 EquipmentDefinition 的 `ActorsToSpawn` 控制。  
4. 修改 `UCombatComponent::AddItemToSlot`：输入改为 `UShootInventoryItemInstance`，内部生成或引用 `UShootWeaponInstance` 并按需 Spawn 武器 Actor。组件只复制 ItemInstance GUID，真正的武器对象由 InventoryManager 负责，防止权威散落；QuickBar 槽保存的引用必须指向 `Persistent` 实例，副本拾取的 `RuntimeOnly` 实例仅能覆盖当前激活槽位且不会写回 SaveGame。  
5. 研究 Lyra 的装备蓝图（`LyraCharacter` → EquipmentSet/EquipmentInstances）以及 `ULyraQuickBarComponent` 行为：Lyra 中 WeaponInstance 为 `UObject`，可见 Mesh 位于独立 AActor/Attachment Blueprint；实现团队需要确认是否完全复用该模式或维持 `AShootWeaponActor` AActor 方案。调研产出写回 STATUS/DesignSpec，避免实现偏离。  
6. 弹药 Reserve：初期仍可保留 `CurrentReserve` 字段；后续改成 InventoryManager + TagStack（例：`Inventory.Ammo.Rifle`）。需要在 `UShootRangedWeaponInstance::ReloadAmmo` 里查询 InventoryManager 并消费 TagStack。  
7. UI：Quickbar 消息改为从 `UShootWeaponInstance`/ItemInstance 读取 Icon/Ammo（实例中缓存预测弹药，或直接访问 WeaponInstance）。  
8. **GA 改造提示**：武器开火/换弹 GA（`ShootGA_Weapon_Fire_*`, `ShootGA_Reload_*`, `ShootGA_Reload_ShotgunPerShell`, 等）必须在本阶段或紧随其后迁移到“Inventory 驱动的 SourceObject + TagStack Cost”。实现团队需：

- 新增 `AShootWeaponPickupActor`：Server 记录 Weapon ItemDefinition + Lifetime + StatTag 快照，支持 AutoOverlap 与 PressToInteract（默认授予 `UShootGA_Interaction_Collect`）。拾取流程：AddRuntimeWeaponItem → CombatComponent.AssignSlotFromInventory → Destroy Actor。  
- Runtime 丢枪：CombatComponent 在释放 RuntimeOnly 槽位时复制 StatTags → `InventoryManager::RemoveItemInstance` → `AShootWeaponPickupActor`（可配置 Lifetime）落地。Persistent 物品仍走原存档流程。  
- 丢枪调用链：`UCombatComponent::DropActiveRuntimeWeapon()`（服务器）→ `RemoveItemFromSlot`（内部处理 InventoryManager/QuickBar）→ `HandleRuntimeWeaponDropVisual` 生成拾取 Actor 并拷贝 StatTags（不再拷贝 WeaponDefinition）；副本结束走 `ClearRuntimeSlots` + `InventoryManager::RemoveRuntimeItems()` 清理所有 RuntimeOnly 实例。  
- 捡枪调用链：`AShootWeaponPickupActor::OnOverlapBegin` → `HandlePickup`（`UShootInventoryManagerComponent::AddRuntimeWeaponItem` + 快照恢复）→ `AutoEquipIfPossible`（`UCombatComponent::GetNextFreeItemSlot` + `AssignSlotFromInventory` + Equip），全流程只影响 RuntimeOnly 数据，不写 SaveGame。  
  - 动画驱动显隐：`UShootWeaponInstance::HandleVisualAnimCue` + `UShootAnimNotify_SetWeaponVisibility` 负责 Spawn/Hide/Destroy 可见壳子，CombatComponent 通过 `HandleWeaponAnimNotify` 转发。默认 OnEquipped/OnUnequipped 也会 Show/Hide，避免遗留动画缺乏 Notify 时看不到武器。  
   - 明确每个 GA 当前如何获取武器与弹药（`SourceObject`=AShootWeaponActor, `UShootAbilityCost_Ammo` 等）。  
   - 设计新的 `SourceObject`（可继续传 AActor，或换成 `UShootInventoryItemInstance`）以及 `ApplyCost` 与 TagStack 的交互。  
   - 确保 PredictionKey 仍在 GA 内使用，客户端预测扣弹改为读 `PredictedAmmo`，服务器通过 TagStack 权威扣减。  
   - 在任务包 STATUS 中记录 GA 改造的里程碑与关联文件。

### QuickBar / ItemLifetime 设计补充（2025-11-16）
1. **QuickBar 数据模型**  
   - `UCombatComponent` 的槽位保存 `UShootInventoryItemInstance` Guid/指针，不再直接引用 `AShootWeaponActor`。  
   - 激活槽 → 通过 InventoryManager 的 `EquipItemInstance` 调用 EquipmentManager（现存 `UShootEquipmentInstance`/`UShootEquipmentManagerComponent`）生成/附加 Weapon Actor。  
   - UI/输入层仅负责改变槽位索引，通过 GameplayMessage 广播 `Msg_Quickbar_SlotsChanged / ActiveIndexChanged`。  
2. **ItemLifetime 与接口**  
   - `UShootInventoryItemInstance` 增补 `EShootItemLifetime ItemLifetime` 字段；默认 Persistent。  
   - InventoryManager 对外提供：  
     - `AddPersistentItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, const FInventoryInitData& InitData)`  
     - `AddRuntimeItem(...)`（副本拾取/临时活动）。  
     - `RemoveRuntimeItems()`（副本结束/死亡清理）。  
   - SaveGame 仅序列化 Persistent 实例 + QuickBar 绑定；RuntimeOnly 通过 Session 入口清空。  
3. **装备/拆卸流程**  
   - Equippable/Weapon Fragments 负责在 `OnInstanceCreated` 中写入初始弹药/GrantedAbilities/自定义 StatTags，完全脱离 WeaponDefinition。  
   - 装备时：InventoryManager 按槽位取 Instance → 装入 EquipmentManager → EquipmentManager 生成 `AShootWeaponActor`（暂时保留 AActor 路线），并通过 `UShootEquipmentInstance::RegisterReplicationFragments` 完成复制。  
   - 运行时 QuickBar 替换 Persistent 槽：仅更新引用，不改动 SaveGame；等回到 Hub 时落盘。  
4. **拾取/制作对接**  
   - 拾取/制作数据先走 ResourceInventory（材料/货币）→ 验证成功后调用 InventoryManager 添加 ItemInstance。  
   - 自动拾取武器：默认 AddRuntimeItem 并立即装备到当前槽（覆盖 RuntimeOnly 引用）；Hub/奖励/制作则 AddPersistentItem 并给出 UI 提示。  
5. **武器实例技术债处理计划**  
   - 短期兼容：QuickBar 继续驱动 `AShootWeaponActor` / `AShootWeaponActor`，但数据权威迁移至 `UShootInventoryItemInstance`。  
   - 长期目标：逐步将武器实例转向 UObject 版本（`UShootRangedWeaponInstance`），并让 AActor 只承担表现层。  
   - 已在 `Docs/Engineering/DevelopmentLogs/2025-11/2025-11-16_InventoryTechDebt_技术债梳理.md` 记录拆分策略与里程碑（Weapon GA、UI、QuickBar 同步改造顺序）。  
6. **待办**  
   - `UCombatComponent` 需要新的 API：`AssignSlotFromInventory(FGuid ItemInstanceId)`、`ReleaseRuntimeSlot(int32 SlotIndex)`。  
   - SaveGame 层新增 `FSavedInventoryEntry` 结构，用于序列化 Persistent 实例/槽位绑定。  
   - 需要验证 `UShootGA_Interact` 在拾取 RuntimeOnly 时不会触发 SaveGame 写入（附测试案例）。  

#### QuickBar 槽位→ItemInstance PoC（2025-11-16）
- 数据结构：新增 `FShootQuickbarSlot`，包含 `FGuid ItemInstanceId`、`TWeakObjectPtr<UShootInventoryItemInstance>`、`EShootItemLifetime Lifetime`、`int32 SlotIndex`、`bool bIsHotbarSlot`。`UCombatComponent` 只同步 Guid 与 Lifetime，客户端根据 Guid 在 PlayerState.InventoryManager 中查询实例。
- RuntimeOnly 缓存：`UShootInventoryManagerComponent` 提供 `FindRuntimeItemByGuid`/`RemoveRuntimeItemByGuid`，QuickBar 槽切换或副本结束时走统一清理函数，避免 AActor 遗留。
- 复制策略：QuickBar FastArray 改为复制轻量结构（Guid + 指标）。客户端在 `OnRep_QuickbarSlots` 中查询 InventoryManager（或请求延迟绑定事件）后广播 `Msg_Quickbar_SlotsChanged`，UI 读取 ItemInstance 缓存的 Icon/StatTags。
- 装备流：`SetActiveSlotIndex` 只负责更新 Guid → 通过 InventoryManager 调用 `EquipInventoryItem(FGuid ItemInstanceId)`，由 EquipmentManager 生成/附着 Weapon Actor，并返回 `AShootWeaponActor` 给 CombatComponent 做旧接口兼容。
- TODO：实现 Guid ↔ Instance 的查找表（InventoryManager 内部 `TMap<FGuid, FShootInventoryEntry*>`），QuickBar 只存 Guid；添加预测友好的消息（客户端若尚未拿到实例，先展示 Loading 状态）。
- 2025-11-17 进展：`FQuickbarSlotData`（GameplayMessage）现包含 `ItemInstanceId` 与 `Lifetime`，HUD QuickBar 订阅者可直接识别 Persistent vs RuntimeOnly；`UCombatComponent` 的 `AddItemToSlot` 已接受 `UShootInventoryItemInstance`，若物品带 Equippable Fragment 则通过 `UShootEquipmentManagerComponent` 装备并缓存 `UShootEquipmentInstance`，未命中时回退到旧 `AShootWeaponActor` 流程。为了便于 Hub/存档直接指定槽位，新增 `AssignSlotFromInventory(SlotIndex, ItemInstanceGuid)`、`ReleaseSlot(SlotIndex)`、`BuildQuickbarSaveData/ApplyQuickbarSaveData` 与 `ClearRuntimeSlots` API，使 SaveGame 只需记录 Guid 即可恢复；下一步将装备/卸载流程完全迁至 EquipmentManager 并让 SaveGame 记录 `FGuid` 绑定。
- 2025-11-17 默认：`UCombatComponent::OnRep_Slots` 若暂未生成 WeaponActor，会直接使用 InventoryManager 查到的 `UShootInventoryItemInstance`，读取 `Inventory.Ammo.Magazine/Reserve` StatTag 更新 UI，保证 Hub 载入或 Actor 尚未复制时 QuickBar 仍能展示弹药信息。
- 2026-07-27 Quickbar UI：`UShootQuickbarWidgetBase` 监听 `Msg_Quickbar_SlotsChanged / Msg_Quickbar_ActiveIndexChanged / Msg_Weapon_AmmoChanged`，初始快照只从 Owning PlayerController 的 `UShootQuickBarComponent` 读取。`/Game/UI/Weapon/WBP_QuickBar` 由当前 GameMode 选择的 Experience 以 LocalPlayer Context 注入 `HUD.Slot.Quickbar`；旧 PlayerController 直挂 `UShootHUDQuickbarComponent` 已删除，禁止恢复这条绕过 CommonUI 和分屏生命周期的路径。
- Quickbar 槽位在 `UCombatComponent::PopulateQuickbarSlotData` 中直接使用 ItemDefinition 的 WeaponBasicConfig 提供 Icon/Name/Reticle，ItemInstance 到达后补足弹药/Fragment 字段，不再存在 WeaponDefinition 兜底路径。
- Quickbar HUD 组件默认挂在 `AShootPlayerController`，仅本地控制器生效；设计师在蓝图设置 QuickbarWidgetClass 即可获得消息驱动的 UI。若 Tab/Inventory 需要刷新 Quickbar，同样通过 `UCombatComponent::GetQuickbarSlotsData` 获取完整快照，避免同步延迟。
- Phase2 启动：QuickBar 槽位新增 `UShootWeaponInstance` 缓存，仅本地保存不复制；`UCombatComponent::GetActiveWeaponInstance()` 供武器 GA / HUD 读取新版 Lyra WeaponInstance，`PopulateQuickbarSlotData` 优先从 UObject 读取弹药与展示数据，旧的 `AShootWeaponActor` 分支作为兼容留存。
- HUD/准星链路已经切换到 `UShootRangedWeaponInstance`：`UShootHUDReticleComponent` 只从 `GetActiveWeaponInstance()` 初始化准星 Widget，缺失 WeaponInstance 即销毁准星，等待新的 WeaponInstance（不再回退到 Actor）。
- 武器开火/换弹 GA 与 `UShootAbilityCost_Ammo` 现统一使用 `UShootRangedWeaponInstance` 作为 `AbilitySpec.SourceObject`：Ability 逻辑通过 WeaponInstance 提供的 API（弹药、伤害、投射物配置）访问数据，如需实际 Actor（创建投射物/播放 Cue）则调用 `GetSpawnedRangedActor()`。客户端预测 `Consume/Reload` 也迁至 WeaponInstance 内部封装。
- `UShootEquipmentManagerComponent::EquipItem` 现在接受 `UObject* Instigator`（通常为 `UShootInventoryItemInstance`），在装备瞬间会自动调用 `UShootEquipmentInstance::SetInstigator`。这意味着所有武器 GA/AbilityCost 都可以通过 `EquipmentInstance->GetInstigator()` 直接找到背包物品的 StatTags，不再需要外层手动赋值。
- Phase2-A 编译修复（2025-11-18）：`UShootGameplayAbility_Weapon_Fire::TraceBulletsInCartridge` 声明与实现统一为 `UShootRangedWeaponInstance*`，调用方仅传入 UObject WeaponInstance，内部若需要散弹 Actor 逻辑可通过 WeaponInstance 查找 Spawned Actor 再行 Cast；`AShootWeaponPickupActor` 明确包含 `Inventory/ShootInventoryItemInstance.h` 并在 C++ 内部通过 PlayerState/Controller 解析 InventoryManager；`AShootResourcePickup::ResolveInventoryManager` 直接通过 `AShootPlayerState->GetInventoryManagerComponent()` 获取背包（去掉失效的 `ShootInventoryFunctionLibrary` include）；`UCombatComponent::ClearRuntimeSlots` 改用手动恢复 `bDisableRuntimeDropSpawn`，不再依赖缺失的 `Misc/GuardValue.h`。

## Phase2-A Runtime 捡枪/丢枪验证步骤

- 入口函数
  - `UCombatComponent::DropActiveRuntimeWeapon`（仅服务器，RuntimeOnly 槽位）。
  - `UCombatComponent::RemoveItemFromSlot` → `HandleRuntimeWeaponDropVisual`（复制 StatTags 并生成 `AShootWeaponPickupActor`）。
  - `AShootWeaponPickupActor::HandlePickup` / `AutoEquipIfPossible`（服务器：创建 Runtime ItemInstance + 填充 QuickBar 空槽）。
- `UCombatComponent::AssignSlotFromInventory`（服务器：由拾取或存档恢复写入 QuickBar，日志包含 `[Server]` 标识）。
- 期望数据流

### TacticalOverload & Ammo Cost（当前实现）
- 弹药扣减权威路径：开火 GA → UShootAbilityCost_AmmoTagStack → UShootAbilityCost_ItemTagStack → UShootInventoryItemInstance.StatTags（Tag=Inventory_Ammo_Magazine，数量=GetAmmoPerShot）。
- AmmoTagStack 行为：CheckCost 按 StatTags 堆栈判断是否足够，ApplyCost 仅服务器扣堆栈，客户端不改数值（等待复制）。
- TacticalOverload：`UShootGA_Male_TacticalOverload` 应用有限时长的
  `UShootEffect_TacticalOverloadState`；该 GE 负责授予/撤销 `Status.Overload`，GA 不再单独维护 Loose GameplayTag。
- Overload 与弹药：ASC 拥有 `Status.Overload` 时，AmmoTagStack 的 CheckCost/ApplyCost 跳过弹药检查与扣减，等价于有限时长的无限弹药；
  GA 按等级把 GE Duration 覆盖为 8/10/12 秒，并在结束或取消时按 ActiveGE Handle 清理。
- UI 与表现读取同一生命周期：战斗 HUD 从 ActiveGE 的 Remaining/Duration 显示剩余时间，Looping GameplayCue 随 GA Add/Remove，
  不维护第二套状态 Tag 或 UI Timer。移速/射速/换弹倍率仍应由正式 Buff GE Modifier 配置。
- 所有标准开火 GA（Projectile/Rifle/Sniper/Shotgun/GrenadeLauncher/RocketLauncher）均挂载 UShootAbilityCost_AmmoTagStack 作为唯一弹药成本；旧 UShootAbilityCost_Ammo 已全面废弃。
- Inventory Snapshot：当前加载时未写回 ItemInstanceId（旧 SetItemInstanceId API 已移除），ID 管理由 InventoryManager/SaveSystem 统一，待新接口确定后再补写。
  1. 当前激活槽位（RuntimeOnly）调用 `DropActiveRuntimeWeapon` → `RemoveItemFromSlot` 将实例从 QuickBar 和 InventoryManager 中移除，同时复制 StatTags。
  2. 服务器根据复制的 Definition + StatTags 生成 `AShootWeaponPickupActor`，日志 `LogCombatComponent` 会打印 `Spawned runtime weapon pickup`.
  3. 拾取 Actor 仅在服务器执行 `HandlePickup`，通过 `InventoryManager->AddRuntimeWeaponItem` 创建子对象，并带着 StatTags 进 QuickBar。
  4. `AutoEquipIfPossible` 寻找空槽，调用 `AssignSlotFromInventory` → `AddItemToSlot`，`LogWeaponPickup`/`LogCombatComponent` 分别记录槽位与 Authority。
  5. 快捷栏消息（`OnRep_Slots`）驱动 HUD/QuickBar 更新；RuntimeOnly 槽位在 `ClearRuntimeSlots` 或 `RemoveRuntimeItems` 时被清空且不会写入 SaveGame。
- 手工测试：Hub 持久武器 → Runtime 丢枪
  1. 在 Hub 关卡启动 PIE，确认 PlayerState 已通过存档加载 Persistent 武器（`LogCombatComponent` 会打印 `Assigned ItemInstance ... (Lifetime=Persistent)`）。
  2. 在场景中放置或命令行生成一个 `AShootWeaponPickupActor`（ItemLifetime=RuntimeOnly，引用同一 ItemDefinition），拾取后 QuickBar 会出现新的 Runtime 槽位。
  3. 使用调试按键或控制台触发 `DropActiveRuntimeWeapon`，观察日志：
     - 服务器输出 `[Server] Dropping runtime weapon...` 与 `Spawned runtime weapon pickup ...`
     - 客户端仅看到拾取 Actor 的可见结果，不会生成拾取。
  4. 再次拾取地面武器，检查 `[Server] Assigned runtime weapon to slot X` 以及 HUD 槽位刷新。
- 手工测试：副本 RuntimeOnly 不写存档
  1. 在副本地图中拾取 RuntimeOnly 武器（同上）。
  2. 触发副本结束逻辑（调用 `UCombatComponent::ClearRuntimeSlots` 或通过 `USaveGameSubsystem::CapturePlayerInventoryState`），日志应包含 `RemoveRuntimeItems` 且没有 Persistent 槽位被写入。
  3. 切回 Hub 或重新加载存档后，QuickBar 只包含 Persistent 槽位，确认 `SavedInventoryTypes` 中未记录 RuntimeOnly InstId。
- 手工测试：Listen Server 权威
  1. PIE 设置为 Listen Server + 1 个客户端。
  2. 在服务器端捡枪/丢枪，确认所有 `[Server]` 日志均只出现一次，而客户端仅看到 `[Client]`（来自 `LogCombatComponent` 或 `LogWeaponPickup`）的输入/表现日志。
  3. 在客户端尝试调用 `DropActiveRuntimeWeapon`，服务器会拒绝（日志 `DropActiveRuntimeWeapon failed: component is not authoritative.`），拾取 Actor 也只在服务器上执行 `HandlePickup`.
  4. 若 Slot 被服务器清理（例如离开副本），客户端通过 `OnRep_Slots` 自动同步，不需要额外指令。

## Phase5 架构与测试（当前）
- 架构：ItemDefinition+Fragments → ItemInstance+StatTags → WeaponInstance（GA/HUD/Reticle）→ WeaponActor 仅表现壳。
- 弹药/成本：全部走 StatTags `Inventory_Ammo_Magazine/Reserve`，`UShootRangedWeaponInstance` 负责扣弹/补给；ResourcePickup 弹药模式直接写当前武器 StatTags。
- 交互/拾取：`AShootWeaponPickupActor`（AutoOverlap/PressToInteract）→ InventoryManager.AddRuntimeWeaponItem → `CombatComponent.AssignSlotFromInventory` → Equip；丢弃走 `DropActiveRuntimeWeapon`。
- HUD/Reticle：Quickbar 消息 + WeaponBasicConfig.ReticleWidgetClass 驱动；不再读取 Actor/WeaponDefinition。
- 资源/HUD：ResourcePickup 仅服务器修改 StatTags 或 ResourceInventory；ResourceInventory 广播 `Msg.UI.Toast.ResourcePickup`，`ShootHUDResourceToastComponent` 本地渲染。
- 投射物表现：服务器生成/爆炸并广播 GameplayCue；客户端依赖 Actor 复制与 OnRep_Config 刷新拖尾，无需在 Simulated 端运行 GA。
- 建议测试（单机 + Listen Server）：
  1) Persistent 武器：装备/切槽/开火/换弹，Ammo Tag 变化与 HUD 一致。
  2) Runtime 拾取/丢弃：捡 → 自动占槽 → 丢 → 地面拾取 → 副本结束清空。
  3) 弹药耗尽后 Reload：Reserve=0 不应加弹，Reserve>0 时按夹紧逻辑扣除。
  4) ResourcePickup 弹药模式：`bGrantAmmoToActiveWeapon=true` 时更新当前武器 StatTags，HUD 同步。
  5) Listen Server：上述流程服务器权威，客户端仅表现，日志区分 [Server]/[Client]。

## Phase2-B QuickBar → WeaponInstance → Ability 流程

- 入口能力与组件
  - `UShootGameplayAbility_Weapon_Fire`：对外仅接受 `UShootRangedWeaponInstance*`，射线/投射物/弹药计算完全依赖 UObject；若必须访问 Legacy Actor（如散布恢复），在函数内部通过 `WeaponInstance->GetSpawnedActors()` Cast。
    - 数据链路：QuickBar Slot → `UShootWeaponInstance`（SourceObject）→ `UShootInventoryItemDefinition`/Fragment（弹药配置、散布、ProjectileConfig 等）。Ability 读取 `UShootRangedWeaponInstance` 的 API（`GetBulletsPerCartridge`、`GetProjectileConfig`、`GetBaseDamage` 等）即可获取全部数值，决不直接访问 `AShootWeaponActor` 字段。
  - `UShootGA_Interact`（原 InteractCore）负责扫描/输入监听，`UShootGA_Interaction_Collect` 统一处理按键拾取，所有 `AShootWeaponPickupActor`/`AShootResourcePickup` 的 PressToInteract 只授予这两个 GA。
  - `UCombatComponent` 通过 `GetOrCreateWeaponInstanceFromInventory` 为每个槽位构建 `UShootWeaponInstance`，`ResolveWeaponInstance` 和 `WeaponActor` 仅作为兼容路径。
- 数据流（文本架构图）：
  ```
  PlayerState
  ├─ UShootInventoryManagerComponent (有身份背包：Persistent/RuntimeOnly)
  └─ UResourceInventoryComponent (数量型资源)

  Character
  └─ UCombatComponent (QuickBar 槽位 + Equip 桥)
       ↘ UShootWeaponInstance (UObject 逻辑层，SourceObject)
           ↘ [可选] AShootWeaponActor / AShootWeaponActor (Legacy 表现壳)

  拾取 Actor (Resource / Weapon) --PressToInteract--> UShootGA_Interact / UShootGA_Interaction_Collect --Server--> InventoryManager / ResourceInventory
  ```
- 现存规则（Phase2-C 后）：  
  1. `UShootHUDReticleComponent`/`UShootReticleWidgetBase` 仅接受 `UShootRangedWeaponInstance`；缺失时销毁准星，不再使用 `AShootWeaponActor/AShootWeaponActor` 回退。  
  2. `UShootAnimNotify_SetWeaponVisibility` 只经 `UCombatComponent::HandleWeaponAnimNotify` → `UShootWeaponInstance::HandleVisualAnimCue`，需要可见壳时在内部 `GetSpawnedActors()` Cast，外部不传 Actor。  
  3. CombatComponent 已移除 `EquippedItem` Actor 缓存，装备流程仅依赖 WeaponInstance + SpawnedActors（表现层）。  
  4. GA 子类（`ShootGA_Weapon_Fire_Sniper/Shotgun/Projectile`）仅在内部将 SpawnedActor 作为 GameplayCue SourceObject；数值/射线/伤害全部来自 WeaponInstance/Fragment。  
- HUD/QuickBar 规则：
  - `UShootQuickbarWidgetBase` 监听 `Msg_Quickbar_SlotsChanged`、`Msg_Quickbar_ActiveIndexChanged`、`Msg_Weapon_AmmoChanged` 并直接读取 `FQuickbarSlotData.DisplayName/Icon/Ammo/Reserve/WeaponId` 渲染 UI。
  - `UShootHUDReticleComponent` 监听 QuickBar 消息并仅使用 `UShootRangedWeaponInstance` 初始化准星；WeaponInstance 缺失时销毁准星而不是回退到 Legacy Actor。
- 测试建议（编辑器/PIE）：
  1. 验证 `SetActiveSlotIndex` 后 HUD/GA 获取的 WeaponInstance 与 QuickBar 数据一致（`LogCombatComponent` 会打印 `[Server] Assigned ItemInstance`）。
  2. PressToInteract 场景：同时放置 ResourcePickup 与 WeaponPickup，确认 GA 只在 PlayerControlled Pawn 生效，AI 依旧走 AutoOverlap（符合“AI 不授予交互 GA”的约束）。
  3. 监听 `[Server]/[Client]` 日志，可快速确认权威流程（Drop → Spawn → HandlePickup → AssignSlotFromInventory）。
- 手工验证步骤（Phase2-B）：
  1. **QuickBar/HUD 同步**：Hub 场景中切换槽位并拾取 RuntimeOnly 武器，确认 Quickbar Widget 与 Reticle 均立即刷新 Icon/Ammo（依赖 `Msg_Quickbar_SlotsChanged` 和 `Msg_Quickbar_ActiveIndexChanged`）。  
  2. 弹药广播：持续射击以触发 `FWeaponAmmoChangedMessage`，QuickBar 槽位无需重新打开即可更新 Ammo/Reserve（`UShootQuickbarWidgetBase::HandleWeaponAmmoChanged`）。
  3. **Listen Server 权威**：在 Listen Server + Client 环境下，Server 日志打印 `[Server] Weapon pickup collected ...`，Client 只能看到 UI 刷新；客户端尝试丢枪会被拒绝（`DropActiveRuntimeWeapon failed: component is not authoritative.`）。

## Phase2-C 收尾（HUD/Notify/GA 去 Actor 化）
- AnimNotify 调用链：`UShootAnimNotify_SetWeaponVisibility::Notify` → `UCombatComponent::HandleWeaponAnimNotify` → `UShootWeaponInstance::HandleVisualAnimCue`。AnimNotify 不再直接查找武器 Actor，若表现层需要 Actor（可见性/Attach），在 `HandleVisualAnimCue` 内通过 `WeaponInstance->GetSpawnedActors()` 局部 Cast。
- GA 子类：`ShootGA_Weapon_Fire_Sniper/Shotgun/Projectile` 仅在内部使用 `GetSpawnedRangedActor()` 作为 GameplayCue SourceObject 或特效挂点；所有数值、射线、伤害均由 `UShootRangedWeaponInstance` 提供。
- HUD/Reticle：准星只读取 `UShootRangedWeaponInstance`，缺失实例即销毁准星，不再回退 `AShootWeaponActor`。
- 手工测试（单机/Listen Server）：
  1. 播放包含 `UShootAnimNotify_SetWeaponVisibility` 的蒙太奇，确认 `[Server]/[Client] HandleWeaponAnimNotify` 出现且武器显隐正常，Actor 仅在 WeaponInstance 内部 SpawnedActors 路径可见。  
  2. Sniper/Shotgun/Projectile GA 开火：确认 GameplayCue SourceObject 优先 WeaponInstance，若需要 Actor 仅在内部 Cast；射线/弹药/伤害全部取自 WeaponInstance API。  
  3. QuickBar/HUD：切换槽位、拾取/丢弃 RuntimeOnly 武器，QuickBar/准星通过消息和 WeaponInstance 数据刷新，无 Actor 直连。

- 2025-11-18 补充：`UShootResourceListWidgetBase`（C++ 基类）监听 `Inventory.Resource.Message.Changed`，并通过 `UResourceInventoryBlueprintLibrary::GetAllResources` 缓存 `FResourceEntry` 列表，蓝图只需实现 `HandleResourceEntriesUpdated` 即可刷新 Tab → 材料界面。

## Phase4（已完成）武器 Actor 重命名与职责确认
- 重命名：`ARangedWeaponInstance` → `AShootWeaponActor`，`AHitscanWeaponInstance` → `AShootWeaponActor`（保留独立命名以承载散布/命中表现），无 Class Redirect，蓝图需手动重绑。
- 对外接口保持 UObject：GA/HUD/Reticle/AnimNotify/GameplayCue 统一使用 `UShootWeaponInstance/UShootRangedWeaponInstance`，Actor 仅在内部通过 `WeaponInstance->GetSpawnedActors()` Cast 控制 Mesh/FX/可见性。
- WeaponDefinition 保留为战斗调参 DataAsset；InventoryItemDefinition 仅通过 Fragment 持有对 WeaponDefinition 的引用；WeaponInstance 负责运行时状态与 TagStack 弹药；WeaponActor 纯表现壳，不对外暴露 API。
- Actor 清理：`AShootWeaponActor/AShootWeaponActor` 已移除弹药/授予能力等旧逻辑，仅保留 Mesh 与 Definition，用作挂点/显隐；QuickBar/HUD 不再从 Actor 读取 Ammo。
- 验收建议（单机 + Listen Server）：
  1. QuickBar → WeaponInstance → GA → HUD：SourceObject 仍为 WeaponInstance，切槽/开火/换弹 HUD 与 Reticle 正常。
  2. 捡枪/丢枪/补弹：RuntimeOnly 流程不受 Actor 改名影响，StatTag 弹药更新正确，退出副本无存档残留。
  3. 榴弹/火箭：`UProjectileWeaponDefinition` 配置仍生效，Projectile Spawn 从 WeaponInstance 读取参数，Actor 仅用于挂点（可选）。

## Phase3（完成）弹药/成本与 Actor 清理
- 目标：所有弹药消耗/补给统一走 `UShootInventoryItemInstance` 的 `StatTagStack`（`Inventory_Ammo_Magazine/Reserve`），按需要通过 ResourceInventory 处理副本内临时补给，退出副本不写存档；外部对 Actor 的依赖仅限 SpawnedActors 表现层。
- 成本层实践：
  - 当前实现：`UShootAbilityCost_AmmoTagStack`（继承 ItemTagStack）替代旧 `UShootAbilityCost_Ammo`，在 Commit 时按 `WeaponInstance->GetAmmoPerShot()` 扣 `Inventory_Ammo_Magazine` TagStack；服务器权威扣减，客户端仅预测激活，不再调用 `ConsumeAmmoPredicted`。
  - `UShootRangedWeaponInstance::ReloadAmmo/ConsumeAmmo` 仍用 `Inventory_Ammo_*` StatTags 读写，不操作 Actor 字段；副本拾取弹药应通过 ResourceInventory/InventoryManager 写入 StatTags（TODO：拾取链路落地后验证）。
  - Self-check: 文档已对齐当前代码，未改行为。
- GA 表现依赖收口：
  - `ShootGA_Weapon_Fire_Sniper/Shotgun/Projectile` 的 GameplayCue `SourceObject` 现统一为 `UShootRangedWeaponInstance`；如需表现挂点，Cue/WeaponInstance 内部自行通过 `GetSpawnedActors()` 处理，不再向外暴露 Actor。
  - 投射物生成：`ShootGA_Weapon_Fire_Projectile` 调用 `AShootProjectileBase::InitializeProjectile` 直接传入 WeaponInstance；`AShootProjectileBase` 只缓存 `UShootRangedWeaponInstance`，SpawnedActor 仅作为可选视觉壳。
  - Rifle GA 同步：GameplayCue SourceObject 统一 WeaponInstance，移除 Actor 回退。
- 弹药补给辅助（TagStack）：
  - `UShootRangedWeaponInstance` 新增 `AddReserveAmmo`/`AddMagazineAmmo`（服务器），直接写入 `Inventory_Ammo_Reserve/Magazine` StatTags。
  - `UCombatComponent::GrantAmmoToActiveWeapon`（服务器）封装当前槽位补给，供拾取/副本补给路径调用，日志记录 `[Server] Granted ammo ...`。
  - ResourcePickup 追加弹药补给路径：`bGrantAmmoToActiveWeapon` 为 true 时直接调用 `GrantAmmoToActiveWeapon(MagDelta, ReserveDelta)`，跳过 ResourceInventory，保证副本拾取的临时弹药走 StatTags/RuntimeOnly，不写存档。

### Phase3 测试建议（单机 / Listen Server）
1. 弹药补给拾取（RuntimeOnly）
   - 放置 `AShootResourcePickup`，勾选 `bGrantAmmoToActiveWeapon`，填入 MagazineDelta/ReserveDelta。
   - Server/Client 拾取时观察 `[Server] Granted ammo ...` 日志；QuickBar/HUD 弹药立即刷新；退出副本或调用 `ClearRuntimeSlots` 后不写存档。
2. 普通资源拾取
   - 关闭 `bGrantAmmoToActiveWeapon`，拾取走 `ResourceInventoryComponent::AddResource`，HUD Toast 触发 `UI.Toast.ResourcePickup`。
3. 开火/换弹 GA
   - 单机 + Listen Server：开火触发 `Msg_Weapon_AmmoChanged`，补给后弹药回升；Sniper/Shotgun/Rifle/Projectile GA 的 GameplayCue SourceObject 为 WeaponInstance。
4. Actor 触点检查
   - 播放包含 `UShootAnimNotify_SetWeaponVisibility` 的蒙太奇，确认显隐由 `UShootWeaponInstance::HandleVisualAnimCue` 处理，SpawnedActors 仅内部使用。

- Actor 清理规则（本阶段执行）：
  - AnimNotify/GA/HUD 不暴露 `AShootWeaponActor/AShootWeaponActor`，仅在内部通过 `WeaponInstance->GetSpawnedActors()` 取得表现挂点。
  - 新增表现需求统一走 SpawnedActors，禁止新增对外 Actor API。
- 测试步骤（建议）：
  1. 单机：多次开火，观察 `Msg_Weapon_AmmoChanged` 与 QuickBar/HUD 弹药同步；在无 Actor Spawn 时仍能正常扣弹（说明走 StatTags）。
  2. Listen Server：服务器/客户端各自开火，确认服务器权威扣弹、客户端预测回滚一致；Lifecycle 日志区分 `[Server]/[Client]`。
  3. 副本内拾取弹药（RuntimeOnly）：通过 ResourceInventory/InventoryManager 增加 TagStack，退出副本后弹药不进入存档（TODO：拾取流程落地后补充验证）。

### SaveGame 接口落地（2025-11-17）
- 新增 `FSavedInventoryItem` / `FSavedQuickbarSlot`（`Inventory/SavedInventoryTypes.h`），`UShootSaveGame` 保存 Persistent Item 与 QuickBar Guid。
- `UShootInventoryManagerComponent::BuildPersistentItemsSaveData / ApplyPersistentItemsSaveData` 负责从运行时 Inventory 导出/导入持久化物品（StatTags→FSavedTagStack，保留 ItemInstanceId）。
- `UCombatComponent::BuildQuickbarSaveData / ApplyQuickbarSaveData / ClearRuntimeSlots` 支持槽位保存与副本退出清理；QuickBar 只需保存 SlotIndex + Guid。
- `USaveGameSubsystem` 暴露 `CapturePlayerInventoryState` / `RestorePlayerInventoryState`（参数 `AShootPlayerState*`）：保存时调用 Inventory/Combat 的 Build 接口，加载时 Apply + 清 Runtime 槽位，作为 Hub/关卡切换的统一入口。
- `AShootPlayerState`（仅服务器）在 `BeginPlay/EndPlay` 自动调用上述接口：进入关卡立刻 Restore，离开关卡/销毁前 Capture，确保 PlayerState 上的 Resource/Inventory/QuickBar 与 SaveGame 始终同步。

#### ItemLifetime API 细化
- `EShootItemLifetime`
  - Persistent：账号资产，参与 SaveGame、QuickBar 配置加载。
  - RuntimeOnly：副本内临时物品，只在 Session 存活，离开副本或 Hub 切换时统一清空。
- InventoryManager 新接口
  - `UShootInventoryItemInstance* AddPersistentItem(const FShootInventoryInitData& InitData, TSubclassOf<UShootInventoryItemDefinition> ItemDef);`
  - `UShootInventoryItemInstance* AddRuntimeItem(const FShootInventoryInitData& InitData, TSubclassOf<UShootInventoryItemDefinition> ItemDef, const FShootRuntimeItemContext& Context);`
  - `void RemoveRuntimeItems();`（副本结束/角色销毁时调用）
  - `bool RemoveItemByGuid(FGuid ItemInstanceId);`（QuickBar 释放槽位、制作消耗）
- QuickBar 绑定：Persistent 槽位保存 `FGuid PersistentLoadoutSlotIds[NumSlots]`（SaveGame 用），RuntimeOnly 槽位只在 Session 内生效，消息层透出 `Lifetime` 以供 HUD 提示。
- SaveGame：新增 `FShootSavedInventoryEntry`（ItemInstanceId、ItemDefPath、StatTags、QuickbarSlotIndex、Lifetime=Persistent），在加载 Hub 时重建 InventoryManager + QuickBar，并为 RuntimeOnly 槽位提供空数据（等待副本拾取）。
- 2025-11-16 实装记录：`UShootInventoryItemInstance` 现在包含 `FGuid ItemInstanceId` 与 `EShootItemLifetime`（Iris/Net 复制），`UShootInventoryManagerComponent` 暴露 `AddPersistentItem`、`AddRuntimeItem`、`RemoveRuntimeItems`、`Find/RemoveItemByInstanceId` 等 API，并维护 Guid→Instance 映射；RuntimeOnly 清理可在副本结束时直接调用组件接口，QuickBar 后续只需读取 Guid 即可定位实例。

## Phase 3：制作/消耗品
1. 在 Hub NPC 交互里添加“制作” Ability/UI（可基于 `CommonUI`）。  
2. `UShootCraftingViewModel`（新类）查询 InventoryManager 与设计图/徽章/材料统计，实时生成制作条件状态。  
3. 点击“制作”按钮 → 服务器 Ability/Action：  
   - 校验 ResourceInventory + TagStack（徽章、图纸、材料、金币）。  
   - 调用 ResourceInventory 扣减资源，再调用 InventoryManager 的 `AddItemDefinition` 产出武器或服装（服装后续传给外观系统）。  
   - 播放动画/GameplayCue，并在完成后广播 `Msg_Crafting_Completed`。  
4. 实现消耗品/材料使用流程（例如未来的回血道具）：ItemDef 指定 `GrantedAbilities`，装备/使用时授予 GAS 能力并在 Ability `Commit` 时扣减 TagStack。

## 后续（Phase5+）持久化与扩展
1. 将 Inventory 数据序列化到保存系统（PlayerState 或 SaveGame）；至少包含 ItemDef Id、Stack、StatTags，且只序列化 Lifetime=Persistent 的物品/武器。  
2. 实装“已拾取物品不重置”：在关卡拾取 Actor 中记录 `PickupId`，加入数据库/Save，重复加载时直接隐藏；资源仓库可直存 Count。  
3. 扩展材料掉落倍率接口（副本评级/随机事件调用 Inventory API 即可）。  
4. 结合任务/成就：Inventory变化触发对应 GameplayMessage，由任务系统监听（例：收集5张设计图→触发成就）。  
5. 审视性能：材料/货币可合并到一个“资源” ItemInstance（仅靠 TagStack 表示），减少子对象数量；仅对武器/装备保持独立实例。

## 协作指引
- 可读取 `Docs/ExampleProjectCode/LyraStarterGame`。  
- 指派任务前，确保其先阅读：`AGENTS.md` → `Docs/SessionGuides/Implementation_代码实现指南.md` → 本任务包下 Context/Requirements/DesignSpec/Implementation。  
- 提交期望：实现 Phase 0-1 的代码骨架与 MVP UI，写完更新 STATUS、Checklist，并在 DevelopmentNotes 记录踩坑。  
- 任何新增 GameplayTag/消息：同步更新 `ShootGameplayTags.*` 与 `Docs/Engineering/Notes/GameplayTagInitialization.md`。
