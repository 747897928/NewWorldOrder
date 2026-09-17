# 宿舍角色切换蓝图接线指南

版本：1.5
最后更新：2026-03-20
状态：可执行

## 目标
- 当前按两条入口接线：
  - 宿舍区 / 世界入口：`PressToInteract` 占位入口，负责提出切换请求
  - CommonUI 菜单入口：长按 `IA_SwitchCharacter`，负责进度条、读满切换、松手回落
- C++ 已提供权威逻辑，蓝图只负责界面表现、菜单焦点与角色展示。
- 世界内确认弹窗方案已经退休，不再作为主线接线方式。

## 对应 C++ 入口
- 交互站点 Actor：`AShootCharacterSwitchStation`
- 交互主能力：`UShootGA_Interact`
- 切换执行能力：`UShootGA_WorldCharacterSwitchRequest`
- 世界入口 Actor 基类：`AShootCharacterSwitchEntryActorBase`
- 控制器长按事件：`OnCharacterSwitchHoldProgressChanged(bVisible, TargetGender, NormalizedProgress, ElapsedSeconds, HoldDuration)`
- 控制器结果事件：`OnCharacterSwitchResult(bSuccess, CurrentGender, Message)`
- UI 模板基类：`UShootCharacterSwitchWidgetBase` 已于 2026-08 删除（全仓零引用），不要再继承。
  - 当前菜单切换由 `UShootWardrobeViewModel` 直连 `AShootPlayerController` 完成。
  - 未来独立菜单 Widget 建议父类 `ULyraActivatableWidget`，并设置 `UCommonActivatableWidget::InputMapping = IMC_CharacterSwitchMenu`。
  - 直接绑定 Controller 事件：`OnCharacterSwitchHoldProgressChanged`、`OnCharacterSwitchResult`。

## 步骤 0：设置关卡为 Hub/安全区
唯一推荐方案（地图配置写在 GameMode，运行时复制到 GameState）：
1. 创建/打开该地图使用的 `BP_ShootGameMode`（父类 `AShootGameModeBase`）。
2. 在 `Class Defaults` 中设置 `bHubOrSafeAreaMap`：
   - Hub/安全区地图设为 `true`
   - 副本/战斗地图设为 `false`
3. 在该地图的 World Settings 中把 `GameMode Override` 指向对应的 `BP_ShootGameMode`。
4. 运行时由服务器 `AShootGameModeBase::BeginPlay` 自动调用 `AShootGameStateBase::SetHubOrSafeArea(...)`，客户端通过复制读取。

## 步骤 1：关卡放置切换入口
1. 打开宿舍关卡。
2. 拖入 `AShootCharacterSwitchStation`（可做蓝图子类，例如 `BP_CharacterSwitchStation`）。
3. 可选配置：
   - `InteractionText`：交互提示文案
   - `InteractionSubText`：副标题文案
4. 确认切换入口位于 Hub/安全区内，否则服务器会拒绝切换。
5. 后续如果改成“另一位主角 NPC”，仍建议复用同一套 Widget 和 Controller 事件，不要再造第二套 UI 协议。
6. 如果未来直接做“另一位主角 NPC”入口，优先新建蓝图子类继承：
   - `AShootCharacterSwitchNPCBase`
   - 再在蓝图里设置 `RepresentedGender`、外观和待机表现

## 步骤 2：确认玩家可交互
1. 玩家角色的交互主能力与执行交互能力现在由 C++ 显式兜底授予：
   - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
   - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
   - 默认能力列表：
     - `UShootGA_Interact`
     - `UShootGA_Interaction_Collect`
     - `UShootGA_Interaction_Revive`
     - `UShootGA_WorldCharacterSwitchRequest`
2. 若按键无反应，先检查：
   - `DA_ShootInputConfig` 是否已有 `IA_Interact -> InputTag.Ability.Interact`
   - 交互输入是否绑定
   - 碰撞配置是否命中 `Interactable_OverlapDynamic`
   - `AShootPlayerState::IsInHubOrSafeArea()` 是否返回 true

## 步骤 3：创建菜单长按进度 Widget
1. 新建 `WBP_CharacterSwitchHoldPanel`，推荐父类使用 `ULyraActivatableWidget`（旧 `UShootCharacterSwitchWidgetBase` 已删除）。
2. 准备控件：
   - 文本：显示目标角色名（男主/女主）
   - 进度条：显示长按百分比
   - 可选图片：目标角色立绘/头像
   - 可选文本：结果提示
3. 默认状态建议隐藏。

### 推荐接线方式（基类已封装）
- 在 Widget 中实现：
  - `BP_UpdateHoldProgress`
  - `BP_ShowSwitchResult`
- 不再需要：
  - 确认按钮
  - 取消按钮
- 节点级连线顺序已经并入本文，不再单独拆一份节点流程图。

### 节点级连线顺序（从旧节点流程图收口）

方案 A（已废弃）：使用 `UShootCharacterSwitchWidgetBase`（类已删除，勿再使用，仅留档）

```text
Event Construct
  -> Set Visibility(Hidden)

Event BP_UpdateHoldProgress(bShowPanel, TargetGender, NormalizedProgress, ElapsedSeconds, HoldDuration)
  -> Branch(bShowPanel)
     True:
       -> Branch(TargetGender == Female)
          True  -> Set Text(Text_TargetName, "切换为女主")
          False -> Set Text(Text_TargetName, "切换为男主")
       -> Set Percent(ProgressBar_Hold, NormalizedProgress)
       -> Set Visibility(Visible)
     False:
       -> Set Percent(ProgressBar_Hold, NormalizedProgress)
       -> Branch(NormalizedProgress <= 0)
          True -> Set Visibility(Hidden)

Event BP_ShowSwitchResult(bSuccess, CurrentGender, Message)
  -> Set Text(Text_Result, Message)
  -> Play Animation(可选)
```

方案 B：不使用基类，手动绑定委托

```text
Event BeginPlay
  -> Bind Event to OnCharacterSwitchHoldProgressChanged
       CustomEvent HandleSwitchHold(...)
         -> Set Percent(ProgressBar_Hold, NormalizedProgress)
         -> Branch(bShowPanel)
            True  -> Set Visibility(HoldPanel, Visible)
            False -> Branch(NormalizedProgress <= 0)
                      True -> Set Visibility(HoldPanel, Hidden)

  -> Bind Event to OnCharacterSwitchResult
       CustomEvent HandleSwitchResult(...)
         -> Set Text(Text_Result, Message)
```

角色名或立绘切换：

```text
HandleSwitchHold
  -> Switch on ECharacterGender(TargetGender)
     Male   -> Set Portrait(Male) / Set Text("切换为男主")
     Female -> Set Portrait(Female) / Set Text("切换为女主")
```

## 步骤 4：在菜单 Widget 或 PlayerController 蓝图中接线
推荐做法：使用方案 B 直接绑定 Controller 委托（`UShootCharacterSwitchWidgetBase` 已删除）。

1. 绑定长按进度
   - `BP_UpdateHoldProgress` 中：
     - `bShowPanel=true` 时显示面板并设置 `ProgressBar.Percent = NormalizedProgress`
     - 用 `TargetGender` 切换目标名字、立绘、头像
     - `bShowPanel=false` 时隐藏或播放收起动画

2. 绑定结果事件
   - `BP_ShowSwitchResult` 中：
     - 显示 `Message`
     - 可选刷新角色展示 UI / 角色卡片

3. 松手回落
   - 当前 C++ 会在玩家松开菜单长按键时持续广播回落进度。
   - 蓝图里最简单的做法就是每次事件到达都直接设置 `ProgressBar.Percent = NormalizedProgress`。
   - 不需要自己再写一套 Tick 反向插值，除非你想要更夸张的动效。

4. 菜单焦点切换
   - 当焦点切到“男主卡片”时，调用 `SetMenuSwitchTargetGender(MALE)`。
   - 当焦点切到“女主卡片”时，调用 `SetMenuSwitchTargetGender(FEMALE)`。
   - 如果蓝图暂时没接这一步，C++ 会按当前主角自动推导默认目标角色。

## 可选做法：手动绑定 Controller 委托
- 直接在 `BP_ShootPlayerController` 或任意 HUD Widget 里绑定（当前推荐方式）：
  - `OnCharacterSwitchHoldProgressChanged`
  - `OnCharacterSwitchResult`
- 但建议优先用基类，避免每个蓝图都重复写绑定/解绑。

## 验收清单
- 靠近切换入口时出现交互提示。
- 世界入口按交互键后，应走同一个 `RequestSwitchCharacter -> TrySwitchCharacter -> SwitchToCharacter` 后端。
- 菜单长按读条推进，松开后进度回落。
- 菜单读满后角色切换成功，并收到“角色切换成功”提示。
- 非 Hub/安全区交互时，收到“仅可在Hub/安全区切换”提示。
- 切换后重新进副本，角色选择保持一致。

## 常见问题
- 看不到进度条：
  - 检查负责菜单切换的 Widget 是否已激活（旧 `UShootCharacterSwitchWidgetBase` 已删除）。
  - 检查蓝图是否实现了 `BP_UpdateHoldProgress`。
- 松手不回落：
   - 检查是不是直接在蓝图里写死了 1.0，没有用事件传进来的 `NormalizedProgress`。
- 读满没切换：
   - 菜单入口先检查 `IA_SwitchCharacter` 和 `IMC_CharacterSwitchMenu` 是否已在蓝图子类设置。
   - 再检查服务器是否进到了 `AShootPlayerController::TrySwitchCharacter`。
- 如果你保留蓝图 `GA_Interact`：
  - 建议后续把它改造成继承 `UShootGA_Interact` 的薄包装资产
  - 不要继续在蓝图事件图上堆新的交互主逻辑
- 如果旧蓝图报找不到确认框函数：
  - 说明该蓝图还停留在旧确认框方案。
  - 需要改接到 `BP_UpdateHoldProgress` 与 `BP_ShowSwitchResult`。
- 提示切换失败：
  - 检查是否在 Hub/安全区。
  - 检查当前目标是否与现有角色相同。
