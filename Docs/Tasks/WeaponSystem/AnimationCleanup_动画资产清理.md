# CC 动画资产清理审计

审计日期：2026-08-23

## 范围与排除项

本次只检查以下两个目录及其子目录：

- `/Game/Characters/Heroes/CC/MF`
- `/Game/Characters/Heroes/CC/MM`

以下目录及其子目录明确排除，不参与枚举、判断和删除：

- `/Game/Characters/Heroes/CC/MF/Animations/Poses`
- `/Game/Characters/Heroes/CC/MM/Animations/Poses`

## 审计方法

通过 Unreal Editor 的 Asset Registry 枚举目录内的动画资产，覆盖 `AnimSequence`、`AnimMontage`、`AnimBlueprint`、`BlendSpace`、`AimOffset`、`PoseAsset` 等动画类。引用查询同时开启硬引用、软引用、可搜索名称、管理引用、编辑器引用和游戏引用。

对 Asset Registry 报告为零引用的候选资产，又通过 `EditorAssetLibrary.find_package_referencers_for_asset(PackagePath, True)` 加载确认引用者包进行二次检查。结果为：非 Poses 目录共枚举 657 个动画资产，确认 86 个没有任何包引用，查询错误为 0。

`ALI_ItemAnimLayers` 的额外核对结果：正式男女 `ABP_Mannequin_Base` 实际依赖的是 `/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ALI_ItemAnimLayers`；MF/MM 目录下同名副本均无 Asset Registry 引用，也没有加载确认后的包引用。因此这两个副本纳入删除清单。旧任务文档中提到的 `/Game/Blueprints/Character/Animation/ALI_ShootWeaponLayers` 在本次 Asset Registry 和磁盘资产名核对中均未找到；本次不创建、不替换，也不把这个文档路径当作可删除目标。

## 已确认无引用资产清单

### MF：25 个

```text
/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MM_Rifle_Idle_Hipfire
/Game/Characters/Heroes/CC/MF/Animations/AimOffsets/MM_Unarmed_Idle_Ready
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_convo_L
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_convo_R
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_idle_loop
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v1
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v2
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v3
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_into_L_45
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_into_L_90
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_into_R_45
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_into_R_90
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_into_fwd
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_out_run
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_out_stand
/Game/Characters/Heroes/CC/MF/Animations/Interactions/Bench/int_sit_bench_sit_out_walk
/Game/Characters/Heroes/CC/MF/Animations/LinkedLayers/ALI_ItemAnimLayers
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Pistol/MF_Pistol_TurnLeft_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Pistol/MF_Pistol_TurnRight_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/MF_Rifle_TurnLeft_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Rifle/MF_Rifle_TurnRight_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_TurnLeft_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_TurnRight_180
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_Walk_Fwd_Start
/Game/Characters/Heroes/CC/MF/Animations/Locomotion/Unarmed/MF_Unarmed_Walk_Fwd_Stop
```

### MM：61 个

```text
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Pistol_Idle_ADS
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Pistol_Idle_ADS_AO_CD_45
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Pistol_Idle_ADS_AO_CU_45
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_CD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_CU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LBC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LBD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LBU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_LU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RBC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RBD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RBU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Crouch_Idle_AO_RU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_CD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_CU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LBC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LBD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LBU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_LU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RBC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RBD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RBU
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RC
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RD
/Game/Characters/Heroes/CC/MM/Animations/AimOffsets/MM_Rifle_Idle_ADS_AO_RU
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_convo_L
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_convo_R
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_idle_loop
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v1
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v2
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_idle_twitch_v3
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_into_L_45
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_into_L_90
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_into_R_45
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_into_R_90
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_into_fwd
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_out_run
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_out_stand
/Game/Characters/Heroes/CC/MM/Animations/Interactions/Bench/int_sit_bench_sit_out_walk
/Game/Characters/Heroes/CC/MM/Animations/LinkedLayers/ALI_ItemAnimLayers
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Pistol/MM_Pistol_Crouch_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Pistol/MM_Pistol_Crouch_TurnRight_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Pistol/MM_Pistol_Jog_Stop
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Pistol/MM_Pistol_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Pistol/MM_Pistol_TurnRight_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnRight_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd_RAW
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Rifle/MM_Rifle_TurnRight_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Shotgun/MM_Shotgun_Idle_ADS
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Unarmed/MM_Unarmed_Crouch_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Unarmed/MM_Unarmed_Crouch_TurnRight_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Unarmed/MM_Unarmed_TurnLeft_180
/Game/Characters/Heroes/CC/MM/Animations/Locomotion/Unarmed/MM_Unarmed_TurnRight_180
```

## 处理结果

上述 86 个资产已通过 Unreal Editor 按精确包路径删除。删除后复核结果为：86 个路径均不存在，Asset Registry 中没有残留清单路径，两个范围目录下没有新增 `ObjectRedirector`。排除目录未被触碰，MF/MM 的 `Animations/Poses` 仍分别有 26/30 个资产。

本次没有触碰两个 `Animations/Poses` 目录、正式男女 Base AnimBP、Mannequin 共享 `ALI_ItemAnimLayers`，也没有使用 PowerShell 删除磁盘文件。

## 对武器系统最小资产闭包的影响

本次删除的主要是旧 AimOffset 样本、旧 locomotion 单帧动作、Bench 交互动作和两份未使用的 CC 动画层接口副本。实现首把武器时，仍需保留并核对：

- 正式男女 Base AnimBP、实际存在且被项目引用的动画层接口/Linked Layer，以及对应性别/武器 Linked Layer；新增武器前应先用 Asset Registry 核对真实包路径，不要照抄已经不存在的文档路径。
- 当前武器真正使用的 locomotion/aim 资产，以及角色 Fire、Reload、Equip Montage。
- 武器表现 Actor 蓝图、武器 SkeletalMesh/Skeleton、枪口和手部附着 Socket。
- Weapon ItemDefinition、WeaponInstance/Equipment 配置、AbilitySet/Gameplay Ability、伤害 GameplayEffect 和 GameplayCue。
- Enhanced Input 的 InputAction、InputMappingContext、输入配置 DataAsset，以及必要的 HUD/准星 Widget。

清理清单中的资产不应再作为新武器系统的必需依赖；新武器应从正式 CC 层和现有武器闭包复制配置，不要重新引用已删除的旧路径。
