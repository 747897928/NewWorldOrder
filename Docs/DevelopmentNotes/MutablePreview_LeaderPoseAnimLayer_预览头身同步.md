# MutablePreview LeaderPoseAnimLayer 预览头身同步

日期：2026-06-28
状态：[可用] [Windows PIE逐帧复验通过] [待Mac与换装人工复验]

## 问题现象

在 `W_Cloth` 衣柜预览中，玩家点击服装后，预览角色会出现头部有动画、身体网格与头部分离或身体姿态不跟随的问题。切换一次男女角色后可能短暂恢复，但再次装备服装又会复现。

这不是单纯少一个蓝图节点。根因是 Mutable 的 `UpdateSkeletalMeshAsync` 会异步替换 `USkeletalMeshComponent` 上的网格，`ApplyAppearance` 返回时网格不一定已经更新完成。如果在旧网格状态下过早调用 `SetLeaderPoseComponent`，当新身体网格稍后替换进来时，身体可能丢失 LeaderPose 同步。

## 当前正确链路

正式角色：

- `AShootCharacter` 持有头部主网格 `GetMesh()` 和身体网格 `BodyMesh`。
- `BP_ShootCharacter` 构造脚本不设置 LeaderPose；构造时两侧仍可能没有最终 Mutable 网格。
- `UMutableAppearanceComponent::InitializeComponents` 绑定 `HeadCSkeletalComponent->GetInstanceUsage()->UpdatedDelegate`。
- Mutable 网格更新完成后进入 `UMutableAppearanceComponent::OnCustomizableSkeletalUpdated`。
- 同一 COI 的普通换衣不清除 `BodyMesh` 的 LeaderPose。UE 会在 `SetSkeletalMesh` 内部自动重建 follower 的 `LeaderBoneMap`，从而保持异步生成期间头身连续同步；只有男女 COI 真正切换前才清除旧映射。
- Updated 回调发生在同一 COI 的 Head/Body 网格均替换完成后；Head 可以有额外面部骨骼，组件不比较 Skeleton 指针或总骨骼数，而是在设置 LeaderPose 后立即核验 `LeaderBoneMap` 是否覆盖 Body 的全部参考骨骼。不完整时立刻解绑，不让该映射进入渲染线程。
- 回调读取 `UCustomizableObjectInstance::GetAnimationGameplayTags()`，再通过 `GetAnimBP(ComponentName, SelectedOptionName)` 获取鞋子、头发、服装等选项对应的动画层蓝图。
- 新增动画标签调用 `HeadMesh->LinkAnimClassLayers(AnimBP)`，移除动画标签调用 `HeadMesh->UnlinkAnimClassLayers(AnimBP)`。
- 服务器正式角色才把最终外观标签写回 `AShootPlayerState`。

衣柜预览：

- `/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 继承自 `AShootWardrobePreviewActor`。
- `AShootWardrobePreviewActor` 在 UMG Viewport 的独立预览世界中生成，不是场景里的真实玩家角色。
- 预览 Actor 没有 `AShootPlayerState`，但仍必须绑定 Mutable Updated 回调。
- `UMutableAppearanceComponent::InitializePreviewComponents` 设置预览专用 Head/Body CSC、Head/Body Mesh 和 `PreviewGender`，并绑定 `UpdatedDelegate`。
- Mutable 更新完成后，预览也进入 `OnCustomizableSkeletalUpdated`，执行动画层链接，但不写回 PlayerState。
- 预览 Actor 不直接绑定 LeaderPose；它只监听 `OnMutableSkeletalMeshUpdated` 以播放试穿表现。
- 正式角色和预览角色都由 `UMutableAppearanceComponent` 在同一条更新完成链路中校验并重建 LeaderPose。

## 正式男女 AnimBP 的 Shoe/Hair 输出顺序

正式角色不能只让衣柜预览 AnimBP 实现鞋子和头发接口。以下两个基础 AnimBP 都必须实现 `ShoeAnimLayerInterface` 与 `HairAnimLayerInterface`：

- `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base`
- `/Game/Characters/Heroes/CC/MM/Animations/ABP_Mannequin_Base`

两个正式 AnimGraph 的最终输出顺序统一为：

```text
既有 locomotion / 武器 Slot / FullBody / DefaultSlot
  -> 既有 FootPlant 与 Control Rig
  -> ShoeAnimationLayer
  -> HairAnimationLayer
  -> Output Pose
```

顺序理由：

- 高跟鞋 AnimLayer 当前会旋转 `foot_l`、`foot_r`、`ball_l`、`ball_r` 并抬高 pelvis，是最终鞋型几何补偿，不是通用 Foot IK。放在 Control Rig 之前可能被后续落脚修正覆盖。
- 所有 Slot 都位于 Shoe 层上游，所以姿势、武器 Montage 和普通 locomotion 最终都会经过当前鞋型补偿；不需要为高跟鞋和平底鞋复制两套角色模型或整套动画。
- Hair 层放在最后，读取已经包含落脚、骨盆和鞋型补偿的最终身体姿势，再执行头发骨骼物理。
- 基础 AnimBP 自身的 `ShoeAnimationLayer`、`HairAnimationLayer` 实现图必须保持 `InPose -> Result` 直通。Mutable 选项对应的 AnimBP 通过 `LinkAnimClassLayers` 覆盖实现；没有选中带动画层的外观时，直通实现保证基础姿势不丢失。

2026-08-16 Windows PIE 复验：

- 女性运行时主 AnimInstance 为 MF Base；从 Sneakers 切换到 Heels 后，Sneakers linked instance 卸载、Heels linked instance 创建，BunHair linked instance 保持存在。
- 高跟鞋状态通过正式 `RequestPlayPoseByIndex(22)` 播放 `mma kick`，角色比例、鞋型和头发均正常。
- 通过 `AShootPlayerState::SwitchToCharacter(MALE)` 切换男性后，运行时主 AnimInstance 为 MM Base；`SneakersMaleAnimBlueprint` 与 `EdgeCutHairHairAnimBlueprint` 均成功创建 linked layer instance。
- Mutable 更新是异步的。切换性别或外观后立刻查询 linked instance 可能暂时得到空值；必须等 `UpdatedDelegate` 完成并确认外观 GameplayTag 已更新后再判定成功或失败。

## 为什么预览也要处理 AnimLayer

Mutable 的 CO 里可以为选项配置动画蓝图。例如：

- `/Game/Characters/Heroes/CC/Animations/LinkedLayers/ALI_ItemAnimLayers`
- `/Game/Characters/Heroes/CC/Animations/LinkedLayers/HairAnimLayerInterface`
- `/Game/Characters/Heroes/CC/Animations/LinkedLayers/ShoeAnimLayerInterface`
- `/Game/Characters/Heroes/CC/Customization/Shoes/HighHeels/HighHeelsAnimBlueprint`

高跟鞋、鞋子、头发、服装可能会通过 `GetAnimationGameplayTags()` 和 `GetAnimBP()` 返回需要 Link 的动画层。如果预览模式因为没有 PlayerState 直接 return，预览角色就不会加载对应动画层，导致“真实角色正常、预览角色异常”的分叉。

预览模式的规则是：

- 可以读 Mutable 的动画标签。
- 可以对 `HeadMesh` Link/Unlink AnimClassLayers。
- 可以广播给预览 Actor 播放试穿表现。
- 禁止写回 PlayerState。
- 禁止写存档。

## AnimInstance 生命周期规则

`SetAnimInstanceClass` 接收的是 AnimBlueprint 类引用，不是让业务代码手动 `NewObject` 一个 AnimInstance。项目里配置的仍然是正式 CC 男女基础 AnimBP：

- `/Game/Characters/Heroes/CC/MF/Animations/ABP_Mannequin_Base`
- `/Game/Characters/Heroes/CC/MM/Animations/ABP_Mannequin_Base`

但 UE 5.8 的 `USkeletalMeshComponent::SetAnimInstanceClass` 在 `NewClass != AnimClass` 或当前不处于 AnimationBlueprint 模式时，会执行 `ClearAnimScriptInstance()` 和 `InitAnim(true)`，重新初始化该 SkeletalMeshComponent 内部持有的 `AnimScriptInstance`。

因此它只能用于男女基础 AnimBP 真正变化的场景，例如从女主切到男主，或预览 Actor 第一次初始化基础动画类。

同一性别换衣服、换鞋、换头发、换披风时，不允许再次调用 `SetAnimInstanceClass`。这些操作只应该：

- 在当前 `UCustomizableObjectInstance` 上应用 Mutable 参数标签。
- 等 `UpdateSkeletalMeshAsync` 完成后读取 `GetAnimationGameplayTags()`。
- 对新增标签调用 `GetAnimBP(ComponentName, SelectedOptionName)`。
- 对返回的 AnimBP 统一调用 `HeadMesh->LinkAnimClassLayers(AnimBP)`。

这条规则不是为了高跟鞋写特判。高跟鞋、披风、头发或后续任何带动画层的外观项，都必须走同一条 Mutable 动画标签到 AnimLayer 的通用链路。

如果同一性别换装误调用 `SetAnimInstanceClass`，可能出现：

- AnimInstance 被重建，之前 `LinkAnimClassLayers` 挂上的动画层丢失。
- 组件里的 `LastAnimationTags` 缓存还以为这些标签已经处理过，下一次 `AddedTags` 为空。
- 结果就是高跟鞋等动画层在预览中看起来没有生效。

当前 C++ 约束：

- `UMutableAppearanceComponent::SwitchGenderInstance` 只有在目标基础 AnimClass 和当前 `HeadMesh->GetAnimClass()` 不一致时，才调用 `SetAnimInstanceClass`。
- 只有基础 AnimClass 真正变化时才清空 `LastAnimationTags` 和 GameplayTag 到 AnimBP 的缓存。
- `UMutableAppearanceComponent::ApplyPreviewAppearanceTags` 可以频繁进入，但同性别换装不会重建 AnimInstance。

## 编辑器冷启动的编译模型就绪边界

UE 5.8 编辑器冷启动时，`FSoftObjectPath::TryLoad` 返回 `UCustomizableObject` 只表示资产包已经进入内存，不表示 Mutable 的编译模型已经从磁盘或 DDC 恢复完成。此时 `UCustomizableObject::IsLoading()` 可能为 true，`IsCompiled()` 为 false。若立刻创建 COI 并调用 `SetIntParameterSelectedOption`，Mutable 会拒绝参数写入；模型稍后加载完成也不会自动重放 PlayerState 标签，角色会一直显示完整兜底身体、无衣服和无头发。

正式角色与衣柜预览统一遵守以下顺序：

- `TryLoad` 后持有 `UCustomizableObject`，但只以官方公开的 `IsCompiled()` 作为实例化放行条件。
- `IsLoading()` 为 true 时只等待加载完成，不写参数、不触发网格更新。
- 编辑器中若加载已结束但仍未编译，通过 `ConditionalAutoCompile()` 请求 Mutable 官方自动编译；不直接依赖编辑器模块私有接口。
- 组件用 World Timer 定期重新检查就绪条件。0.05 秒只是检查频率，不是固定延时后假定成功；只有 `IsCompiled()` 为 true 才继续。
- 就绪后才创建男女 COI、绑定目标性别，并从 PlayerState 重新读取最新外观标签；预览模式重放等待期间保存的最新预览标签。
- EndPlay 和 BeginDestroy 都清理检查 Timer，不让旧世界或已销毁组件保留回调。

这条就绪检查与下文禁止的 LeaderPose Tick 重试不是一回事。LeaderPose 仍只由 Mutable 的 `UpdatedDelegate` 驱动；Timer 只覆盖 Mutable 官方明确说明“可能需要数帧”的编译模型加载阶段。

## LeaderPose 设置时机

当前结论：普通换衣必须保留现有 LeaderPose；只有初次生成或真正切换男女 COI 时，才先隐藏 Body 并解除旧映射。每次 Mutable 更新完成后，都用最终 Head/Body 网格重新校验 LeaderPose。

原因：

- UE 5.8 的 `USkeletalMeshComponent::SetSkeletalMesh` 会自动为 follower 重建 `LeaderBoneMap`。因此同一 COI 的换衣必须保留当前 LeaderPose；提前设为 `nullptr` 会在 Mutable 异步更新完成前产生可见的头身不同步闪帧。只有切换男女 COI、可能先写入另一套参考网格时才清除旧映射。
- Mutable 在替换该 COI 的全部 usage 后才调用 usage 的 `UpdatedDelegate`，所以 `OnCustomizableSkeletalUpdated` 是正式角色与预览角色共享的唯一安全绑定点。
- 不使用 Tick 或延时猜测时序；每次绑定前必须检查 Head/Body 都有 SkeletalMesh；设置后必须确认 `BodyMesh->GetLeaderBoneMap().Num()` 等于 Body 的参考骨骼数。Head 有额外面部骨骼时，只要其包含 Body 所需骨骼，仍可合法作为 Leader。

当前 C++ 规则：

- `AShootWardrobePreviewActor` 禁止用 Tick 重试 LeaderPose。
- `AShootWardrobePreviewActor` 禁止给 BodyMesh 设置独立 AnimClass 作为 fallback。
- `AShootWardrobePreviewActor` 不拥有任何 LeaderPose 绑定函数；它不能绕过 `UMutableAppearanceComponent`。
- 正式角色和衣柜预览 Actor 的 Body 初始隐藏；Mutable 完成并建立完整 `LeaderBoneMap` 后才显示。
- 骨架校验失败时组件只记录一次 Head/Body 网格、Skeleton、骨骼数和映射数量，并保持 Body 隐藏，不把错误映射送入渲染线程。
- 如果未来某套 Head/Body 资源无法 LeaderPose，同步问题应优先检查 Mutable CO 的骨架配置，而不是在预览 Actor 里增加新的 fallback 分支。

## 维护边界

C++ 职责：

- `UMutableAppearanceComponent`：负责正式角色与预览角色共同的 Mutable 标签、动画层和 Updated 回调。
- `AShootWardrobePreviewActor`：负责 UMG Viewport 独立预览世界中的试穿表现、相机锚点和影棚组件。

蓝图职责：

- `/Game/UI/Mutable/BP_ShootWardrobePreviewActor`：调灯光、相机锚点、背景、后处理、`PreviewFacingYawOffset`。
- `/Game/UI/Mutable/W_Cloth`：调布局、按钮、页签、列表、Tooltip 和预览 Viewport 的屏幕位置。

如果出现角色不是面向玩家：

- 优先调 `/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 上的 `PreviewFacingYawOffset`。
- 不要在 `W_Cloth` 里写死角色旋转。

如果出现角色被 UI 挡住：

- 优先调 `W_Cloth` 中预览 Viewport、服装列表和详情区的布局锚点。
- 如果角色构图本身不对，再调 `BP_ShootWardrobePreviewActor` 的相机锚点。

如果出现头身动画再次分离：

- 先检查 `UMutableAppearanceComponent::InitializePreviewComponents` 是否仍绑定 `UpdatedDelegate`。
- 再检查普通换衣是否错误进入 `PrepareBodyForMutableInstanceSwitch`；该函数只允许初次实例绑定和男女 COI 切换调用。
- 最后检查 Mutable CO 的 Head/Body 是否仍使用相同 Skeleton、相同参考骨骼数的可 LeaderPose 配置。

## 当前纠偏记录

2026-07-18 修正：

- 删除 `AShootWardrobePreviewActor` 的 Tick 重试链路。
- 删除蓝图 Construction Script 和预览 Actor 对 LeaderPose 的直接绑定。
- 删除 BodyMesh 独立 AnimClass fallback。
- `UMutableAppearanceComponent` 只在 COI 真正切换前清除旧 LeaderPose，在 Updated 回调中校验并重建；仍负责通用动画层 Link/Unlink，不回写预览的 PlayerState 或 SaveGame。

2026-07-20 首帧重叠修正：

- MCP 逐帧采样确认场景始终只有一个 `BP_ShootCharacter` Pawn，不是网络重复出生。
- 第 1 tick 时 Head 与 Body 两个组件都引用完整 `ChenHaoYu` 兜底网格；两者同时可见才造成“两个角色重叠”的截图。
- `AShootCharacter` 和 `AShootWardrobePreviewActor` 创建 Body 组件时先隐藏；`UMutableAppearanceComponent` 仅在最终网格与 LeaderPose 完整后显示 Body。
- 修复后第 1 tick 为 Head=`ChenHaoYu` 可见、Body=`ChenHaoYu` 隐藏；第 3 tick 为 Head=`head_2`、Body=`Body_2`、Leader=`CharacterMesh0` 且 Body 可见。
- 同轮 PIE 中 `Leader pose component skeleton doesn't match follower` 为 0 次。

## 复验

- `HomeMap -> Play -> 打开 W_Cloth -> 装备/卸下上衣、裤子、鞋子、发型`。
- 重点看高跟鞋是否加载对应动画层。
- 重点看换衣后头部和身体是否保持同一姿态。
- 重点看切换男女角色后再次装备是否仍稳定。
- 旋转、缩放、重置视角 UI 仍需要单独恢复和验收，本笔记只覆盖头身同步与动画层。
