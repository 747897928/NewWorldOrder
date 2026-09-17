# CombatSystem - WeaponStateComponent 决策记录

日期：2026-01-30

## 结论
- 本项目不引入 Lyra 的 WeaponStateComponent。
- 命中反馈与严格校验不做 Lyra 级别实现。
- 维持现有 `UShootGameplayAbility_Weapon_Fire` TargetData 回传流程即可。
- 服务器权威仍由 `CommitAbility` / `ApplyCost` 保证。

## 理由
- 项目为 PVE 且非独服，过度校验会增加复杂度与服务器成本。
- WeaponStateComponent 主要服务玩家命中反馈（屏幕命中标记/确认），与核心伤害结算解耦。

## 相关代码
- `Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.cpp`
- `Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootAbilityCost_ItemTagStack.cpp`
- `Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.cpp`

## 相关参考（只读）
- `Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Weapons/LyraWeaponStateComponent.*`
