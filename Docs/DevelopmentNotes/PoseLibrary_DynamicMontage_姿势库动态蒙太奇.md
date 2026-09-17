# PoseLibrary DynamicMontage 姿势库动态蒙太奇

日期：2026-06-30
状态：[Windows PIE 验证通过] [男性姿势库待补]

# 背景

姿势库资源当前扫描到的是 `AnimSequence`，不是已经制作好的 `AnimMontage`。需求是玩家从 CommonUI 姿势库选择一个姿势，角色播放完成后自动回到原本 locomotion。

# 当前方案

- 姿势条目使用 DataAsset 配置，不在 C++ 写死资源路径。
- 客户端只提交 DataAsset 中的 `PoseIndex`。
- 服务器校验当前角色性别、索引和动画资源。
- 服务器调用角色 NetMulticast。
- 每台机器用 `UAnimInstance::PlaySlotAnimationAsDynamicMontage` 把 `AnimSequence` 动态包装成 Montage 播放。

# 必要条件

AnimBlueprint 的 AnimGraph 里必须有同名 Slot 节点。

当前 `/Game/Characters/Heroes/CC/Animations/Shared/Poses/DA_PoseLibrary` 中的 26 个姿势都是完整身体、非 Additive 的女性 `AnimSequence`，因此统一使用 `FullBody` Slot。正式男女 AnimBP 都在既有 locomotion、武器 Slot 与 FootPlant/Control Rig 链路上游承接 `FullBody`。

禁止把这些绝对姿势配置到 `UpperBodyAdditive`。该 Slot 的输出会进入 `Apply Additive`；把绝对骨骼变换当作增量再次叠加，会造成角色整体放大、肢体扭曲等现象。是否禁用手 IK、脚 IK 或 FK 必须按具体动画做视觉 A/B 后单独决定，不能为了修复本问题给全部姿势批量添加曲线。

# 不采用方案

- 直接调用 SkeletalMeshComponent 的 `PlayAnimation`：
  - 会切换组件动画模式，打断项目现有 AnimBlueprint、Mutable 动画层和 locomotion。
- 客户端直接传动画资源：
  - 多人下不安全，也不利于后续做解锁、性别、状态校验。

# 2026-08-16 验证结果

- 修复前 26 个绝对姿势全部配置为 `UpperBodyAdditive`，这是选择 `mma kick` 后角色数倍放大和扭曲的直接原因。
- 把 26 个条目的 `SlotName` 统一改为 `FullBody` 后，通过 `AShootPlayerController::RequestPlayPoseByIndex(22)` 实际播放 `mma kick`，Windows PIE 截图确认角色比例正常，无整身放大或扭曲。
- 女性正式 AnimBP 的最终 Shoe/Hair linked layer 链路保持生效，高跟鞋状态下播放 `mma kick` 仍保持正常比例。
- 当前 26 个条目的 `CompatibleGender` 全部是 `FEMALE`。男性请求同一索引会被服务器正确拒绝；这不是播放故障。男性需要后续建立独立姿势资产或补充标为 `MALE`/`UNKNOWN` 的条目后再做姿势视觉验收。
