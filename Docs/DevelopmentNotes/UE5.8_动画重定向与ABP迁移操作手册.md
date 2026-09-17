# UE5.8 动画重定向与 AnimBlueprint 迁移操作手册

适用环境：UE 5.8 编辑器、Unreal Python、Editor C++、VibeUE 或其他 MCP 自动化工具。

目标：把动画资产、AnimBlueprint 和 Linked Anim Layer 迁移到目标骨骼；源动画目录移除后，目标资产仍能编译并正常运行。

## 1. 核心原则

1. 先修好 IK Rig、Retarget Chains、Retarget Pose、Root 和 Chain 设置，再批量导出。批量操作会放大配置错误。
2. 优先让 IK Retargeter 同时处理 AnimBlueprint 和被引用的动画资产。引擎生成的资产映射通常比事后逐个改引用可靠。
3. 显式加入完整的 ABP 继承树和 Linked Layer 蓝图。`include_referenced_assets` 会处理动画依赖，不保证自动发现所有子蓝图、接口、类和类型依赖。
4. 迁移到不同骨骼时，父 ABP、子 ABP 和 Linked Layer 一起迁移。只修改子 AnimBlueprint 的 `target_skeleton` 容易留下旧父类和旧图层引用。
5. 引用检查必须覆盖图节点、父节点覆盖表、生成类 CDO 默认值和类/类型引用。只查 AnimGraph 节点会漏掉大量引用。
6. 所有修改都要经过编译、保存、重新加载和依赖复查。编辑器缓存会让未保存或失效的结果看起来正常。
7. 删除源资产是最后一步，并在独立分支或备份中执行。
8. Additive AnimSequence 不能带着源资产的 Additive 标记直接批量重定向。先生成目标骨架的绝对姿势，再只在目标结果上恢复 Additive 类型和目标侧 Base Pose。

## 2. 批量重定向

入口：

```python
unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
```

`inputs` 使用 `unreal.IKRetargetBatchOperationInputs`，常用字段如下：

1. `assets_to_retarget`：`AssetData` 数组。可包含 AnimSequence、BlendSpace、AimOffset、PoseAsset 和 AnimBlueprint。
2. `source_mesh`：源 SkeletalMesh。
3. `target_mesh`：目标 SkeletalMesh。
4. `ik_retarget_asset`：已配置好的 IKRetargeter。
5. `target_path`：输出目录。
6. `use_source_path`：是否保留源目录结构。
7. `include_referenced_assets`：复制并重定向输入资产引用的动画资产。AnimBlueprint 和 BlendSpace 常依赖此项。
8. `overwrite_existing_files`：UE 5.8 的实现不是原对象内更新。目标同名资产存在时，引擎会先生成数字后缀副本，再执行 `ForceReplaceReferences`、`ForceDeleteObjects` 和重命名。需要保留正式资产对象身份、禁止 Force Delete，或工作区存在未提交资产时，必须保持 `False`。

最小模板：

```python
import unreal

inputs = unreal.IKRetargetBatchOperationInputs()
inputs.set_editor_property("assets_to_retarget", asset_data_list)
inputs.set_editor_property("source_mesh", source_mesh)
inputs.set_editor_property("target_mesh", target_mesh)
inputs.set_editor_property("ik_retarget_asset", retargeter)
inputs.set_editor_property("target_path", target_path)
inputs.set_editor_property("use_source_path", False)
inputs.set_editor_property("include_referenced_assets", True)
inputs.set_editor_property("overwrite_existing_files", False)

created = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
```

### 2.1 禁止把 overwrite 当作原对象内更新

UE 5.8 `UIKRetargetBatchOperation::OverwriteExistingAssets` 的实际顺序如下：

1. `DuplicateRetargetAssets` 先复制并重定向源资产。同名目标已经存在时，新资产会得到 `Name1` 之类的唯一名称。
2. `ObjectTools::ForceReplaceReferences` 把旧正式资产的引用改到新副本。
3. `ObjectTools::ForceDeleteObjects` 强制删除旧正式资产。
4. `AssetToolsModule.RenameAssets` 再把新副本改回正式名称。

因此 `overwrite_existing_files=True` 会主动进入 Force Delete 路径；是否脏化、是否被编辑器打开只会影响该路径能否顺利结束，不改变其删除语义。NewWorldOrder 的正式 CC 动画批处理禁止再使用此选项。

安全流程必须满足：

1. 重定向输出到独立临时目录，并设置 `overwrite_existing_files=False`。
2. 逐项检查 `run_batch_retarget` 返回的 `AssetData`；输出路径不符合预期或出现数字后缀时立即停批。
3. 通过 AnimSequence 数据控制器把验证通过的临时结果写入现有正式 AnimSequence，保留正式 UObject、包路径和引用关系。该复制链必须先在非正式测试资产上验证 Skeleton、关键帧、曲线、Notify、Sync Marker、Root Motion、Additive/Base Pose 和辅助骨轨。
4. 临时资产只有在 Asset Registry 确认零引用后，才允许通过非 Force 的单资产 Unreal API 清理；任何失败都停止并保留现场。
5. 正式目标脏化、已打开，或同目录已经存在数字后缀副本时，禁止开始下一次重定向。

### 2.2 Additive 动画必须先转为绝对姿势

`Jog Lean`、Aim Offset 辅助序列等 Additive AnimSequence 若在批量重定向时保留源 Additive 标记，IK Retargeter 输出的目标骨架关键帧可能被再次当作相对差值解释。项目中的直接症状是左右 Lean 结果字节和姿势近似相同，骨盆、脊柱出现约 100 到 168 度异常旋转，运行时快速反向会让角色全身折叠或卡在极端侧倾。

正确流程如下：

1. 以源 Manny 动画和目标 CC SkeletalMesh 运行 IK Retarget。
2. 在 `IKRetargetBatchOperationInputs` 上设置 `retain_additive_flags=False`，让重定向先输出目标骨架的正常绝对姿势。
3. 分别检查目标 Left、Center、Right 绝对姿势；左右必须互异，骨盆和脊柱旋转必须在合理范围内。
4. 只在目标 Left/Right AnimSequence 上恢复 `Local Space` Additive 类型。
5. 每个目标资产使用同一性别、同一目录的 CC Center 动画作为 Base Pose，不能继续引用 Manny Center，也不能跨性别共用 Base Pose。
6. 保存并重新加载目标资产，确认 Additive 类型、Base Pose、Skeleton 和关键帧均持久化。
7. 恢复 AnimBP 中 Apply Additive 的正式 Alpha，编译后用真实 Enhanced Input 连续左右反转、前后混合和 Pivot 验收。

NewWorldOrder 已用此流程重建男女 Rifle Jog Lean 的四个正式 Left/Right 资产。2026-08-08 的本地双人 TestMap 验收中，先通过权威拾取入口给男女角色装备 Rifle，再把键鼠 P1 仅在 PIE 运行时依次控制男女 Pawn，执行真实 `IA_Move` 快速反转；`Saved/Diagnostics/LeanFix_Phase_07.png`、`LeanFix_Phase_15.png`、`LeanFix_Phase_23.png` 未再出现女性全身折叠。

### 2.3 从已有 Montage 派生短片段时必须复制完整播放参数

Fire 这类 Mesh Space Additive 也必须遵守 2.2 的“先绝对姿势、后恢复 Additive”原则。NewWorldOrder 的 Rifle、Pistol、Shotgun Fire 分别使用目标同名序列自身的第 `22/24/28` 帧作为 Base Pose。重定向时应先设置 `retain_additive_flags=False`，完成后恢复 `AAT_ROTATION_OFFSET_MESH_SPACE + ABPT_ANIM_FRAME + 目标自引用 Base Pose`。若目标骨架额外维护了 `weapon_r` 等源 Retarget Chain 不处理的辅助轨道，重定向覆盖后必须重新迁移并逐帧核对。

只复制 Slot Track 和 AnimSegment 不足以得到与 Lyra 一致的 Montage。`RateScale`、Blend In、Blend Out、Blend Option、`BlendOutTriggerTime`、自动 Blend Out、Section 起止时间和 Slot Group 都会改变实际可见时长。短 Montage 尤其容易暴露这个问题：如果从较长 Equip Montage 复制后只把片段裁短，却保留较长的 Blend Out，动画可能刚开始就进入自动淡出。

NewWorldOrder 的男女 `AM_Generic_Unequip` 曾保留 Pistol Equip 的 `RateScale=1.1`、Blend In `0.25/Hermite Cubic` 和 Blend Out `0.40/Hermite Cubic`。Montage 总长只有 `0.5` 秒，因此运行时约在 `0.06` 秒就开始淡出，看起来像第二帧被取消。Lyra 原资产的精确设置是 `RateScale=0.9`、Blend In `0.20/Cubic`、Blend Out `0.30/Cubic`；恢复后男女资产都能推进到约 `0.23` 秒再按官方配置淡出。

派生 Montage 后必须逐项核对：

1. Slot 名、Slot Group、Track 数量和每个 Segment 的源动画、Montage Start、Anim Start、Anim End、Play Rate。
2. Montage `SequenceLength`、`RateScale`、Blend In/Out 时间与曲线类型、`BlendOutTriggerTime`、自动 Blend Out。
3. Section 的 Start、End、Next Section 和循环标记。
4. 使用引擎逐帧回调记录 Montage Position，不能只在调用 `Montage_Play` 的同一帧看到 Active 就宣布通过。
5. 最终必须走真实 Enhanced Input/GAS 入口验证；直接调用组件函数只能作为测试准备或单变量诊断。

网络 PIE 还必须分别记录服务器模拟代理与客户端本地 Pawn。装备列表使用 FastArray、装备实例又单独复制属性时，`PostReplicatedAdd` 和实例 `OnRep` 的先后顺序没有保证；如果 Montage 选择依赖 `Instigator -> ItemDefinition`，不能假设 `OnEquipped` 首次进入时配置已经可读。正确处理是记录“首次表现因依赖未到而缺失”的一次性状态，在依赖属性的 `OnRep` 扩展点只补缺失的表现，不要重复执行完整装备生命周期，也不要用固定 Timer 掩盖复制顺序。

### 2.4 重定向后的 Equip 前段必须按最终武器挂点验收

角色动作本身与源 Manny 接近，不等于项目最终持枪效果可接受。武器 Actor 的 Socket 和相对 Transform 会改变手、枪和 weapon-space IK 的整体关系；必须先对齐来源项目的 EquipmentDefinition，再判断是否需要裁剪重定向 Montage。

NewWorldOrder 的 Rifle、Pistol、Shotgun 最初挂在项目旧 Socket `weapon_socket_hand_r`，相对 Transform 为单位变换。Lyra 11000 的三个 `WID_*` 实际都挂在 `weapon_r`，并使用绕 Z 轴 `-90` 度的相对旋转。项目三份 `BP_Equipment_*` 已对齐为 `weapon_r/-90`；这一步修复了横移开火时枪体方向与双手空间不一致的问题。

2026-08-14 玩家复验推翻了“裁到 `0.6s` 即完成”的结论。项目 CC Equip/Unequip 在重定向后漏掉了 Lyra Montage 中参与 IK/FK 过渡的 `ScaleDownWeaponR` 与 `DisableLHandIK` 曲线；跳过前段只是在隐藏缺失数据，同时丢失完整装备动作。正确修复是把来源 Montage 曲线复制到同侧 CC Montage，再让 `EquipMontageStartPosition` 回到 `0.0s`。Generic Unequip 若使用源 Montage 的 `0.1-0.6s` 子片段，还必须把曲线时间平移到自身 `0.0-0.5s` 轴上。2026-08-12 复读发现 MM `AM_Shotgun_Equip` 当前在 `0.0001s` 有一枚 `AN_ShootPlayWeaponMontage`，所以不得继续引用“六个正式 Equip Montage 均为零 Notify”的旧审计结论；新增武器和已被用户修改的 Montage 都必须重新读取 Notify。

验收必须同时满足：

1. 走真实 `IA_Interact`、`IA_WeaponNext` 和 Equipment 生命周期，不能只手工 `Montage_Play`。
2. 日志记录实际 Montage Position 和附着 Socket，截图覆盖 Equip 起播帧。
3. 持续注入真实 `IA_Move` 直到速度稳定，再触发 `IA_Attack`；只注入一帧 Move 得到的速度 `0` 不能算移动开火证据。
4. 男女、三枪和快速切换分别验收；Fire/Idle 左手握持与 Reload/Unequip 合法释放分开判断。

执行后立即记录返回的 `AssetData`，保存目标资产，并核对：

1. 动画资产的 Skeleton 指向目标骨骼。
2. AnimBlueprint 的 Target Skeleton 指向目标骨骼。
3. 目标 ABP 的父类指向目标侧父 ABP。
4. Linked Anim Layer、Anim Layer Interface 和关联 ABP 的路径符合预期。
5. BlendSpace、AimOffset、Montage、PoseAsset 的内部动画引用已映射到目标资产。

## 3. AnimBlueprint 的四类引用

### 3.1 当前蓝图拥有的图节点

常见位置包括 Sequence Player、Sequence Evaluator、Blend Space Player、Aim Offset、状态机状态图和动画层图。

这类引用存在于编辑器图节点及其运行时 AnimNode 结构中。

### 3.2 `ParentAssetOverrides`

子 AnimBlueprint 可覆盖父蓝图资产播放器节点。`UAnimBlueprint::ParentAssetOverrides` 保存：

1. 父节点 GUID：`ParentNodeGuid`
2. 新动画资产：`NewAsset`

`AnimationLibrary.add_node_asset_override()` 只适合可覆盖的父资产节点。它不会遍历并替换当前蓝图全部节点，也不会修改 CDO 变量。

### 3.3 生成类 CDO 默认值

动画资产可以保存在 Blueprint 变量默认值中，容器还可能包含数组、Map、Set 和嵌套 Struct。

Lyra 的武器 Linked Layer 采用这种结构：基础 Linked Layer ABP 声明逻辑和变量，每个武器子 ABP 填写自己的动画变量。此类子蓝图可能没有自己的 AnimGraph 节点，但仍会硬引用大量动画资产。

`get_nodes_of_class()` 返回 0，只能说明当前查询没有枚举到该蓝图拥有的图节点。随后应检查父蓝图、CDO 和 `ParentAssetOverrides`。

### 3.4 类和类型引用

需要单独检查：

1. 父 Blueprint Generated Class
2. Linked Anim Layer 类
3. Anim Layer Interface
4. AnimNotify 和 AnimNotifyState 类
5. Control Rig 类
6. Enum、Struct、DataAsset 和其他逻辑资产

这些资产可能属于共享逻辑，不应按动画目录前缀盲目替换。建立允许保留的依赖白名单。

## 4. ABP 修复顺序

### 4.1 修复继承关系

先确认目标父 ABP 已存在，再把目标子 ABP 指向目标父类。父类仍在源目录时，子蓝图会继续继承源 Skeleton、图、变量和默认值。

UE 的 Child AnimBlueprint Asset Override 工作流只适合替换父图中的动画序列。跨 Skeleton 迁移应生成完整的目标 ABP 层级。

### 4.2 修复当前图节点

先枚举目标 ABP 自己拥有的所有动画图、状态图和动画层图，再收集节点使用的动画资产。

Editor C++ 可对继承自 `UAnimGraphNode_AssetPlayerBase` 的节点调用：

```cpp
UAnimationAsset* OldAsset = Node->GetAnimationAsset();
Node->SetAnimationAsset(NewAsset);
```

该基类覆盖 Sequence Player、Sequence Evaluator 和多种 BlendSpace 播放节点。其他节点按具体 `UAnimGraphNode_*` 类型处理。

修改后调用 Blueprint 修改标记，编译并保存。

VibeUE 的 API 会更新。调用前先发现本机版本：

```text
discover_python_class("unreal.AnimGraphService")
discover_python_class("unreal.BlueprintService")
```

使用实际存在的节点读取和设置方法，避免猜测方法名。某个服务没有暴露目标节点时，使用 Unreal Python 对象属性或 Editor C++ 工具。

### 4.3 修复父节点覆盖表

遍历 `ParentAssetOverrides`，将 `NewAsset` 按资产映射表换成目标动画。每条记录还要验证 `ParentNodeGuid` 在新的父 ABP 层级中仍有效。

父 ABP 被重新创建或图节点 GUID 改变时，旧覆盖记录可能失效。此时重新建立覆盖记录。

### 4.4 修复 CDO 默认值

先编译蓝图，再取得生成类和 CDO：

```python
import unreal

bp = unreal.load_asset(bp_path)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cls = bp.generated_class()
cdo = unreal.get_default_object(cls)
```

对已知变量执行路径映射：

```python
old_value = cdo.get_editor_property(variable_name)
new_value = remap_value(old_value, asset_map)

if new_value != old_value:
    cdo.set_editor_property(variable_name, new_value)
```

`remap_value` 至少要处理：

1. `UObject` 和 `UAnimationAsset` 引用
2. Array
3. Set
4. Map 的键和值
5. Struct 的全部字段
6. 嵌套容器

修改要求：

1. 按完整对象路径查映射，不能只按资产名匹配。
2. 目标资产缺失时停止并报告，不能保留一半新引用、一半旧引用。
3. 修改整个容器值，避免原地修改后未触发编辑器变更通知。
4. 调用 `set_editor_property()`，让编辑器执行属性变更通知。
5. 标记 Blueprint 和 CDO 已修改，保存后重新加载验证。
6. 编译后若默认值恢复，改用 Blueprint 默认值 API 或 Editor C++ 反射工具写入。

Python 无法可靠枚举或写入某些动态 Struct、受保护属性和复杂容器时，使用 Editor C++。可用 `FProperty` 递归遍历，或在限定根对象范围内使用 `FArchiveReplaceObjectRef` 应用 `旧 UObject -> 新 UObject` 映射。

## 5. Linked Anim Layer 和 Lyra 结构

Lyra 的典型结构：

1. 主 AnimBlueprint 通过 Linked Anim Layer 切换武器动画逻辑。
2. Anim Layer Interface 定义图层接口。
3. 基础 Linked Layer ABP 保存图和变量声明。
4. 每种武器的子 ABP 在变量默认值中填写动画资产。

迁移时显式包含这四层资产。检查目标武器子 ABP 的父类、接口、CDO 动画变量和主 ABP 的 Linked Layer 类引用。

同一子 ABP 可以同时引用多套动画。生成目标动画清单时应从实际依赖和 CDO 值出发，不能只按文件名前缀分类。

## 6. 禁止使用的捷径

### 6.1 Replace References

Content Browser 的 `Asset Actions > Replace References` 是资产合并工具。它会把被合并资产的全局引用改到目标资产，并尝试删除被合并资产。

仅在确认多个资产应永久合并为同一个资产时使用。ABP 迁移和一对一动画重定向不使用该工具。

### 6.2 直接编辑 `.uasset`

`.uasset` 是序列化二进制资产。直接替换字符串会破坏名称表、导入表、长度、索引和版本数据。

二进制字符串搜索只能作为残留路径线索。字符串存在不等于当前有效引用仍存在。

### 6.3 只修改 `target_skeleton`

该属性只设置 AnimBlueprint 的目标 Skeleton。它不会自动修改父类、图节点、CDO、覆盖表、Linked Layer 和类型引用。

## 7. 验证清单

每个目标 ABP 都执行以下检查：

1. 编译结果无错误；警告已逐条判断。
2. Target Skeleton 正确。
3. 父类链全部指向目标侧或明确允许共享的类。
4. 当前图节点的动画资产兼容目标 Skeleton。
5. `ParentAssetOverrides.NewAsset` 不含意外的源动画路径。
6. CDO 的动画对象、容器和 Struct 不含意外的源动画路径。
7. Linked Anim Layer、接口、Notify、Control Rig 和类型引用符合白名单。
8. Asset Registry 的硬依赖和软依赖中没有意外源包。
9. 保存全部目标资产，重启编辑器，再次编译和扫描。
10. PIE 中覆盖全部武器、移动状态、瞄准、转身、跳跃、蒙太奇和图层切换。

Asset Registry 是包级依赖检查，不能定位具体属性。发现残留后，按“图节点、覆盖表、CDO、类和类型”四层定位。

## 8. MCP 自动化规则

1. 批次大小按本机耗时和内存调整，不写死固定数量。
2. 每批完成后保存结果和进度清单，脚本按目标资产是否已完成实现幂等重跑。
3. MCP 超时后先查询目标资产和编辑器状态。编辑器可能仍在执行任务。
4. 避免任何模态窗口。覆盖、删除和保存策略应提前写入脚本参数。
5. 长任务分阶段执行：动画资产、ABP 层级、图节点、CDO、类型引用、验证。
6. 每次插件或引擎升级后重新发现 Python 和 VibeUE API，旧方法名和参数不能视为稳定接口。

## 9. 最小完成标准

满足以下条件后，迁移才算完成：

1. 所有目标动画资产使用目标 Skeleton。
2. 所有目标 ABP 能编译。
3. 目标 ABP 层级不依赖旧 Skeleton 的父 ABP。
4. 图节点、覆盖表和 CDO 中的动画引用已映射。
5. 源目录仅剩明确允许共享的逻辑依赖。
6. 重启编辑器后结果保持不变。
7. PIE 验证通过。
8. 在备份或独立分支删除源动画资产后，项目仍能加载、编译和运行。

## 参考

1. Epic：IKRetargetBatchOperation Python API 5.8  
   https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/IKRetargetBatchOperation
2. Epic：Auto Retargeting，含 Include Referenced Assets 说明  
   https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-retargeting-in-unreal-engine
3. Epic：Animation Blueprint Override  
   https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprint-override-in-unreal-engine
4. Epic：Animation in Lyra  
   https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-in-lyra-sample-game-in-unreal-engine
5. Epic：UAnimBlueprint 与 ParentAssetOverrides  
   https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UAnimBlueprint
6. Epic：Consolidating Assets，Replace References 的删除行为  
   https://dev.epicgames.com/documentation/en-us/unreal-engine/consolidating-assets-in-unreal-engine
7. VibeUE  
   https://github.com/kevinpbuckley/VibeUE
