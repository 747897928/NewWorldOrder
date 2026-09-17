# CC AnimBP 骨架依赖清理记录

更新：2026-09-13

## 结论

正式 CC 动画链中，以下四个 AnimBlueprint 已完成目标 Skeleton、嵌套图节点、状态机过渡 Profile 和生成类缓存的 Mannequin 依赖清理：

- `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base`
- `/Game/Characters/Heroes/CC/MM/Animations/ABP_Mannequin_Base`
- `/Game/Characters/Heroes/CC/MF/Animations/LinkedLayers/ABP_ItemAnimLayersBase`
- `/Game/Characters/Heroes/CC/MM/Animations/LinkedLayers/ABP_ItemAnimLayersBase`

当前对应关系为：

| AnimBlueprint | Target Skeleton | Linked Layer 的 `FastFeet` Profile |
| --- | --- | --- |
| MF Base / MF Item | `ShenWanYun_Skeleton` | `ShenWanYun_Skeleton:FastFeet` |
| MM Base / MM Item | `ChenHaoYu_Skeleton` | `ChenHaoYu_Skeleton:FastFeet` |

`/Game/Characters/Heroes/CC/Animations/LinkedLayers/ALI_ItemAnimLayers` 也已迁移到 CC 目录，正式四个 ABP 使用的是这个 CC 接口路径。

## 为什么会引用 `SK_Mannequin`

IK Retarget 或复制 AnimBlueprint 时，Target Skeleton 可以被改成 CC，但不会自动改写所有嵌套对象。这个资产的引用不只在顶层 AnimGraph，还可能存在于：

1. Linked Anim Layer 内部状态图中的 `Layered Bone Blend` 节点。
2. 状态机的 `AnimStateTransitionNode` 保存的 `BlendProfileInterfaceWrapper`。
3. AnimBlueprint 编译后生成类中的烘焙状态机数据。

因此只检查顶层节点、只检查 `target_skeleton`，或者只看到编辑器能打开，均不能证明已经脱离 Mannequin。此前四个 ABP 的可见/嵌套图节点已经指向 CC，但 Linked Layer 的 Pivot 状态仍有 `FastFeet` 过渡 Profile 指向：

```text
/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin:FastFeet
```

这条 Profile 引用会让 Asset Registry 继续报告 `SK_Mannequin`，即使 Layered Bone Blend 节点本身已经看不到 Mannequin 路径。

## Blend Mask 和同名 Weight Profile 的区别

主 MF/MM Base 的 `AnimGraphNode_LayeredBoneBlend_2` 当前配置为：

```text
BlendMode = BlendMask
BlendMasks = 对应 CC Skeleton:UpperBodyLowerBodySplitMask
```

当 `BlendMode=BlendMask` 时，节点需要真正的 `UBlendProfile`，并且该 Profile 的 `Mode` 必须是 `BlendMask`。它按骨骼逐根保存 0 到 1 的混合权重，节点才能决定上半身姿势影响如何从躯干逐渐过渡到下半身。

同名但 `Mode=WeightFactor` 的 Profile 不是同一种数据。Weight Profile 会把骨骼权重作为普通混合权重的缩放因素使用，不能替代 Layered Bone Blend 的 Blend Mask。名称相同不代表类型和运行语义相同。

这也是为什么 CC Skeleton 上必须保留真正的 `UpperBodyLowerBodySplitMask`，而不能只创建一个同名普通 Weight Profile。之前编辑器曾因为把 `NoBlend` 重命名到已有 `UpperBodyLowerBodySplitMask` 上而崩溃；后续操作必须先确认 Profile 名称和模式，不要用重命名覆盖已有 UObject。

## CC Skeleton 的 Profile 状态

两个 CC Skeleton 当前都存在以下 Profile：

- `UpperBodyMask`
- `LowerBodyMask`
- `LeftFingersMask`
- `UpperBodyLowerBodySplitMask`
- `FastFeet`

其中 `UpperBodyMask`、`LowerBodyMask`、`LeftFingersMask` 和 `UpperBodyLowerBodySplitMask` 是真正的 Blend Mask；`FastFeet` 是真正的 `TimeFactor` Profile，不能使用普通 Weight Profile 替换。

复读结果：

- 两个 CC Skeleton 的四个 `*Mask` Profile 均已按各自 123 根 CC 骨骼的有效权重语义复核。
- `UpperBodyLowerBodySplitMask` 从 Mannequin 源 Profile 中成功匹配 23 根同名骨骼；源侧独有的 42 根辅助/修正骨骼跳过，目标侧其余骨骼保留默认权重，不伪造骨骼。
- `FastFeet` 在两个 CC Skeleton 上均有 11 根通用脚部骨骼，权重全为 `0.5`：`thigh_l/r`、`calf_l/r`、`foot_l/r`、`ball_l/r`、`ik_foot_root`、`ik_foot_l/r`。
- 源 Profile 是稀疏存储：`GetBoneBlendScale` 返回 `1.0` 的骨骼不会写入 `BoneNames`，所以“源 Profile 中没有这根骨骼”表示目标应保留默认权重 `1.0`，不能把它当成 `0.0`。
- 源 `LeftFingersMask` 虽然导出的非默认条目全部为 `0.0`，但左手和左手指等未出现在条目中的骨骼仍是有效权重 `1.0`；目标不能因此做成全零。

## 2026-09-13 PIE 姿势回归的根因与修复

本轮迁移后出现了“Montage 预览正常、PIE 姿势异常”的回归。正式检查确认四个 ABP 的图节点结构和 IK 参数没有被迁移改动；错误在 CC Skeleton Profile 的迁移算法：它把目标 Skeleton 的 123 根骨骼全部写进 Profile，并把源侧没有同名条目的骨骼默认成 `0.0`。

这与源 Profile 的语义相反。VibeUE 的 `GetBlendProfile` 只导出 `Scale != 1.0` 的条目；缺少条目的骨骼在运行时就是默认权重 `1.0`。因此迁移前后实际发生了这些变化：

- `UpperBodyLowerBodySplitMask` 的 `upperarm_l/r`、`lowerarm_l/r`、`hand_l/r`、`ik_hand_*`、`weapon_l/r` 等源侧未记录骨骼被错误变成 `0.0`，上半身 Montage 传不到手臂和武器辅助空间。
- `UpperBodyMask` 的手、手指、IK 和武器辅助骨骼同样被错误清零。
- `LeftFingersMask` 的左手/左手指默认权重被错误清零，Linked Layer 的左手覆盖链因此失去输入。

已对 Shen、Chen 两个 CC Skeleton 的四个 `*Mask` Profile 应用同一修复规则：同名源条目复制源值；源侧没有条目的目标骨骼恢复有效默认值 `1.0`。本次共纠正 644 个错误的显式权重；`FastFeet`、所有 AnimSequence、Montage 和 Control Rig 关键帧均未修改。修复后两个 Skeleton 的有效非默认条目数分别为：`UpperBodyMask=27`、`LowerBodyMask=65`、`LeftFingersMask=55`、`UpperBodyLowerBodySplitMask=23`，符合 CC Skeleton 的同名骨匹配结果。

## `weapon_l` 在当前运行时链中的作用

`weapon_l` 不是武器网格，也不是单独生成武器的对象。它是角色 Skeleton 中的辅助骨骼，位于 `hand_l` 下，用来保存左侧武器/握持相关的动画输入；`weapon_r` 位于 `hand_r` 下，是对应的右侧辅助骨骼。它们是否直接参与武器挂接，要看具体角色和武器实现。

这里有一个容易被骨骼名称误导的细节：当前 CC Linked Layer 的 `CopyBone` 直接读取的不是 `weapon_l`，而是虚拟骨骼 `VB IK_Hand_L_weaponSpace`。当前 Shen Skeleton 的层级查询为：

```text
动画中的 VB IK_Hand_L_weaponSpace
    -> CopyBone：写入 ik_hand_l
    -> TwoBoneIK：以 ik_hand_l 作为 hand_l 的 Effector
    -> HandIKRetargeting：继续使用 ik_hand_l / ik_hand_gun 进行手部对齐
```

层级上，`weapon_l` 是 `hand_l` 的子骨骼，`weapon_r` 是 `hand_r` 的子骨骼，而 `VB IK_Hand_L_weaponSpace` 当前挂在 `weapon_r` 的参考空间下。因此不能仅凭 `weapon_l` 这个名字断言它就是 CopyBone 的直接输入；它是动画/武器辅助链的一部分，但当前运行时直接消费的是虚拟骨骼和 `ik_hand_*` 骨骼。

因此 `weapon_l` 相关轨道的价值不是“让武器网格显示出来”，而是参与角色的武器/握持辅助姿势；真正给当前左手 TwoBoneIK 提供目标的是上述虚拟骨骼经过 `CopyBone` 写入的 `ik_hand_l`。Montage 单独预览时，看到的是原始动画姿势；PIE 中还会经过 Main ABP 的 Layered Bone Blend、Linked Layer 的 CopyBone/TwoBoneIK 和 Post Process。上次错误的 `0.0` Mask 在进入这条链之前截断了虚拟骨骼、IK 和武器辅助骨骼输入，所以同一条源序列会出现“预览正常、PIE 异常”。

本次修复没有重写 `weapon_l` 轨道，也没有把用户 Control Rig 动画改成 Mannequin 数据；只是恢复了这些辅助骨骼在正确 Blend Mask 中应有的运行时传递权重。

## 本轮修改内容

### Layered Bone Blend

- 主 MF/MM Base 的上下身节点改为对应 CC Skeleton 的真正 `UpperBodyLowerBodySplitMask` Blend Mask。
- MF/MM Linked Layer 内部状态图中原来指向 Mannequin `UpperBodyMask` 的 8 个嵌套节点分别替换为同侧 CC `UpperBodyMask`。
- CC Skeleton 的 `UpperBodyMask`、`LowerBodyMask`、`LeftFingersMask` 和上下身 Split Mask 均已按稀疏 Profile 语义修复并保存；源侧没有同名骨骼的目标骨骼保留有效默认权重 `1.0`。

### FastFeet 状态机过渡

- MF Base 的 5 条 Mannequin `FastFeet` 过渡 Profile 改为 Shen 的 CC `FastFeet`。
- MM Base 的 6 条 Mannequin `FastFeet` 过渡 Profile 改为 Chen 的 CC `FastFeet`。
- MF Linked Layer 的 PivotSM 两条过渡改为 Shen 的 CC `FastFeet`。
- MM Linked Layer 的 PivotSM 两条过渡改为 Chen 的 CC `FastFeet`。
- 替换时只修改 `BlendProfileInterfaceWrapper`，没有改变过渡时长、BlendMode、规则或连线。

### 结构化编译

VibeUE 的 ObjectTools 可以写入嵌套状态机节点的 Profile，但这种写入不一定会使 `AnimBlueprintGeneratedClass` 的烘焙数据失效。为此，在用户明确允许修改的项目定制插件 `Plugins/AnimationAssetFixer` 中增加了：

```text
AnimationAssetFixerLibrary.force_structural_compile_and_save(abp_path)
```

该入口只做 `MarkBlueprintAsStructurallyModified`、编译和保存，不猜测或改变图节点内容。它用于在编辑器反射修改嵌套节点后清理生成类缓存；没有修改 VibeUE、Unreal Engine 或 Lyra/第三方插件。

## 验证结果

使用 Asset Registry 的硬依赖和软依赖联合检查：

- 四个正式 MF/MM Base 和 Item ABP 不再直接依赖 `/Game/Characters/Heroes/Mannequin` 包。
- Linked Layer ABP 现在依赖同侧 CC 主 ABP，这是预期的正式 CC 类型链，不是源 Mannequin 依赖。
- `ALI_ItemAnimLayers` 的 CC 路径无 Mannequin 包依赖。
- 四个 ABP 的 `target_skeleton` 分别是 `ShenWanYun_Skeleton` 和 `ChenHaoYu_Skeleton`。
- `AnimationAssetFixer.list_graph_node_runtime_properties` 对四个 ABP 的 Mannequin 路径命中数均为 `0`。
- 四个 ABP 均完成编译并保存；结构化编译入口返回成功。

## 尚未在本记录中删除的源资产

这次清理没有删除 `/Game/Characters/Heroes/Mannequin` 源资产，也没有删除旧武器源 Montage。旧源 Montage 已有正式 CC 替代且此前检查没有外部引用，但它们自身仍可能保存 Mannequin Skeleton 依赖；是否删除应在全局引用审计和用户确认后单独处理，不能把“正式 ABP 已清零”直接等同于“Mannequin 目录可以删除”。

`CR_ShootFootPlant` 的旧调查路径当前无法在 Asset Registry 或编辑器中重新定位，因此本轮没有创建猜测性的替代资产。它应作为单独的低优先级路径核对项处理。

## 用户验收

本轮不需要用户再手动创建这五类 Profile。建议在编辑器恢复后由用户做以下快速验证：

1. 打开两个 CC Skeleton，确认 Profile 名称存在；`UpperBodyLowerBodySplitMask` 的模式为 `BlendMask`，`FastFeet` 的模式为 `TimeFactor`。
2. 打开 MF/MM Base 和 Linked Layer ABP，确认 Target Skeleton 分别为 Shen/Chen，编译无错误。
3. 在空手、Rifle、Pistol、Shotgun 的静止和移动姿势中观察上下身混合、脚部过渡和左手姿势。
4. 如果出现姿势问题，先记录具体 ABP、状态、武器和时间点；不要把 Profile 改回 Mannequin，也不要用同名 Weight Profile 覆盖 Blend Mask。
