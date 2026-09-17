# C++ GameplayEffect SetByCaller 初始化与兼容规则

## 问题现象

- PIE 启动时出现 `SetByCaller ... Data None`。
- `ShootEffect_Passive_ArmorEnhancement` 与 `ShootEffect_Passive_MedicalExpertise` 虽由 GA 写入有效 GameplayTag 数值，运行时 Modifier 仍读取不到。
- 冷启动复读进一步确认 MoveSpeed、ReloadSpeed、ShieldWall、Heal、SnapshotRestore 也存在同类潜在缺陷。

## 根因

- 这些 C++ `UGameplayEffect` 在 CDO 构造函数中调用 `FGameplayTag::RequestGameplayTag(..., false)`，并把结果写入 `FSetByCallerFloat::DataTag`。
- GE CDO 可能早于 `FShootGameplayTags::InitializeNativeGameplayTags` 构造；此时请求返回无效 Tag，CDO Modifier 会永久保存 `None`，后续注册同名原生 Tag 不会回填旧 CDO。
- 这不是 GA 忘记写 Magnitude，也不能靠延迟 PIE 或 Live Coding 修好；必须修正 GE CDO 的持久配置并冷启动验证。

## 项目规则

1. C++ `UGameplayEffect` 构造函数中的 SetByCaller Modifier 使用稳定的 `DataName=FName("SetByCaller.*")`，不在 CDO 阶段请求 GameplayTag。
2. C++ 创建 `FGameplayEffectSpec` 后统一调用：

```cpp
FShootGameplayTags::SetSetByCallerMagnitude(Spec, DataTag, Magnitude);
```

3. 该 helper 同时写入：
   - FName 通道：供本项目 C++ GE 的 `DataName` 读取。
   - GameplayTag 通道：兼容蓝图 GE，以及 PlayerState 现有 ActiveEffect 快照保存/恢复格式。
4. 禁止在新调用点直接只写 `Spec.SetSetByCallerMagnitude(DataTag, Value)`；否则 C++ GE 的 FName Modifier 会再次读不到数值。
5. 修改 C++ GE 构造函数后必须关闭项目编辑器冷编译并重启。Live Coding 不会可靠重建已经存在的 GE CDO。

## 套件被动幂等规则

- `AShootPlayerState::ApplyGenderAbilityKit` 继承读档恢复、初始装配和角色切换三个入口的共同职责。
- 被动通过 `GiveAbilityAndActivateOnce` 激活后 AbilitySpec 会结束并移除，但它施加的永久 GE 会保留；因此仅检查 AbilitySpec 或只清理另一性别都不能防重复。
- 每次重建前必须清理 Male 与 Female 两个 KitTag 对应的 Ability 和 ActiveEffect，再授予当前性别套件。

## 2026-08-22 验收证据

- 冷编译成功，NewWorldOrder 编辑器重启后全部相关 GE CDO 的 `DataName` 复读正确；`DataTag` 为空是刻意设计。
- TestMap_SplitScreen：
  - 男性：`ShieldCapacityBonus = 0.2`，ArmorEnhancement ActiveEffect 1 份。
  - 女性：`HealingDoneMultiplier = 1.2`、`ReloadSpeedMultiplier = 1.05`，MedicalExpertise 与 SmartAssist 各 1 份。
- 对两名 PlayerState 分别连续调用两次 `ApplyGenderAbilityKit` 后，上述数值与 ActiveEffect 数量保持不变。
- 当前启动日志无 `Data None`、Blueprint Runtime Error 或 Accessed None。
