# 武器系统迁移策略

# 决策

采用 Lyra ShooterCore 的设计模式，不复制 Lyra 的完整工程，也不重建已经完成且符合该模式的项目层。

具体而言：保留 PlayerState 库存、ItemInstance、EquipmentManager、WeaponInstance、WeaponActor 表现壳、AbilitySet 授予和 SourceObject 链路；重做武器数据配置、动画接口、HUD 页面和表现接线中未完成或不符合项目规则的部分。

这不是保守妥协。当前主线已经是 Lyra 的核心形态，推倒这些对象会破坏 Persistent/RuntimeOnly、QuickBar、角色切换、衣柜和分屏所有权，却不会让新枪更容易配置。正确的“大改”是删除旧 Actor 武器兼容路径和重复表现链，而不是删除已验证的对象分层。

# 目标模型

```text
Weapon ItemDefinition
  WeaponBasicConfig Fragment
  RangedWeaponConfig Fragment
  EquippableItem Fragment
        |
        v
Inventory ItemInstance
  Persistent 或 RuntimeOnly
  Ammo StatTagStack
        |
        v
CombatComponent -> EquipmentManager
        |
        v
WeaponInstance
  开火散布、能力 SourceObject、动画配置读取
        |
        +-> WeaponActor
        |     武器网格、手部附着、枪口 socket、武器自身 Montage
        |
        +-> Character AnimBP / AnimLayer
        |     空手、持枪、瞄准、角色 Montage、IK
        |
        +-> LocalPlayer HUD Layer
              准星与本地反馈
```

# 为什么不按武器数量复制一整套 C++ 类

- 一把新枪应主要新增 Weapon ItemDefinition、EquipmentDefinition、WeaponActor 蓝图和可选动画/准星蓝图。
- 步枪、手枪、霰弹枪、狙击枪、投射物武器分别只保留确有行为差异的 GA 子类；同类武器通过 Fragment 参数区分。
- 网格、枪口 socket、附着 socket、动画层、Montage、准星类都必须来自蓝图或 DataAsset，不能写在 C++ 构造函数。
- 新武器配置完成后必须能由 QuickBar 装备，而不是新建一套直接 Spawn Actor 的入口。

# 迁移阶段

1. 建立武器表现契约

- C++ 只提供持枪状态、姿态类别、瞄准状态、可选的角色/武器 Montage 请求和本地后坐力事件。
- AnimBP 只根据这些状态切换空手/持枪状态机，并调用定义明确的 AnimLayer 接口。
- WeaponActor 蓝图负责网格、手部和枪口 socket；武器自身动画由同步 Notify 驱动。

2. 打通一把参考步枪

- 选择一把项目步枪网格作为参考武器。
- 为男女主各自完成空手、持枪、瞄准、开火、换弹和武器 Montage 同步。
- 使用项目现有 `AN_PlayWeaponMontage`；确认角色与武器 Montage 使用同一 Sync Group。
- 完成后，这条链路是所有弹匣式长枪的模板。

3. 完成参考手枪和弹匣式霰弹枪

- 手枪验证单手姿态、不同 AnimLayer 和独立准星。
- 弹匣式霰弹枪走与步枪相同的整匣换弹 GA，不使用逐发装填 GA。
- 逐发装填霰弹枪只在项目实际有该武器与动画时再接入。

2026-08-25 状态：商城 `/Game/Weapons/Shotgun_A` 已满足“实际武器与动画”门槛，并完成独立逐发装填第一纵切；既有 `/Game/Weapons/Shotgun` 仍保持整匣换弹。后续同类泵动式武器复用该数据/能力模板，不得把逐发 GA 批量替换到所有 Shotgun。

4. 收敛 HUD、散布与后坐力

- 准星迁入目标 LocalPlayer 的 `UI.Layer.HUD`，由 WeaponBasicConfig 选择 Widget 类。
- WeaponInstance 计算服务器权威的散布；移动、蹲伏、空中、瞄准倍率由数据驱动。
- 后坐力是本地相机表现，不能改变服务器命中判定；准星只展示散布，不能反向决定散布。

5. 批量扩展

- 以参考武器的 DataAsset/蓝图为模板创建其余枪支。
- 每个新骨架或新换弹形式先完成一个验证样本，不能假定所有商城武器动画兼容。

# 明确不进入第一阶段

- Lyra WeaponStateComponent。
- 严格服务器命中替换、未确认命中队列和独服级反作弊。
- 完整复制 Lyra 的所有 ShooterCore 蓝图与 Niagara 表现。
- 为每一把武器建立独立 C++ 开火和换弹类。

# 首批需要迁移的 Lyra 资产

当前项目未挂载 `/ShooterCore`，不能直接引用以下资产。由用户通过 UE 编辑器的 Asset Actions -> Migrate 迁入项目后，再由 C++/蓝图接线任务使用。

- `/ShooterCore/Weapons/Rifle/W_Reticle_Rifle` 及其依赖的材质、纹理、父 Widget 和必要样式资产。
- `/ShooterCore/Weapons/Rifle/B_Rifle`，仅作为 Rifle WeaponActor 配置和 socket 的参考或可用表现资产。
- `/ShooterCore/Weapons/Pistol/B_Pistol`，仅作为 Pistol WeaponActor 配置和 socket 的参考或可用表现资产。
- `/ShooterCore/Weapons/Shotgun/B_Shotgun`，仅作为弹匣式 Shotgun WeaponActor 配置和 socket 的参考或可用表现资产。
- `AbilitySet_ShooterPistol` 与其直接依赖的开火、换弹、瞄准 GA，作为资产配置样本。项目不会直接依赖 Lyra C++ 父类；迁移后需改为项目 `UShootAbilitySet` 和项目 GA。

不要迁移 Lyra WeaponStateComponent、相关 HitMarker/UI 确认资产或仅服务 Lyra 特定 C++ 类的蓝图。

# 用户协作请求格式

当需要用户迁移资产时，实施任务必须给出：

- 源项目的准确资产路径。
- 是否勾选依赖项。
- 本项目的目标 Content 路径。
- 迁移后不应修改的 Lyra C++ 父类依赖。
- 迁移完成后的验证方式。
