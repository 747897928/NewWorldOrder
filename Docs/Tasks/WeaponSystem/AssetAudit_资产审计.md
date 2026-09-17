# 武器资产审计

# 审计方式

2026-07-25 通过 UE 5.8 MCP AssetRegistry 只读查询完成。没有读取 `.uasset` 文件，也没有修改或保存资产。

# 当前项目已有内容

- `/Game/Assets/FPS_Weapon_Bundle/Weapons/Meshes`：103 个资产，包含多种武器 SkeletalMesh、Skeleton、StaticMesh 和附件。
- `/Game/Assets/MilitaryWeapSilver/Weapons`：81 个资产，包含 Assault Rifle、Grenade Launcher、Knife 等武器 SkeletalMesh 和 Skeleton。
- `/Game/Assets/Animations/CombatMagicAnims/Demo/Mannequins/Anims/Rifle`：39 个资产，包含持枪 Idle、Equip、Fire、Reload、AimOffset、八方向 Jog/Walk 和跳跃动画。
- `/Game/Assets/Animations/CombatMagicAnims/Demo/Mannequins/Anims/Pistol`：29 个资产，包含持枪 Idle、Equip、Fire、Reload、一个 Fire Montage、AimOffset、八方向 Jog/Walk 和跳跃动画。
- `/Game/Blueprints/Animations/AnimNotifies/AN_PlayWeaponMontage`：已经存在。
- `/Game/Assets/Weapons/Rifle/Animations/AM_Weap_Rifle_Fire`、`AM_Weap_Rifle_Reload`：已经存在。
- `/Game/NiagaraExamples/Gallery/Weapons/Rifle/Animations/AM_MM_Rifle_Fire` 与 `ABP_Weap_Rifle`：已经存在，但先作为来源参考，是否与项目角色骨架兼容需单独验证。
- Lyra 动画同步参考路径：`/Game/Weapons/Rifle/Animations/AM_MM_Rifle_Fire`、`AM_Weap_Rifle_Fire`、`AM_Weap_Rifle_Reload`、`ABP_Weap_Rifle`，以及 `/Game/Characters/Heroes/Mannequin/Animations/AnimNotifies/AN_PlayWeaponMontage`。其中 Notify 已在 Lyra MCP 验证使用角色 Montage Leader 和武器 AnimInstance Follower 的 `Montage Sync Follow` 链路。

# 骨架兼容结论

现有的 Rifle/Pistol 角色动画位于 Mannequin Demo 路径。当前主角 AnimBP 使用的目标骨架为：

- 女主 `BP_ShootAnimInstance_F`：`/Game/Assets/Characters/CC/ShenWanYun/ShenWanYun_Skeleton`。
- 男主 `BP_ShootAnimInstance_M`：`/Game/Assets/Characters/CC/ChenHaoYu/ChenHaoYu_Skeleton`。

因此不能假定现有 Mannequin 步枪/手枪动画可直接用于男女主。后续任务必须先检查骨架兼容性和 Retargeter；若没有可用 Retargeter，须为男女主各自创建或迁移动画，而不是在 AnimBP 中强行引用不兼容 Sequence。

# 当前缺失内容

- `/ShooterCore/Weapons/Rifle`、`Pistol`、`Shotgun` 不在当前编辑器的 AssetRegistry 中。
- `W_Reticle_Rifle`、`AbilitySet_ShooterPistol`、`B_Rifle`、`B_Pistol`、`B_Shotgun` 均未迁入本项目。
- 项目没有检索到 `/Game/Weapons/Rifle/Animations`，用户此前提供的旧路径与实际已存在资产路径不同；当前已验证的项目路径是 `/Game/Assets/Weapons/Rifle/Animations`。
- 当前项目没有检索到专用 Shotgun 动画目录，不能承诺弹匣式霰弹枪已有完整角色动画。

# 首批参考武器建议

先选一把步枪网格作为技术验证，不要一次选全部武器。建议从 `FPS_Weapon_Bundle` 的 AR4 或 Ka47 中任选一把，因为其 SkeletalMesh、Skeleton 和独立弹匣网格均已存在。

完成参考步枪后，手枪和弹匣式霰弹枪各选择一把；选择依据是角色动画是否可重定向和 WeaponActor 是否具备枪口 socket，而不是现实枪械名称。

# 下一步资产检查

持枪状态任务开始前，需要通过 MCP 检查：

- 男女主 SkeletalMesh 的手部 socket 名称和现有的武器附着 socket。
- 参考步枪 WeaponActor 蓝图或待创建蓝图的 Mesh、枪口 socket 和装备附着配置。
- Rifle/Pistol 动画资产的实际 Skeleton 和可用 Retargeter。
- `AN_PlayWeaponMontage` 的 C++ 父类、可配置字段和当前 Event/Notify 调用链。
