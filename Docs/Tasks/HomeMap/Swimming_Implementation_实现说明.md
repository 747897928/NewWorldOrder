# HomeMap 游泳玩法实现说明

## 状态

已完成。正式地图为 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`。

本实现未修改 Lyra、第三方插件或 Unreal Engine 源码。

## 当前实现

### 水体与移动

- 深水区使用原生 `PhysicsVolume`，关卡标签为 `HM_Pool_SwimmingVolume_Deep`。
- 体积范围：X=-425..425，Y=640..1890，Z=-570..-18；`bWaterVolume=true`，`FluidFriction=0.25`，`Priority=10`。2026-09-08 屋顶跳水环境深化将深水池底从 -398 下沉到 -518，水深 500 cm；浅水台阶保持原位，未修改游泳代码。最新环境路线与验证见 RooftopRoute_屋顶观景与高跳台.md。
- Y=640 之前的中央台阶不在水体内，角色继续行走；进入深水后使用引擎原生 `MOVE_Swimming`。
- `UShootCharacterMovementComponent` 通过 `CanEverSwim`、`SetMovementMode` 和 `PhysicsVolumeChanged` 执行 Experience 开关，并保留 CharacterMovement 的网络复制与预测链路。
- 游泳复用现有 `MoveAction` 和 `LookAction`。游泳时前进方向使用完整控制旋转，因此俯仰可以控制上浮和下潜。
- `MaxSwimSpeed`、`Buoyancy` 等属性来自 `UShootCharacterMovementComponent` 的父类 `UCharacterMovementComponent`，调参位置是 `BP_ShootCharacter` 的 Character Movement 组件 Details。

### 高处入水判定

- 不为“本副本有水”增加全局 GameplayTag。水是否存在由关卡中实际放置的 `PhysicsVolume` 决定，当前 Experience 的 `bAllowSwimming` 只负责开放或关闭角色使用游泳移动。
- 坠落角色不需要先预测落点：只要实际轨迹进入 `bWaterVolume=true` 的水体，`CharacterMovement` 就会从 `MOVE_Falling` 切换到 `MOVE_Swimming`；没有水体或没有进入水体就不会触发。这条路径支持二楼、三楼以及更高处的坠落。
- `AShootCharacter` 在 `MOVE_Falling` 中用当前速度、重力和关卡 PhysicsVolume 采样预测落点，并用角色胶囊扫掠排除中途撞到地面或墙体的路径。预测将在短时间内进入水体时，提前播放 `Swimming_Diving_Start`，结束后循环 `Swimming_Diving_Loop`。
- 真正切换到 `MOVE_Swimming` 时复用空中的跳水 Montage，不重复播放 `Swimming_Diving_Start`；如果预测路径失效或先落到普通地面，会立即停止跳水 Montage，交还给正常 Falling/Walking locomotion。
- 这仍然不需要“本副本有水”的全局 GameplayTag：没有可到达的 `bWaterVolume=true` 体积就不会触发，Experience 的 `bAllowSwimming` 继续作为能力总开关。

### Experience、GA 与 GameplayTag

- `UShootExperienceDefinition` 自身定义 `Movement|Swimming` 下的 `bAllowSwimming`，默认 `false`；`DA_Experience_Home` 设置为 `true`，其他当前 Experience 保持关闭。
- 当前没有 Swimming GA，也没有游泳状态 GameplayTag。水中状态的权威来源是 CharacterMovement 的 `MovementMode` 和当前 `PhysicsVolume`。
- Experience 可以通过 AbilitySet 生命周期授予和取回技能 GA，但 Experience 上的每个配置字段都不等于一个 GA；本次 `bAllowSwimming` 只是连续移动能力的门控。
- 目前保留 bool 是有意的：这里只有一个 Experience 开关，语义直接且改动小。
- 以后增加水下攻击、潜水冲刺、氧气、溺水或水中交互时，动作适合做 GA，持续属性和临时状态适合用 GameplayEffect/GameplayTag；可由 Experience 的 AbilitySet 提供这些 GA/GE。
- 如果 GAS 或 UI 需要“正在游泳”的标签，可以用临时 GE 镜像 `MOVE_Swimming`，例如 `State.Movement.Swimming`，但它不能反过来替代 CharacterMovement 的移动真相。只有出现多种可独立开关的能力时，才值得把 `bAllowSwimming` 迁移为 Experience capability tag。

## 动画

角色使用正式男女 AnimBP 的 `FullBody` Slot 动态播放已存在的 MF 游泳序列，不改既有 Locomotion 状态机：

- 入水：`Swimming_Crawl_Fwd_Start`
- 静止：`Swimming_Idle`
- 移动：`Swimming_Crawl_Fwd`
- Falling 预测入水：空中先播放 `Swimming_Diving_Start`，随后循环 `Swimming_Diving_Loop`，入水后再切换到正常游泳动画

这些序列由 `AShootCharacter` 的 `Animation|Swimming` 属性配置在 `BP_ShootCharacter` 中。循环动作使用动态 Montage，状态变化时用短 Blend 切换。

离开 `MOVE_Swimming` 时现在会在同一状态切换中立即停止活动的 FullBody 游泳 Montage，并清理游泳动画状态，不再播放约 3 秒的 `Swimming_Crawl_Fwd_Stop`。这样正式 locomotion 会在角色回到 Walking/Falling 的第一个状态更新中接管，避免角色已经离水却继续保持水平游泳姿势。

## 验证

- 正式 HomeMap PIE：浅台阶为 `MOVE_Walking`，深水为 `MOVE_Swimming`。
- 入水播放起始动作，随后正确切换到 Idle/Forward；动态 Montage 使用 `FullBody`，循环计数为 1000。
- 从 `(0,1090,700)` 的高处坠落时，角色先处于 `MOVE_Falling`，实际穿过泳池水体后切换为 `MOVE_Swimming`；该路径配置使用 `Swimming_Diving_Start` 和 `Swimming_Diving_Loop`。
- 用户 PIE 验收确认：离开高处平台后，角色在尚未接触水面时已进入 `Swimming_Diving_Loop` 的俯冲姿势，并以该姿势入水。
- 从深水向台阶和池外移动后，模式恢复为 `MOVE_Walking`，水体查询为 false，离水后没有残留游泳 Montage。
- 将 Home Experience 的 `bAllowSwimming` 临时设为 false 后进入深水，角色保持 `MOVE_Walking` 且不播放游泳 Montage；测试结束已恢复为 true。
- `Scripts/Build_Windows.ps1` 冷编译结果为 `Succeeded`，编辑器重新就绪。

## 调整入口

- 水体边界：正式地图中的 `HM_Pool_SwimmingVolume_Deep` Brush 变换；上边界保持 Z=-18，不覆盖中央浅台阶。
- 手感：`BP_ShootCharacter` 的 Character Movement 组件，调整父类 `UCharacterMovementComponent` 的游泳属性。
- 动画：`BP_ShootCharacter` 的 `Animation|Swimming` 属性和 `AShootCharacter` 的短 Blend 参数。
