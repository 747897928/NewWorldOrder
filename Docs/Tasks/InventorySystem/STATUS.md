---
task_id: InventorySystem
status: in_progress
assigned_to: Codex
progress: 85%
started: 2025-11-14
last_updated: 2026-01-30
---

# 任务状态

当前状态：in_progress
负责人：Codex（已接替）
进度：85%

当前实现摘要（以 Source/NewWorldOrder 为准）
- PlayerState 持有 InventoryManager 与 ResourceInventory
- InventoryManager：FastArray + SubObject 复制，提供 Persistent/RuntimeOnly 入口
- QuickBar 在 UCombatComponent（Pawn），槽位存 ItemInstanceId 与 Lifetime，装备通过 EquipmentManager
- WeaponInstance 为逻辑层，WeaponActor 仅作表现壳（SpawnedActors 内部使用）
- SaveGameSubsystem 保存 Persistent 物品/资源与 QuickBar，RuntimeOnly 清理
- 2026-06-13：衣柜第一版已接入 InventoryManager
  - 服装通过 `UShootInventoryFragment_WardrobeItem` 表达为 Persistent 有身份物品
  - 衣柜页面通过 `UShootWardrobeViewModel` 从 PlayerState 的 `UShootInventoryManagerComponent` 读取服装 ItemInstance（旧 `UShootWardrobeWidgetBase` 已随 MVVM 迁移删除）
  - `AShootInventoryGrantActor` 可在场景中发放服装、武器、资源，用于玩家路径调试
  - `AShootResourcePickup` 与 `AShootInventoryGrantActor` 只有在物品带 `UShootInventoryFragment_EquippableItem` 时才自动写入 QuickBar，避免服装误占用战斗槽
- 2026-06-13：根据 HomeMap 玩家验收反馈，补齐调试拾取与存档链路
  - `AShootInventoryGrantActor` 授予资源或 Persistent 物品成功后，会捕获 PlayerState 库存状态并异步保存当前槽位
  - RuntimeOnly 调试物不会触发账号存档写入，避免副本临时武器污染 Persistent SaveGame
  - `USaveGameSubsystem` 的 PIE 特殊处理改为仅在 `_PIE` 槽位不存在时新建存档，避免新 PIE 会话覆盖上一轮已获得服装
  - PressToInteract 已按 Lyra 收敛：`UShootGA_Interact` 常驻扫描，具体拾取能力由目标动态授予，不再保留近距离 Overlap 兼容兜底
  - HomeMap 的衣柜全部获取/全部清除调试 Actor 已由用户确认单按 E 后立即生效，当前 PressToInteract 授予链已通过实际输入验证

已知缺口（仍未完成）
- QuickBar 槽位类型校验需要资产侧配置 `UShootInventoryFragment_QuickbarSlotRules`（目前未配置时默认放行）
- 资源/物品堆栈上限与唯一性依赖 `UShootInventoryFragment_StackRules`，需补齐物品 DataAsset 配置
- 安全区判断仍保留 MapName 兜底（已优先读取 WorldSettings Tags）

下一步
- 资产侧补齐 `StackRules/QuickbarSlotRules` 配置并验证拾取/制作流程
- 用 GameState/安全区 Volume 替换 MapName 兜底判断

证据路径
- `Source/NewWorldOrder/Public/Inventory/ShootInventoryManagerComponent.h`
- `Source/NewWorldOrder/Private/Inventory/ShootInventoryManagerComponent.cpp`
- `Source/NewWorldOrder/Public/Inventory/ResourceInventoryComponent.h`
- `Source/NewWorldOrder/Private/Inventory/ResourceInventoryComponent.cpp`
- `Source/NewWorldOrder/Public/Character/CombatComponent.h`
- `Source/NewWorldOrder/Private/Character/CombatComponent.cpp`
- `Source/NewWorldOrder/Public/System/ShootSaveGame.h`
- `Source/NewWorldOrder/Private/System/SaveGameSubsystem.cpp`
