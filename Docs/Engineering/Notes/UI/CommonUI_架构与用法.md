# CommonUI 架构与用法

**note_id**: `ui-001-commongui-architecture`
**category**: UI
**status**:  Active
**last_updated**: 2026-09-07

## 概述

本项目使用 Lyra 改进版的 CommonUI 系统进行 UI 管理。该系统基于 UE5 的 CommonUI 插件，通过 GameplayTag 驱动的分层架构实现灵活的 UI 管理。

## 核心架构

### 类层次结构

```
UCommonActivatableWidget (CommonUI插件基类)
    ↓
ULyraActivatableWidget (项目改进版基类)
    ↓
具体 UI Widget (如 UShootReticleWidgetBase)
```

### UI 管理系统

```
GameUIManagerSubsystem (全局UI管理子系统)
    ↓
GameUIPolicy (策略模式，管理多玩家布局)
    ↓
PrimaryGameLayout (每个玩家的根UI布局)
    ↓
UI Layers (GameplayTag驱动的多层UI容器)
```

## ULyraActivatableWidget 改进

相比 CommonUI 原生的 `UCommonActivatableWidget`，项目的 `ULyraActivatableWidget` 添加了自动输入配置功能：

```cpp
// 路径: Source/NewWorldOrder/Public/UI/LyraActivatableWidget.h

UENUM(BlueprintType)
enum class ELyraWidgetInputMode : uint8
{
    Default,        // 使用默认输入模式
    GameAndMenu,    // 游戏和菜单都接收输入
    Game,           // 仅游戏接收输入
    Menu            // 仅菜单接收输入
};

UCLASS(Abstract, Blueprintable)
class ULyraActivatableWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

    UPROPERTY(EditDefaultsOnly, Category = Input)
    ELyraWidgetInputMode InputConfig = ELyraWidgetInputMode::Default;

    UPROPERTY(EditDefaultsOnly, Category = Input)
    EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
```

**关键特性**：
- 自动配置输入模式（游戏/菜单/混合）
- 自动配置鼠标捕获模式
- Widget 激活时自动应用输入配置

## UI 层级系统 (Layer System)

### GameplayTag 驱动的层级

UI 通过 GameplayTag 组织成多个层级，常见层级包括：

- `UI.Layer.HUD` - HUD层（准星、血条等）
- `UI.Layer.Menu` - 菜单层
- `UI.Layer.Modal` - 模态对话框层
- `UI.Layer.Game` - 游戏内UI层

### PrimaryGameLayout

```cpp
// 路径: Plugins/CommonGame/Source/Public/PrimaryGameLayout.h

UCLASS(Abstract, meta = (DisableNativeTick))
class UPrimaryGameLayout : public UCommonUserWidget
{
public:
    // 获取主玩家的 PrimaryGameLayout
    static UPrimaryGameLayout* GetPrimaryGameLayoutForPrimaryPlayer(const UObject* WorldContextObject);

    // 注册层级
    void RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget);

    // 获取层级容器
    UCommonActivatableWidgetContainerBase* GetLayerWidget(FGameplayTag LayerName);

    // 推送 Widget 到层级栈（同步）
    template <typename ActivatableWidgetT = UCommonActivatableWidget>
    ActivatableWidgetT* PushWidgetToLayerStack(FGameplayTag LayerName, UClass* ActivatableWidgetClass);

    // 推送 Widget 到层级栈（异步加载）
    template <typename ActivatableWidgetT = UCommonActivatableWidget>
    TSharedPtr<FStreamableHandle> PushWidgetToLayerStackAsync(/*...*/);

    // 从层级中移除 Widget
    void FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* ActivatableWidget);

private:
    // 层级映射表
    TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};
```

## UI 操作标准方法

### 显示 UI：PushContentToLayerForPlayer

**蓝图节点名称**: `Push Content to Layer for Player`

**C++ 实现类**: `UAsyncAction_PushContentToLayerForPlayer`

**位置**: `Plugins/CommonGame/Source/Public/Actions/AsyncAction_PushContentToLayerForPlayer.h`

```cpp
UCLASS(MinimalAPI, BlueprintType)
class UAsyncAction_PushContentToLayerForPlayer : public UCancellableAsyncAction
{
public:
    // 蓝图可调用的静态方法
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly="true"))
    static UAsyncAction_PushContentToLayerForPlayer* PushContentToLayerForPlayer(
        APlayerController* OwningPlayer,                                    // 玩家控制器
        UPARAM(meta = (AllowAbstract=false)) TSoftClassPtr<UCommonActivatableWidget> WidgetClass,  // Widget 类（支持异步加载）
        UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerName,    // 目标层级（必须是 UI.Layer.* 标签）
        bool bSuspendInputUntilComplete = true                              // 加载期间是否暂停输入
    );

    // 推送前回调（Widget已创建但未推送到层级）
    UPROPERTY(BlueprintAssignable)
    FPushContentToLayerForPlayerAsyncDelegate BeforePush;

    // 推送后回调（Widget已推送到层级并激活）
    UPROPERTY(BlueprintAssignable)
    FPushContentToLayerForPlayerAsyncDelegate AfterPush;
};
```

**使用示例**：

```cpp
// C++ 中使用
UAsyncAction_PushContentToLayerForPlayer* Action = UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(
    PlayerController,
    MyWidgetClass,
    FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Menu")),
    true  // 加载期间暂停输入
);

Action->BeforePush.AddDynamic(this, &UMyClass::OnBeforePush);
Action->AfterPush.AddDynamic(this, &UMyClass::OnAfterPush);
Action->Activate();
```

**关键特性**：
- 支持异步加载（`TSoftClassPtr`）
- 提供加载前后回调
- 可选择加载期间暂停输入
- 自动处理 StreamableHandle 生命周期

### 关闭 UI：DeactivateWidget

**方法来源**: 继承自 `UCommonActivatableWidget`

**使用**: 直接在 Widget 实例上调用

```cpp
// C++ 中使用
void UMyWidget::CloseThisWidget()
{
    DeactivateWidget();
}
```

```blueprint
// 蓝图中使用
DeactivateWidget (Target: Self)
```

**行为**：
- 触发 Widget 的反激活流程
- 从所属层级栈中移除 Widget
- 恢复之前的输入配置
- 可选择性销毁 Widget（取决于配置）

## CommonUI + Enhanced Input 长按（C++）

### 结论

菜单里的长按确认，不需要经过角色 `UShootInputComponent -> InputTag -> ASC` 这条游戏输入链。

CommonUI 自己就有一套适合菜单层的长按机制，而且支持增强输入与手柄。核心入口有两种：

1. Widget 级 C++ 绑定
   - `UCommonUserWidget::RegisterUIActionBinding(const FBindUIActionArgs&)`
   - `FBindUIActionArgs` 可直接绑定 `UInputAction`
   - 可监听：
     - `OnHoldActionProgressed`
     - `OnHoldActionPressed`
     - `OnHoldActionReleased`
   - 可通过 `bForceHold` 强制把绑定动作按“长按”处理

2. 按钮级长按
   - `UCommonButtonBase`
   - 关键字段：
     - `TriggeringEnhancedInputAction`
     - `bRequiresHold`
     - `HoldData`
   - 关键回调：
     - `OnActionProgress`
     - `OnActionComplete`

### 项目里的正确分工

- 游戏内角色技能/武器输入：
  - 继续走 `AShootCharacter::SetupPlayerInputComponent`
  - 继续走 `UShootInputComponent::BindAbilityActions`
  - 继续走 `UShootAbilitySystemComponent::AbilityInputTagPressed / Released / Held`
- CommonUI 菜单里的角色切换长按：
  - 不走 GAS 输入标签
  - 不走 `UShootGA_Interact`
  - 直接由菜单 Widget 在 CommonUI 层处理长按

中文结论：
- 游戏玩法输入和菜单输入不是一条链。
- 菜单入口应该走 CommonUI Action Router。

### 为什么菜单长按不该复用 `AbilityInputTagHeld`

- `AbilityInputTagHeld` 是给角色身上的 Gameplay Ability 用的。
- 菜单长按属于 UI 确认流程，不属于 Pawn 身上的战斗/交互能力。
- 如果把菜单长按塞回 ASC 输入链，会重新把“菜单入口”和“世界交互入口”搅在一起。

### 对本项目的推荐实现

- 角色切换菜单做一个 C++ 基类 Widget，继承 `ULyraActivatableWidget`
- 激活时使用专用 UI Mapping Context
  - 当前仓库可优先核对并复用 `IMC_Confirm_Select_Character`
- 使用 `IA_Confirm` 或专用 `IA_SwitchCharacter` 作为动作
- 在 C++ 中注册 `RegisterUIActionBinding(FBindUIActionArgs(InputAction, ...))`
- 用 hold 委托驱动进度条
- 长按完成后调用控制器的“角色切换请求入口”，再由服务器执行真正切换

### 输入上下文切换（Input Mapping Context）

- `UCommonActivatableWidget` 本身就有：
  - `InputMapping`
  - `InputMappingPriority`
- UE 5.7 的 CommonUI 源码里，Widget 激活时会自动：
  - `UEnhancedInputLocalPlayerSubsystem::AddMappingContext(InputMapping, InputMappingPriority)`
- Widget 反激活时会自动：
  - `RemoveMappingContext(InputMapping)`
- 对角色切换菜单，优先推荐这种方式：
  - 常规前端导航继续走 `IMC_FrontEnd`
  - 角色切换面板激活时，再叠加 `IMC_Confirm_Select_Character`
  - 面板关闭时自动移除，不需要自己手写一套切入/切出代码
- 只有下面两种情况，才建议手动调 `AddMappingContext / RemoveMappingContext`：
  - 输入上下文不跟某个 ActivatableWidget 生命周期绑定
  - 需要由 Controller/Subsystem 跨多个 Widget 统一管理一组临时输入层

### Trigger 选择建议

- 菜单层如果准备让 CommonUI 自己管理长按进度：
  - 优先用普通确认动作
  - 不要先在 `UInputAction` 上叠一层 `Hold` / `Hold And Release`
- 原因：
  - CommonUI 自己已经有 hold 生命周期和进度回调
  - 如果同一个动作同时用了 Enhanced Input 的 Hold Trigger 和 CommonUI 的 hold，语义会叠两层，后续排查会很混乱
- 角色切换菜单第一版推荐：
  - `IA_Confirm` 或专用 `IA_SwitchCharacter`
  - Trigger 保持简单（`Pressed` 或默认数字按钮语义）
  - 长按阈值交给 CommonUI 的 `bForceHold` / `HoldData`
- `Hold`
  - 更适合“到阈值即触发”的纯增强输入场景
  - 例如玩法输入、非 CommonUI 流程
- `Hold And Release`
  - 更适合“达到阈值后，松手才提交”的增强输入语义
  - 如果后续真想做“松手才确认”，也优先先在 CommonUI 层实现；不要一开始就在输入资产上改复杂 Trigger

### Generic Input Action 元数据注意点

- `UCommonInputMetadata::bIsGenericInputAction` 会影响 CommonUI 如何处理这个增强输入动作。
- UE 5.7 CommonUI 源码里，非 Generic 动作会优先注入 Enhanced Input，而不是直接执行 Widget 绑定委托。
- 这意味着：
  - 如果菜单角色切换想走 `RegisterUIActionBinding(FBindUIActionArgs(...))` 这条 Widget 级绑定路径
  - 那么确认动作更适合作为 UI Generic Action 处理
  - 否则 CommonUI 可能把动作注入到增强输入，而不是直接触发 Widget 里的执行委托
- 中文结论：
  - 菜单确认输入要么老老实实走 CommonUI Generic Action
  - 要么就彻底改成自己监听 Enhanced Input
  - 不要一半走 CommonUI，一半走增强输入注入

### 本项目当前推荐组合

- 主菜单 / 角色切换面板：
  - 基类：`ULyraActivatableWidget`
  - 输入上下文：`IMC_FrontEnd` + 面板激活时叠加 `IMC_Confirm_Select_Character`
  - 动作：
    - 通用确认优先复用 `IA_Confirm`
    - 如果角色切换菜单要占用独立按键位，例如手柄 X、键盘 E，则新增专用 `IA_SwitchCharacter`
  - 元数据：按 UI Generic Action 使用
  - 长按实现：`RegisterUIActionBinding(FBindUIActionArgs)` 或 `UCommonButtonBase`
  - 结果提交：`AShootPlayerController::RequestSwitchCharacter`
- 游戏玩法层：
  - 继续使用 `UShootInputComponent` + `UShootInputConfig` + `InputTag`
  - 不让菜单长按回流到 `UShootAbilitySystemComponent::AbilityInputTagHeld`

### `RegisterUIActionBinding` 和新建 `IA / IMC` 的关系

- `RegisterUIActionBinding(FBindUIActionArgs)` 不是用来替代 `UInputAction` 或 `Input Mapping Context` 的。
- 它只是 CommonUI Widget 侧的监听入口。
- 正确分工是：
  - `UInputAction`
    - 定义“菜单角色切换确认”这个动作本身
  - `Input Mapping Context`
    - 定义这个动作在当前菜单上下文里由哪个键位 / 手柄键触发
  - `DT_UniversalActions`
    - 给 `UCommonActionWidget`、Action Bar、按钮提示提供图标和文案
  - `RegisterUIActionBinding(FBindUIActionArgs)`
    - 让具体 Widget 收到这个动作，并拿到执行、长按进度、完成、释放等回调
- 中文结论：
  - `IA / IMC / DT` 负责“这个动作是什么、按什么、显示什么”
  - `RegisterUIActionBinding` 负责“这个 Widget 怎么响应它”

### `RegisterUIActionBinding` 不是全局监听

- 它不是“项目里注册一次，全局都能收到”的监听。
- 绑定是挂在具体 Widget 上的。
- UE 5.7 CommonUI 源码里，动作路由至少会检查：
  - 绑定 Widget 是否可达、可接收输入
  - 当前 `ECommonInputMode` 是否匹配
  - 如果走 `UInputAction` 路线，还会查询当前 LocalPlayer 已激活输入上下文里，这个动作到底映射到了哪些键
- 中文结论：
  - 你切不切输入上下文，会直接影响 `UInputAction` 绑定能不能收到输入
  - 所以它不是脱离 IMC 独立存在的一套“全局监听”

### 纯 `UInputAction` 绑定和 CommonUI 长按不是一回事

- 这点需要单独强调。
- UE 5.7 的 CommonUI 源码里：
  - `FBindUIActionArgs(UInputAction*)` 这条路径会记录动作引用
  - 运行时通过 `QueryKeysMappedToAction` 去查当前激活 IMC 里这个动作映射到了哪些键
- 但是 CommonUI 的 hold / rollback 进度，并不是自动从 `UInputAction` 本身长出来的。
- 从当前源码实现看，CommonUI 的 hold mappings 更直接地来源于：
  - CommonUI Action Data / DataTable 行
  - 或 CommonUI 默认 HoldData
- 这意味着：
  - 如果你只给 Widget 一个 `IA_SwitchCharacter`
  - 再直接 `RegisterUIActionBinding(FBindUIActionArgs(InputAction, ...))`
  - 那么“执行回调”这部分没问题
  - 但“长按进度 / 松手回落”这部分，不应想当然地认为它会完整自动工作

### 当前项目约定：`DT_UniversalActions` 只负责图标与提示

- 用户已明确说明：
  - `DT_UniversalActions` 在当前项目里主要给 `UCommonActionWidget` 做跨设备按键图标与提示
  - 原项目独立输入提示表已经合并到 Lyra 主表；当前不再把独立旧表作为运行时资产
  - 当前不要把它当成角色切换长按的逻辑配置表
- 所以当前菜单角色切换方案里：
  - `DT_UniversalActions` 继续负责显示
  - hold 时间、触发时机、进度、回落由 `IA_SwitchCharacter` + C++ 逻辑负责

### 当前项目里更稳的方案 B

- 如果确定菜单角色切换要走独立键位，推荐方案改成下面这套：
  1. 新建 `IA_SwitchCharacter`
  2. 新建 `IMC_CharacterSwitchMenu`
  3. 在 `IMC_CharacterSwitchMenu` 里绑定：
     - 键盘：E
     - 手柄：X
  4. `IA_SwitchCharacter` 本身配置：
     - `Value Type = Digital(bool)`
     - Trigger：优先用 `Hold`
     - `HoldTimeThreshold`：由动作 Trigger 决定，例如 0.6 秒
     - `bIsOneShot = true`
  5. 在 `DT_UniversalActions` 中仅增加对应显示行：
     - `Input_SwitchCharacter`
     - 只负责按钮图标与提示文案
  6. 角色切换菜单 Widget 激活时叠加 `IMC_CharacterSwitchMenu`
  7. C++ 中手动监听增强输入事件：
     - `Started`：开始显示进度
     - `Ongoing`：用 `FInputActionInstance::GetElapsedTime()` 刷新进度
     - `Triggered`：达到阈值，正式提交切换
     - `Canceled`：中途松开，开始回落动画
  8. 回落时间由 C++ 自己控制，例如 0.25 秒
  9. 达到阈值后调用 `AShootPlayerController::RequestSwitchCharacter`

### 为什么方案 B 里仍然需要 IMC

- 即便菜单长按最终通过 CommonUI 的 DataTable 行来驱动 hold/rollback，IMC 仍然有价值：
  - 菜单里不只有一个“切换角色”动作
  - 你还会有返回、切页、左右切换、确认、取消等动作
  - 这些动作都应该跟着菜单上下文切换，而不是继续占用玩法层输入
- 中文结论：
  - IMC 负责菜单动作的整体输入上下文隔离
  - DataTable 行负责 CommonUI 图标与提示
  - `IA_SwitchCharacter` 与 C++ 负责 hold/rollback 行为
  - 三者不是替代关系，而是配合关系

### 当前前端资产结论

- 用户已确认：
  - `IA_Confirm` 对应键盘 Enter、手柄 A
  - `DT_UniversalActions` 已配置 `Input_Confirm`
  - `IMC_FrontEnd` 已在前端地图流程里使用
- 这说明当前前端已经具备：
  - 通用确认输入
  - 跨设备图标显示
  - 前端专用输入上下文
- 所以后续角色切换菜单不用重新发明前端输入体系，而是在现有前端体系上加一层更具体的菜单动作即可。

### 角色切换菜单的资产建议

- 如果菜单角色切换确认就用“通用确认键”：
  - 继续用 `IA_Confirm`
  - 不需要再建新的切换确认动作
  - 适合“焦点在角色卡片上，按住 A / Enter 确认切换”的设计
- 如果菜单角色切换确认要占独立键位：
  - 新建 `IA_SwitchCharacter`
  - 新建 `IMC_CharacterSwitchMenu`
  - 在新的 IMC 里把：
    - 手柄：例如 X
    - 键盘：例如 E
    - 绑定到 `IA_SwitchCharacter`
  - 再在 `DT_UniversalActions` 增加一行：
    - `Input_SwitchCharacter`
    - 只配置图标和提示
  - Widget 激活时叠加这个 IMC
  - Widget 侧通过增强输入事件 + C++ 计时来处理长按，而不是把 hold 语义塞回提示表

### 对本项目更稳的建议

- 当前用户已经明确不走“焦点确认键复用”路线，而是独立切换键路线。
- 因此本项目当前更稳的做法是：
  - 前端基础输入继续走 `IMC_FrontEnd`
  - 角色切换相关界面叠加 `IMC_CharacterSwitchMenu`
  - 独立动作使用 `IA_SwitchCharacter`
  - `DT_UniversalActions` 只负责 `Input_SwitchCharacter` 的图标提示
  - 长按监听与回落由增强输入 + C++ 自己实现
  - CommonUI 负责 Widget 生命周期、焦点和显示，不强行承担这条自定义长按的全部状态机

### 用户已选定的当前方向

- 当前用户已明确选择方案 B：
  - 角色切换菜单使用独立键位
  - 后续主菜单相关输入都走专用菜单 IMC
- 因此后续实现按下面资产推进：
  - `IA_SwitchCharacter`
  - `IMC_CharacterSwitchMenu`
  - `DT_UniversalActions` 中的 `Input_SwitchCharacter`
- 菜单长按和进度回落，优先走增强输入触发器 + C++ 自己的计时与回落，而不是继续把这条逻辑压到 `UShootInputComponent` 或 `RegisterUIActionBinding` 上

### 参考

- 官方文档：
  - https://dev.epicgames.com/documentation/en-us/unreal-engine/using-commonui-with-enhnaced-input-in-unreal-engine
  - https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/CommonUI/UCommonButtonBase
  - https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/CommonUI/FBindUIActionArgs
- 引擎源码：
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/Input/CommonUIInputTypes.h`
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonButtonBase.h`
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonUserWidget.h`
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonActivatableWidget.h`
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/CommonButtonBase.cpp`
  - `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Input/UIActionRouterTypes.cpp`

## 项目 UI 模式示例

### 模式1：C++ 组件驱动 UI

**示例**: `UShootHUDReticleComponent` (准星系统)

**位置**: `Source/NewWorldOrder/Public/UI/Weapons/ShootHUDReticleComponent.h`

```cpp
// 组件附加在 PlayerController 上（不是 Character）
UCLASS(ClassGroup="UI", meta=(BlueprintSpawnableComponent))
class UShootHUDReticleComponent : public UActorComponent
{
public:
    // 通过消息系统监听游戏事件
    void HandleHitNotification(FGameplayTag Channel, const FShootReticleHitNotifyMessage& Message);
    void HandleAdsMessage(FGameplayTag Channel, const FShootReticleADSMessage& Message);

    // 管理准星 Widget
    void RefreshActiveReticle();
    void InitializeReticleForWeapon(ARangedWeaponInstance* Weapon);

private:
    // 弱指针持有活动的准星 Widget
    TWeakObjectPtr<UShootReticleWidgetBase> ActiveReticleWidget;

    // 消息监听器句柄
    FGameplayMessageListenerHandle HitNotifyHandle;
    FGameplayMessageListenerHandle AdsHandle;
};
```

**设计特点**：
- **组件化**: UI 逻辑封装在 ActorComponent 中
- **消息驱动**: 通过 GameplayMessage 系统通信
- **生命周期管理**: 使用 TWeakObjectPtr 避免悬空指针
- **C++ 优先**: 核心逻辑在 C++，Blueprint 仅用于布局和样式

### 模式2：继承 ULyraActivatableWidget

**示例**: `UShootReticleWidgetBase`

**位置**: `Source/NewWorldOrder/Public/UI/Weapons/ShootReticleWidgetBase.h`

```cpp
UCLASS(Abstract)
class UShootReticleWidgetBase : public ULyraActivatableWidget
{
public:
    // 从武器初始化准星
    void InitializeFromWeapon(ARangedWeaponInstance* InWeapon);

    // 处理游戏事件
    void HandleHitNotification(const FShootReticleHitNotifyMessage& Message);
    void HandleAdsState(bool bIsAds);

    // 计算扩散角度
    float ComputeSpreadAngle() const;

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 子 Widget（使用 BindWidgetOptional）
    UPROPERTY(meta=(BindWidgetOptional))
    UShootCircumferenceMarkerWidget* CrosshairWidget;

    UPROPERTY(meta=(BindWidgetOptional))
    UShootHitMarkerConfirmationWidget* HitMarkerWidget;

    // 武器弱引用
    UPROPERTY(BlueprintReadOnly, Transient)
    TWeakObjectPtr<ARangedWeaponInstance> WeaponInstance;
};
```

**设计特点**：
- **抽象基类**: 定义通用准星接口
- **可选子 Widget**: 使用 `BindWidgetOptional` 允许灵活布局
- **NativeTick**: C++ 实现性能敏感的更新逻辑
- **Blueprint 友好**: 暴露 BlueprintReadOnly 属性供 Blueprint 使用

## 常见错误

### CommonInput 键位图标数据表的空键误命中

项目键鼠 ControllerData：

```text
/Game/UI/Foundation/Platform/Input/KeyboardMouse/CommonInput_KeyboardMouse
```

UE 5.8.1 的 `InputBrushDataMap` 前五项曾为：

| 索引 | Key | Brush |
|---:|---|---|
| 0 | `#` | `T_PC_Acute_Light_64x64` |
| 1 | `´` | `T_PC_Grave_Light_64x64` |
| 2 | `ß` | `T_PC_Qmark_Light_64x64` |
| 3 | `+` | `T_PC_Star_Light_64x64` |
| 4 | `^` | `T_PC_Cflex_Light_64x64` |

这五个名称在当前 UE 5.8.1 InputCore 键注册表中没有有效键定义。`T_PC_Acute_Light_64x64` 是该数据行配置的 Brush，CommonUI 不会根据 `#` 自动推导出 Acute 图标。

CommonUI 5.8.1 的旧 DataTable 输入路径包含以下调用链：

```text
CommonUI::GetIconForInputActions
    -> UCommonInputPlatformSettings::TryGetInputBrush
    -> CommonUIUtils::TryGetInputBrushFromDataMap
    -> FKey::IsSameResolvedKey
```

动作行的当前设备键为空时，旧路径直接把空 `FKey` 传入 `TryGetInputBrushFromDataMap`。该函数使用 `IsSameResolvedKey` 查找第一条匹配记录；未解析键的 `GetVirtualKey()` 返回空 `FKey`，因此空键会命中前面的无效键行并取得对应 Brush。`CommonActionWidget::UpdateActionWidget` 收到真实 Brush 后会显示键帽；收到 `NoBrush` 时才会折叠控件。

已从 `CommonInput_KeyboardMouse` 删除索引 0–4 的五项映射：

```text
90 项 -> 85 项
首个保留键：MouseX
```

删除后，空键跳过这些无效记录，后续第一个有效键不会与空 `FKey` 解析为同一虚拟键，动作控件回到 `NoBrush` 与折叠行为。对应的纹理资产继续保留，供其他有效映射或后续键位审计使用。

责任边界：

- 前五项属于 ControllerData 中的无效键映射，已作为项目资产清理项处理。
- 空 `FKey` 与未解析键通过 `IsSameResolvedKey` 误匹配，属于 UE 5.8.1 CommonUI 旧查找路径的引擎行为。

相关 UE 源码：

- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/CommonUITypes.cpp`
- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/CommonInputBaseTypes.cpp`
- `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/CommonActionWidget.cpp`
- `Engine/Source/Runtime/InputCore/Private/InputCoreTypes.cpp`

### 错误1：直接使用 CreateWidget 而不是 PushContentToLayerForPlayer

```cpp
// 错误：不使用层级系统
UUserWidget* Widget = CreateWidget<UMyWidget>(GetWorld(), WidgetClass);
Widget->AddToViewport();

//  正确：使用层级系统
UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(
    PlayerController, WidgetClass, LayerTag, true
);
```

**原因**:
- 直接 AddToViewport 绕过了输入管理系统
- 无法利用层级栈的优势
- 无法正确处理 Widget 激活/反激活生命周期

### 错误2：Widget 销毁而不是反激活

```cpp
// 错误：直接销毁
MyWidget->RemoveFromParent();
MyWidget->ConditionalBeginDestroy();

//  正确：反激活（会自动从层级移除）
MyWidget->DeactivateWidget();
```

**原因**:
- `DeactivateWidget()` 会正确处理输入恢复
- 会触发层级栈的清理逻辑
- 允许 Widget 执行清理动画

### 错误3：在 Character 上附加 UI 组件

```cpp
// 错误：在 Character 上创建 UI 组件
UPROPERTY()
UShootHUDReticleComponent* ReticleComponent;  // 在 ACharacter 中

//  正确：在 PlayerController 上创建 UI 组件
UPROPERTY()
UShootHUDReticleComponent* ReticleComponent;  // 在 APlayerController 中
```

**原因**:
- Character 可能被销毁/重新生成（死亡、切换角色）
- UI 组件应该跟随 PlayerController 的生命周期
- PlayerController 在玩家会话期间持久存在

### 错误4：UI 轴输入依赖 Widget 蓝图增强输入事件（CommonUI 只注入布尔值）

**现象**（2026-08-14 W_Cloth 右摇杆旋转排查确认）：Widget 蓝图里的 IA 增强输入事件节点（K2Node_EnhancedInputAction）对 2D 轴输入完全不触发，右摇杆 360° 推均无反应；而同 IMC 下的按钮（TriggeringEnhancedInputAction）工作正常。

**根因（UE 5.8 引擎源码）**：
- CommonUI 的 FUIActionBinding::ProcessNormalInput（Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Input/UIActionRouterTypes.cpp 约 866-884 行）处理**非 Generic** 的 InputAction 时：
  - 构造 FInputActionValue RawValue = FInputActionValue(true) —— 硬编码布尔 true，不是实际轴值
  - 调 CommonUI::InjectEnhancedInputForAction(...) 注入到 UEnhancedInputLocalPlayerSubsystem（CommonUITypes.cpp:363）
  - **不执行 OnExecuteAction**，源码注释明确 "Non generic actions should inject enhanced input so users can bind to enhanced input events"
- 即 CommonUI 的 UI 输入链路只支持**布尔注入**（按下/释放语义），2D 轴的真实模拟值永远到不了 UI 层。
- 按钮（TriggeringEnhancedInputAction → RegisterUIActionBinding）回调是 FSimpleDelegate 无参数（CommonButtonBase.cpp:721-731），只适合布尔触发，拿不到轴值。

**正确做法**：C++ BlueprintPure 查询真实轴值，蓝图 Tick 驱动：
- C++：GetPreviewOrbitAxisX()：GetOwningPlayer → GetLocalPlayer → UEnhancedInputLocalPlayerSubsystem::GetPlayerInput()->GetActionValue(IA).Get<FVector2D>().X（读 EIS 真实轴值，不经 CommonUI 注入）
- 蓝图：Event Tick 每帧查询 → 死区短路（|X| > 0.05）→ 速度 × DeltaTime → 调 BlueprintCallable 旋转动作
- 性能：单次 C++ 查询开销可忽略；死区短路保证摇杆回中时不调用旋转方法

**示例资产**：W_Cloth（右摇杆旋转预览，IA_WardrobePreviewOrbit）+ UShootWardrobeScreen::GetPreviewOrbitAxisX / RotatePreviewByDelta。

## 最佳实践

1. **始终使用层级系统**: 所有 UI 都应该通过 `PushContentToLayerForPlayer` 推送到对应层级
2. **继承 ULyraActivatableWidget**: 获得自动输入配置功能
3. **使用消息系统通信**: 避免 UI 组件直接耦合游戏逻辑
4. **C++ 处理逻辑，Blueprint 处理表现**: 核心逻辑用 C++ 实现，样式和布局用 Blueprint
5. **组件化 UI 管理**: 将 UI 管理逻辑封装在 ActorComponent 中
6. **使用弱指针**: UI 组件持有 Widget 引用时使用 `TWeakObjectPtr`

## 相关文件

- `Plugins/CommonGame/Source/Public/PrimaryGameLayout.h` - 主UI布局系统
- `Plugins/CommonGame/Source/Public/Actions/AsyncAction_PushContentToLayerForPlayer.h` - UI推送操作
- `Source/NewWorldOrder/Public/UI/LyraActivatableWidget.h` - 项目基础 Widget 类
- `Source/NewWorldOrder/Public/UI/Weapons/ShootHUDReticleComponent.h` - UI组件模式示例
- `Source/NewWorldOrder/Public/UI/Weapons/ShootReticleWidgetBase.h` - Widget基类示例

## 标签

`#UI` `#CommonUI` `#Lyra` `#GameplayTag` `#InputManagement` `#Architecture`
