# Session生命周期技术限制调研 - 问题陈述

文档目的：记录UE Listen-Server架构下的技术限制和我们的游戏需求，用于咨询技术方案。

日期：2025-11-10

---

## 一、项目需求概述

### 游戏类型
- 双主角动作游戏（男主陈浩宇、女主沈芸皖）
- 支持三种游戏模式：
  1. **单人模式**：1个玩家 + 1个AI队友
  2. **本地分屏**：2个本地玩家
  3. **在线合作**：2-4个在线玩家（通过Steam P2P）

### 架构选择
- 使用Listen Server架构（不是Dedicated Server）
- 单人模式也运行Listen Server（无其他玩家加入）
- 在线模式Host创建Session，Client通过JoinSession加入

### 核心技术栈
- Gameplay Ability System (GAS)
- ASC (AbilitySystemComponent) 在 PlayerState 上
- Seamless Travel 用于地图切换
- Steam Online Subsystem

---

## 二、Session生命周期研究结果

### 1. Session的核心机制

**生命周期流程：**
```
CreateSession → StartSession → JoinSession → EndSession → DestroySession
```

**关键特性：**
- 所有Session操作（CreateSession、JoinSession、DestroySession）都是异步的
- 必须等待对应的OnComplete委托（OnCreateSessionComplete、OnJoinSessionComplete、OnDestroySessionComplete）才能执行后续操作
- Host调用StartSession后，Session才对外可见（客户端才能FindSessions找到）

**官方文档来源：**
- [Online Subsystem Session Interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-session-interface-in-unreal-engine)
- IOnlineSession::CreateSession() 会在后台线程创建Session，调用后立即返回，Session实际创建完成时触发 OnCreateSessionComplete 委托

### 2. Host与Client的DestroySession行为差异

**Host调用DestroySession：**
- 服务器会通知所有客户端断开连接（包括自己）
- 所有客户端会被自动踢回主菜单
- Steam Lobby（如果使用Steam）会被关闭
- Session从在线列表中移除

**Client调用DestroySession：**
- **仅影响客户端本地的Session状态**
- **不会影响Host的Session继续存在**
- 客户端需要配合 HandleDisconnect 和 ClientTravel 才能正确离开
- 其他客户端不受影响，可以继续游戏

**社区验证：**
来自UE论坛和Stack Overflow的讨论证实：
> "客户端调用 DestroySession 通常只影响客户端本地的 session 状态或根本无效"
> "当Host取消游戏时，他们应该销毁session和hosting net driver，通过LoadMap和HandleDisconnect返回主菜单"

**来源：**
- [How to destroy all Client-Sessions when the Host leaves the game](https://stackoverflow.com/questions/66952247/)
- [Do clients need to call EndSession or just the host?](https://forums.unrealengine.com/t/538547)

### 3. ReturnToMainMenu实现

**UE官方实现：**
```cpp
// UGameInstance::ReturnToMainMenu 内部实现
void UGameInstance::ReturnToMainMenu()
{
    // 调用 HandleDisconnect（在 OnlineSession 或 Engine 上）
    // 触发 Hard Travel 回到项目设置的 Default Map
}
```

**关键行为：**
- ReturnToMainMenu 触发的是 **Hard Travel**，不是 Seamless Travel
- Hard Travel 会重建 PlayerController、PlayerState、Character、ASC 等所有对象
- 返回主菜单后，所有游戏内状态都会丢失

**服务端断开客户端的方法：**
```cpp
// 服务端通知客户端返回主菜单
APlayerController::ClientReturnToMainMenuWithTextReason(FText Reason)
```

**官方文档来源：**
- [UGameInstance::ReturnToMainMenu API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameInstance/ReturnToMainMenu)
- 文档描述："Calls HandleDisconnect on either the OnlineSession if it exists or the engine, to cause a travel back to the default map."

### 4. Seamless Travel vs Hard Travel的持久化差异

| 对象类型 | Seamless Travel | Hard Travel |
|---------|----------------|-------------|
| PlayerController | **保留** | 重建 |
| PlayerState | **保留**（通过CopyProperties） | 重建 |
| ASC (在PlayerState上) | **保留** | 重建 |
| Character/Pawn | 重建 | 重建 |
| GameMode | 重建 | 重建 |

**Seamless Travel的持久化机制：**
- PlayerState 通过 `APlayerState::CopyProperties()` 将数据从旧实例复制到新实例
- PlayerController 可以通过 `GetSeamlessTravelActorList()` 指定需要跨地图保留的Actor
- PlayerState 通过 `OverrideWith()` 处理断线重连场景

**技术文档来源：**
- [Travelling in Multiplayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine)
- [Unreal Engine Persistent Data Compendium](https://wizardcell.com/unreal/persistent-data/)
- [Cedric Neukirchen - Traveling in Multiplayer](https://cedric-neukirchen.net/docs/multiplayer-compendium/traveling-in-multiplayer/)

**关键引用：**
> "Seamless travel is a non-blocking operation, while non-seamless will be a blocking call. It is recommended that Unreal Engine multiplayer games use seamless travel when possible."

> "Data is copied from old instance of PlayerState over to a new one upon seamless traveling or disconnecting. The PlayerState class has two critical methods: CopyProperties() and OverrideWith()."

### 5. 单机/分屏/在线的架构统一性

**研究结论：**
- UE的Listen Server代码可以用在单机上
- 单人模式 = Listen Server（无其他玩家加入）
- 本地分屏 = Listen Server（多个本地PlayerController，通过CreateLocalPlayer创建）
- 在线模式 = Listen Server（远程玩家通过Session加入）

**本地分屏实现：**
```cpp
// 创建第二个本地玩家
UGameInstance::CreateLocalPlayer(int32 ControllerId, FString& OutError, bool bSpawnPlayerController)
```

**社区最佳实践：**
> "Understanding Unreal's Gameplay Framework will not only make your code well-structured, but also keep you from reinventing the wheel, and multiplayer-wise game framework Objects can behave differently over the network."

> "Functions like GetPlayerController can do more harm than good unless you're doing local multiplayer (split-screen) only, and indices won't be consistent across different clients and servers."

**来源：**
- [Unreal Engine Multiplayer Tips and Tricks](https://wizardcell.com/unreal/multiplayer-tips-and-tricks/)
- [Local Multiplayer Tips](https://unrealcommunity.wiki/local-multiplayer-tips-993f4t24)

---

## 三、我们遇到的技术限制

### 限制1：DestroySession必要性

**问题陈述：**
如果玩家离开游戏（从副本返回主菜单）时不调用DestroySession，下次尝试CreateSession或FindSessions会失败。

**社区验证：**
> "If you want to leave a game, make sure to destroy the session, otherwise your user will encounter an error when they try to create or join a session again."

> "There are cases where bSuccess is true but the session isn't destroyed - as a workaround, you should try to destroy any session 'NAME_GameSession' before you join or create one."

**来源：**
- [Networking: How to properly leave a session as a client?](https://forums.unrealengine.com/t/346022)
- [DestroySession not working in UE4 4.25](https://communityforums.atmeta.com/discussions/843705)

### 限制2：ReturnToMainMenu的Hard Travel特性

**问题陈述：**
`UGameInstance::ReturnToMainMenu()` 触发Hard Travel，会重建所有对象（PlayerController、PlayerState、ASC）。这意味着：
- 无法像《艾尔登法环》那样从副本返回到一个持久的大世界
- 必须返回到一个"空白"的主菜单地图

**技术原因：**
ReturnToMainMenu 内部调用 HandleDisconnect，这是一个 Absolute 或 Partial Travel（取决于断开场景），都会导致Hard Travel。

**来源：**
- [UGameInstance::ReturnToMainMenu Documentation](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/UGameInstance/ReturnToMainMenu/)
- 官方描述："HandleDisconnect is called by UEngine::HandleDisconnectCommand(), and disconnection is in fact a travel of type Absolute or Partial which in either case results in hard travel."

### 限制3：ServerTravel后的Session状态

**观察结果：**
- 使用 `GetWorld()->ServerTravel(MapName, true)` (Seamless Travel) 可以保持Session存活
- 所有客户端会跟随ServerTravel到新地图
- Session不会被销毁，玩家可以继续组队

**但是：**
- 如果玩家想"离开队伍"，必须先DestroySession再返回主菜单
- 离开队伍时似乎必须触发Hard Travel（ReturnToMainMenu），无法Seamless Travel回到一个"持久世界"

**疑问：**
能否在保持Session的情况下，ServerTravel回到一个"Lobby Level"，让玩家选择"继续组队"或"离开队伍"？

---

## 四、我们的具体需求场景

### 场景1：单人模式完成副本

**期望流程：**
```
玩家在副本中完成任务
  ↓
想要返回到某个地方（主菜单？家园？大厅？）
  ↓
可以继续选择下一个副本，或者切换角色、购买装备
```

**问题：**
- 应该返回到哪里？
- 如果返回主菜单，是否需要每次重新Travel到家园？
- 能否有一个"持久的家园地图"，像《致命解药》那样？

### 场景2：在线模式完成副本

**期望流程：**
```
4个玩家在副本中完成任务
  ↓
想要继续组队打下一个副本
  ↓
但某个玩家想离开队伍
  ↓
剩余玩家继续组队（不需要重新匹配）
```

**问题：**
- 副本完成后应该ServerTravel到哪里？
- 能否ServerTravel回到一个"Lobby Level"，保持Session？
- 如果玩家离开Lobby，必须DestroySession，然后ReturnToMainMenu（Hard Travel）吗？
- 有没有办法让玩家"优雅地离开"，而不是直接Hard Travel丢失所有状态？

### 场景3：本地分屏

**期望流程：**
```
玩家1在家园准备
  ↓
选择副本，想要本地双人分屏
  ↓
CreateLocalPlayer(1)，玩家2加入
  ↓
完成副本后
  ↓
返回某个地方（主菜单？）
  ↓
移除玩家2（RemoveLocalPlayer）
```

**问题：**
- 本地分屏能否进入家园（Hub）？
- 如果不能，本地分屏从哪里开始？直接从主菜单选副本吗？
- 分屏模式下，两个玩家各自有独立的存档，如何管理？

---

## 五、关键疑问汇总

### 疑问1：能否有一个"持久的Hub地图"？

**背景：**
我们希望设计一个"家园（Hub）"地图，类似《致命解药》的安全屋：
- 玩家可以在这里自由探索、切换角色、购买装备
- 完成副本后可以返回这里
- 这个地图应该"持久"存在，不会因为Hard Travel而丢失状态

**问题：**
- 如果从Hub进入副本（Seamless Travel），再从副本返回Hub（Seamless Travel），ASC和PlayerState可以持久保留吗？
- 如果玩家选择"退出游戏"，必须DestroySession + ReturnToMainMenu（Hard Travel），这时Hub的状态会丢失，对吗？
- 有没有办法让Hub的某些状态（比如NPC对话进度）在Hard Travel后恢复？

### 疑问2：在线模式的Lobby应该是Level还是Widget？

**背景：**
我们需要一个"匹配大厅"，让在线玩家等待和准备：
- Host创建Session后，进入大厅
- Client通过JoinSession加入大厅
- 所有玩家准备后，Host点击"开始"，ServerTravel到副本
- 副本完成后，想要返回大厅，继续组队打下一个副本

**问题：**
- 如果大厅是Widget（纯UI），副本完成后能ServerTravel回去吗？（应该不行，因为ServerTravel需要目标Map）
- 如果大厅必须是Level，那它和Hub有什么区别？
- 玩家在Lobby Level中选择"离开队伍"，必须DestroySession + ReturnToMainMenu（Hard Travel）吗？
- 能否有一种"软离开"的方式，比如只是断开连接，但不触发Hard Travel？

### 疑问3：MainMenu应该是Level还是纯UI？

**背景：**
我们希望MainMenu像《致命解药》那样，有3D场景展示角色模型，而不是纯UI。

**问题：**
- 如果MainMenu是Level，玩家从副本ReturnToMainMenu（Hard Travel）回来时，能否保留存档信息？
- MainMenu Level应该创建PlayerState和ASC吗？还是只是展示用的空关卡？
- 如果MainMenu不创建ASC，那么玩家Travel到Hub时，何时初始化ASC？

### 疑问4：DestroySession + ReturnToMainMenu的正确顺序

**背景：**
客户端想离开队伍时，应该先DestroySession还是先ReturnToMainMenu？

**问题：**
- 正确的顺序是什么？
  - 方案A：DestroySession → 等待OnDestroySessionComplete → ReturnToMainMenu
  - 方案B：ReturnToMainMenu → 在回到MainMenu后调用DestroySession
- 如果先ReturnToMainMenu（Hard Travel），会不会导致Session状态残留？
- 如果先DestroySession，是否需要等待异步完成才能ReturnToMainMenu？

### 疑问5：Session能否跨多个地图Travel？

**背景：**
我们希望在线模式的流程是：
```
MainMenu → CreateSession → Lobby Level → Dungeon Level → 返回Lobby Level
```

**问题：**
- Session能否在多次ServerTravel后保持存活？
- 如果Host在Dungeon中调用 `GetWorld()->ServerTravel("/Game/Maps/Lobby?listen", true)`，Session会保持吗？
- 如果Client在Lobby中想离开，调用DestroySession后，是否必须ReturnToMainMenu？还是可以只是断开连接？

---

## 六、参考资料

### UE官方文档
1. [Online Subsystem Session Interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-session-interface-in-unreal-engine)
2. [Travelling in Multiplayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine)
3. [UGameInstance::ReturnToMainMenu API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameInstance/ReturnToMainMenu)
4. [IOnlineSession::DestroySession API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/OnlineSubsystem/Interfaces/IOnlineSession/DestroySession)

### 社区资源
1. [Cedric Neukirchen's Multiplayer Compendium](https://cedric-neukirchen.net/docs/multiplayer-compendium/)
2. [ikrima's GameDev Guide - Session Management](https://ikrima.dev/ue4guide/networking/online-subsystem/)
3. [WizardCell - Persistent Data Compendium](https://wizardcell.com/unreal/persistent-data/)
4. [WizardCell - Multiplayer Tips and Tricks](https://wizardcell.com/unreal/multiplayer-tips-and-tricks/)

### 论坛讨论
1. [Stack Overflow - How to destroy all Client-Sessions](https://stackoverflow.com/questions/66952247/)
2. [UE Forums - Do clients need to call EndSession](https://forums.unrealengine.com/t/538547)
3. [UE Forums - How to disconnect from listen server](https://forums.unrealengine.com/t/752906)
4. [UE Forums - Networking: How to properly leave a session](https://forums.unrealengine.com/t/346022)

---

## 七、请其他AI帮助解答的问题

我们希望设计一个合理的游戏流程，支持以下需求：
1. 单人模式、本地分屏、在线合作三种模式
2. 有一个"家园（Hub）"地图，供玩家准备、切换角色、购买装备
3. 在线模式需要一个"大厅（Lobby）"，供玩家等待和准备
4. 副本完成后，玩家可以选择"继续组队"或"离开队伍"
5. 保持ASC和PlayerState的持久性（使用Seamless Travel）

但是我们遇到了以下技术限制：
- ReturnToMainMenu触发Hard Travel，会丢失所有状态
- DestroySession必须在离开前调用，否则下次无法创建/加入Session
- 似乎无法设计一个"持久的大世界"，让玩家在副本和大世界之间Seamless Travel

**核心问题：**
在Listen Server + Session的架构下，如何设计一个合理的地图流程，既能支持在线组队（Session管理），又能保持玩家状态的持久性（Seamless Travel + ASC保留）？

---

文档结束。请基于以上技术限制和需求，提供可行的技术方案或架构建议。
