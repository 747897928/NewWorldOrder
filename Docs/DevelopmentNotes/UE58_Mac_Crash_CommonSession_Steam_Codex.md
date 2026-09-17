# UE 5.8 macOS Crash / CommonSession / Steam 对接修复文档（给 Codex）

日期：2026-06-30  
目标：把当前 UE 5.8 macOS 崩溃/ensure 调查落地为可执行修复任务，同时明确“以 `UCommonSessionSubsystem` 为主线、删除旧 `UMultiplayerSessionsSubsystem` 后，如何继续对接 Steam”。

---

## 0. 当前环境和结论边界

### 已知环境

- Unreal Engine：UE 5.8.0 正式版，崩溃报告中版本为 `5.8.0 (55116800.0.8)`。
- macOS：`macOS 26.5.1 (25F80)`。
- 设备：Apple Silicon，`MacBookPro18,1`，M1 Pro，16GB RAM。
- 本次 macOS 报告里的崩溃进程是 `CrashReportClientEditor`，父进程才是 `UnrealEditor`。
- 当前项目存在 Lyra/CommonGame/CommonUser 迁入痕迹，也仍保留旧教程式 `UMultiplayerSessionsSubsystem`。

### 不能误判的点

- 不要把 `CrashReportClientEditor` 的崩溃直接写成 `UnrealEditor` 主进程崩在 `FOutputDeviceRedirector`。
- Epic 文档说明 UE 的 ensure 一般是“记录错误并向日志/Crash Reporter 报告，但不直接崩溃执行流”。因此本项目当前更像是多个 ensure 触发后，Mac 上 CrashReportClientEditor 处理报告/退出时二次崩溃。
- macOS 26.5.1 是环境风险。Epic UE 5.8 Mac 开发要求页面列出的官方基线仍以 Sonoma 14 / Xcode 15.2+ / 15.4+ 为主，没有把 macOS 26.5.1 当作明确基线。先修项目侧 P0 ensure，再单独判断是否还有 macOS/Metal/CrashReportClient 环境问题。

---

## 1. 外部资料核对后的在线架构结论

### 1.1 `UCommonSessionSubsystem` 可以作为 Steam 的上层入口，不是只能配 EOS

Epic 文档说明：

- Common User 插件提供 C++、蓝图与 Online Subsystem（OSS）或其他在线后端之间的通用接口。
- Common User 默认使用现有 Online Subsystem v1；Online Services v2 是另一路可选测试方向。
- Common Session Subsystem 旨在通过会话接口创建、搜索、加入 gameplay session。
- Lyra 默认在编辑器/局域网里使用 Null 子系统；Lyra 的公开文档主要把 EOS 当作完整示例。

因此：

- 以 `UCommonSessionSubsystem` 为主线并不阻止后续接 Steam。
- Steam 是 `OnlineSubsystemSteam`，属于 OSS 实现之一；CommonSession 在上层调用 session 接口，底层可以是 Null、Steam、EOS 等。
- 但是，Lyra/CommonSession 的公开文档主要保证 Null/EOS 示例路径；文档也提示：如果要对其他在线后端启用完整支持，可能需要修改传给 `FOnlineSessionSearch` 等内部对象的选项。

### 1.2 旧 `UMultiplayerSessionsSubsystem` 应删除或完全降级为非持有型 facade

当前项目更适合：

- 删除旧 `UMultiplayerSessionsSubsystem` 作为独立 session 生命周期拥有者。
- 不允许它长期缓存 `IOnlineSessionPtr` / `IOnlineFriendsPtr`。
- 如果 UI/蓝图已经大量依赖旧函数名，可以新建一个很薄的项目 facade，例如 `UNWOSessionFacadeSubsystem`，但它只能转发到 `UCommonSessionSubsystem`，不能自己持有 OnlineSession 接口，也不能自己创建/销毁底层 session。

原因：

- Epic Session Interface 文档明确：`IOnlineSession` 由 Online Subsystem 创建和拥有，每个平台一次只有一个会话接口。
- 项目当前 `SessionInterface.IsUnique()` ensure 很可能就是旧子系统长期持有 `IOnlineSessionPtr`，与 PIE reload / Null subsystem shutdown 生命周期冲突。

---

## 2. Steam 接入路线：小白可理解版

可以把结构理解成三层：

```text
你的主菜单 / UI / 游戏代码
        ↓
UCommonSessionSubsystem（项目唯一会话入口）
        ↓
OnlineSubsystemSteam / OnlineSubsystemNull / EOS 等具体后端
```

所以：

- 删除旧 `UMultiplayerSessionsSubsystem` 后，不是“Steam 没入口了”。
- Steam 入口不是旧子系统，Steam 入口是 `OnlineSubsystemSteam` 插件 + `DefaultEngine.ini` 配置 + CommonSession 调用底层 session interface。
- UI 不应该直接关心 Steam API。UI 应该调用 CommonSession 的 Host / Find / Join。

---

## 3. Codex 需要执行的修复任务

## P0-A：删除废弃 CVar

文件：`Config/DefaultEngine.ini`

任务：

- 删除：

```ini
r.Mobile.VirtualTextures=False
```

- 保留或按平台拆分：

```ini
r.VirtualTextures=False
```

验收：启动/PIE 不再出现：

```text
r.Mobile.VirtualTextures is deprecated
```

---

## P0-B：移除旧 `UMultiplayerSessionsSubsystem` 主线

### 需要搜索的符号

Codex 全项目搜索：

```text
UMultiplayerSessionsSubsystem
MultiplayerSessionsSubsystem
CreateSession
FindSessions
JoinSession
DestroySession
InviteFriends
GetFriendsList
GetSubsystemName
```

### 处理规则

1. 删除或停用旧 `UMultiplayerSessionsSubsystem` 的 C++ 类文件：
   - `Source/NewWorldOrder/Public/MultiplayerSessions/...`
   - `Source/NewWorldOrder/Private/MultiplayerSessions/...`

2. 从 `Build.cs`、`.uproject`、模块依赖、蓝图调用、UI 调用中移除旧子系统引用。

3. 不允许保留这种写法：

```cpp
IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
SessionInterface = Subsystem->GetSessionInterface();
FriendsInterface = Subsystem->GetFriendsInterface();
```

4. 如果短期因为蓝图引用太多不能立刻删除旧类，只允许把旧类改成 facade：
   - 不缓存 `IOnlineSessionPtr`。
   - 不缓存 `IOnlineFriendsPtr`。
   - 不直接 Create/Find/Join/Destroy 底层 session。
   - 只转发到 `UCommonSessionSubsystem`。
   - `Deinitialize()` 必须清 delegate，并重置任何临时引用。

验收：PIE 启动时不再出现：

```text
Ensure condition failed: SessionInterface.IsUnique()
```

---

## P0-C：以 `UCommonSessionSubsystem` 接管 Host / Find / Join

### Codex 需要先检查的源码

重点看：

```text
Plugins/CommonUser/Source/CommonUser/Private/CommonSessionSubsystem.cpp
Plugins/CommonUser/Source/CommonUser/Public/CommonSessionSubsystem.h
```

尤其检查这些 OSSv1 路径：

```text
CreateOnlineSessionInternalOSSv1
FindSessionsInternalOSSv1
JoinSessionInternalOSSv1
CleanUpSessionsOSSv1
BindOnlineDelegates
```

### 目标

- 主菜单 Host Game 调用 `UCommonSessionSubsystem::HostSession`。
- Server Browser 调用 `UCommonSessionSubsystem::FindSessions`。
- Join 调用 `UCommonSessionSubsystem::JoinSession`。
- Quick Play 可后续再做，不要第一轮就把 QuickPlay、Steam lobby、好友邀请全部一起修。

### 如果项目使用 Lyra 风格 Experience

检查项目是否已有或需要补齐类似 Lyra 的：

```text
ULyraUserFacingExperienceDefinition
UCommonSession_HostSessionRequest
UCommonSession_SearchSessionRequest
```

Host request 里应能表达：

- 地图。
- Experience / GameMode。
- 最大人数。
- LAN / Online。
- 是否 private。
- 是否 dedicated。

---

## P0-D：Steam 配置最小路线

> 目标不是一次做完整商业 Steam 集成，而是让 CommonSession 能在 Steam OSS 下创建、搜索、加入会话。

### 1. 插件和模块

`.uproject` 确认启用或显式列出：

```json
OnlineSubsystemSteam
OnlineSubsystemUtils
CommonUser
CommonGame
```

`NewWorldOrder.Build.cs` 确认依赖：

```cpp
PrivateDependencyModuleNames.AddRange(new string[]
{
    "OnlineSubsystem",
    "OnlineSubsystemUtils",
    "CommonUser",
    "CommonGame"
});

DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
```

如果项目已在其他地方声明 OnlineSubsystemSteam，不要重复导致风格混乱；统一整理即可。

### 2. `DefaultEngine.ini` Steam 基础配置

参考 Epic 官方 Steam OSS 文档，先采用 Sessions 路线：

```ini
[/Script/Engine.GameEngine]
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")

[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
bInitServerOnClient=true

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="OnlineSubsystemSteam.SteamNetConnection"
```

注意：

- `SteamDevAppId=480` 只用于开发测试，发行前必须换成项目自己的 Steam App ID。
- 如果已有 `NetDriverDefinitions`，Codex 不要盲目追加多条冲突配置。必要时用 `!NetDriverDefinitions=ClearArray` 后只保留一条项目认可的定义。
- 如果未来改用 Steam Lobby，创建 session 的 `FOnlineSessionSettings` 里通常需要：

```cpp
SessionSettings.bUsesPresence = true;
SessionSettings.bUseLobbiesIfAvailable = true;
```

Codex 要检查 `UCommonSessionSubsystem` 当前项目版本是否已经设置这些字段；如果没有，需要在项目子类或局部 fork 的 CommonSession 中为 Steam 路径补齐。

### 3. Mac 上的 Steam 测试注意

- Steam Overlay 在 Mac 上要求通过 Steam 客户端启动游戏。Epic 文档明确写了这一点。
- 不要只用 PIE 判断 Steam 是否正常。Steam OSS 通常更适合用 Standalone、打包构建、两个 Steam 账号/两台机器测试。
- 如果只是在编辑器里测本地多人，先用 Null 子系统验证项目逻辑；Steam 路线单独验证。

---

## P0-E：Mutable / LeaderPose 修复

当前判断：LeaderPose 需求本身合理，问题是绑定时机。

任务：

1. 从 `BP_ShootCharacter` Construction Script 删除：

```text
BodyMesh.SetLeaderPoseComponent(GetMesh)
```

2. 在 `UMutableAppearanceComponent` 或角色外观统一管理点中，等待 Mutable 更新完成后再绑定。

3. 绑定前必须校验：

- Head mesh 有效。
- Body mesh 有效。
- Head / Body 都有 `SkinnedAsset`。
- Skeleton 或 RefSkeleton 映射兼容。

4. 校验失败时不要调用 `SetLeaderPoseComponent`，只输出项目日志，记录：

- Head mesh 名称。
- Head skeleton 名称。
- Head ref skeleton bone count。
- Body mesh 名称。
- Body skeleton 名称。
- Body ref skeleton bone count。
- 当前 Mutable component name / gender / COI。

验收：PIE 不再出现：

```text
Leader pose component skeleton doesn't match follower
```

---

## P1：运行期日志清理

### `W_Healthbar` 除零

修复：

- 蓝图中所有 `Health / MaxHealth` 前加保护。
- `MaxHealth <= 0` 时返回 0 或跳过材质更新。

验收：不再出现：

```text
Divide by zero: Divide_DoubleDouble
```

### GameplayEffect SetByCaller Data None

修复方向：

- 检查 `ShootEffect_Passive_ArmorEnhancement` 的 modifier `SetByCaller` DataTag 在 CDO 中是否为 None。
- 不要在 GameplayTag 尚未稳定注册的构造时机依赖 `RequestGameplayTag(..., false)` 得到有效值。
- 如果 CDO/热重载污染，重建 GE CDO 或按项目 GameplayTags 初始化规范修复。

验收：不再出现：

```text
FGameplayEffectSpec::GetMagnitude called for Data None
```

---

## P2：项目依赖整理

`.uproject` 补齐项目 Build.cs 已依赖但未显式列出的插件，例如：

```text
UIExtension
ModularGameplay
```

目标：消除 UBT 警告，避免跨机器或打包时插件加载顺序问题。

---

## 4. 验证流程

### 第一轮：只验证项目侧 ensure 清理

1. 不启用 Steam，使用 Null 或当前 editor 默认路径。
2. 连续启动 PIE 3 次。
3. 确认不再出现：

```text
r.Mobile.VirtualTextures is deprecated
SessionInterface.IsUnique()
Leader pose component skeleton doesn't match follower
```

### 第二轮：验证 Steam OSS 配置是否接通

1. Steam 客户端保持运行。
2. 使用 `SteamDevAppId=480` 或真实 App ID。
3. 用 Standalone 或打包构建测试，不要只看 PIE。
4. 两个账号/两台机器测试：
   - A 创建会话。
   - B 搜索会话。
   - B 加入会话。
   - 观察是否成功 ClientTravel 到 A。

### 第三轮：如果 Mac 仍然异常，再查环境问题

只有在 P0 ensure 清干净后仍然崩溃，才单独调查：

- macOS 26.5.1 / Xcode / Metal toolchain。
- UE 5.8 Mac CrashReportClientEditor 是否存在独立 bug。
- 是否空白项目也启动崩溃。
- 是否出现 MetalRHI / shader converter / bindless 相关栈。

不要在 P0 未完成前先改 Unreal Engine 源码。

---

## 5. Codex 禁止事项

- 不要把 `CrashReportClientEditor` 二次崩溃直接当成主进程根因。
- 不要重新引入旧 `UMultiplayerSessionsSubsystem` 作为会话主线。
- 不要长期持有 `IOnlineSessionPtr` / `IOnlineFriendsPtr`。
- 不要同时让旧子系统和 `UCommonSessionSubsystem` 都创建/销毁 session。
- 不要为了 Steam 改回教程式旧系统。
- 不要先改 UE 引擎源码。
- 不要把 LeaderPose 需求本身删掉；应改绑定时机和骨架校验。
- 不要把 Lyra 的 EOS 配置直接当 Steam 配置照抄。

---

## 6. 2026-07-22 Mac Shipping 新反馈的证据边界

用户的新实测分成两个现象：Windows 创建 Steam 会话后 Mac 搜索不到；Mac 创建会话并 Travel 到 HomeMap 后画面冻结。它们发生在 Find 和 Host Travel 阶段，不能直接套用“离开会话时 DestroySession 回调丢失”的解释。

目前只有二次整理报告，没有这次冻结对应的原始项目日志、`.ips`、活动监视器采样或线程栈，因此以下结论仍未成立：

- 不能确认 Mac Steam `DestroySession` 回调丢失，因为本次冻结发生在 Host 创建和地图 Travel 后。
- 不能在模块 `ShutdownModule` 手工关闭 OnlineSubsystem；这可能与引擎自己的模块卸载顺序发生二次竞争。
- 不能在三秒后强制把 Coordinator 设成 Idle；平台仍有 `NAME_GameSession` 时开放下一次 Host/Join 会制造重叠请求。
- 不能通过无条件 Find 重试掩盖搜索结果不足、版本不兼容或平台回调未完成。

这轮项目侧安全改动是：

- 删除项目 `BuildIdOverride`，恢复引擎默认网络版本，同时保留 UI 中的实际 BuildId 诊断。
- 针对 SteamDevAppId 480 的公共 Spacewar Lobby 池，把项目层 Steam Find 候选窗口从 CommonUser 固定的 10 条扩大到 1000；不修改 CommonUser 插件。
- 使用官方 `SteamSocketsNetDriver` 并保留 `IpNetDriver` fallback。
- 为每次 Coordinator 状态迁移输出旧状态、新状态、地图和 NetMode；HomeHub BeginPlay 输出 `listen`、`bIsLanMatch` 与是否执行 Standalone cleanup。

下一次 Mac 冻结时保留：

1. `.app/Saved/Logs/NewWorldOrder.log`，不要只截取最后几行。
2. 活动监视器对无响应进程执行“取样进程”得到的完整文本。
3. 如果产生崩溃或强制退出报告，保存原始 `.ips`。
4. 记录点击 Host 到冻结之间 UI 文案最后停在哪个状态，以及日志最后一条 `LogShootSessionCoordinator`、`LogShootHomeHubState`、`LogNet`、`LogLoad`。

这些证据可以区分：Steam Create 回调未完成、ServerTravel/Map Load 阻塞、HomeHub 错误清理、SteamSockets 建链问题、渲染线程卡住和纯 UI 输入锁定。证据收齐前不改引擎源码，也不宣布 Mac 根因已关闭。

---

## 7. 参考资料

- Epic：Common User Plugin / Lyra Sample Game（UE 5.8）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/common-user-plugin-in-unreal-engine-for-lyra-sample-game

- Epic：Online Subsystem Steam Interface（UE 5.8）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/online-subsystem-steam-interface-in-unreal-engine

- Epic：Online Subsystem Session Interface（UE 5.8）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/online-subsystem-session-interface-in-unreal-engine

- Epic：Crash Reporting in Unreal Engine（UE 5.8）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/crash-reporting-in-unreal-engine

- Epic：macOS Development Requirements（UE 5.8）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/macos-development-requirements-for-unreal-engine

- Epic Forum：Lyra with Steam 讨论  
  https://forums.unrealengine.com/t/lyra-with-steam/1693860
