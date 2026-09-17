# 武器拾取表现与配置冗余审计

审计日期：2026-08-26

状态：[父类与 BasicConfig 源代码已完成清理；7 个拾取蓝图已统一为武器本体 StaticMesh；商城歧义资产已完成视觉核对、重命名与引用清理；UE5.8.2 重载、资产编译保存与 CDO 回读已完成；WorldLabel 是否删除待产品选择]

范围：`AShootWeaponPickupActor` 父类 C++、`/Game/Weapons` 下 7 个武器拾取蓝图、`Sniper_Rifle_A` 与 `Shotgun_A` 的实际网格资产，以及 `ID_*` 的 `WeaponBasicConfig` 字段。

## 结论

方案 B 下必须保留父类 `AShootWeaponPickupActor` 创建的 `VisualComponent`。它是父类上的 `UStaticMeshComponent`，应成为所有拾取物唯一的静态网格入口。

这里的“保留”不是沿用旧 AI 的配置习惯，而是由当前类的组件类型决定的：`AShootWeaponPickupActor` 继承自 `AActor`，Actor 本身没有可渲染 Mesh；当前根组件 `CollisionComponent` 的实际类型是 `USphereComponent`，只提供交互范围、Overlap 和 PressToInteract 扫描，也没有 `StaticMesh` 属性。因此删除 `VisualComponent` 后，拾取 Actor 将只剩一个不可渲染的球形碰撞体，不能显示枪模型。

应删除的是各拾取蓝图自己新增的 `SkeletalMesh` 组件，而不是 `VisualComponent`：

```text
AShootWeaponPickupActor
└── Visual（父类 C++ 创建的 UStaticMeshComponent）
    └── StaticMesh = 当前拾取物的低模静态网格
```

当前蓝图里的 `SkeletalMesh` 与父类 `Visual` 不是同一个属性。它们是两个独立的 SceneComponent；两个组件都可见且都设置网格时，UE 会同时提交两个网格进行渲染。当前蓝图没有隐藏其中一个组件的 EventGraph 逻辑。

清理前 7 个本地 `SkeletalMesh` 组件都没有 `AnimClass`，但仍保持 `AnimationMode=AnimationBlueprint` 和 `AlwaysTickPoseAndRefreshBones`。这不是有效的拾取动画功能，反而让每个拾取物携带一条没有消费者的骨骼表现配置；7 个蓝图现在都已删除这条无消费者的表现链。

不能把 `CollisionComponent` 改成 StaticMeshComponent 来“省掉” `VisualComponent`：这会把当前独立的 60cm 球形交互范围替换成网格碰撞，碰撞形状、Overlap profile 和交互可达范围都会随资产改变。除非产品明确要用网格碰撞，否则“球形交互 + 无碰撞 StaticMesh 表现”是当前系统的最小正确组件组合。

标准新武器模板应以 7 把武器的同一规则为准：在父类继承的 `Visual` 组件上配置一个代表武器本体的 `StaticMesh`，不要添加本地 `SkeletalMesh`。此前 `Grenade Launcher`、`Rocket Launcher`、`Sniper Rifle A` 的 `Visual.StaticMesh` 是历史误配置，不是新增武器必须照抄的特殊规则；它们现已改为各自武器本体的 StaticMesh。

## 7 个拾取蓝图回读结果

| 拾取蓝图 | 父类 Visual.StaticMesh | 蓝图本地 SkeletalMesh | 结论 |
| --- | --- | --- | --- |
| `BP_WeaponPickup_Grenade_Launcher_A` | `SM_Grenade_Launcher_A_Pickup_Generated` | 已删除 | 由原 `Grenade_Launcher_A` SkeletalMesh 转换；旧 `GrenadeLauncher_Pickup` 实际是榴弹弹药表现 |
| `BP_WeaponPickup_Pistol` | `SM_Pistol_Pickup_Generated` | 已删除 | 已由原 `SK_Pistol` 转换为同目录 StaticMesh，并配置到父类 Visual |
| `BP_WeaponPickup_Rifle` | `SM_Rifle_Pickup_Generated` | 已删除 | 已由原 `SK_Rifle` 转换为同目录 StaticMesh，并配置到父类 Visual |
| `BP_WeaponPickup_Rocket_Launcher_A` | `SM_Rocket_Launcher_A_Pickup_Generated` | 已删除 | 由原 `Rocket_Launcher_A` SkeletalMesh 转换；旧 `RocketLauncher_Pickup` 实际是火箭弹表现 |
| `BP_WeaponPickup_Shotgun` | `SM_Shotgun_Pickup_Generated` | 已删除 | 已由原 `SKM_Shotgun` 转换为同目录 StaticMesh，并配置到父类 Visual |
| `BP_WeaponPickup_Shotgun_A` | `SM_Shotgun_A_Pickup_Generated` | 已删除 | 由原 `Shotgun_A` SkeletalMesh 转换；旧 `Shotgun_Pickup` 实际是霰弹箱 |
| `BP_WeaponPickup_Sniper_Rifle_A` | `SM_Sniper_Rifle_A_Pickup_Generated` | 已删除 | 由原 `Sniper_Rifle_A` SkeletalMesh 转换；旧 `SniperRifle_Pickup` 是小型拾取/弹药道具表现 |

## 商城资产命名陷阱与视觉核对

不能把资产名中的 `Pickup` 后缀直接解释为“武器拾取模型”。本轮通过 UE5.8.2 静态网格预览、包围盒和材质回读核对了四个历史资产：

| 原资产名 | 视觉语义 | 新名称 |
| --- | --- | --- |
| `GrenadeLauncher_Pickup` | 多枚榴弹组成的弹药表现 | `SM_Grenade_Launcher_A_AmmoPickup` |
| `RocketLauncher_Pickup` | 火箭弹/弹药表现 | `SM_Rocket_Launcher_A_AmmoPickup` |
| `Shotgun_Pickup` | 打开的霰弹箱 | `SM_Shotgun_A_AmmoBox` |
| `SniperRifle_Pickup` | 小型拾取/弹药道具，不是完整狙击枪 | `SM_Sniper_Rifle_A_AmmoPickup` |

这四个旧资产在替换拾取蓝图引用后已无包引用，再通过 UE 编辑器资产重命名完成迁移；旧路径不再存在。对应的 `GrenadeLauncherA_Ammo`、`RocketLauncherA_Ammo`、`ShotgunA_Ammo`、`SniperRifleA_Ammo` 是另一组弹药网格，未因名称相似而误删或合并。

这条规则必须留给后续维护者和 AI：新增拾取视觉时先看资产预览，再核对包围盒和材质；`Pickup` 只是商城资产命名的一部分，不代表它是武器本体，也不能替代 `B_*` Equipment Actor 的装备视觉。

`Pistol`、`Rifle`、旧 `Shotgun` 各自目录中原本没有与当前 SkeletalMesh 配套的静态拾取网格。`/Game/Weapons/Pistols_A` 和 `/Game/Weapons/Assault_Rifle_A` 虽然有名为 `Pistol_Pickup`、`AssaultRifle_Pickup` 的 StaticMesh，但它们属于其他枪族目录，不能未经视觉确认直接填给当前 `Pistol`/`Rifle`。

为避免跨枪族猜测复用，本轮通过 UE 编辑器资产接口把 7 个武器中缺少合适武器本体 StaticMesh 的 7 个原 SkeletalMesh 导出为 FBX，再导入为同目录 StaticMesh，并重新写入原 SkeletalMesh 的材质实例：

- `/Game/Weapons/Pistol/Mesh/SM_Pistol_Pickup_Generated`
- `/Game/Weapons/Rifle/Mesh/SM_Rifle_Pickup_Generated`
- `/Game/Weapons/Shotgun/Mesh/SM_Shotgun_Pickup_Generated`
- `/Game/Weapons/Grenade_Launcher_A/Mesh/SM_Grenade_Launcher_A_Pickup_Generated`
- `/Game/Weapons/Rocket_Launcher_A/Mesh/SM_Rocket_Launcher_A_Pickup_Generated`
- `/Game/Weapons/Shotgun_A/Mesh/SM_Shotgun_A_Pickup_Generated`
- `/Game/Weapons/Sniper_Rifle_A/Mesh/SM_Sniper_Rifle_A_Pickup_Generated`

7 项资产均已回读为 `StaticMesh`，几何 Bounds 与对应原 SkeletalMesh 一致，且材质沿用原武器材质。原 SkeletalMesh 未删除，因为它们仍属于装备后的 Equipment Actor 表现链；本次只替换拾取蓝图的表现组件。

## Sniper_Rifle_A 与 Shotgun_A 的具体问题

### Sniper_Rifle_A

- `Visual.StaticMesh` 现为 `/Game/Weapons/Sniper_Rifle_A/Mesh/SM_Sniper_Rifle_A_Pickup_Generated`，静态网格预览为完整狙击枪。
- 清理前本地 `SkeletalMesh` 设置为 `/Game/Weapons/Sniper_Rifle_A/Mesh/Sniper_Rifle_A`，可见、无 AnimClass、没有任何图表逻辑隐藏它；本轮已删除该组件。
- 原 `SniperRifle_Pickup` 不是完整狙击枪，已重命名为 `SM_Sniper_Rifle_A_AmmoPickup`，不再被拾取蓝图引用。方案 B 只保留父类 `Visual`。
- `WorldLabel` 本轮已校正为 `Sniper Rifle A`；如果产品不需要世界悬浮文字，后续可单独删除该本地组件。

### Shotgun_A

- 本地 `SkeletalMesh` 设置为 `/Game/Weapons/Shotgun_A/Mesh/Shotgun_A`。
- 父类 `Visual` 现配置同目录的 `/Game/Weapons/Shotgun_A/Mesh/SM_Shotgun_A_Pickup_Generated`，预览为完整霰弹枪；原 `Shotgun_Pickup` 已重命名为 `SM_Shotgun_A_AmmoBox`。
- 本轮已删除本地 `SkeletalMesh`，不再把霰弹箱误当成霰弹枪拾取模型。
- `WorldLabel` 为 `Shotgun A`，目前文本正确。

## 蓝图图表冗余

7 个 `BP_WeaponPickup_*` 的 `EventGraph` 原本都只有 3 个未连接事件节点：`BeginPlay`、`ActorBeginOverlap`、`Tick`；节点数为 3，连接数为 0。父类已经在 C++ 的 `BeginPlay` 中绑定 AutoOverlap，并实现拾取和交互逻辑，因此这些空事件节点没有运行时作用；本轮已从 7 个蓝图全部删除。

7 个蓝图都只有默认的 `UserConstructionScript` 空图，没有自定义节点；不需要为了“清理”删除 UE 自动保留的图表本身。

## 拾取交互字段中的复制错误

当前 C++ 已直接读取父类继承的 `WeaponItemDefinition` 对应 ItemDefinition 的 `DisplayName`。7 个 ID 的根部 `DisplayName` 均已正确配置：

- `ID_Grenade_Launcher_A`：`Grenade Launcher A`
- `ID_Rocket_Launcher_A`：`Rocket Launcher A`
- `ID_Sniper_Rifle_A`：`Sniper Rifle A`

7 个拾取蓝图的 `InteractionText` 原本都是重复的固定文本，其中榴弹发射器、火箭发射器、狙击枪还被错误复制成“拾取步枪”。固定文本既重复 ItemDefinition，又会覆盖 C++ 的正确名称；现已清空 7 个蓝图实例的覆盖值，并从武器拾取父类 C++ 删除这个覆盖字段，交互名称统一由 ItemDefinition `DisplayName` 提供。`InteractionSubText` 的 RuntimeOnly 提示仍保留。

`WorldLabel` 是世界中的纯视觉文字，不参与交互选项。它不是 `VisualComponent` 的替代物，也不是拾取逻辑必需项；本轮保留该实际可见组件，并把 7 个标签校正为各自 ItemDefinition 名称，避免新武器接手时看到错误的枪名。若产品不需要世界悬浮文字，可在后续单独删除这一组件，但不能把它当作 StaticMesh 入口。

## 父类 C++ 清理状态

### 已删除

- `AShootWeaponPickupActor::InitializeFromItemInstance`：仓库内没有调用点，只是对 `InitializeFromDrop` 的一层转发；该函数也没有 `UFUNCTION`，不能被蓝图调用。
- `AShootWeaponPickupActor::GetItemLifetime`：仓库内没有对拾取 Actor 的调用点；不要与 `UShootInventoryItemInstance::GetItemLifetime` 混淆，后者仍被库存、QuickBar 和存档链使用。
- `AShootWeaponPickupActor::InteractionText`：7 个武器拾取蓝图的覆盖值已全部为空，源码没有其他写入点；它只会重复或覆盖 ItemDefinition 根部的 `DisplayName`，因此交互名称现在只保留一个数据源。
- `ShootWeaponPickupActor.cpp` 的 `AbilitySystemComponent.h` 和 `GameplayMessageSubsystem.h`：实现没有使用这两个类型或 API。

这三项已从父类 C++ 中移除；`VisualComponent` 仍保留并明确作为方案 B 的唯一 StaticMesh 配置入口。

### 暂不删除

- `VisualComponent`：方案 B 的唯一静态网格入口，必须保留。
- `CollisionComponent`：交互扫描、Overlap 和 PressToInteract 都依赖它。
- `WeaponItemDefinition`、`ItemLifetime`、`StoredStatTags`：分别承载物品身份、Persistent/RuntimeOnly 生命周期和掉落弹药快照。
- `TriggerMode`：固定点拾取使用 PressToInteract，掉落初始化也会强制切到 PressToInteract；虽然当前蓝图配置趋同，但它仍参与运行行为。
- `bDestroyOnPickup`：控制拾取后是否销毁 Actor，不能仅因为当前 7 个值都是 true 就删除语义。
- `InteractionAbilityClass`、`UserFilter`：当前 7 个武器都使用默认 Collect/PlayerOnly，配置存在重复，但是否收窄为武器专用常量需要单独确认 AI 拾取边界后再改。

## ID_* Fragment 继续审计结果与已清理字段

`WeaponBasicConfig` 中有一组独立于拾取网格的高置信度冗余：

| 字段 | 当前证据 | 建议 |
| --- | --- | --- |
| `WeaponId` | `CombatComponent` 写入 QuickBar 消息并用于槽位刷新 | 保留 |
| `ReticleWidgetClass` | Reticle Host 读取 | 保留 |
| `DroppedPickupActorClass` | 丢枪时选择具体 `BP_WeaponPickup_*` | 保留 |
| `EquipmentDefinition` | `EquippableItem` 连接 InventoryManager 与 EquipmentManager | 保留 |
| `Icon` | 运行时使用 ItemDefinition 根部的 `Icon`；7 个 ID 两份图标值相同 | 已从 BasicConfig 删除，继续使用根部 Icon |
| `DisplayName` | 与 ItemDefinition 根部 `DisplayName` 重复；7 个 ID 两份文本相同，只用于空值兜底 | 已从 BasicConfig 删除，根部 DisplayName 是唯一来源 |
| `AmmoIcon` | 当前 C++、武器弹药 Widget 和 7 个 ID 图表都没有读取点 | 已从 BasicConfig 删除 |
| `WeaponTags` | 当前 C++ 无读取点，7 个 ID 容器均为空，图表也没有引用 | 已从 BasicConfig 删除 |

`RangedWeaponConfig` 当前保留字段仍有真实调用：射程、弹药容量、每发消耗、动画样式与骨架 Montage、球扫半径、霰弹 Pellet 数、四个本地相机后坐力值和枪口 Socket。`ProjectileWeaponConfig` 的运动、引信、径向范围/衰减、材质倍率开关、爆炸 Cue、拖尾二选一和投射物网格也都有调用，不应因为“字段多”而整体删除。

以上 4 个字段已从 `UShootInventoryFragment_WeaponBasicConfig` 的 C++ 定义移除，并删除了 `CombatComponent` 中仅用于 `DisplayName` 的兼容兜底。`ReticleWidgetClass`、`DroppedPickupActorClass` 和 `WeaponId` 仍保留；`EquipmentDefinition`、`SetStats` 等其他 Fragment 的运行时职责不受影响。

## 拾取表现与装备表现的边界

两条表现链是明确分离的，拾取蓝图不需要承担装备后的武器模型：

```text
ID_* ItemDefinition
└── EquippableItem.EquipmentDefinition
    └── ActorsToSpawn[0]
        └── B_*（继承 B_Weapon 的 SkeletalMeshComponent）
```

UE5.8.2 编辑器回读了 7 个 `ID_*` 的 `EquipmentDefinition`：每个都只有 1 个 `ActorsToSpawn`，挂到 `weapon_r`；7 个实际 `B_*` 类的父类都是 `B_Weapon_C`，而 `B_Weapon` 自身提供 `SkeletalMeshComponent`。这就是装备后角色手里的视觉来源，与地面拾取物的 `Visual.StaticMesh` 没有共享组件，也不应让拾取蓝图再复制一份 SkeletalMesh。

## BasicConfig 资产重载与最终回读

用户已在 UE5.8.2 下重新冷编译并重启编辑器。本轮没有重复编译项目，只使用当前已加载的新类完成资产处理。

UE5.8.2 版本字符串为 `5.8.2-56702186+++UE5+Release-5.8`。7 个拾取蓝图和 7 个 `ID_*` 蓝图均已由编辑器编译并保存成功，14/14 没有编译或保存失败。

重载后的 CDO 回读结果：

- 7 个拾取蓝图的 `InteractionText` 均已无法找到；`InteractionSubText` 仍为 RuntimeOnly 提示。
- 7 个拾取蓝图的 `Visual` 均为继承的 `StaticMeshComponent`，且均指向武器本体 StaticMesh；本地组件只剩 `WorldLabel`，没有本地 `SkeletalMesh`。四个历史弹药/道具 StaticMesh 已重命名且不再被拾取蓝图引用。
- 7 个 `WeaponBasicConfig` 均仍保留 `WeaponId`、`ReticleWidgetClass`、`DroppedPickupActorClass`，旧的 `DisplayName`、`Icon`、`AmmoIcon`、`WeaponTags` 均已无法找到。
- 这次回读已经证明新类和资产的编辑器状态收敛，不再只是“旧 DLL 里看不到调用点”的间接证据。

`WorldLabel` 仍是唯一未决定的可选项：它是 7 个蓝图各自的本地 `TextRenderComponent`，源码没有任何读取或写入点，只显示世界悬浮文字。如果产品不需要枪旁边的悬浮名称，可以再单独删除这 7 个组件；这不会影响 `Visual`、交互扫描或装备链。本轮不擅自删除，因为它是可见产品表现选择，不是代码冗余入口。

## 推荐实施顺序

1. 给 7 个拾取蓝图的父类 `Visual` 配置唯一的、代表武器本体的 StaticMesh；本轮已完成。新增武器必须先做资产视觉核对，不能按 `Pickup` 后缀猜语义。
2. 删除 7 个蓝图本地 `SkeletalMesh` 组件；本轮已完成。
3. 交互名称统一使用 ItemDefinition `DisplayName`；武器拾取父类的 `InteractionText` 覆盖字段已删除；`InteractionSubText` 仍是 RuntimeOnly 提示。
4. 删除 7 个空 EventGraph 事件节点（已完成）。
5. 保留或删除 `WorldLabel` 取决于产品是否需要世界悬浮文字；本轮保留并校正了 7 个标签。
6. 父类 C++ 的高置信度死代码、未使用 include、`InteractionText` 覆盖字段和 BasicConfig 高置信度冗余字段已从源代码删除；UE5.8.2 下 7 个拾取蓝图与 7 个 ID 已重新编译保存，旧字段 CDO 回读均已确认消失。
7. 进行单人、双本地玩家和 Listen Server 的拾取/丢弃回归；确认掉落仍保留 RuntimeOnly StatTags，且只改变视觉组件，不改变 Equipment Actor 的 SkeletalMesh 表现链。

## 验证状态

- 7 个拾取蓝图已在 UE 编辑器中回读：全部只保留父类 `Visual` 和对应 StaticMesh，蓝图本地 `SkeletalMesh` 均不存在；7 个 EventGraph 节点数均为 0。默认保留的 `UserConstructionScript` 每个只有 UE 自动入口节点，不是用户逻辑。
- 7 个父类 `Visual` 的 StaticMesh、材质、组件列表和 CDO 配置均已回读；Pistol、Rifle、旧 Shotgun 的转换资产均为 `StaticMesh`，且材质与原 SkeletalMesh 对应。
- 7 个父类 `Visual` 的 StaticMesh、材质、组件列表和 CDO 配置均已回读；四个特殊武器的新 StaticMesh 与原武器 SkeletalMesh 的 Bounds 一致，Rocket/Sniper 的静态网格预览确认是完整武器本体；四个旧歧义模型已重命名为 AmmoPickup/AmmoBox。
- 7 个拾取蓝图的 `InteractionText` 覆盖值均已清空，父类 C++ 的该覆盖字段已删除；`InteractionSubText` 均仍为 RuntimeOnly 提示；7 个 `WorldLabel` 已校正为 ItemDefinition 名称。
- 当前打开的 `TestMap_ListenServer` 已完成单人 PIE 拾取回归：Pistol、Rifle、旧 Shotgun、Shotgun_A 均通过真实 `F` 输入拾取，拾取 Actor 被销毁，Controller QuickBar 读到 `Pistol`、`Rifle`、`Shotgun`、`Shotgun_A`。为避免移动输入一次越过测试点，测试只把 PIE 中的玩家临时传送到各拾取物附近作为定位夹具，没有直接调用 `HandlePickup` 或 QuickBar 发放函数。
- 两客户端 Standalone PIE（Play Settings 的 `PlayNumberOfClients=2`）已完成隔离回归：两个独立 `UEDPIE` 世界各有一个本地 Controller 和 4 个拾取物；对预览客户端发送真实 `F` 后，只有该客户端的 Pistol 被销毁并进入 `Active=0 / B_WeaponInstance_Pistol_C / RuntimeOnly`，另一客户端仍保持 `Active=-1` 且 Pistol 仍存在。该项是两个独立 PIE 客户端，不冒充同一视口的分屏验收。
- Listen Server PIE 已完成服务器权威回归：服务器世界包含本地 Host 与远端 Client，客户端世界包含一个本地 Client；从 `Client 1` 窗口发送真实 `F` 后，服务器远端 PC 与客户端本地 PC 都得到 `Active=0 / B_WeaponInstance_Pistol_C / RuntimeOnly`，服务器和客户端的 Pistol Actor 均销毁，Host 仍为 `Active=-1`。为消除当前测试点与网络 Pawn 的碰撞定位误差，测试仅在 PIE 世界临时移动远端 Pawn 和 Pistol Actor，没有写回地图，也没有直接调用 `HandlePickup` 或 QuickBar 发放函数。
- PIE 画面回读确认新生成的 Pistol、Rifle、旧 Shotgun StaticMesh 均可渲染；编辑器静态网格预览确认新生成的 Grenade Launcher、Rocket Launcher、Shotgun_A、Sniper StaticMesh 均为武器本体。装备后的 `B_WeaponInstance_Pistol_C` 仍按原 Equipment 链生成，未把拾取 StaticMesh 错当成装备 SkeletalMesh。
- 修改拾取父类和 BasicConfig 后的 UHT 已通过，`ShootWeaponPickupActor.generated.h` 不再包含 `InteractionText`，`ShootInventoryFragment_WeaponBasicConfig.generated.h` 不再包含 4 个已删除字段；UE5.8.2 重启后 7 个拾取蓝图和 7 个 ID 均已编译保存，最终 CDO 回读确认旧字段消失。7 个拾取蓝图的组件树均为本地 `WorldLabel` + 继承 `Collision`/`Visual`，没有本地 `SkeletalMesh`。
- 标准 `Build_Windows.ps1` 的旧 UE5.8.1 记录保留在历史构建证据中；本轮用户已完成 UE5.8.2 冷编译，因此没有重复编译。当前工作区另有用户/编译过程生成的未跟踪 `Plugins/VibeUE/Source/VibeUE/PythonAPI/*` 文件，本轮未读取、未修改、未暂存，也不会纳入本任务提交。
- 本轮审计已修改父类 C++ 的高置信度死代码、未使用 include、`InteractionText` 覆盖字段、BasicConfig 高置信度冗余字段，以及 `VisualComponent` 的分类和方案 B 注释；已通过 UE 编辑器资产接口统一 7 个拾取蓝图、删除 7 个空 EventGraph、清空重复交互文本、校正世界标签、把四个商城歧义模型重命名为明确的弹药/道具资产，并为四个特殊武器生成完整武器 StaticMesh。单人、两个独立 PIE 客户端和 Listen Server 的既有拾取回归仍然有效；UE5.8.2 重载后的资产结构与 CDO 已完成最终复核。`WorldLabel` 是否保留仍由产品视觉选择决定。
