# RegisterUIActionBinding 学习笔记

note_id: `ui-002-register-ui-action-binding`
category: UI
status: Active
last_updated: 2026-03-18

## 它是什么

- `RegisterUIActionBinding(const FBindUIActionArgs&)` 是 CommonUI 给 Widget 提供的动作监听接口。
- 它的作用是：
  - 让某个具体 Widget 对某个 UI 动作作出响应
  - 可绑定执行回调
  - 可绑定按住时的进度、按下、释放等回调

常见入口：

```cpp
FUIActionBindingHandle Handle = RegisterUIActionBinding(
    FBindUIActionArgs(SomeInputAction, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleAction))
);
```

或：

```cpp
FUIActionBindingHandle Handle = RegisterUIActionBinding(
    FBindUIActionArgs(SomeRowHandle, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleAction))
);
```

## 它不是什么

- 它不是全局监听器。
- 它不是 `UInputAction` 的替代品。
- 它不是 `Input Mapping Context` 的替代品。
- 它也不是“注册后不管输入上下文怎么切都能收到”的万能入口。

中文结论：
- 它只是 Widget 对动作的响应绑定。

## 它受什么限制

UE 5.7 CommonUI 源码里，这条绑定至少会受下面因素影响：

- 当前 Widget 是否可达、可接收输入
- 当前 `ECommonInputMode` 是否匹配
- 如果绑定的是 `UInputAction`
  - 当前激活的 IMC 里是否真的把这个动作映射到了键

中文结论：
- 你切换 IMC 之后，`RegisterUIActionBinding` 是否还能收到输入，会直接变化。

## 它和 `UInputAction` / `IMC` 的关系

正确分工：

- `UInputAction`
  - 定义动作本身
- `Input Mapping Context`
  - 定义当前上下文里这个动作由什么键触发
- `RegisterUIActionBinding`
  - 让 Widget 响应这个动作

中文结论：
- `IA` 定义动作
- `IMC` 定义键位
- `RegisterUIActionBinding` 定义 Widget 怎么响应

## 它和长按的关系

这部分最容易误解。

### 情况 A：绑定 CommonUI 的 Action Data / DataTable 行

- 这条路径更接近 CommonUI 原生设计。
- CommonUI 自己有 hold / rollback 机制。
- 这时可以通过：
  - `OnHoldActionProgressed`
  - `OnHoldActionPressed`
  - `OnHoldActionReleased`
  - 拿到进度和释放事件

适合：
- 想完全按 CommonUI 传统方式做按钮/Action Bar
- 动作本身主要是 UI 动作

### 情况 B：绑定纯 `UInputAction`

- UE 5.7 源码里，`FBindUIActionArgs(UInputAction*)` 这条路径会记录动作，并通过当前 IMC 查询键位。
- 但不要默认假设它就能自动给你一套完整的 CommonUI 风格长按进度和松手回落。

中文结论：
- 纯 `UInputAction` 绑定更像“让 Widget 能收到这个动作”
- 不是“直接送你一个完整的长按状态机”

## 为什么这对当前项目重要

当前角色切换菜单已经明确采用：

- `IA_SwitchCharacter`
- `IMC_CharacterSwitchMenu`

并且用户明确要求：

- 菜单输入上下文和玩法输入上下文隔离
- 长按支持手柄
- 松手时进度条要回落
- 主菜单相关输入都走菜单 IMC

在这个前提下，当前更稳的实现不是把长按语义硬塞给 `RegisterUIActionBinding`，而是：

- `IA_SwitchCharacter` 负责动作
- `IMC_CharacterSwitchMenu` 负责键位
- Enhanced Input Trigger 负责“什么时候认为这是长按”
- C++ 自己负责：
  - 进度更新
  - 松手回落
  - 完成提交

## 当前项目推荐做法

### 方案 B：独立切换键 + 手动 hold

- `IA_SwitchCharacter`
  - `Value Type = Digital(bool)`
  - Trigger：使用 `Hold`，不要叠 `Pressed + Released`
  - 不推荐第一版使用 `Hold And Release`
  - `HoldTimeThreshold`：0.6 秒
  - `bIsOneShot = true`
  - `bAffectedByTimeDilation = false`
- `IMC_CharacterSwitchMenu`
  - 键盘：E
  - 手柄：X
- `DT_UniversalActions`
  - 仅负责 `Input_SwitchCharacter` 的图标和提示

监听方式：

- 在前端菜单输入接收者里绑定：
  - `Started`
  - `Ongoing`
  - `Triggered`
  - `Canceled`
  - `Completed` 可选，仅用于成功后的收尾，不作为主逻辑入口
- `Started`
  - 显示进度条
- `Ongoing`
  - 用 `FInputActionInstance::GetElapsedTime()` 更新进度
- `Triggered`
  - 达到阈值，正式发起切换
- `Canceled`
  - 中途松手，启动回落

回落：

- Enhanced Input 自己没有“回落时间”这个概念
- 回落是 UI / C++ 自己的逻辑
- 例如：
  - `HoldTime = 0.6f`
  - `RollbackTime = 0.25f`

中文结论：
- 长按多久，是 `IA_SwitchCharacter` Trigger 的事情
- 松手回落多久，是你自己的 UI/C++ 状态机的事情

## 为什么当前选 `Hold`，不选 `Hold And Release`

- `Hold`
  - 达到阈值当帧就会触发 `Triggered`
  - 更适合“进度条填满立刻切换”的菜单体验
  - 用户在阈值前松手，会收到 `Canceled`，正好接回落逻辑
- `Hold And Release`
  - 必须先按满阈值，再松手才触发
  - 更适合“蓄力后松手执行”的动作
  - 不适合当前“进度满即切换角色”的需求

中文结论：
- 菜单切换第一版应选 `Hold`
- 不带 `Release`

## `Affected by Time Dilation` 要不要勾

- 当前菜单角色切换不建议勾选。
- 引擎源码里，`UInputTriggerTimedBase::CalculateHeldDuration()` 只有在 `bAffectedByTimeDilation = true` 时，才会把 `PlayerInput->GetEffectiveTimeDilation()` 乘进持续时间。
- 菜单长按属于 UI 输入，不应该因为慢动作、暂停或世界时间膨胀而变慢或变快。
- 只有当你明确希望“游戏世界慢动作时，长按判定也一起变慢”时，才考虑打开。

中文结论：
- 当前项目菜单切换：`bAffectedByTimeDilation = false`

## 什么时候再考虑 `RegisterUIActionBinding`

它仍然有用，但放在正确的位置：

- 处理菜单里的普通动作执行
- 处理 CommonUI 传统按钮动作
- 处理不需要自定义 hold 状态机的动作

如果后续角色切换菜单改成：
- 焦点在按钮上
- 按住通用确认键切换
- 并且愿意完全按 CommonUI Action Data 方式配置

再回头把长按交给 `RegisterUIActionBinding + CommonUI Action Data` 也可以。

但在当前项目约束下，不建议再把它当作“菜单独立切换键长按”的唯一核心方案。

## 参考

- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonUserWidget.h`
- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/Input/CommonUIInputTypes.h`
- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Input/UIActionRouterTypes.cpp`
- `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputComponent.h`
- `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/InputAction.h`
- `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/InputTriggers.h`
