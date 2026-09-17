# CommonSession 与 Steam 接入审计

# 当前项目事实

- 当前项目已经包含 CommonGame、CommonUser，并且 `UShootGameInstance` 继承 `UCommonGameInstance`。
- 当前 `CommonSessionSubsystem.h/.cpp` 与本机 UE 5.8 LyraStarterGame 对应源码的 SHA-256 完全一致，不需要修改 CommonUser 插件源码。
- 当前项目仍保留旧 `UMultiplayerSessionsSubsystem`，但正式源码中未发现其他调用者。
- 当前项目只启用了 `OnlineSubsystemSteam` 插件和项目模块依赖；Config 中没有 Steam 的 `DefaultPlatformService`、Steam NetDriver、Steam AppId 或 CommonSession Steam 特殊配置。
- 删除旧子系统本身不是主要难点。真正需要走通的是 CommonSession、CommonUser、Steam OSSv1、SteamSockets、前端 UI、Travel 和打包验收的完整链路。

# Lyra 5.8 Steam 配置结论

本机同版本 Lyra 使用：

```text
Config/Custom/Steam/DefaultEngine.ini
```

Steam 测试通过 `-CustomConfig=Steam` 选择这组配置，使普通编辑器测试与 Steam 测试互不污染。

关键配置包括：

- `[OnlineSubsystem] DefaultPlatformService=Steam`
- 启用 SteamSockets，并将 GameNetDriver 与 BeaconNetDriver 指向 `SteamSocketsNetDriver`
- `CompatibleUniqueNetIdTypes` 只保留 Steam
- CommonSession 设置 `bUseBeacons=false`

Lyra 的配置注释明确说明：SteamSocketsNetDriver 当前不支持 CommonSession 的 Reservation Beacon 流程，因此 Steam 配置必须关闭 `bUseBeacons`。否则 JoinSession 成功后仍可能卡在 Beacon 预留阶段，无法进入最终 ClientTravel。

开发期没有正式 Steam App ID 时，可先使用公开测试 App ID `480` 做联通性验证；发布前必须替换成项目自己的 App ID。

# CommonSession 调用链

- Host：`CreateOnlineHostSessionRequest` -> 设置 OnlineMode、MapID、ModeName、MaxPlayerCount -> `HostSession`
- Find：`CreateOnlineSearchSessionRequest` -> 绑定 `OnSearchFinished` -> `FindSessions`
- Join：保存 `UCommonSession_SearchResult` -> `JoinSession`
- Leave/Return：进入前端流程时调用 `CleanUpSessions`
- Steam 邀请：`OnUserRequestedSession` -> `UCommonGameInstance` 保存请求 -> 前端登录完成后 `JoinSession`
- HostSession 成功后 CommonSession 负责 ServerTravel；JoinSession 成功后 CommonSession 解析连接字符串并 ClientTravel。

# 本地分屏边界

CommonSession 的 OSSv1 Host 和 Join 流程中仍保留 Lyra 的 TODO：第二个本地玩家注册到在线 Session 的代码没有启用。因此不能把“CommonSession 支持 LocalPlayer 参数”误写成“在线分屏已经天然完成”。

本项目当前模式边界是：

- 本地双人分屏是独立的同机合作模式。
- 在线合作每台设备只有一个 LocalPlayer，最多四台设备加入 Listen Server。

因此第一阶段先走通单 LocalPlayer 的 Steam Listen Server，不把在线加本地分屏混为同一验收项。未来如果产品确实需要一台设备上的两个本地玩家共同加入在线 Session，必须专门实现并验证本地玩家注册、UniqueNetId、Session 成员、Travel 和输入归属。

# 前端 UI 迁移原则

- 13000 旧项目只读，不修改其资产和源码。
- 旧菜单可参考视觉、文案和 Host/Find/Join 交互流程，不迁移旧 `ShootUIBaseContainer`、OwnerContainer、自建 WidgetStack 或旧 Session C++。
- 当前项目前端页面继承稳定 CommonUI 类，通过目标 OwningPlayer 调用 `UCommonSessionSubsystem`。
- Session 列表直接使用 `UCommonSession_SearchResult` 与 ListView，不再暴露 `FOnlineSessionSearchResult` 给 Widget。
- CommonSession 已提供请求对象和异步完成委托，初期不额外创建第二个 Session 服务或全局 GameplayMessage 包装层。

# 后续实施顺序

1. 增加与 Lyra 5.8 对齐的 Steam CustomConfig 和 SteamSockets 配置。
2. 在当前项目重建旧菜单中有价值的视觉与交互流程，并接入 CommonSession。
3. 使用 Development Standalone 或打包程序、Steam 客户端和两个不同 Steam 账号验证 Host、Find、Join、Travel、Leave 和邀请。
4. 新链通过验收后，删除 `UMultiplayerSessionsSubsystem` 和仅由它使用的项目模块依赖；不保留双轨。
5. 单 LocalPlayer 在线链稳定后，再根据产品需求决定是否开展在线分屏专项。
