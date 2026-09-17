# 游戏流程设计 Game Flow Design

文档版本: 1.1
最后更新: 2025-11-11
状态: 设计阶段(部分已实现)

---

## 实现状态

**已实现的基础设施**:
-  USaveGameSubsystem (SaveInfo/SaveGame分离模式)
-  FCharacterSnapshot双主角存档结构
-  MutableAppearanceComponent性别切换

**待实现的核心功能**:

- Hub/Lobby/Dungeon地图转换流程
- Session管理 (MultiplayerSessionsSubsystem)
- 角色切换完整集成
- 男女主角专属技能系统

**GameplayTag命名规范**: 项目使用`Abilities.Kit.Protagonist.Male/Female`, `Ability.Type.*`, `GameplayCue.*`, 不使用`Combat.*`。

---

## 文档目的

本文档定义《新世界秩序》的完整游戏流程设计，包括：
- 三种游戏模式的流程（单人、本地分屏、在线合作）
- 地图转换和Session管理
- 副本解锁和进度管理
- 存档同步机制

---

## 设计原则

### 原则1：统一架构

- 所有模式使用相同的Listen Server架构
- 单人模式 = Listen Server（无其他玩家加入）
- 本地分屏 = Listen Server（多个本地PlayerController）
- 在线模式 = Listen Server（远程玩家通过Session加入）

**优势：**
- 代码复用率高
- 逻辑一致性强
- 便于测试和维护

### 原则2：Session生命周期管理

**关键规则：**
- 离开Lobby必须回到MainMenu（技术限制）
- 完成副本后可以ServerTravel回Lobby（保持Session）
- 玩家选择离开时DestroySession → ReturnToMainMenu
- Host解散队伍时DestroySession → 强制所有客户端回MainMenu

### 原则3：家园（Hub）是单机专属

**设计决策：**
- 家园中的"另一个主角"是本地存档中的另一个性别角色
- 在线模式不能进家园（避免逻辑混乱）
- 本地分屏不能进家园（2个玩家，2个存档，逻辑冲突）
- 只有单人模式可以进家园

### 原则4：动态模式选择

- MainMenu不分"单人/分屏/在线"三种入口
- 玩家在选择副本后才决定游戏模式
- 灵活性高，用户体验统一

---

## 游戏模式

### 模式1：单人模式

**特点：**
- 1个本地玩家 
- 可以进入家园（Hub）
- 可以切换角色

**流程：**
```
MainMenu
  → 继续游戏（选择存档）
  → Hub（加载存档，生成AI队友）
  → 选择副本
  → 副本准备界面：单人开始
  → Dungeon
  → 完成副本
  → ReturnToMainMenu
  → 循环
```

### 模式2：本地分屏

**特点：**
- 2个本地玩家（玩家1 + 玩家2）
- 不能进入家园
- 不能切换角色（每个玩家固定角色）
- 类似《双人成行》的体验

**流程：**
```
MainMenu
  → 继续游戏（玩家1选择存档）
  → Hub（玩家1，单人）
  → 选择副本
  → 副本准备界面：添加本地玩家
      → CreateLocalPlayer(1)
      → 玩家2选择存档
      → 加载玩家2的存档到PlayerState
  → Dungeon（分屏显示）
  → 完成副本
  → ReturnToMainMenu（移除玩家2，RemoveLocalPlayer）
```

### 模式3：在线合作

**特点：**
- 2-4个在线玩家
- 不能进入家园
- 不能切换角色
- 需要Lobby Level等待队友

**流程：**

```
Host流程：
MainMenu
  → 继续游戏（选择存档）
  → Hub（单人，准备角色装备）
  → 选择副本
  → 副本准备界面：在线匹配
      → 创建房间（选择副本+难度）
      → CreateSession
      → ServerTravel到Lobby Level
  → Lobby（等待其他玩家JoinSession）
      → 显示队友信息
      → 所有玩家"准备"
      → Host点击"开始"
  → Dungeon
  → 完成副本
  → ServerTravel回Lobby（Session保持）
      → 选择1：继续组队 → 选择下一个副本
      → 选择2：解散队伍 → DestroySession → ReturnToMainMenu

Client流程：
MainMenu
  → 继续游戏（选择存档）
  → Hub（单人，准备角色装备）
  → 选择副本（可选，也可直接查找房间）
  → 副本准备界面：在线匹配
      → 查找房间（FindSessions）
      → 选择房间
      → JoinSession
      → 跟随ServerTravel到Lobby Level
  → Lobby（等待Host开始）
      → 查看队友信息
      → 点击"准备"
      → 等待Host开始
  → Dungeon
  → 完成副本
  → ServerTravel回Lobby（Session保持）
      → 选择1：继续组队 → 等待Host选择下一个副本
      → 选择2：离开队伍 → DestroySession → ReturnToMainMenu
```

---

## 地图和GameMode设计

### MainMenu（主菜单）

**用途：** 游戏启动后的主界面

**GameMode：** AMainMenuGameMode

**功能：**
- 新游戏（创建存档）
- 继续游戏（存档列表）
- 存档选择和预览
- 副本直接选择（可跳过Hub）
- 设置
- 退出

**设计特点（参考《致命解药》）：**
- 3D场景展示角色模型
- 左右切换按钮预览男/女主角（仅视觉效果，不影响实际选择）
- 存档槽位显示截图和时间戳
- 两个入口："开始游戏"(进Hub) 和 "在线游戏"(进Lobby)

**技术特点：**
- 不需要Session
- 不创建PlayerState或ASC
- 使用SaveInfo/SaveGame分离模式快速加载
  - SaveInfo(USaveGameSlotInfo): 轻量级元数据，同步加载(<1KB/槽位)
  - SaveGame(UShootSaveGame): 完整数据，异步加载(选择后)
  - 详见 `Engineering/DevelopmentLogs/2025-11/2025-11-10_技术决策与实现方案总结.md`

**注意：** 角色定制功能已移至Hub，MainMenu不提供定制功能

### Hub（家园）

**用途：** 单人模式的核心社交空间和准备区域

**GameMode：** AHubGameMode

**功能：**
- 自由探索（游泳、瑜伽、摆pose）
- 与另一个主角互动（AI控制，存档中另一个性别角色）
- 切换角色（单人模式专属）
- 购买武器、升级技能
- 选择副本

**特点：**
- bAllowCharacterSwitch = true
- 不创建Session
- 只允许单人模式进入
- 类似《致命解药》的安全屋设计

**限制：**
- 本地分屏不能进入
- 在线模式不能进入

### Lobby（匹配大厅）

**用途：** 在线模式的等待和准备区域

**GameMode：** ALobbyGameMode

**功能：**
- 显示队友信息（2-4人）
- 玩家准备状态
- Host选择副本和难度
- Host点击"开始游戏"

**特点：**
- bAllowCharacterSwitch = false
- 需要Session（Host的CreateSession）
- 只在在线模式使用
- 一个小的3D场景（电梯、飞机内部、作战指挥室）

**实现：**
```cpp
class ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    // 玩家进入Lobby
    virtual void PostLogin(APlayerController* NewPlayer) override;

    // 玩家离开Lobby
    virtual void Logout(AController* Exiting) override;

    // Host选择开始游戏
    UFUNCTION(BlueprintCallable)
    void StartDungeon(FName DungeonID);

private:
    // 当前选择的副本
    FName CurrentDungeonID;

    // 玩家准备状态
    TMap<APlayerController*, bool> PlayerReadyStatus;
};
```

### Dungeon（副本）

**用途：** 实际游戏关卡

**GameMode：** AShootGameMode 或其子类

**功能：**
- 战斗、探索、任务目标
- 敌人生成（根据玩家数量调整难度）
- 完成副本后结算

**特点：**
- bAllowCharacterSwitch = false（战斗中不允许切换）
- 继承Host的Session（如果有）
- 支持1-4个玩家

---

## 副本准备界面设计

### 位置和触发

在Hub选择副本后，弹出副本准备界面Widget。

### 界面选项

```
┌─────────────────────────────────────┐
│     副本准备界面                      │
│                                     │
│  副本名称: [第一章 - 觉醒]           │
│  预计时长: 30-45分钟                 │
│  推荐等级: 5级                       │
│                                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐│
│  │单人开始  │  │添加本地  │  │在线匹配│ │
│  │         │  │玩家      │  │        │ │
│  └─────────┘  └─────────┘  └─────────┘│
│                                     │
│              [返回]                  │
└─────────────────────────────────────┘
```

### 选项行为

**单人开始：**
```cpp
void UDungeonPrepareWidget::OnSinglePlayerClicked()
{
    // 直接ServerTravel进副本
    // AI队友已经在Hub中生成
    UGameplayStatics::GetPlayerController(this, 0)->ServerTravel(DungeonMapName);
}
```

**添加本地玩家：**
```cpp
void UDungeonPrepareWidget::OnLocalCoopClicked()
{
    // 1. CreateLocalPlayer(1)
    UGameInstance* GI = GetGameInstance();
    FString OutError;
    GI->CreateLocalPlayer(1, OutError, true);

    // 2. 显示玩家2存档选择界面
    ShowPlayer2SaveGameSelection();

    // 3. 加载玩家2存档
    // ...

    // 4. 移除AI队友
    RemoveAITeammate();

    // 5. ServerTravel进副本
    UGameplayStatics::GetPlayerController(this, 0)->ServerTravel(DungeonMapName);
}
```

**在线匹配：**
```cpp
void UDungeonPrepareWidget::OnOnlineMatchClicked()
{
    // 显示"创建房间"或"加入房间"选择
    ShowOnlineOptions();
}
```

---

## Session管理

### CreateSession（Host创建房间）

**时机：** 玩家选择"在线匹配" → "创建房间"

**关键：Session清理守卫模式**

```cpp
void UMultiplayerSessionsSubsystem::CreateSession(FName SessionName, int32 NumPublicConnections, ...)
{
    IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();

    //  关键：检查并清理残留Session
    FNamedOnlineSession* ExistingSession = Session->GetNamedSession(SessionName);
    if (ExistingSession != nullptr)
    {
        // 缓存参数，先销毁残留Session
        LastNumPublicConnections = NumPublicConnections;
        DestroySession(SessionName);
        return;  // 等待OnDestroySessionComplete回调后重新创建
    }

    // 配置Session设置
    FOnlineSessionSettings SessionSettings;
    SessionSettings.NumPublicConnections = 4;  // 最多4人
    SessionSettings.bIsLANMatch = false;       // 使用Steam P2P
    SessionSettings.bUsesPresence = true;
    SessionSettings.bAllowJoinInProgress = false;  // 开始后不允许加入
    SessionSettings.bShouldAdvertise = true;

    // 设置自定义参数（副本ID、难度）
    SessionSettings.Set(
        FName("DungeonID"),
        DungeonID.ToString(),
        EOnlineDataAdvertisementType::ViaOnlineService
    );
    SessionSettings.Set(
        FName("Difficulty"),
        (int32)Difficulty,
        EOnlineDataAdvertisementType::ViaOnlineService
    );

    // 绑定回调
    Session->OnCreateSessionCompleteDelegates.AddUObject(
        this, &UMultiplayerSessionsSubsystem::OnCreateSessionComplete);

    // 创建Session
    Session->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bSuccess)
{
    if (bSuccess)
    {
        // ServerTravel到Lobby（使用Seamless Travel）
        GetWorld()->ServerTravel("/Game/Maps/Lobby?listen", true);
    }
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
    //  关键：如果有缓存参数，重新创建Session
    if (bSuccess && LastNumPublicConnections > 0)
    {
        CreateSession(SessionName, LastNumPublicConnections, ...);
        LastNumPublicConnections = 0;  // 清除缓存
    }
}
```

**为什么需要Session清理守卫？**
- 如果上次未正确DestroySession直接退出，会留下残留Session
- 残留Session会导致下次CreateSession失败或行为异常
- 自动检测并清理确保每次都能成功创建
- 详见 `Engineering/Session_Lifecycle_Technical_Constraints.md`

### FindSessions（Client查找房间）

**时机：** 玩家选择"在线匹配" → "加入房间"

**流程：**
```cpp
void UOnlineSubsystem::FindSessions()
{
    IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();

    // 创建搜索设置
    SearchSettings = MakeShareable(new FOnlineSessionSearch());
    SearchSettings->MaxSearchResults = 20;
    SearchSettings->bIsLanQuery = false;
    SearchSettings->QuerySettings.Set(
        SEARCH_PRESENCE,
        true,
        EOnlineComparisonOp::Equals
    );

    // 绑定回调
    Session->OnFindSessionsCompleteDelegates.AddUObject(
        this, &UYourClass::OnFindSessionsComplete);

    // 查找Sessions
    Session->FindSessions(0, SearchSettings.ToSharedRef());
}

void UYourClass::OnFindSessionsComplete(bool bSuccess)
{
    if (bSuccess && SearchSettings->SearchResults.Num() > 0)
    {
        // 显示房间列表
        for (const FOnlineSessionSearchResult& Result : SearchSettings->SearchResults)
        {
            FString DungeonID;
            Result.Session.SessionSettings.Get(FName("DungeonID"), DungeonID);

            int32 Difficulty;
            Result.Session.SessionSettings.Get(FName("Difficulty"), Difficulty);

            // 添加到UI列表
            AddRoomToList(Result, DungeonID, (EDungeonDifficulty)Difficulty);
        }
    }
}
```

### JoinSession（Client加入房间）

**时机：** 玩家从房间列表选择一个房间

**流程：**
```cpp
void UOnlineSubsystem::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
    IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();

    // 绑定回调
    Session->OnJoinSessionCompleteDelegates.AddUObject(
        this, &UYourClass::OnJoinSessionComplete);

    // 加入Session
    Session->JoinSession(0, NAME_GameSession, SearchResult);
}

void UYourClass::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        // 获取连接字符串
        FString ConnectString;
        IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();
        Session->GetResolvedConnectString(SessionName, ConnectString);

        // ClientTravel到Host的Lobby
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        PC->ClientTravel(ConnectString, TRAVEL_Absolute);
    }
}
```

### DestroySession（销毁Session）

**时机1：** 客户端选择"离开队伍"

```cpp
void ULobbyWidget::OnClientLeaveButtonClicked()
{
    IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();

    // 绑定回调
    Session->OnDestroySessionCompleteDelegates.AddUObject(
        this, &ULobbyWidget::OnDestroySessionComplete);

    // 销毁本地Session
    Session->DestroySession(NAME_GameSession);
}

void ULobbyWidget::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
    if (bSuccess)
    {
        // 返回主菜单
        UGameInstance* GI = GetGameInstance();
        GI->ReturnToMainMenu();
    }
}
```

**时机2：** Host选择"解散队伍"

```cpp
void ULobbyWidget::OnHostDisbandButtonClicked()
{
    if (GetWorld()->GetNetMode() == NM_ListenServer)
    {
        // 通知所有客户端即将断开
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && !PC->IsLocalController())
            {
                PC->ClientReturnToMainMenuWithTextReason(
                    FText::FromString("Host disbanded the party"));
            }
        }

        // Host销毁Session
        IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface();
        Session->OnDestroySessionCompleteDelegates.AddUObject(
            this, &ULobbyWidget::OnHostDestroySessionComplete);
        Session->DestroySession(NAME_GameSession);
    }
}

void ULobbyWidget::OnHostDestroySessionComplete(FName SessionName, bool bSuccess)
{
    // Host返回主菜单
    UGameInstance* GI = GetGameInstance();
    GI->ReturnToMainMenu();
}
```

---

## 副本解锁和进度管理

### 方案选择：个人解锁 + 允许跳关

**核心逻辑：**
- 每个玩家有独立的副本解锁进度
- Host可以选择自己已解锁的任何副本
- Client如果副本未解锁，可以选择"跳关"参与
- 跳关完成标记为"辅助完成"，不计入"首次完成"

### 存档结构

```cpp
// ShootSaveGame.h
USTRUCT(BlueprintType)
struct FDungeonProgress
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName DungeonID;

    UPROPERTY(SaveGame)
    bool bUnlocked = false;  // 是否解锁

    UPROPERTY(SaveGame)
    bool bFirstCleared = false;  // 是否首次完成

    UPROPERTY(SaveGame)
    bool bAssistedCleared = false;  // 是否辅助完成（跳关）

    UPROPERTY(SaveGame)
    float BestClearTime = 0.0f;  // 最快通关时间

    UPROPERTY(SaveGame)
    int32 ClearCount = 0;  // 完成次数

    UPROPERTY(SaveGame)
    EDungeonDifficulty HighestDifficulty = EDungeonDifficulty::Normal;
};

UCLASS()
class UShootSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    TArray<FDungeonProgress> DungeonProgressList;

    FDungeonProgress* FindDungeonProgress(FName DungeonID);
    void UnlockDungeon(FName DungeonID);
    void MarkDungeonCleared(FName DungeonID, bool bAsHost, float ClearTime, EDungeonDifficulty Difficulty);
};
```

### Hub中的副本列表

```cpp
void UHubWidget::RefreshDungeonList()
{
    UShootSaveGame* SaveGame = LoadCurrentSaveGame();
    UDataTable* DungeonTable = LoadDungeonDataTable();

    TArray<FDungeonData*> AllDungeons;
    DungeonTable->GetAllRows<FDungeonData>("", AllDungeons);

    for (FDungeonData* Dungeon : AllDungeons)
    {
        UDungeonListItemWidget* Item = CreateWidget<UDungeonListItemWidget>(...);

        // 检查是否解锁
        FDungeonProgress* Progress = SaveGame->FindDungeonProgress(Dungeon->DungeonID);
        bool bUnlocked = Progress && Progress->bUnlocked;

        Item->SetDungeonData(Dungeon);

        if (bUnlocked)
        {
            Item->SetUnlocked(true);
            Item->SetProgress(Progress);  // 显示完成次数、最快时间
        }
        else
        {
            Item->SetLocked(true);  // 显示锁定图标和解锁条件
        }

        DungeonListPanel->AddChild(Item);
    }
}
```

### Lobby中的跳关警告

```cpp
// Host选择副本后，通知所有客户端检查解锁状态
void ALobbyGameMode::OnHostSelectDungeon(FName DungeonID)
{
    CurrentDungeonID = DungeonID;

    // 通知所有客户端
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && !PC->IsLocalController())
        {
            Cast<AShootPlayerController>(PC)->ClientCheckDungeonUnlock(DungeonID);
        }
    }
}

// 客户端检查副本解锁状态
void AShootPlayerController::ClientCheckDungeonUnlock_Implementation(FName DungeonID)
{
    UShootSaveGame* SaveGame = LoadMySaveGame();
    FDungeonProgress* Progress = SaveGame->FindDungeonProgress(DungeonID);

    bool bUnlocked = Progress && Progress->bUnlocked;

    if (!bUnlocked)
    {
        // 弹出警告对话框
        ShowSkipDungeonWarning(DungeonID);
    }

    // 报告给服务端（可选，用于显示队友进度）
    ServerReportDungeonUnlock(DungeonID, bUnlocked);
}
```

### 副本完成后更新进度

```cpp
void AShootGameMode::OnDungeonComplete(float ClearTime)
{
    // 遍历所有玩家，更新存档
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AShootPlayerController* PC = Cast<AShootPlayerController>(It->Get());
        if (PC)
        {
            bool bIsHost = PC->IsLocalController();
            PC->ClientUpdateDungeonProgress(CurrentDungeonID, bIsHost, ClearTime, CurrentDifficulty);
        }
    }

    // ServerTravel回Lobby
    GetWorld()->ServerTravel("/Game/Maps/Lobby?listen", true);
}

void AShootPlayerController::ClientUpdateDungeonProgress_Implementation(
    FName DungeonID, bool bAsHost, float ClearTime, EDungeonDifficulty Difficulty)
{
    UShootSaveGame* SaveGame = LoadMySaveGame();

    // 更新副本进度
    SaveGame->MarkDungeonCleared(DungeonID, bAsHost, ClearTime, Difficulty);

    // 解锁下一个副本
    TArray<FName> UnlockedDungeons = GetNextUnlockedDungeons(DungeonID);
    for (FName NextDungeonID : UnlockedDungeons)
    {
        SaveGame->UnlockDungeon(NextDungeonID);
    }

    // 保存到磁盘
    UGameplayStatics::SaveGameToSlot(SaveGame, CurrentSaveSlotName, 0);
}
```

---

## 存档同步机制

### PlayerState的可复制属性

**原则：** 只复制"其他玩家需要看到"的属性，不维护SaveGame指针。

```cpp
// AShootPlayerState.h
UCLASS()
class AShootPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // ============ 可复制的存档属性 ============

    UPROPERTY(Replicated, ReplicatedUsing=OnRep_CharacterLevel)
    int32 CharacterLevel;

    UPROPERTY(Replicated, ReplicatedUsing=OnRep_CharacterXP)
    int32 CharacterXP;

    UPROPERTY(Replicated, ReplicatedUsing=OnRep_CharacterGender)
    ECharacterGender CharacterGender;

    UPROPERTY(Replicated)
    FString CharacterName;

    // ============ 不可复制的本地属性 ============
    // （技能树、剧情进度等只影响本地的数据不需要复制）

    void LoadFromSaveGame(UShootSaveGame* SaveGame);
    void SaveToSaveGame();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_CharacterLevel();

    UFUNCTION()
    void OnRep_CharacterXP();

    UFUNCTION()
    void OnRep_CharacterGender();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    FString CurrentSaveSlotName;
};
```

### 加载和保存逻辑

```cpp
// AShootPlayerState.cpp
void AShootPlayerState::BeginPlay()
{
    Super::BeginPlay();

    // 仅在本地玩家的PlayerState中加载存档
    if (IsLocalPlayerController())
    {
        UShootSaveGame* SaveGame = Cast<UShootSaveGame>(
            UGameplayStatics::LoadGameFromSlot(CurrentSaveSlotName, 0));

        if (SaveGame)
        {
            LoadFromSaveGame(SaveGame);
        }
    }
}

void AShootPlayerState::LoadFromSaveGame(UShootSaveGame* SaveGame)
{
    // 从SaveGame加载到可复制属性（会自动同步到服务端和其他客户端）
    CharacterLevel = SaveGame->CharacterLevel;
    CharacterXP = SaveGame->CharacterXP;
    CharacterGender = SaveGame->CharacterGender;
    CharacterName = SaveGame->CharacterName;
}

void AShootPlayerState::SaveToSaveGame()
{
    if (!IsLocalPlayerController())
    {
        return;  // 只有本地玩家才能保存
    }

    UShootSaveGame* SaveGame = Cast<UShootSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSaveSlotName, 0));

    if (!SaveGame)
    {
        SaveGame = Cast<UShootSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UShootSaveGame::StaticClass()));
    }

    // 将PlayerState的数据保存到SaveGame
    SaveGame->CharacterLevel = CharacterLevel;
    SaveGame->CharacterXP = CharacterXP;
    SaveGame->CharacterGender = CharacterGender;
    SaveGame->CharacterName = CharacterName;

    // 保存到磁盘
    UGameplayStatics::SaveGameToSlot(SaveGame, CurrentSaveSlotName, 0);
}

void AShootPlayerState::OnRep_CharacterLevel()
{
    // 本地玩家：保存到存档
    if (IsLocalPlayerController())
    {
        SaveToSaveGame();
    }

    // 所有客户端：更新UI
    OnCharacterLevelChanged.Broadcast(CharacterLevel);
}
```

---

## 难度调整机制

### GameMode中的难度缩放

```cpp
// AShootGameMode.h
class AShootGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category="Difficulty")
    float DifficultyMultiplierPerPlayer = 0.3f;  // 每增加1人，难度+30%

    UFUNCTION(BlueprintCallable)
    float GetDifficultyMultiplier() const;

protected:
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

private:
    void RecalculateDifficulty();
};

// AShootGameMode.cpp
void AShootGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    RecalculateDifficulty();
}

void AShootGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    RecalculateDifficulty();
}

void AShootGameMode::RecalculateDifficulty()
{
    int32 PlayerCount = GetNumPlayers();
    float Multiplier = 1.0f + (PlayerCount - 1) * DifficultyMultiplierPerPlayer;

    UE_LOG(LogTemp, Log, TEXT("Difficulty multiplier: %.2f (Players: %d)"), Multiplier, PlayerCount);

    // 通知所有子系统难度变化
    // 例如：AI生成器、敌人血量、奖励掉落率等
    OnDifficultyChanged.Broadcast(Multiplier);
}

float AShootGameMode::GetDifficultyMultiplier() const
{
    int32 PlayerCount = GetNumPlayers();
    return 1.0f + (PlayerCount - 1) * DifficultyMultiplierPerPlayer;
}
```

### 应用难度缩放

**敌人生成器：**
```cpp
void AEnemySpawner::SpawnEnemies()
{
    AShootGameMode* GM = Cast<AShootGameMode>(GetWorld()->GetAuthGameMode());
    float Multiplier = GM->GetDifficultyMultiplier();

    int32 BaseEnemyCount = 10;
    int32 ActualEnemyCount = FMath::RoundToInt(BaseEnemyCount * Multiplier);

    for (int32 i = 0; i < ActualEnemyCount; ++i)
    {
        SpawnEnemy();
    }
}
```

**敌人属性：**
```cpp
void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    AShootGameMode* GM = Cast<AShootGameMode>(GetWorld()->GetAuthGameMode());
    float Multiplier = GM->GetDifficultyMultiplier();

    // 调整敌人血量
    float BaseHealth = 100.0f;
    float ActualHealth = BaseHealth * Multiplier;
    AttributeSet->SetHealth(ActualHealth);
}
```

---

## 技术配置要求

### Seamless Travel配置

**为什么需要Seamless Travel？**
- Hub ↔ Dungeon 和 Lobby ↔ Dungeon 切换时保持PlayerState和ASC
- 避免玩家数据丢失，保持技能和属性

**配置步骤：**

1. **在GameMode中启用**
```cpp
// HubGameMode, LobbyGameMode, DungeonGameMode
AMyGameMode::AMyGameMode()
{
    bUseSeamlessTravel = true;
}
```

2. **项目设置中配置TransitionMap**
```
Edit > Project Settings > Maps & Modes
    → Default Maps
        → Transition Map: /Game/Maps/TransitionMap
```

3. **创建TransitionMap**
- 创建空白小地图用于过渡加载
- 地图内容可以为空或显示Loading界面

4. **使用ServerTravel**
```cpp
// Hub → Dungeon
GetWorld()->ServerTravel("/Game/Maps/Dungeons/Chapter1?listen", true);  // true = Seamless

// Dungeon → Hub
GetWorld()->ServerTravel("/Game/Maps/Hub?listen", true);

// Dungeon → Lobby (在线模式)
GetWorld()->ServerTravel("/Game/Maps/Lobby?listen", true);
```

**什么会保留？什么会重建？**

| 对象 | Seamless Travel | Hard Travel |
|------|----------------|-------------|
| PlayerController | 保留 | 重建 |
| PlayerState | 保留(CopyProperties) | 重建 |
| ASC (在PlayerState上) | 保留 | 重建 |
| Character/Pawn | 重建 | 重建 |
| GameMode | 重建 | 重建 |

**PlayerState数据迁移：**
```cpp
// 重写此方法以迁移自定义数据
void AShootPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    AShootPlayerState* OldPS = Cast<AShootPlayerState>(PlayerState);
    if (OldPS)
    {
        // 复制自定义属性
        this->CurrentGender = OldPS->CurrentGender;
        this->SaveSlotIndex = OldPS->SaveSlotIndex;
    }
}
```

详细技术说明见：`Engineering/DevelopmentLogs/2025-11/2025-11-10_技术决策与实现方案总结.md`

---

## 总结

### 关键设计决策

1. **统一架构**：所有模式使用Listen Server，代码高度复用
2. **动态模式选择**：MainMenu不分三种入口，在副本准备界面选择
3. **家园单机专属**：避免在线/分屏的逻辑混乱
4. **Lobby必须是Level**：支持副本完成后保持Session继续组队
5. **个人解锁+允许跳关**：平衡进度管理和组队灵活性
6. **可复制属性同步**：不维护SaveGame指针，减少网络开销
7. **Seamless Travel**：保持PlayerState/ASC，使用CopyProperties迁移数据
8. **Session清理守卫**：自动检测并清理残留Session，确保可靠性

### 技术限制

- 离开Lobby必须回到MainMenu（UE引擎限制）
- ReturnToMainMenu触发Hard Travel，丢失所有运行时状态
- 副本完成后可以ServerTravel回Lobby保持Session
- 玩家选择离开时必须DestroySession才能再次FindSessions
- Seamless Travel需要配置TransitionMap
- PlayerState自定义数据需要实现CopyProperties

---

参考文档：
- `CharacterSwitching/DesignGuide_设计指南.md` - 角色切换设计
- `Engineering/Session_Lifecycle_Technical_Constraints.md` - Session生命周期技术限制
- `Engineering/DevelopmentLogs/2025-11/2025-11-10_技术决策与实现方案总结.md` - 完整技术决策记录

最后更新: 2025-11-10
维护者: 项目团队
状态: 设计阶段
