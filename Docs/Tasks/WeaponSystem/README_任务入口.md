# 射击系统任务入口

本任务包负责把项目现有的 Lyra 风格库存、装备、GAS 武器主链收敛为可交付的第三人称合作射击体验。

目标不是复制完整 Lyra ShooterCore。项目采用 Listen Server，支持单人、本地分屏和在线合作；不引入 `WeaponStateComponent`，不实现 Lyra 级命中确认、严格反作弊校验或命中标记预测队列。

# 接手顺序

1. `STATUS.md`
2. `Audit_现状审计.md`
3. `Docs/Engineering/Tech_Constraints.md`
4. `Docs/Tasks/InventorySystem/STATUS.md`
5. 具体小任务的需求、实施和验收文档

# 当前架构结论

- `AShootPlayerState` 持有 ASC、`UShootInventoryManagerComponent` 和 `UResourceInventoryComponent`。
- 当前 `UCombatComponent` 位于 Pawn，负责 QuickBar 切换和装备调度；它是迁移中的兼容层，目标架构见 `QuickbarArchitecture_架构决策.md`。
- `UShootWeaponInstance` / `UShootRangedWeaponInstance` 是武器逻辑对象；`AShootWeaponActor` 只承载网格、插槽和表现。
- 有身份的武器物品、弹药和 Persistent/RuntimeOnly 生命周期由 InventoryManager 与 ItemInstance 管理。
- 武器能力由 Equipment 授予，`AbilitySpec.SourceObject` 必须是当前 WeaponInstance。
- 武器 HUD 是目标 LocalPlayer 私有 UI，必须走对应 `UPrimaryGameLayout` 的 CommonUI 层级，不可用全局 `AddToViewport`。

# 不做事项

- 不把现有武器逻辑改回 Actor 实例。
- 不把库存从 PlayerState 移到 Character。
- 不引入 Lyra `WeaponStateComponent`。
- 不以“重构”为由改动衣柜、角色切换、会话或网络主链。
- 不从 C++ 硬编码网格、动画、准星、武器列表或输入设备分支。

# 历史文档处理

- `Docs/DevelopmentNotes/Lyra武器系统与库存集成_架构分析.md` 的 Actor 改 UObject、StatTags、Equipment 思路已经大多落地；其中将“下一步”描述为未来工作的章节不再是实施依据。
- `Docs/DevelopmentNotes/Lyra武器系统_蓝图配置与射击流程详解.md` 可保留为 Lyra 资产/GA 学习参考，但不得覆盖本任务包的 PlayerState 库存、PVE 校验边界与 CommonUI 规则。
- `Docs/Engineering/Notes/Weapons/Hitscan_命中实现.md` 仍写 `AHitscanWeaponInstance` / `ARangedWeaponInstance`，与当前源码不符，列为待更新文档，不得据此新增旧类。

# 首批小任务顺序

1. 武器资产与动画审计
2. QuickBar 主线收敛与无副作用当前武器查询
3. 持枪状态与第三人称基础姿态
4. 拔枪、收枪、开火、换弹蒙太奇与武器 Actor 同步
5. 准星页面迁入 CommonUI 并接入扩散数据
6. 扩散、瞄准、移动和后坐力参数收敛
7. 步枪、手枪、弹匣式霰弹枪的 AbilitySet 与 DataAsset 验收
8. 其余武器资产批量接入和联机/分屏回归

每个小任务必须独立规定资产配置、网络职责、验收地图和本地分屏/在线联机验收项。前一任务验收前不开始下一项的破坏性改动。
