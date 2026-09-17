# Inventory Quickbar Phase1 记录

## 背景
- Phase1 目标要求 Quickbar/HUD UI 改由 GameplayMessage 驱动，并且在 ItemInstance 尚未复制完成时也能渲染基础信息。
- HUD 组件需要提供标准化入口，方便设计师设置 Quickbar/Resource 面板蓝图而不用修改 C++。

## 本次变更
- CombatComponent.PopulateQuickbarSlotData 现会在缺失 ItemInstance 时使用 WeaponDefinition 的 DisplayName/Icon 进行兜底，避免加载中出现空槽。
- 当 ItemInstance 到达客户端后，会改用缓存的 DisplayName/Icon 并填充 ItemDefinition Class；Quickbar UI 可以直接使用 FQuickbarSlotData，无需再解析 WeaponActor。
- UShootHUDQuickbarComponent 继续默认挂载在 AShootPlayerController，只在本地控制器上创建 Widget。设计师只需在蓝图设定 QuickbarWidgetClass 即可。
- 新增文档说明 UShootResourceListWidgetBase 的使用方式（Activate → RefreshResourceEntries → 监听 ResourceChanged）。

## 待办
- Phase2 迁移 WeaponInstance 到 UObject 后，需要让 PopulateQuickbarSlotData 直接读取 Instance 中的 PredictedAmmo，减少对 ARangedWeaponInstance 的依赖。
- 等 UI 资产准备完毕后，将 QuickbarWidgetClass/ResourceListWidgetClass 默认指向实际蓝图，省去每个关卡单独配置的麻烦。

## Phase2 启动记录（2025-11-18）
- CombatComponent 的槽位现额外缓存 `UShootWeaponInstance`（非复制，仅本地），并提供 `GetActiveWeaponInstance()`，供 GA/调试快速读取 Lyra WeaponInstance。
- PopulateQuickbarSlotData 优先使用 `UShootRangedWeaponInstance` 的弹药与 Definition 数据，再回退到旧 `ARangedWeaponInstance`；UI 侧效果是 ItemInstance 尚未复制时依旧能看到武器信息。
- 清理流程（Unequip/RemoveSlot）会同步清空 `WeaponInstance` 缓存，避免指向废弃对象；ItemInstance→EquipmentInstance→WeaponInstance 的引用链得到保持。
- 为解决 ammo cost 找不到 Instigator 的问题，`UShootEquipmentManagerComponent::EquipItem`/`FShootEquipmentList::AddEntry` 新增 Instigator 参数并在创建实例时调用 `SetInstigator`。CombatComponent、InventoryFragment 等不再需要手动赋值，能力成本可直接通过 `EquipmentInstance->GetInstigator()` 读取 StatTags。
- 2025-11-18：开火/换弹 GA 及 `UShootAbilityCost_Ammo` 改为使用 `UShootRangedWeaponInstance` 作为 SourceObject。WeaponInstance 提供 BaseDamage/ProjectileConfig/预测换弹等接口，并且暴露 `GetSpawnedRangedActor()` 供需要 Actor 的系统（投射物、GameplayCue）自取；Ability 代码不再直接 Cast 到 `ARangedWeaponInstance`。
- 2025-11-18：HUD 准星组件与 Widget 优先读取 `UShootRangedWeaponInstance`（从 CombatComponent 的 `GetActiveWeaponInstance()` 初始化），未迁移完的武器仍会 fallback 到旧 Actor，确保 Phase2 迁移过程中准星/UI 不会缺失。
- 2025-11-18：新增 `AShootWeaponPickupActor` + `UShootGA_Interaction_Collect`，RuntimeOnly 武器拾取/丢弃走 InventoryManager → CombatComponent 流水，拾取 Actor 保存 StatTag 快照。CombatComponent 释放 RuntimeOnly 槽位时生成拾取 Actor，副本结束通过 `bDisableRuntimeDropSpawn` 抑制刷地。AnimNotify `UShootAnimNotify_SetWeaponVisibility` 联动 `UShootWeaponInstance::HandleVisualAnimCue`，允许 GA/动画在按需节点 Spawn/Hide 武器壳子。
- 2025-11-18：引入 `UShootInventoryItemDefinition_Weapon`/`UShootEquipmentDefinition_Weapon`，ItemDef 仅承担账号层静态信息（Icon/稀有度/分类）并保留 Legacy WeaponDefinition 指针；EquipmentDefinition 统一驱动 `UShootRangedWeaponInstance`、Equip/Unequip Montage 与 ActorsToSpawn。`UShootEquipmentManagerComponent::AddEntry` 现会在装备阶段自动将 Legacy WeaponDefinition 注入 WeaponInstance，若 Definition 未配置则回退到 ItemDef，确保 GA/Reticle 始终能从 SourceObject 读取武器数值。
