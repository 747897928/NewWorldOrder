# 武器系统现状审计

# 证据范围

审计依据为 2026-07-25 的当前源码、任务包和本机 LyraStarterGame 源码。未读取 `.uasset` 文件；蓝图和 DataAsset 的具体配置必须通过 UE MCP 资产查询另行审计。

# 主线与兼容链路

- 库存：PlayerState InventoryManager + ItemInstance，保留。
- 装备：CombatComponent -> EquipmentManager -> WeaponInstance，保留。
- 武器表现：Equipment SpawnedActors 中的 AShootWeaponActor，保留。
- 能力：Equipment AbilitySet，SourceObject 为 WeaponInstance，保留。
- 弹药：ItemInstance StatTagStack，保留。
- 命中：GAS TargetData 与服务器权威伤害，保留并按 PVE 简化。
- 命中确认：WeaponStateComponent，明确不引入。
- 准星：HUDReticleComponent + ReticleWidget，需要重接 CommonUI。
- 动画：WeaponInstance AnimLayer + ShootAnimInstance，框架已在，状态桥接未完成。

# 发现的问题

1. 动画状态断链

`UShootAnimInstance::NativeUpdateAnimation` 只计算速度、空中、蹲伏、YawOffset 和 Lean。`bWeaponEquipped`、`bAiming`、FABRIK、AimOffset、左右手武器对齐仍是注释掉的旧链路，且没有读取 `UCombatComponent::GetActiveWeaponInstance()`。

影响：装备武器后，即使 `UShootWeaponInstance::OnEquipped` 已调用 AnimLayer，主 AnimBP 仍没有可靠的“空手/持枪/瞄准”状态输入。

2. 武器动画层配置位置不合理

`UShootWeaponInstance::LayersToApply` 位于运行时 Instance 类。项目规则要求美术可调引用由 DataAsset 或蓝图子类配置；同时当前的 `FShootAnimLayerSelectionSet` 只有一份层列表，不能像 Lyra 一样按装备/未装备状态与外观标签选择最佳层。

影响：第一批任务应先决定 AnimLayer 的数据归属和角色骨架兼容策略，不能直接为每把武器硬编码 AnimBP 类。

3. 准星已存在但违反当前 UI 架构

`UShootHUDReticleComponent::InitializeReticleForWeapon` 使用 `CreateWidget` 后 `AddToViewport`，`DestroyActiveReticle` 又直接 `RemoveFromParent`。这绕过了 `UPrimaryGameLayout` 和 CommonUI 层级栈，可能在本地分屏时显示到错误玩家或恢复错误输入状态。

影响：准星工作不是从复制 `W_Reticle_Rifle` 开始，而是先把创建入口迁到目标 LocalPlayer 的 HUD Layer，再以 WeaponBasicConfig 的 `ReticleWidgetClass` 选择具体页面。

4. 扩散已具备最小闭环，但不是完整后坐力

`UShootRangedWeaponInstance` 已有基础散布、每发增加、时间恢复和锥形随机射线。当前倍率恒为 1，未使用移动、蹲伏、空中、瞄准状态；也没有本地相机/控制旋转的后坐力表现。

影响：后续必须将“子弹散布”和“镜头后坐力”拆开：前者服务器权威、由 WeaponInstance 数据驱动；后者仅拥有者本地表现，通过 Input/Camera 或 GameplayCue 实现。不能以 UI 准星缩放代替实际散布，也不能由客户端改变权威命中方向。

5. 旧资料会误导实现

旧命中文档的 `AHitscanWeaponInstance`、`ARangedWeaponInstance`、`UWeaponDefinition` 在当前源码中不存在。两份 DevelopmentNotes 的部分“未来改造”也已经被现有 Inventory/Equipment 实现取代。

影响：保留历史资料，但以本审计和当前代码作为实施入口；下一次专门文档维护任务更新旧 Hitscan 笔记并在历史笔记增加状态说明。

# 与 Lyra ShooterCore 的取舍

应借鉴：Equipment -> WeaponInstance -> AbilitySet，SourceObject，ItemInstance 弹药，AnimLayer，RangedWeaponInstance 的热度/散布模型，角色与武器蒙太奇的同步。

不借鉴：WeaponStateComponent、服务器命中替换、未确认命中队列、严格校验和完整的输入设备属性系统。

# 下一任务需要用户提供或通过 MCP 核对的资产事实

- 男主和女主各自使用的 SkeletalMesh、Skeleton、主 AnimBP 与空手 Locomotion 状态。
- 首批三类武器的 WeaponActor 蓝图、网格骨架及其枪口 socket。
- 每个角色骨架可用的持枪 Idle/移动、拔枪、收枪、瞄准、开火、换弹蒙太奇。
- 哪些武器动画与角色骨架不兼容，是否只有武器自身骨骼动画。
- `AN_PlayWeaponMontage` 的实际类实现与蓝图 Notify 配置，以及期望使用的 Montage Sync Group 名称。

# 首个实施小任务建议

任务名称：持枪状态桥接与空手/持枪基础姿态。

范围：在不修改资产内容的前提下，让 AnimInstance 从当前 Avatar 的 CombatComponent 和本地瞄准状态取得明确的 `bWeaponEquipped`、`bAiming`、武器类别/姿态标签；保证角色切换和装备/卸载后状态刷新。该任务不做 IK、不做后坐力、不做蒙太奇同步，也不修改库存和网络模型。

完成条件：空手、持枪、切枪、角色切换在单人、双本地玩家、Listen Server 客户端三种场景下都只影响目标 Pawn；AnimBP 能以稳定变量驱动状态机。
