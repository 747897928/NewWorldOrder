# UE 5.8 Mac CrashReportClient 与 PIE ensure 深查记录

日期：2026-06-30
状态：[项目侧目标 Ensure 已修复并通过 Windows PIE] [Mac Shipping 退出仍待原始证据复验]

## 2026-07-20 后续结论

- 旧 `UMultiplayerSessionsSubsystem` 已完全删除，会话唯一主线为 `UCommonSessionSubsystem`；项目代码不再长期持有 `IOnlineSessionPtr`。
- `Plugins/CommonUser` 保持 Lyra 上游原样。项目层 Coordinator 在实际 `NAME_GameSession` 为 `NoSession` 时不调用上游清理，避免 pending destroy 边界。
- `BP_ShootCharacter` Construction Script 的盲目 LeaderPose 已删除。`UMutableAppearanceComponent` 在 Mutable Updated 回调中建立并核验映射，普通换衣不提前解绑。
- Windows HomeMap PIE 逐帧复验中，Body 首帧隐藏，最终 Mutable 网格与 LeaderPose 准备完成后显示；两个目标 Ensure 均为 0 次。
- 用户提供的 `NewWorldOrder-Mac-Shipping 退出崩溃完整诊断与修复报告.txt` 是二次整理报告，不是 macOS 原始 `.ips` 或打包实例 `Saved/Logs`。其中“模块 Shutdown 主动调用 `IOnlineSubsystem::Shutdown()`”目前没有足够证据支持，项目不会据此手动关闭引擎拥有的全局 OnlineSubsystem；正常 Windows 日志已显示引擎按自己的退出顺序关闭 Steam。
- Mac Shipping 退出若仍崩溃，需要同时保存原始 `.ips`、`NewWorldOrder/Saved/Logs` 最后一份日志、构建提交与配置、是否由 Steam 客户端启动。只有这些证据能区分项目对象退出顺序、Steam API、CrashReportClient 二次崩溃或 UE 5.8 平台问题。

## 本次结论摘要

- 这次用户提供的两份 macOS 崩溃报告，崩溃进程都是 `CrashReportClientEditor`，不是 `UnrealEditor` 主进程本身。
- `UnrealEditor` 在 PIE 前后触发了多个 ensure，其中确定存在：
  - 废弃 CVar：`r.Mobile.VirtualTextures`
  - OnlineSubsystemNull：`SessionInterface.IsUnique()`
  - Mutable / LeaderPose：`Leader pose component skeleton doesn't match follower`
- `LeaderPose` 的需求背景是合理的：联机游戏、实时 Mutable 生成、男女角色、头身网格动画同步。不能把“头身同步需求”本身当成错误。
- MCP 已确认：`BP_ShootCharacter` 的 `UserConstructionScript` 在运行时执行 `BodyMesh.SetLeaderPoseComponent(GetMesh)`。这解释了为什么 C++ 和 CDO 默认值里看不到 LeaderPose，但日志里确实出现 `CharacterBodyMesh0` 跟随 `CharacterMesh0`。
- 我认为当前最可疑的两个项目侧问题是：
  - 旧 `UMultiplayerSessionsSubsystem` 长期缓存 `IOnlineSessionPtr`，和 UE 5.8 PIE 重载 OnlineSubsystemNull 的生命周期冲突。
  - LeaderPose 绑定放在蓝图 Construction Script，早于 Mutable 异步替换 SkeletalMesh 的完成点，缺少骨架一致性校验。
- 不建议、也不需要修改 Unreal Engine 源码。应该先修项目侧配置、会话接口持有方式、Mutable 绑定时机和资产配置。

## 调查范围

- 用户提供的两份 macOS 崩溃报告：
  - 2026-06-30 21:04:07，`CrashReportClientEditor [42055]`
  - 2026-06-30 21:08:29，`CrashReportClientEditor [45503]`
- 本机日志：
  - `/Users/zhaoyijie/Library/Logs/Unreal Engine/NewWorldOrderEditor/NewWorldOrder.log`
  - `/Users/zhaoyijie/Library/Logs/Unreal Engine/CrashReportClient/CrashReportClient.log`
  - `/Users/zhaoyijie/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Crashes/EnsureReport-UE-NewWorldOrder-pid-41999-85A0CDD64F4192E54E9DC3B22AEEA6A0`
- 项目代码：
  - `Source/NewWorldOrder`
  - `Plugins/CommonUser`
  - `Config/DefaultEngine.ini`
  - `NewWorldOrder.uproject`
- MCP 读取的资产：
  - `/Game/Blueprints/Character/BP_ShootCharacter`
  - `/Game/Blueprints/Mutable/Characters/Male/CO_Character_M`
  - `/Game/Blueprints/Mutable/Characters/Female/CO_Character_F`
  - `/Game/UI/Hud/W_Healthbar`
  - `/Game/Blueprints/AbilitySystem/GameplayEffects/PrimaryAttributes/GE_Shoot_PrimaryAttributes`

## 需求背景

### Mutable 头身同步需求

确定事实：

- 项目是联机游戏，角色状态和外观不能只按本地单机表现处理。
- 角色外观需要支持实时生成或更新。
- 角色需要支持男女主角。
- 当前角色把头部和身体拆成两套 SkeletalMesh/CustomizableSkeletalComponent：
  - `GetMesh()` / `CharacterMesh0`：头部主网格
  - `BodyMesh` / `CharacterBodyMesh0`：身体网格
  - `HeadCSkeletalComponent`：挂在 `GetMesh()`
  - `BodyCSkeletalComponent`：挂在 `BodyMesh`
- `Docs/Tasks/CharacterSwitching/Overview_总览.md` 记录了男女主切换和 PlayerState 数据分层。
- `Docs/DevelopmentNotes/MutablePreview_LeaderPoseAnimLayer_预览头身同步.md` 记录了 Mutable 异步更新、头身同步和 AnimLayer Link/Unlink 的现有设计。

判断：

- 头身拆分和 LeaderPose 同步不是无意义写法，它服务于实时换装、男女角色和头身动画同步。
- 真正要查的是：当前实现在哪个时机绑定 LeaderPose、Mutable 生成出来的头身网格是否同骨架、以及异步替换 SkeletalMesh 后是否重新校验同步关系。

### OnlineSubsystem 需求

确定事实：

- 项目目标包含联机合作，最低发行目标是 Steam。
- `Source/NewWorldOrder/NewWorldOrder.Build.cs` 依赖 `OnlineSubsystem` 和 `OnlineSubsystemSteam`。
- 项目迁入了 Lyra 相关体系，包括 `CommonGame`、`CommonUser`、`UIExtension`、`ModularGameplay`、`ModularGameplayActors`、`GameplayMessageRuntime`。
- `Plugins/CommonUser` 内含 `UCommonSessionSubsystem`，这是 Lyra/CommonUser 的会话子系统。
- 项目自身仍保留 `UMultiplayerSessionsSubsystem`，这是另一套早期会话代码。

判断：

- OnlineSubsystem 不能只按“Steam 接入代码”看，它现在同时存在旧项目会话子系统和 Lyra/CommonUser 会话子系统。
- 如果两条链路都碰 OnlineSession，在 PIE、Null、Steam、多平台切换时都容易出现生命周期和职责重叠。

## CrashReportClientEditor 崩溃报告

确定事实：

- 两份 macOS 报告的崩溃进程都是 `CrashReportClientEditor`。
- 两份报告的父进程都是同一个 `UnrealEditor [41999]`。
- 崩溃点一致：
  - `FOutputDeviceRedirector::IsRedirectingTo(FOutputDevice*)`
  - 调用链进入 `FMacErrorOutputDevice::Serialize`
  - 调用链还经过 `LowLevelTasks::FScheduler::StopWorkers` 和 `exit`

判断：

- 这说明 macOS 弹出的崩溃报告记录的是崩溃报告客户端在处理 ensure 报告或退出阶段发生二次崩溃。
- 当前证据不能写成 “UnrealEditor 主进程崩溃在 FOutputDeviceRedirector”。
- 这部分如果是 UE 5.8 CrashReportClientEditor 自身缺陷，应由 Epic 修；项目侧能做的是减少触发 ensure 的来源。

## Ensure 1：废弃 CVar

确定事实：

- CrashContext 中记录：
  - `CrashType` 为 `Ensure`
  - `ErrorMessage` 为 `r.Mobile.VirtualTextures is deprecated, use r.VirtualTextures in platform specific engine .ini files instead`
- `Config/DefaultEngine.ini:19` 存在：
  - `r.Mobile.VirtualTextures=False`
- `Config/DefaultEngine.ini:27` 已存在：
  - `r.VirtualTextures=False`

判断：

- 这是 UE 5.8 对旧配置项的启动期 ensure。
- 这不是 Source C++ 崩溃。
- 修复方向很明确：删掉旧的 `r.Mobile.VirtualTextures`，保留 `r.VirtualTextures` 或按平台 ini 拆分。

## Ensure 2：OnlineSubsystemNull SessionInterface.IsUnique

### 已确认事实

- `NewWorldOrder.log` 在 2026-06-30 21:08:19 记录：
  - `Ensure condition failed: SessionInterface.IsUnique()`
  - 文件：`Plugins/Online/OnlineSubsystemNull/Source/Private/OnlineSubsystemNull.cpp`
  - 调用链：`FOnlineSubsystemNull::Shutdown` → `FOnlineSubsystemModule::DestroyOnlineSubsystem` → `ReloadDefaultSubsystem` → `UEditorEngine::StartPlayInEditorSession`
- 旧项目子系统 `UMultiplayerSessionsSubsystem` 在构造函数中长期缓存接口：
  - `Source/NewWorldOrder/Private/MultiplayerSessions/MultiplayerSessionsSubsystem.cpp:25`
  - `IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();`
  - `SessionInterface = Subsystem->GetSessionInterface();`
  - `FriendsInterface = Subsystem->GetFriendsInterface();`
- 同一类后续直接复用缓存接口：
  - `CreateSession`
  - `FindSessions`
  - `JoinSession`
  - `DestroySession`
  - `GetFriendsList`
  - `InviteFriends`
- `GetSubsystemName()` 直接调用：
  - `IOnlineSubsystem::Get()->GetSubsystemName()`
  - 这里没有空指针保护。
- `UCommonSessionSubsystem` 也绑定 OnlineSession delegates：
  - `Plugins/CommonUser/Source/CommonUser/Private/CommonSessionSubsystem.cpp:316`
  - 初始化时 `BindOnlineDelegates()`
  - OSSv1 下用 `Online::GetSubsystem(GetWorld())`
  - 获取 `const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface()`
- `UCommonSessionSubsystem` 在创建、查找、清理、加入会话时也按 `GetWorld()` 临时获取接口：
  - `CreateOnlineSessionInternalOSSv1`
  - `FindSessionsInternalOSSv1`
  - `CleanUpSessionsOSSv1`
  - `JoinSessionInternalOSSv1`

### 判断

- 我认为 `SessionInterface.IsUnique()` 的直接项目侧触发条件，很可能是旧 `UMultiplayerSessionsSubsystem` 在 GameInstance 生命周期内长期持有 `IOnlineSessionPtr`。
- UE 5.8 PIE 开始时会 `ReloadDefaultSubsystem`，Null 子系统 shutdown 时希望 SessionInterface 没有额外共享引用；项目侧缓存会破坏这个假设。
- 这个判断和日志调用链、代码持有方式相符，但还没有通过代码修改后的复测验证，所以仍标为“我认为”。
- Lyra/CommonUser 不是完全无关，它也注册 OnlineSession delegate；但它的模式是按 `GetWorld()` 获取接口，不像旧子系统在构造函数里长期缓存 `IOnlineSessionPtr`。

### 建议修复路线

优先路线：

- 选择一个会话主线。由于项目已经迁入 CommonUser/CommonSession，并且未来可能支持 Steam 之外的平台，建议以 `UCommonSessionSubsystem` 为主线。
- 将 `UMultiplayerSessionsSubsystem` 逐步改成兼容适配层，或者明确废弃，不再直接拥有会话生命周期。

短期保守修复：

- 不在 `UMultiplayerSessionsSubsystem` 构造函数缓存 `IOnlineSessionPtr` / `IOnlineFriendsPtr`。
- 每次操作时用 `Online::GetSessionInterface(GetWorld())` 或 `Online::GetSubsystem(GetWorld())` 临时获取接口。
- 在 `Deinitialize()` 中清理所有 delegate handle，并 `Reset()` 缓存指针。
- 对 `GetWorld()`、`LocalPlayer`、`GetPreferredUniqueNetId()`、`IOnlineSubsystem::Get()` 全部加有效性保护。
- 避免同一 UI 同时调用 `UMultiplayerSessionsSubsystem` 和 `UCommonSessionSubsystem` 去创建、查找、加入同一个 session。

## Ensure 3：Mutable / LeaderPose 骨架不匹配

### 日志事实

- `NewWorldOrder.log` 在 2026-06-30 21:08:23 记录：
  - `Ensure condition failed: InLeaderPoseComponent == nullptr || InSkinnedMeshComponent->GetLeaderBoneMap().Num() == InSkinnedMeshComponent->GetSkinnedAsset()->GetRefSkeleton().GetNum()`
  - `Leader pose component skeleton doesn't match follower`
  - Leader：`BP_ShootCharacter_C_0.CharacterMesh0`
  - Follower：`BP_ShootCharacter_C_0.CharacterBodyMesh0`
- 调用链经过：
  - `USkeletalMeshComponent::SetSkeletalMesh`
  - `UCustomizableObjectSystemPrivate::TickPendingSetReferenceSkeletalMesh`
  - `UCustomizableObjectSystemPrivate::UpdateTick`

### C++ 事实

- `AShootCharacter` 创建 `BodyMesh` 并挂到 `GetMesh()` 下：
  - `Source/NewWorldOrder/Private/Character/ShootCharacter.cpp:87`
  - `BodyMesh->SetupAttachment(GetMesh())`
- `BodyCSkeletalComponent` 挂到 `BodyMesh`：
  - `Source/NewWorldOrder/Private/Character/ShootCharacter.cpp:103`
- `HeadCSkeletalComponent` 挂到 `GetMesh()`：
  - `Source/NewWorldOrder/Private/Character/ShootCharacter.cpp:110`
- `InitAppearanceComponent()` 在服务端 `PossessedBy` 和客户端 `OnRep_PlayerState` 都会执行：
  - `Source/NewWorldOrder/Private/Character/ShootCharacter.cpp:144`
  - `Source/NewWorldOrder/Private/Character/ShootCharacter.cpp:155`
- `UMutableAppearanceComponent::InitializeComponents` 设置 Mutable 组件名：
  - `BodyCSkeletalComponent->SetComponentName(BodyComponentName)`
  - `HeadCSkeletalComponent->SetComponentName(HeadComponentName)`
- `SwitchGenderInstance` 中 Head/Body 两个 CSC 会绑定到同一个 COI。
- `MutableAppearanceComponent` 只绑定 Head 的 UpdatedDelegate，注释解释其原因是 Head 和 Body 共享同一个 COI，BodyMesh 通过 LeaderPose 跟随 GetMesh()。

### MCP 事实

- `BP_ShootCharacter` 的生成类为：
  - `/Game/Blueprints/Character/BP_ShootCharacter.BP_ShootCharacter_C`
- `BP_ShootCharacter` 的 CDO 默认组件值：
  - `CharacterMesh0`：`skeletal_mesh=None`，`anim_class=None`，`leader_pose_component=None`
  - `CharacterBodyMesh0`：`skeletal_mesh=None`，`anim_class=None`，`leader_pose_component=None`
  - `BodyCSkeletalComponent`：`component_name=None`，`customizable_object_instance=None`，`replicates=True`
  - `HeadCSkeletalComponent`：`component_name=None`，`customizable_object_instance=None`，`replicates=True`
  - `AppearanceComponent`：`replicates=True`
- `BP_ShootCharacter` 的 `UserConstructionScript` 有 4 个节点：
  - `Construction Script`
  - `Set Leader Pose Component Target is Skinned Mesh Component`
  - `Get Mesh`
  - `Get BodyMesh`
- MCP 连接关系确认：
  - `Construction Script.then` → `Set Leader Pose Component.execute`
  - `Get BodyMesh.BodyMesh` → `Set Leader Pose Component.self`
  - `Get Mesh.Mesh` → `Set Leader Pose Component.NewLeaderBoneComponent`
- 也就是说运行时蓝图执行的是：
  - `BodyMesh.SetLeaderPoseComponent(GetMesh)`
- MCP 读取 Mutable CO 资产：
  - `/Game/Blueprints/Mutable/Characters/Male/CO_Character_M`
  - `/Game/Blueprints/Mutable/Characters/Female/CO_Character_F`
  - 两个资产都存在，类为 `CustomizableObject`
  - 但 Python API 返回 `get_component_count = 0`、`get_parameter_count = 0`、`get_state_count = 0`

### 判断

- LeaderPose 不是由 C++ 构造函数直接设置，而是由 `BP_ShootCharacter` 的 Construction Script 设置。
- CDO 默认值里 `leader_pose_component=None` 并不代表运行时没有 LeaderPose；Construction Script 会在实例构建时建立 BodyMesh → Mesh 的跟随关系。
- 报错发生在 Mutable 异步更新设置 SkeletalMesh 的调用链中，这说明 follower 的 SkeletalMesh 被 Mutable 替换时，UE 发现 follower 当前骨架映射和 leader 不匹配。
- 我认为更具体的问题不是“用了 LeaderPose”，而是“LeaderPose 绑定过早且没有跟随 Mutable 异步完成点重新校验”。
- `get_component_count = 0` 不应直接写成 CO 资产坏了。可能是资产未编译、Python API 不暴露当前图中组件、或该 CO 的数据需要在 Mutable 编辑器/运行期生成后读取。这里只能写成 MCP 当前读取结果。

### 为什么 Windows 只报错而 Mac 更严重

确定事实：

- 用户反馈 Windows 也会报同类错误，但代码还能继续跑。
- macOS 这次报告里 CrashReportClientEditor 发生二次崩溃。

判断：

- 这符合 ensure 的常见行为：ensure 本身通常是“记录严重错误并继续”，不是 fatal crash。
- Mac 上更严重的体验可能来自两个叠加因素：
  - UE 5.8 Mac CrashReportClientEditor 在处理 ensure 报告时二次崩溃。
  - Mutable 异步更新、渲染线程、骨架映射 ensure 在 Mac 上更容易造成卡顿或长时间等待。
- 当前没有证据证明 LeaderPose ensure 是 Mac 编辑器卡死的唯一根因；更准确的说法是它是确定存在的运行期错误，会放大 Mac 端不稳定性。

### 建议修复路线

目标不是取消头身同步，而是让同步关系由 Mutable 生命周期统一管理。

建议方案：

- 从 `BP_ShootCharacter` 的 Construction Script 移除 `BodyMesh.SetLeaderPoseComponent(GetMesh)`。
- 在 C++ 的 `UMutableAppearanceComponent` 中集中管理 LeaderPose：
  - 初始化时先不要盲目绑定。
  - 在 Mutable 更新完成回调 `OnCustomizableSkeletalUpdated()` 后检查 `HeadMesh` 和 `BodyMesh` 当前是否都有有效 `SkinnedAsset`。
  - 校验两者 skeleton 或 RefSkeleton 骨骼数量/映射是否一致。
  - 校验通过后再调用 `BodyMesh->SetLeaderPoseComponent(HeadMesh, true, false)`。
  - 校验失败时不要调用 SetLeaderPoseComponent，而是输出一次明确项目日志，记录 Head/Body 的 mesh、skeleton、ref skeleton bone count、CO component name。
- 如果 Head/Body 的 Mutable 输出确实不是同骨架，则应修 Mutable CO 资产或源 SkeletalMesh 资产，确保 Head 和 Body 输出使用兼容 skeleton。
- 如果短期必须继续运行，可以把“校验失败时跳过 LeaderPose”作为降级保护，但这只是避免 ensure，不是最终满足头身同步需求的方案。

## 运行期错误：W_Healthbar 除零

确定事实：

- PIE 日志中记录：
  - `Script Msg: Divide by zero: Divide_DoubleDouble`
  - 调用者：`W_Healthbar_C ... W_DefaultHUD_C_0.WidgetTree_0.W_Healthbar`
- MCP 读取 `W_Healthbar`：
  - `EventGraph` 有两个 `float / float` 节点。
  - 第一个节点：
    - A 输入来自 `HealthOldValue`
    - B 输入来自 `MaxHealth`
    - 输出写入材质参数 `Health_Current`
  - 第二个节点：
    - A 输入来自 `HealthNewValue`
    - B 输入来自 `MaxHealth`
    - 输出写入材质参数 `Health_Updated`

判断：

- 除零来源已经明确：`MaxHealth` 为 0 时，`W_Healthbar` 用 `HealthOldValue / MaxHealth` 或 `HealthNewValue / MaxHealth` 更新材质。
- 这不是 CrashReportClientEditor 二次崩溃的直接原因，但会污染 PIE 日志。
- 修复应在 Widget 蓝图或 ViewModel 输入层处理：`MaxHealth <= 0` 时使用 0 或延迟更新，不直接除。

## 运行期错误：SetByCaller Data None

确定事实：

- PIE 日志中记录：
  - `FGameplayEffectSpec::GetMagnitude called for Data None on Def Default__ShootEffect_Passive_ArmorEnhancement when magnitude had not yet been set by caller.`
- C++ 中 `UShootEffect_Passive_ArmorEnhancement` 构造函数尝试设置两个 SetByCaller tag：
  - `SetByCaller.ShieldCapacityBonus`
  - `SetByCaller.DamageReductionBonus`
- `UShootGA_Passive_ArmorEnhancement::ActivateAbility` 也设置了这两个 SetByCaller magnitude。
- MCP 读取 CDO：
  - `/Script/NewWorldOrder.ShootEffect_Passive_ArmorEnhancement`
  - `duration_policy = HAS_DURATION`
  - `modifiers` 数量为 2

判断：

- 日志说明实际计算时至少一个 Modifier 的 `FSetByCallerFloat.DataTag` 是 None。
- C++ 源码看起来有设置 tag；因此我怀疑问题在 GameplayTag 初始化时序、构造函数中 `RequestGameplayTag(..., false)` 没拿到有效 tag、或 CDO/热重载状态导致 modifier 数据不是预期值。
- 这一点还没有通过更底层的 C++ 断点或导出 GameplayEffect modifier 详细字段验证，不能写成最终结论。
- 修复方向是避免在构造函数过早依赖可能尚未注册的 GameplayTag，或者用项目既有的 GameplayTag 初始化规范处理。

## 编译和项目配置检查

确定事实：

- 已执行 Mac Editor 编译：
  - 引擎路径：`/Users/Shared/Epic Games/UE_5.8`
  - Target：`NewWorldOrderEditor Mac Development`
  - 结果：`Succeeded`
- 当前 `Source/NewWorldOrder` 没有阻断 Mac Editor 编译的 C++ 语法或链接错误。
- UBT 警告：
  - `.uproject does not list plugin 'UIExtension' as a dependency, but module 'NewWorldOrder' depends on 'UIExtension'`
  - `.uproject does not list plugin 'ModularGameplay' as a dependency, but module 'NewWorldOrder' depends on 'ModularGameplay'`
- `NewWorldOrder.Build.cs` 依赖 `UIExtension`、`ModularGameplay`、`ModularGameplayActors`。
- `NewWorldOrder.uproject` 当前显式列出 `ModularGameplayActors`，但没有显式列出 `UIExtension` 和 `ModularGameplay`。

判断：

- 这是项目描述文件和 Build.cs 依赖不一致。
- 当前不阻断编译，但建议修复，避免插件加载顺序和跨机器构建问题。

## 历史日志中另一个旧崩溃

确定事实：

- 本机旧日志 `NewWorldOrder_2.log` 中存在 2025-11-02 的真正 SIGSEGV。
- 调用链经过：
  - `UGameplayStatics::SaveGameToMemory`
  - `UGameplayStatics::SaveGameToSlot`
  - `USaveGameSubsystem::LoadPlayerSaveGame`
  - `AShootCharacter::LoadProgress`
- 崩溃点在 UObject 属性序列化路径。

判断：

- 这不是用户这次提供的 2026-06-30 CrashReportClientEditor 报告，不能混为同一个事故。
- 但它提示 SaveGame 中可能曾经序列化了不安全的 UObject 引用；后续如再遇到启动/进 PIE 真崩溃，需要单独排查 SaveGame 数据结构。

## 不应写成事实的内容

- 不能说 UnrealEditor 主进程这次崩溃在 `FOutputDeviceRedirector`；报告显示崩的是 `CrashReportClientEditor`。
- 不能说 LeaderPose 需求本身错误；它有明确的头身同步和实时生成需求背景。
- 不能说 LeaderPose ensure 一定是 Mac 卡死唯一根因；目前只能确认它是一个确定存在的运行期错误。
- 不能说 OnlineSubsystemNull 是引擎 bug；当前更像项目侧持有接口引用和 PIE 重载生命周期冲突。
- 不能说 Lyra 插件一定冲突；更准确的是旧项目会话子系统和 CommonUser/CommonSession 同时存在，职责需要收敛。
- 不能说 Mutable CO 资产一定坏了；MCP 当前读到 `component_count=0`，但原因未验证。
- 不能说 `ShootEffect_Passive_ArmorEnhancement` C++ 一定错；只能确认运行时出现 `Data None`，源码意图和运行结果不一致。

## 建议修复优先级

### P0：先减少每次 PIE 都触发的 ensure

- 删除 `Config/DefaultEngine.ini` 中废弃的 `r.Mobile.VirtualTextures`。
- 处理 `UMultiplayerSessionsSubsystem` 对 OnlineSubsystem 接口的长期持有。
- 把 `BP_ShootCharacter` Construction Script 里的 LeaderPose 绑定迁到 Mutable 更新完成后的 C++ 管理点，并加骨架校验。

### P1：修运行期蓝图和 GameplayEffect 错误

- `W_Healthbar` 除法前保护 `MaxHealth > 0`。
- 复查 `ShootEffect_Passive_ArmorEnhancement` 的 SetByCaller DataTag 是否在 CDO 中有效。
- 如果确认为 GameplayTag 初始化时序问题，按项目 GameplayTag 初始化规范修复。

### P2：整理项目结构

- `NewWorldOrder.uproject` 显式补齐 `UIExtension` 和 `ModularGameplay`。
- 明确 Online 会话主线：CommonSession 主线，旧 `UMultiplayerSessionsSubsystem` 只做兼容适配或逐步移除。
- 清理 `BP_ShootCharacter` EventGraph 中仍保留的调试打印、Mutable 组件枚举、按键调试入口，避免 PIE 日志噪声影响判断。

## 建议验证步骤

- 修 P0 后连续启动 PIE 3 次，确认不再出现：
  - `r.Mobile.VirtualTextures is deprecated`
  - `SessionInterface.IsUnique()`
  - `Leader pose component skeleton doesn't match follower`
- 在 Mac 和 Windows 各做一次同样验证，确认平台差异是否消失。
- 用日志单独观察 Mutable 更新完成后 Head/Body 的 mesh、skeleton、ref skeleton bone count。
- 再运行联机 PIE 或 Steam dev 环境，确认 CommonSession/旧会话适配层不会重复创建或销毁 session。
