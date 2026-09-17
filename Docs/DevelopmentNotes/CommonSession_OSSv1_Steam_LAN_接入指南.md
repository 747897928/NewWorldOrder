# CommonSession OSSv1 Steam LAN 接入指南

日期：2026-07-18
状态：[当前项目已接入] [待Steam双账号打包验收]

## 目标

本项目以 CommonUser 的 `UCommonSessionSubsystem` 作为唯一会话服务。它覆盖 Host、Find、Join、CleanUp 和平台邀请，不再保留自定义 `IOnlineSessionPtr`、`IOnlineFriendsPtr` 或 `UMultiplayerSessionsSubsystem`。

这套方式适用于 Listen Server、Steam 联机和 Null/LAN 测试，并且会话入口始终接收目标 `APlayerController`，因此不会把本地分屏玩家错误地固定到 Player 0。

## 空白项目准备

1. 从同一 UE 版本的 Lyra 复制 `Plugins/CommonUser`、`Plugins/CommonGame`。若空白项目还没有 Lyra UI 基础设施，同时复制 `UIExtension`、`ModularGameplayActors`、`GameplayMessageRouter`；CommonUI、Enhanced Input、OnlineSubsystemSteam 使用引擎插件。不要混用不同引擎版本的插件源码。
2. 在 `.uproject` 启用 `CommonUI`、`OnlineSubsystemSteam`、`UIExtension`、`ModularGameplayActors`，并确认上述项目插件能编译。
3. 让项目 GameInstance 继承 `UCommonGameInstance`。本项目的 `UShootGameInstance` 已继承它；CommonGame 会在 `Init` 中把平台邀请和销毁请求接到 `UCommonSessionSubsystem`。
4. 不创建第二个 `UGameInstanceSubsystem` 直接缓存 OSSv1 interface。游戏代码和 Widget 只拿 `GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>()`。
5. 不把 Host、Find、Join、Destroy 的 OSS delegate 写进 Widget EventGraph。Widget 只创建请求、调用 CommonSession，并监听请求或子系统公开事件。

本项目模块至少依赖：

```csharp
PublicDependencyModuleNames.AddRange(new[]
{
    "CommonGame",
    "CommonUser",
    "CommonUI",
    "CommonInput",
    "EnhancedInput",
    "OnlineSubsystem",
    "OnlineSubsystemSteam",
    "OnlineSubsystemUtils",
    "UIExtension"
});
```

具体项目可以按头文件实际暴露范围拆成 Public/Private Dependency，但不能因为 Widget 需要会话状态就把 OSSv1 interface 重新暴露给 UI。

## DefaultEngine.ini

开发期 Steam 配置如下。`480` 是 Steam Spacewar 测试 AppID，正式包必须替换成自己的 Steam AppID，并在 Steamworks 后台配置对应的 Depot 与联机能力。

```ini
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
bRelaunchInSteam=false
bInitServerOnClient=true
bUseSteamNetworking=true

[/Script/Engine.GameEngine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/CommonUser.CommonSessionSubsystem]
bUseLobbiesDefault=true
bUseLobbiesVoiceChatDefault=false
bUseBeacons=false
```

`bInitServerOnClient=true` 对 Steam Listen Server 必需。第一阶段关闭 Reservation Beacon，先验证基础 Create、Find、Join、Travel；不要为了暂未使用的预留流程增加额外失败点。

`bRelaunchInSteam=false` 允许开发机从编辑器或命令行直接启动测试实例；正式 Steam 包由 Steam 客户端启动，不应依赖这个开发期便捷设置。

### BuildUniqueId 与跨平台搜索

UE 5.8 的 OSSv1 Steam Lobby 搜索并不是“找到 Lobby 就显示”。Lobby 数据转换成搜索结果后，引擎会比较远端 `SessionSettings.BuildUniqueId` 与本机 `GetBuildUniqueId()`；不一致的结果会被直接移除，日志包含 `Removed incompatible build`。因此 Windows Development、Windows Shipping、Mac Shipping 即使使用相同 AppID，也可能因为自动生成的 BuildId 不同而互相看不见。

本项目已按当前验收决定删除 `BuildIdOverride`，恢复 Lyra/引擎默认的网络版本计算。删除覆盖值不等于禁用兼容性检查：Steam OSS 仍会把本机 `GetBuildUniqueId()` 与 Lobby 广告值比较。规则如下：

- Windows 与 macOS 必须来自同一提交，并优先使用相同构建配置，再比较 Find 结果。
- Development 与 Shipping 可以用于诊断，但正式跨平台验收应优先使用相同构建配置，排除配置差异。
- 如果日志出现 `Removed incompatible build`，先对照 SessionScreen/日志中的实际 BuildId；只有产品确实需要自建网络协议版本策略时才重新引入项目级覆盖值。
- SessionScreen 会显示实际 NamedSession 的 BuildId，测试人员不需要只靠日志猜测。

### SteamDevAppId 480 与十条搜索上限

CommonUser OSSv1 的 `FCommonOnlineSearchSettingsOSSv1` 固定 `MaxSearchResults=10`。`SteamDevAppId=480` 是所有 Spacewar 测试项目共享的公共 Lobby 池，因此“搜索成功但本项目结果为零”不一定表示对方没有创建 Lobby，也可能是本项目 Lobby 没进入前十条候选。

当前项目不修改 `Plugins/CommonUser`。`UShootSessionCoordinatorSubsystem` 仅在 Steam Online 搜索期间临时使用与 CommonSession 相同的 `OSSv1`、Lobby 和 `GameSession` 查询条件，把候选上限扩大为 `SteamSearchMaxResults=1000`，再包装回 `UCommonSession_SearchResult`。LAN、Host、Join、Cleanup 和邀请仍走 CommonSession 主线。正式 Steam AppId 不再共享 Spacewar 池后，可在 `DefaultGame.ini` 调低该值。

项目启用 `SteamSockets`，GameNetDriver 使用 `SteamSocketsNetDriver` 并保留 `IpNetDriver` 兜底。这是项目配置，不是 CommonUser 插件补丁；空白项目复制接入时还要在 `.uproject` 启用 `SteamSockets`。

打包 Steam 测试时，运行目录仍需有 `steam_appid.txt`，内容为开发期或正式期 AppID。该文件不应提交到仓库的发布配置；正式发布由 Steam 客户端和 Depot 提供应用身份。

### Windows Shipping 打包与开发插件

开发插件和资产运行时依赖必须分开判断，不能简单地把所有 Toolset 一起开启或一起删除：

- `VibeUE`、MCP 和 AI Toolset 只服务编辑器开发，打包时可以禁用；需要再次编辑蓝图时重新启用所需插件并重启编辑器。
- `SequencerAnimMixerToolset` 是 EditorOnly 工具，可以禁用。
- `/Game/UI/Mutable/Animation/ABP_WardrobePreview_Female` 保存了 `AnimBlueprintExtension_SequencerMixerTarget`，其类属于运行时插件 `MovieSceneAnimMixer`。即使不使用对应 Toolset，也必须在 `NewWorldOrder.uproject` 显式启用该运行时插件，否则 Cook 会报 `Unknown structure` 和缺失 `/Script/MovieSceneAnimMixer`。
- 上述配置只修改项目的插件启用清单，不修改引擎插件、CommonUser 或 Lyra 源码。

本机 UE 5.8 还存在 Zen project store 的 IPv6 回环问题。项目通过 `Config/DefaultGame.ini` 的官方 `UProjectPackagingSettings.bUseZenStore=False` 让编辑器 Package Project 使用本地 Cook 输出，最终 Pak/IoStore 格式不变。这条打包基础设施问题与 Steam Session 状态机无关；只有成功生成同一提交、同一配置的两端包后，才进入 Host、Find、Join 和邀请验收。

## 地图注册与 UE 5.8 MapID

CommonSession 的 `MapID` 不是 `FName("HomeMap")`，也不是 `/Game/Maps/HomeMap.HomeMap` 的软对象路径。UE 5.8 的 World Primary Asset 名称使用完整 PackageName，因此本项目的正确值是：

```text
Map:/Game/Maps/HomeMap
```

同时需要在 AssetManager 中把 Map 注册为运行时资产并参与 Cook。本项目采用：

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Map",AssetBaseClass="/Script/Engine.World",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/Maps")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
```

如果日志出现 `Can't find asset data for MapID Map:HomeMap`，优先检查完整 PrimaryAssetId 和 AssetManager 扫描规则，不要改成 C++ 硬编码 `OpenLevel` 绕过 CommonSession。

## Host

Host 不直接创建 `FOnlineSessionSettings`，而是创建 CommonSession 请求。MapID 必须是有效的 World Primary Asset ID，不能传裸地图字符串。

```cpp
UCommonSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>();
UCommonSession_HostSessionRequest* Request = Sessions->CreateOnlineHostSessionRequest();
Request->OnlineMode = ECommonSessionOnlineMode::Online;
Request->bUseLobbies = true;
Request->bUsePresence = true;
Request->MaxPlayerCount = 4;
Request->ModeNameForAdvertisement = TEXT("CampaignOnline");
Request->MapID = FPrimaryAssetId::FromString(TEXT("Map:/Game/Maps/HomeMap"));
Sessions->HostSession(TargetPlayerController, Request);
```

`HostSession` 成功后由 CommonSession 建立 `NAME_GameSession`，广播 `OnCreateSessionCompleteEvent`，再执行 `ServerTravel(Request->ConstructTravelURL())`。不要在回调中再次手动 OpenLevel 或 StartSession。

## Find 与 Join

```cpp
UCommonSession_SearchSessionRequest* Search = Sessions->CreateOnlineSearchSessionRequest();
Search->OnlineMode = ECommonSessionOnlineMode::Online;
Search->bUseLobbies = true;
Search->OnSearchFinished.AddLambda([Sessions, TargetPlayerController, Search](bool bSucceeded, const FText& Error)
{
    if (bSucceeded && !Search->Results.IsEmpty())
    {
        Sessions->JoinSession(TargetPlayerController, Search->Results[0]);
    }
});
Sessions->FindSessions(TargetPlayerController, Search);
```

真实菜单应把 `Search->Results` 映射为 MVVM 列表项，而不是自动加入第一个结果。`UCommonSession_SearchResult` 已提供房主、公开余位、最大人数、Ping 和自定义 Session Setting 的读取接口。点击某一行时把该行的 SearchResult 传给 `JoinSession`。

Join 成功后 CommonSession 解析连接地址并执行 ClientTravel。监听 `OnJoinSessionCompleteEvent` 仅用于关闭 loading UI 或显示错误，不要重复 ResolveConnectString 或 ClientTravel。

## LAN

LAN 使用同一套 UI 与调用点，只替换请求配置：

```cpp
Request->OnlineMode = ECommonSessionOnlineMode::LAN;
Request->bUseLobbies = false;
Search->OnlineMode = ECommonSessionOnlineMode::LAN;
Search->bUseLobbies = false;
```

要运行 Null/LAN 验收，可通过启动参数或平台 ini 覆盖 `DefaultPlatformService=Null`。不要为 LAN 重新启用旧 `UMultiplayerSessionsSubsystem`。

Steam Online 与 LAN 使用同一个 Coordinator 和同一个页面。差异只存在于请求配置，不需要两套 Session 服务：

- Online：`OnlineMode=Online`、`bUseLobbies=true`、`bUsePresence=true`。
- LAN：`OnlineMode=LAN`、`bUseLobbies=false`、`bUsePresence=false`。

## Lyra 前端状态组件到底做了什么

Lyra 的 `ULyraFrontendStateComponent` 不是单纯的 Widget 创建器。它等 Experience 加载完成后运行一条 ControlFlow：

```text
Frontend Experience Loaded
  -> Wait For User Initialization
     -> hard disconnect 时 ResetUserState
     -> 每次进入专用前端图都 CleanUpSessions
  -> Try Show Press Start Screen
     -> 已登录则跳过
     -> 平台需要选择用户时显示 Press Start
     -> 否则自动初始化 LocalPlayer 0
  -> Try Join Requested Session
     -> GameInstance 有缓存邀请且允许 Join：执行 JoinRequestedSession
     -> Join 成功：取消进入主菜单的流程
     -> Join 失败：继续显示主菜单
  -> Push W_LyraFrontEnd to UI.Layer.Menu
```

这里最值得复制的不是 ControlFlow 语法，而是三个生命周期边界：

- 先把用户、Session、邀请状态整理到可预测状态，再显示可操作菜单。
- 平台邀请优先于普通主菜单；接受邀请不应先让玩家手动打开 Find 页面。
- MainScreen 通过目标 LocalPlayer 的 `UPrimaryGameLayout` 异步推入层级，不能无 Owner 地 AddToViewport。

### 为什么本项目暂时没有无条件 CleanUp

Lyra 的 FrontendMap 与比赛地图分离，所以加载 FrontendMap 就意味着玩家应当离开旧会话。当前项目的 HomeMap 暂时同时承担：

- Standalone 前端/大厅入口。
- Listen Server 创建会话后的目标图。
- Client Join 成功后的目标图。

如果在 HomeMap 每次 BeginPlay 都无条件 CleanUp，Listen Server 刚完成 ServerTravel 就会销毁新会话，Client 刚完成 Join 也会立即离开。因此 `UShootHomeHubStateComponent` 当前只在真正的 Standalone HomeMap 入口清理残留会话；除 NetMode 外还检查 Travel URL 的 `listen` 和 `bIsLanMatch`，避免新 World 初始化早期仍报告 `NM_Standalone` 的竞态。它不负责自动打开游戏菜单；`WBP_GameMenu` 只由玩家输入触发。

项目不修改 Lyra 的 CommonUser 插件。`UShootSessionCoordinatorSubsystem` 在项目层先检查 OSSv1 是否真的存在会话：`NoSession` 时不调用上游 `CleanUpSessions()`；存在会话时调用上游清理，并轮询到 `NoSession` 后才恢复 `Idle`。这样既保留官方插件的可复制性，也避免清理尚未完成时立即 Host/Find。

CommonUser OSSv1 存在一个上游边界：在 `NoSession` 状态调用清理可能留下 pending destroy，使下一次 Host 被立即 End/Destroy。本项目不补丁插件源码，而是由 Coordinator 的 `NoSession` 预检绕开该调用。

最终推荐拆成两张职责明确的地图：

```text
FrontEndMap（启动标题、继续游戏、设置、制作人员、退出）
  无条件 CleanUpSessions
  初始化用户
  处理缓存邀请
  推 MainMenu

HomeMap / HomeHub（玩家家园、换装、收藏、联机大厅）或 GameplayMap
  Host/Join Travel 目标
  绝不执行前端 CleanUp
```

拆图后，本项目就可以把前端组件恢复成 Lyra 的“进入专用前端图始终 CleanUp”模式。

## CleanUp、离开、销毁与邀请

- 客户端离开：调用本机 `GameInstance::ReturnToMainMenu()`；本机 CleanUp 后 Travel 到配置的返回地图，不影响主机。
- 主机销毁：调用服务器 `GameMode::ReturnToMainMenuHost()`，让远端客户端分别清理自己的 CommonSession 状态。远端连接关闭还会触发 Engine `HandleDisconnect`；本项目监听 NetworkFailure，并在下一帧用同一个 `SessionReturnMap` 覆盖默认 `GameDefaultMap`，避免客户端被送回 FrontEndMap。
- 返回地图由 `UShootGameInstance` 定义的 `TSoftObjectPtr<UWorld> SessionReturnMap` 提供。它是 Blueprint 可读、Class Defaults 可配置的世界资产引用，同时支持 `DefaultGame.ini` 默认值；本项目默认 HomeMap，Widget 不硬编码地图路径。
- 清理入口收敛到 `UShootSessionCoordinatorSubsystem::CleanUpResidualSession()`，它同时复位 UI 状态、角色和搜索结果，再调用 CommonSession。不要在 Widget 激活/停用时盲目 CleanUp。
- Steam 邀请好友按钮通过当前平台的 External UI 打开 GameSession 邀请面板。按钮执行时以 OSSv1 实际存在的 `NAME_GameSession` 为最终依据，再同步 Coordinator 状态；不能只信任地图切换前的内存 `InSession` 标记，否则 Shipping 包可能误报“不在会话中”。
- Steam Overlay 接受邀请时，CommonSession 触发 `OnUserRequestedSessionEvent`。`UCommonGameInstance` 保存 `RequestedSession`；如果正在另一个会话，先退出并回到返回地图，PlayerController 恢复后再 `JoinRequestedSession()`。
- Lyra 的 CommonGame 默认用 FirstGamePlayer 执行邀请 Join。本项目覆盖 `OnUserRequestedSession/JoinRequestedSession`，优先把事件携带的 `PlatformUserId` 映射到对应 LocalPlayer；平台没有映射时才使用主玩家兜底，以满足未来本地分屏归属约束。
- CommonGame 默认允许邀请到达后立刻 Join。本项目的 `UShootGameInstance::CanJoinRequestedSession()` 额外要求当前已在可配置 `SessionReturnMap`、目标 LocalPlayer 的 PlayerController 已建立、旧会话已清理。冷启动或从 FrontEndMap 接受邀请时先进入 HomeMap，再由短时重试调用 Coordinator 的 `JoinSession()`；邀请链不会绕过项目的 Joining/InSession 状态机。
- 需要自定义“是否接受邀请”弹窗时，由目标 LocalPlayer 的 CommonUI Modal Layer 展示；确认后仍调用 CommonGame/CommonSession 的邀请链，不直接使用 `IOnlineFriends`。

完整角色生命周期如下：

```text
Host
  Idle -> Hosting -> InSession -> ReturnToMainMenuHost
       -> 通知客户端返回 -> 各机器 CleanUp -> 返回地图 -> Idle

Client
  Idle -> Searching -> Results -> Joining -> InSession
       -> ReturnToMainMenu -> 本机 CleanUp -> 返回地图 -> Idle

Invite
  Overlay Accept -> OnUserRequestedSession -> RequestedSession
       -> FrontEndMap/冷启动：进入 SessionReturnMap -> 等 LocalPlayer/PC -> Join
       -> HomeMap 且当前空闲：Coordinator Joining -> Join
       -> 已在会话：退出/CleanUp/返回地图 -> Coordinator Joining -> Join
```

## CommonUI 与本地分屏

- Host、Find、Join 的入口必须显式持有发起动作的 `APlayerController` 或 `ULocalPlayer`。
- Widget 通过 OwningPlayer 获得目标 Controller，不能用 `GetFirstPlayerController`、`GetPlayerController(0)` 或无 Owner 的 CreateWidget。
- 菜单页面通过 `UPrimaryGameLayout` 和 GameplayTag Layer 展示；关闭时调用 `DeactivateWidget()`，禁止 `RemoveFromParent()`。
- `UCommonActivatableWidget` 蓝图勾选 `Is Back Handler` 后，C++ 的 `NativeOnHandleBackAction()` 才能承接 CommonUI 返回。MainMenu、SessionScreen 和确认弹窗都必须有可见按钮，Back 只是跨设备补充，不替代屏幕按钮。
- 固定按钮使用 LyraButtonBase 时，注意 `bOverride_ButtonText` 的含义：它允许 InputAction 的 DisplayName 覆盖固定 ButtonText。普通“联机/返回”等按钮应设为 false，否则获得焦点后可能显示成 `CONFIRM`。
- 搜索结果行需要保留服务器动态文本，本项目直接继承 `UCommonButtonBase`，结构由 Widget 蓝图维护为“图标、服务器文字、最右 InputActionWidget”，C++ 不覆盖 Slot 对齐、颜色或偏移。
- Session 页面遵循严格的数据/表现分层：`UShootSessionCoordinatorSubsystem` 输出原始会话与玩家结构，C++ Widget 父类只转发数据和暴露业务动作；文本控件、格式化、按钮点击、排版与视觉位于 Widget Blueprint。当前 Steam 浏览链使用迁入的 `W_SessionBrowserScreen` 与 `W_SessionBrowserEntry`，它们分别继承项目的 `UShootSessionScreen` 与 `UShootSessionBrowserEntry`；搜索和 Join 进入 Coordinator，Lyra 专属 Experience Cast、直接 CommonSession Join 和进度页依赖已删除。浏览页的可见返回按钮在 EventGraph 调用 `CloseSessionScreen`。不要为方便刷新列表重新引入强制 `BindWidget` 或 C++ `SetText/CreateWidget`。

## 验收顺序

1. 关闭编辑器，确认没有残留 `UnrealEditor` 进程。
2. PIE 只用于游戏逻辑与 LeaderPose 验收，不作为 Steam 验收依据。当前 UE 5.8 Editor PIE 会关闭编辑器进程持有的 Steam OSS 并为 PIE World 创建 Null 实例，因此日志中的 `NULL` 不代表打包实例的 Steam 配置失效。`UnrealEditor.exe -game` 也不是 Steam 最终验收载体；应使用项目自己的独立 Development 或 Shipping 包。
3. Null/LAN：两台同网段机器或两个独立运行实例，Host、Find、Join、返回菜单各一次。
4. Steam：两个不同 Steam 账号、两个独立 Development 或 Shipping 打包实例，Host、Find、Join、Steam Overlay 邀请、客户端离开、主机关闭各一次。
5. Listen Server：确认主机和客户端都进入目标地图，客户端不会二次 ClientTravel。
6. 本地分屏：验证第二 LocalPlayer 不会抢第一玩家的会话 UI 或输入焦点。
7. 检查日志不得再出现 `SessionInterface.IsUnique()`、旧 `MultiplayerSessionsSubsystem`、重复 `CreateSession` 或重复 `CleanUpSessions`。

开发期可用以下非 Shipping 参数驱动本机独立进程回归；它调用的仍是 Coordinator/CommonSession 正式主线，不使用 `open 127.0.0.1` 绕过 Find/Join：

```text
-nosteam
-ShootSessionAutomation=HostHold|ClientLeave|HostDestroy|ClientHold
-ShootSessionAutomationMap=Map:/Game/Maps/HomeMap
```

先运行 `HostHold + ClientLeave` 验证客户端退出不影响主机，再运行 `HostDestroy + ClientHold` 验证主机和客户端都回到配置地图。最终 Steam 验收仍必须使用两个不同账号的独立打包实例。

## 从空白项目到可测试 UI 的最短实施顺序

1. 复制并启用 CommonUser/CommonGame 及依赖，先让空白项目编译。
2. GameInstance 继承 `UCommonGameInstance`，配置项目 GameInstanceClass。
3. 配置 Steam OSS、NetDriver、CommonSession 默认 Lobby 行为。
4. 注册 Map Primary Asset；确认命令或代码查询得到 `Map:/Game/...`。
5. 写一个项目 Coordinator，只保存 CommonSession 请求、结果和页面需要的状态，不保存 `IOnlineSessionPtr`。
6. 建立 PrimaryGameLayout 和 Menu/Modal GameplayTag 层。
7. GameMenu、SessionScreen、结果行和确认 Modal 全部遵循 CommonUI 生命周期，逐页提供可见返回；固定按钮 OnClicked 放在 Widget Blueprint EventGraph，C++ 只暴露业务函数。
8. 如果有专用 FrontEndMap，可在其 GameState Component 中执行 CleanUp、用户初始化和邀请恢复；如果 HomeMap 同时是联机目标图，只在 Standalone 入口清理，不能无条件 CleanUp。
9. 先 Null/LAN 两实例验证，再用两个不同 Steam 账号的独立打包验证 Online 和邀请。

不要从“先把蓝图按钮接到 CreateSession”开始。前端状态、LocalPlayer 归属、返回地图和 CleanUp 边界没有先确定时，按钮虽然能创建一次会话，第二次进入菜单就很容易触发重复 OSS interface、残留 Delegate 或 `SessionInterface.IsUnique()`。

### 为什么蓝图禁用按钮还不够

异步 Host、Find、Join 和 CleanUp 的并发门禁必须位于项目 Coordinator，而不是只依赖 Widget 的 Enabled 状态。按钮快速双击、页面重复激活、平台邀请和自动化入口都可能绕过单个 Widget 的视觉状态。当前项目仅在稳定菜单状态且实际不存在 `NAME_GameSession` 时接收新请求；请求执行中直接拒绝第二次调用，并保持原状态等待真实回调。这样才能从调用入口防止重复 Create/Find/Join，而不是在 OSS 委托已经重叠后处理错误。

平台邀请还有独立的失败收口：邀请解析失败会同时进入 CommonGame 系统消息和 Coordinator Error；冷启动邀请等待 HomeMap、目标 LocalPlayer、PlayerController 或旧会话清理超过十秒时，清除缓存 `RequestedSession` 并向 SessionScreen 保留可见错误，玩家可以重新接受邀请或手动 Find。

### OSSv1 FindSessions 的同步失败回调

部分 OSSv1 实现可能在 `FindSessions()` 返回 `false` 之前同步广播完成委托。若调用方同时在返回值为 `false` 的分支手工通知失败，就会向 UI 发送两次终态，表现为状态文字反复覆盖、解锁动画重复或请求对象被二次消费。

项目层处理方式是：

1. 调用前保存本次 `FOnlineSessionSearch` 指针。
2. 完成委托负责清理当前请求。
3. `FindSessions()` 返回 `false` 时，只有当前请求仍等于调用前保存的指针才手工通知。
4. 若委托已经同步 Reset 请求，返回值分支不再通知。

这项保护位于 `UShootSessionCoordinatorSubsystem::StartExpandedSteamSearch`，不需要修改 `Plugins/CommonUser`。空白项目若没有项目层扩展搜索，不要为预防性处理去补丁 Lyra 插件；只有自己直接调用 OSSv1 `FindSessions()` 时才需要遵守这一防重边界。

## 本项目迁移结论

## 2026-07-24 运行时警示：Mac Steam 冻结与浏览器搜索尚未闭环

截至 2026-07-24，本项目的 macOS 实机验证发现 Steam Online Host 和平台邀请 Join 均可能在进入 HomeMap、Pawn 已出现后冻结。Host 同样会复现，所以不能只把问题归于邀请回调；也不能仅凭二次整理的崩溃报告断言 Steam SDK、`DestroySession` 或某个同步 API 是根因。

同一轮验收还确认 Session Browser 的真实 Find 入口没有闭环。打开浏览器与页面视觉加载不是搜索成功的证据；必须在日志中看到 Coordinator 的 Searching 与完成回调，且页面从结果、空结果或错误三种终态之一退出加载。

因此本指南中“项目迁移结论”描述的是已落地架构，而不是 Steam 发布级可用性声明。继续实现或移植时，先读 `Docs/Tasks/SessionUI/MacSteamFreezeAndFind_Handoff_交接.md`，取得原始日志后再决定是否调整清理超时、Steam 搜索重试或地图后初始化。禁止为了绕过冻结盲目修改 CommonUser/Lyra 插件、Engine 或强制把 Coordinator 置为 Idle。

2026-07-20 已删除 `UMultiplayerSessionsSubsystem` 的头文件和实现文件，当前没有项目源码引用它。会话唯一主线是 CommonUser 的 `UCommonSessionSubsystem`；`WBP_GameMenu`、SessionScreen、结果项、玩家行、确认 Modal 和轻量 HomeHub 状态组件均已接入。SessionScreen 的蓝图现在显示实际平台、`NAME_GameSession` 状态、主客机角色、地图、BuildId 和当前 `GameState.PlayerArray` 玩家/延迟，C++ 仅提供原始数据。平台邀请统一经过项目 Coordinator，并等待 HomeMap、目标 LocalPlayer、PlayerController 与清理状态全部就绪。`Plugins/CommonUser` 保持上游原样。Windows Editor、PIE 与四角色 Null/LAN 双进程已经覆盖代码、Host/Find/Join、双方退出和目标 Ensure；2026-07-23 又完成了 GameMenu、SessionBrowser 和衣柜的真实蓝图按钮返回链。真正 Steam 双账号 Host/Find/Join/邀请、物理按 Q 和确认 Modal 仍需人工完成最终验收。
