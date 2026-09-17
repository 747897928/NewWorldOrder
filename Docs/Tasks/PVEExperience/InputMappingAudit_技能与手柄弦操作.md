# 技能与手柄弦操作输入审计

日期：2026-09-05

状态：最终手柄弦操作、技能 IA 类型、旧条件 IA 清理和设置页动作表迁移已完成；等待 PIE 做输入阻断与手感回归

## 1. 本文目的

- 记录 `/Game/Blueprints/Input/IMC_Default` 当前全部键鼠和手柄映射，避免后续把弦操作误判成四条重复按键。
- 明确黑神话悟空式 `LT + X/Y/A/B` 方案与本项目瞄准、射击、交互、跳跃和换弹的冲突边界。
- 最终修改仍只通过 Enhanced Input 资产完成，不在 C++ 中硬编码键盘或手柄分支。

## 2. 当前键鼠映射

| 功能 | Input Action | 当前键位 |
|---|---|---|
| 跳跃 | `IA_Jump` | SpaceBar |
| 移动 | `IA_Move` | W/S/A/D、方向键 |
| 视角 | `IA_Look` | Mouse2D |
| 交互 | `IA_Interact` | F |
| 技能槽 1 | `IA_Skill1` | Q |
| 技能槽 2 | `IA_Skill2` | E |
| 技能槽 3 | `IA_Skill3` | C |
| 技能槽 4 | `IA_Skill4` | X |
| 奔跑 | `IA_Run` | LeftShift |
| 蹲伏 | `IA_Crouch` | LeftControl |
| 换弹 | `IA_Reload` | R |
| 瞄准 | `IA_Aiming` | RightMouseButton |
| 攻击 | `IA_Attack` | LeftMouseButton |
| 打开菜单 | `IA_OpenMenu` | M |
| 下一武器 | `IA_WeaponNext` | MouseScrollUp |
| 上一武器 | `IA_WeaponPrevious` | MouseScrollDown |
| 丢弃武器 | `IA_WeaponDrop` | G |
| 切换视角 | `IA_ToggleCameraPerspective` | V，支持玩家改键 |

## 3. 当前手柄映射

| 功能 | Input Action | 当前按键 |
|---|---|---|
| 跳跃 | `IA_Jump` | FaceButton Bottom / A |
| 移动 | `IA_Move` | Left 2D |
| 视角 | `IA_Look` | Right 2D |
| 交互 | `IA_Interact` | FaceButton Top / Y |
| 奔跑 | `IA_Run` | Left Thumbstick |
| 蹲伏 | `IA_Crouch` | Right Thumbstick |
| 换弹 | `IA_Reload` | FaceButton Left / X |
| 瞄准 | `IA_Aiming` | Left Trigger Axis / LT |
| 攻击 | `IA_Attack` | Right Trigger Axis / RT |
| 打开菜单 | `IA_OpenMenu` | Special Right |
| 下一武器 | `IA_WeaponNext` | D-Pad Right |
| 上一武器 | `IA_WeaponPrevious` | D-Pad Left |
| 丢弃武器 | `IA_WeaponDrop` | D-Pad Down |

## 4. 当前四技能的最终弦操作方向

当前资产使用一个独立的 Boolean 修饰键 IA 映射 LB，再让四个正式槽 IA 分别占用四个面键：

| 正式槽 IA | 物理主键 | `InputTriggerChordAction::ChordAction` | 实际组合 |
|---|---|---|---|
| `IA_Skill1` | Y | `IA_SkillModifier` | LB + Y |
| `IA_Skill2` | X | `IA_SkillModifier` | LB + X |
| `IA_Skill3` | A | `IA_SkillModifier` | LB + A |
| `IA_Skill4` | B | `IA_SkillModifier` | LB + B |

`IA_SkillModifier` 只映射 Gamepad Left Shoulder（LB）。`IA_Skill1-4` 直接映射 Gamepad Face Button Top/Left/Bottom/Right
（Y/X/A/B），每条映射的 `InputTriggerChordAction::ChordAction` 都指向 `IA_SkillModifier`。正式技能输入仍然只有
`IA_Skill1-4 -> InputTag.Q/E/C/X -> ASC -> GA` 一条链；修饰键只参与 Enhanced Input 的弦条件，不单独授予技能。

`IA_Skill1-4` 与 `IA_SkillModifier` 现在全部使用 Boolean。旧的 `IA_SkillY/X/A/B` 已从资产链路移除，不保留第二套
只为读取面键状态的条件 Action。

旧方向下，四条正式技能映射的物理主键都是 LB，`InjectChordBlockers` 最多只能阻断其他 LB 映射，不能阻断 Y 上的交互、
X 上的换弹或 A 上的跳跃。当前方向把正式技能映射主键改为面键，`InputTriggerChordAction` 的阻断方向与基础动作共享物理主键一致；
组合键是否完全抑制基础动作，仍需在 PIE 中验证映射优先级和按键先后次序。

## 5. 黑神话悟空式 LT + X/Y/A/B 的项目冲突

- 用户补充的参考方案是 LT + X/Y/A/B，同样应使用 `InputTriggerChordAction`，不是在 C++ 判断两个物理键。
- 本项目 LT 已由 `IA_Aiming` 占用，RT 已由 `IA_Attack` 占用。直接把技能修饰键从 LB 改成 LT，会让玩家进入瞄准状态后再选技能，
  也可能在技能组合期间持续触发 ADS；这不是无冲突替换。
- X、Y、A 还分别承担换弹、交互和跳跃。无论修饰键选择 LB 还是 LT，都必须验证弦动作能否阻断对应裸动作，不能只验证技能 GA 被激活。
- D-Pad Left/Right/Down 已用于武器切换和丢弃，不能在未迁移这些功能前拿来填技能槽。

## 6. 已执行的调整与运行时验收边界

1. 已保留 LB 作为技能修饰键；LT 继续绑定 `IA_Aiming`，RT 继续绑定 `IA_Attack`。
2. 已创建并配置 Boolean `IA_SkillModifier`，只映射 LB。
3. 已将 `IA_Skill1-4` 直接映射到 Y/X/A/B，并让每条映射的 `InputTriggerChordAction::ChordAction` 指向
   `IA_SkillModifier`。
4. 已将 `IA_Skill1-4`、`IA_SkillModifier` 统一为 Boolean，并删除旧的 `IA_SkillY/X/A/B` 条件 IA。
5. 已按左到右 Y/X/A/B 固定四槽顺序；键鼠槽位仍为 Q/E/C/X，InputTag 与 InputAction 顺序不变。
6. 设置页与 CommonUI 动作提示已统一使用 Lyra 主表 `/Game/UI/DT_UniversalActions`；`W_LyraSettingScreen` 的返回、应用、
   取消和恢复默认行均已切换到该表。
7. `W_SkillSlot` 继续绑定 `IA_Skill1-4`。CommonUI 图标由当前输入设备和 Lyra ControllerData 决定，不在 Widget C++ 中固定手柄键。
8. PIE 必须同时验证：技能只触发对应槽；按住 LB 时不再触发 Y 交互、X 换弹、A 跳跃；不按 LB 时基础动作仍正常；轻点、
   先按面键再按 LB，以及松开顺序变化都不产生卡住或重复触发。

## 7. 方案结论

| 方案 | 结论 | 原因 |
|---|---|---|
| LB + Y/X/A/B | 推荐 | LB 当前没有独立玩法占用，保留 LT ADS、RT 攻击；正确配置 Chord 后可自动抑制面键基础动作。 |
| LT + Y/X/A/B | 不推荐 | LT 已是持续 ADS；除非重新定义整套瞄准状态机，否则组合期间会与瞄准竞争。 |
| D-Pad 四技能 | 不推荐 | Left/Right/Down 已承担武器切换和丢弃，且战斗中右手离开摇杆的成本高。 |

最终产品方案已经确定并落地：LB 是 `IA_SkillModifier`，四槽从左到右依次为 LB+Y、LB+X、LB+A、LB+B；LT/RT 继续承担
ADS/攻击。资产和设置页已完成静态核验，剩余工作是 PIE 验证实际阻断、触发顺序和图标显示。
