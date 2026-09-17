# Lyra 武器系统与库存系统集成架构分析

> **关键发现**：Lyra 的武器实例是 **UObject**，不是 Actor！这是与我们当前系统的最大区别。

## 目录
- [为什么武器实例是 UObject](#为什么武器实例是-uobject)
- [核心架构](#核心架构)
- [三层系统关系](#三层系统关系)
- [弹药管理机制](#弹药管理机制)
- [网络复制与预测](#网络复制与预测)
- [与我们系统的对比](#与我们系统的对比)
- [改造方案](#改造方案)

---

## 为什么武器实例是 UObject

### 设计理念：职责分离

Lyra 刻意将"武器逻辑"和"武器表现"拆开：

```
UWeaponInstance (UObject) = "武器的脑子"（逻辑）
AWeaponActor (AActor) = "武器的身体"（表现）
```

- **UObject 负责**：弹道、散布、距离衰减、冷却、动画层选择、状态
- **AActor 负责**：Mesh、特效、音效、摄像机抖动、弹壳

### 五大优势

#### 1. 职责拆分清晰

如果武器实例是 Actor，容易把所有东西都塞进去：状态、算法、特效、网格、碰撞、Tick...
结果：超级胖类，复用困难，测试麻烦。

Lyra 方式：
- 逻辑层不绑场景
- 表现层不背业务
- 经典分层架构

#### 2. 网络和生命周期管理简单

- UObject 可作为 FastArray 元素自然管理
- Owned subobject，生命周期挂在 Pawn/Controller
- 不占用 World Actor 列表
- 装备/卸载只需：创建/销毁 UObject + AActor，无需考虑 BeginPlay/EndPlay 细节

#### 3. 多系统共享引用

一个武器实例可在多个系统共享：
- Ability System
- HUD
- 动画图层
- Inventory / QuickBar

不需要每个系统都知道"场景里那个 AActor 是谁"。

#### 4. 支持多视角/多持有者

极端情况：
- 观战/回放模式
- 同一把武器逻辑在不同 Pawn 或 UI 上展示

逻辑 UObject 只有一份，表现 Actor 可以有多个，不被"武器 == 场景 Actor"强耦合。

#### 5. GC & 性能 & 编辑器干净

- 不进 World Outliner
- 不被场景扫描逻辑遍历
- 不需要 Tick（避免满地 Tick Actor）
- 纯数据/逻辑可在无 World 环境下单元测试

---

## 核心架构

### 1. 库存系统（Inventory System）

**核心类**：
- `ULyraInventoryItemDefinition`：物品定义（DataAsset）
- `ULyraInventoryItemFragment`：功能片段基类
- `ULyraInventoryItemInstance`：物品实例（UObject）
- `ULyraInventoryManagerComponent`：库存管理组件

**关键 Fragment**：
```cpp
// 装备桥接 Fragment
UCLASS()
class UInventoryFragment_EquippableItem : public ULyraInventoryItemFragment
{
    GENERATED_BODY()
public:
    // 关联装备定义
    UPROPERTY(EditAnywhere, Category=Lyra)
    TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition;
};

// 初始属性 Fragment
UCLASS()
class UInventoryFragment_SetStats : public ULyraInventoryItemFragment
{
    GENERATED_BODY()
protected:
    // 初始统计数据（如弹药）
    UPROPERTY(EditDefaultsOnly, Category=Equipment)
    TMap<FGameplayTag, int32> InitialItemStats;

public:
    // 在实例创建时调用
    virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const override
    {
        for (const auto& KVP : InitialItemStats)
        {
            Instance->AddStatTagStack(KVP.Key, KVP.Value);
        }
    }
};
```

### 2. 装备系统（Equipment System）

**核心类**：
- `ULyraEquipmentDefinition`：装备定义（定义要生成的 Instance 和 AbilitySet）
- `ULyraEquipmentInstance`：装备实例基类（UObject）
- `ULyraEquipmentManagerComponent`：装备管理组件

```cpp
// 装备定义
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class ULyraEquipmentDefinition : public UObject
{
    GENERATED_BODY()
public:
    // 要生成的装备实例类型
    UPROPERTY(EditDefaultsOnly, Category=Equipment)
    TSubclassOf<ULyraEquipmentInstance> InstanceType;

    // 装备时授予的 Ability
    UPROPERTY(EditDefaultsOnly, Category=Equipment)
    TArray<TObjectPtr<const ULyraAbilitySet>> AbilitySetsToGrant;

    // 要生成的 Actor（如武器 Mesh）
    UPROPERTY(EditDefaultsOnly, Category=Equipment)
    TArray<FLyraEquipmentActorToSpawn> ActorsToSpawn;
};

// 装备实例基类
UCLASS(BlueprintType, Blueprintable)
class ULyraEquipmentInstance : public UObject
{
    GENERATED_BODY()
public:
    // 支持网络复制
    virtual bool IsSupportedForNetworking() const override { return true; }

    // Instigator（拥有者）
    UPROPERTY(ReplicatedUsing=OnRep_Instigator)
    TObjectPtr<UObject> Instigator;

    // 生成的 Actor（如武器 Mesh）
    UPROPERTY(Replicated)
    TArray<TObjectPtr<AActor>> SpawnedActors;

    virtual void OnEquipped();
    virtual void OnUnequipped();
};
```

### 3. 武器系统（Weapon System）

**核心类**：
- `ULyraWeaponInstance`：武器实例基类
- `ULyraRangedWeaponInstance`：远程武器实例

```cpp
// 武器实例基类
UCLASS(MinimalAPI)
class ULyraWeaponInstance : public ULyraEquipmentInstance
{
    GENERATED_BODY()
public:
    // 装备/卸载时的动画集
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
    FLyraAnimLayerSelectionSet EquippedAnimSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
    FLyraAnimLayerSelectionSet UneuippedAnimSet;

    // 跟踪时间
    double TimeLastEquipped = 0.0;
    double TimeLastFired = 0.0;
};

// 远程武器实例
UCLASS()
class ULyraRangedWeaponInstance : public ULyraWeaponInstance
{
    GENERATED_BODY()
public:
    // 射击参数
    UPROPERTY(EditAnywhere, Category="Weapon Config")
    int32 BulletsPerCartridge = 1;

    UPROPERTY(EditAnywhere, Category="Weapon Config")
    float MaxDamageRange = 25000.0f;

    // 扩散系统
    UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
    FRuntimeFloatCurve HeatToSpreadCurve;

    // 当前状态
    float CurrentHeat = 0.0f;
    float CurrentSpreadAngle = 0.0f;

    // 每帧更新
    void Tick(float DeltaSeconds);
};
```

---

## 三层系统关系

```
库存系统                    装备系统                    武器系统
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ ItemDefinition  │       │ EquipmentDef    │       │ WeaponInstance  │
│ (DataAsset)     │       │ (DataAsset)     │       │ (UObject)       │
├─────────────────┤       ├─────────────────┤       ├─────────────────┤
│ DisplayName     │       │ InstanceType◄───┼──────►│ EquippedAnimSet │
│ Fragments[]     │       │ AbilitySets[]   │       │ SpreadParams    │
│  ├─Equippable◄──┼──────►│ ActorsToSpawn[] │       │ DamageParams    │
│  ├─SetStats     │       └─────────────────┘       │ CurrentHeat     │
│  └─QuickBarIcon │                                 └─────────────────┘
└─────────────────┘                                         │
        │                                                   │
        ▼                                                   ▼
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ ItemInstance    │◄──────│EquipmentInstance│       │RangedWeaponInst │
│ (UObject)       │  Instigator (UObject)   │       │ (UObject)       │
├─────────────────┤       ├─────────────────┤       ├─────────────────┤
│ ItemDef         │       │ Instigator*     │       │ Tick()          │
│ StatTags        │       │ SpawnedActors[] │       │ AddSpread()     │
│  ├─Ammo.Rifle   │       │ OnEquipped()    │       │ GetSpreadAngle()│
│  └─Ammo.Reserve │       │ OnUnequipped()  │       └─────────────────┘
└─────────────────┘       └─────────────────┘
  (持久，始终存在)           (临时，装备时创建)
        │                         │
        └─────────┬───────────────┘
                  ▼
          ┌──────────────┐
          │ QuickBar     │
          │ Component    │
          ├──────────────┤
          │ Slots[]      │
          │ ActiveIndex  │
          └──────────────┘
```

### 重要：Instigator 连接机制

**EquipmentInstance.Instigator** 是连接装备和库存的关键：

```cpp
// QuickBar 设置连接（切枪时）
EquipmentInstance->SetInstigator(ItemInstance);

// GA 通过连接获取数据
UInventoryItemInstance* ItemInstance = EquipmentInstance->GetInstigator();
int32 Ammo = ItemInstance->GetStatTagStackCount(AmmoTag);
```

为什么重要：
1. 装备实例需要访问持久状态（弹药、品质等）
2. GA 需要同时访问装备参数和物品状态
3. 解耦装备逻辑和库存数据

### "取出/放回库存"的正确理解

关键认知：
```
库存数据从未离开 InventoryComponent
"取出" = 创建 EquipmentInstance + Spawn Actor
"放回" = 销毁 EquipmentInstance + Destroy Actor
```

数据流：
```
InventoryItemInstance (持久数据，始终在库存)
    ↓ EquipFromSource
EquipmentInstance (临时逻辑对象)
    ↓ SpawnActors
WeaponActor (临时表现对象)
```

切枪流程：
```cpp
// 1. 卸下当前武器（"放回库存"）
if (OldItem) {
    EquipmentComponent->UnequipItemFromSource(OldItem);
    // → EquipInstance->OnUnequipped()
    // → EquipInstance->DestroySpawnedActors()
    // → RemoveEntry (销毁 UObject)
    // ⚠️ ItemInstance 不动！始终在 InventoryComponent
}

// 2. 装备新武器（"从库存取出"）
if (NewItem) {
    EquipmentComponent->EquipItemFromSource(NewItem);
    // → NewObject<EquipmentInstance>
    // → SetInstigator(ItemInstance)  // 关键连接
    // → SpawnActors()
    // → GiveAbilities()
    // → OnEquipped()
}
```

状态持久化策略：
- 临时状态（散布热度、开火时间）→ EquipmentInstance（卸下时丢弃）
- 持久状态（弹药、解锁、品质）→ ItemInstance（OnUnequipped 时回写）

### 工作流程：

1. **创建库存物品**：
   ```cpp
   // 添加物品到库存
   ULyraInventoryItemInstance* Instance = InventoryManager->AddItemDefinition(ItemDefClass, 1);

   // SetStats Fragment 自动初始化 StatTags
   Instance->StatTags.GetStackCount(Tag_Ammo_Rifle); // 例如：120
   ```

2. **添加到快捷栏**：
   ```cpp
   // 将物品添加到快捷栏槽位
   QuickBarComponent->AddItemToSlot(0, Instance);
   ```

3. **装备物品**：
   ```cpp
   // 从 Instance 获取 EquipmentDefinition
   const UInventoryFragment_EquippableItem* EquipFragment =
       Instance->FindFragmentByClass<UInventoryFragment_EquippableItem>();

   // 装备
   ULyraEquipmentInstance* EquipmentInstance =
       EquipmentManager->EquipItem(EquipFragment->EquipmentDefinition);

   // EquipmentInstance 实际上是 ULyraWeaponInstance
   ULyraWeaponInstance* WeaponInstance = Cast<ULyraWeaponInstance>(EquipmentInstance);
   ```

4. **武器使用**：
   ```cpp
   // 武器实例每帧 Tick
   WeaponInstance->Tick(DeltaTime);

   // 开火时更新扩散
   WeaponInstance->AddSpread();

   // 获取当前扩散角度
   float Spread = WeaponInstance->GetCalculatedSpreadAngle();
   ```

---

## 弹药管理机制

### 弹药存储位置

Lyra 使用 **StatTags** 存储弹药，而不是独立的 InventoryItem：

```cpp
// 在 ItemDefinition 的 SetStats Fragment 中定义初始弹药
InitialItemStats:
  - Lyra.ShooterGame.Weapon.Rifle.Ammo.Magazine: 30
  - Lyra.ShooterGame.Weapon.Rifle.Ammo.Reserve: 120

// 在 ItemInstance 的 StatTags 中存储
Instance->AddStatTagStack(Tag_Ammo_Magazine, 30);
Instance->AddStatTagStack(Tag_Ammo_Reserve, 120);
```

### 弹药操作

```cpp
// 消耗弹药
Instance->RemoveStatTagStack(Tag_Ammo_Magazine, 1);

// 装填弹药
int32 Reserve = Instance->GetStatTagStackCount(Tag_Ammo_Reserve);
int32 ToReload = FMath::Min(Reserve, 30);
Instance->RemoveStatTagStack(Tag_Ammo_Reserve, ToReload);
Instance->AddStatTagStack(Tag_Ammo_Magazine, ToReload);

// 查询弹药
int32 CurrentAmmo = Instance->GetStatTagStackCount(Tag_Ammo_Magazine);
```

### 关键优势：

1. **统一管理**：弹药作为物品的属性，统一通过 StatTags 管理
2. **网络复制**：StatTags 自动复制，无需额外代码
3. **UI 集成**：UI 可以监听 Inventory.Message.StackChanged 更新弹药显示

---

## 网络复制与预测

### 1. 弹药预测机制

**问题**：高射速武器在网络延迟下会出现弹药回滚

**Lyra 解决方案**（来自 GAS Documentation）：

```cpp
// 在 PreReplication 中禁用弹药复制
void ULyraRangedWeaponInstance::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
    Super::PreReplication(ChangedPropertyTracker);

    // 如果玩家正在开火，禁用弹药的网络复制
    ULyraInventoryItemInstance* ItemInstance = GetAssociatedItemInstance();
    if (ItemInstance && ItemInstance->GetOwner()->HasMatchingGameplayTag(Tag_IsFiring))
    {
        // 禁用 StatTags 复制
        DOREPLIFETIME_ACTIVE_OVERRIDE(ULyraInventoryItemInstance, StatTags, false);
    }
}
```

**工作原理**：
1. 客户端预测：开火时立即扣减本地弹药
2. 服务器权威：服务器计算正确的弹药数量
3. 复制控制：开火期间禁用复制，避免服务器覆盖客户端预测值
4. 同步恢复：停火后恢复复制，服务器值覆盖客户端（此时应该已同步）

### 2. StatTags 复制

```cpp
// FGameplayTagStackContainer 使用 FastArray 增量复制
UPROPERTY(Replicated)
FGameplayTagStackContainer StatTags;

// 仅复制变化的堆栈
void FGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
    // ...
    MarkItemDirty(Stack);  // 仅标记此项为脏
}
```

### 3. EquipmentInstance 复制

```cpp
// EquipmentInstance 作为 SubObject 复制
void ULyraEquipmentManagerComponent::EquipItem(TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition)
{
    // 创建 Instance（Outer 必须是 Actor）
    ULyraEquipmentInstance* Instance = NewObject<ULyraEquipmentInstance>(GetOwner());

    // 注册 SubObject
    if (IsUsingRegisteredSubObjectList())
    {
        AddReplicatedSubObject(Instance);
    }
}
```

---

## 与我们系统的对比

| 方面 | Lyra | 我们当前 | 差异分析 |
|------|------|----------|----------|
| **武器实例类型** | UObject (ULyraWeaponInstance) | Actor (ARangedWeaponInstance) | ❌ 需要改造 |
| **弹药存储** | StatTags (GameplayTagStack) | Magazine/Reserve 成员变量 | ❌ 需要改造 |
| **装备系统** | Equipment + Inventory + QuickBar | CombatComponent + Quickbar | ⚠️ 需要集成 |
| **网络复制** | SubObject + FastArray | Actor 复制 | ❌ 需要改造 |
| **弹药预测** | PreReplication 控制 | 无预测 | ❌ 需要实现 |
| **Ability 集成** | EquipmentDefinition.AbilitySetsToGrant | 手动授予 | ⚠️ 可优化 |
| **UI 集成** | GameplayMessage 解耦 | 直接引用 | ✅ 已实现 |

### 关键问题：

1. **武器实例是 Actor**：
   - 当前：`ARangedWeaponInstance` 继承自 `AActor`
   - Lyra：`ULyraWeaponInstance` 继承自 `UObject`
   - **影响**：无法直接集成到库存系统的 SubObject 复制

2. **弹药存储分散**：
   - 当前：`Magazine`、`Reserve` 是 Actor 的成员变量
   - Lyra：统一存储在 `ItemInstance->StatTags`
   - **影响**：无法利用 GameplayTagStack 的网络复制和预测机制

3. **装备流程不统一**：
   - 当前：`CombatComponent` 直接管理武器 Actor
   - Lyra：库存 → 快捷栏 → 装备管理器 → 武器实例
   - **影响**：无法实现材料、徽章、设计图等库存物品的统一管理

---

## 改造方案

### Phase 0：架构调研与设计（当前）

- [x] 深入研究 Lyra 武器系统架构
- [ ] 记录学习笔记
- [ ] 分析当前系统与 Lyra 的差异
- [ ] 设计改造方案

### Phase 1：装备系统基础设施

**目标**：创建装备系统，但暂时兼容现有武器 Actor

1. **创建装备系统核心类**：
   ```cpp
   // ShootEquipmentDefinition.h
   UCLASS(Blueprintable, Const, Abstract)
   class UShootEquipmentDefinition : public UObject
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditDefaultsOnly)
       TSubclassOf<UShootEquipmentInstance> InstanceType;

       UPROPERTY(EditDefaultsOnly)
       TArray<TObjectPtr<const UShootAbilitySet>> AbilitySetsToGrant;

       // 暂时保留：生成 Actor 的武器
       UPROPERTY(EditDefaultsOnly)
       TArray<FShootEquipmentActorToSpawn> ActorsToSpawn;
   };

   // ShootEquipmentInstance.h
   UCLASS(BlueprintType, Blueprintable)
   class UShootEquipmentInstance : public UObject
   {
       GENERATED_BODY()
   public:
       virtual bool IsSupportedForNetworking() const override { return true; }

       UPROPERTY(Replicated)
       TObjectPtr<UObject> Instigator;

       UPROPERTY(Replicated)
       TArray<TObjectPtr<AActor>> SpawnedActors;  // 保留现有 Actor 武器

       virtual void OnEquipped();
       virtual void OnUnequipped();
   };

   // ShootEquipmentManagerComponent.h
   UCLASS()
   class UShootEquipmentManagerComponent : public UPawnComponent
   {
       GENERATED_BODY()
   public:
       UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
       UShootEquipmentInstance* EquipItem(TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition);

       UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
       void UnequipItem(UShootEquipmentInstance* ItemInstance);

   private:
       UPROPERTY(Replicated)
       FShootEquipmentList EquipmentList;
   };
   ```

2. **集成到 ShootCharacter**：
   ```cpp
   UPROPERTY(Category=Equipment, VisibleAnywhere, BlueprintReadOnly)
   TObjectPtr<UShootEquipmentManagerComponent> EquipmentManagerComponent;
   ```

3. **创建库存-装备桥接 Fragment**：
   ```cpp
   UCLASS()
   class UShootInventoryFragment_EquippableItem : public UShootInventoryItemFragment
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere, Category=Equipment)
       TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition;
   };
   ```

### Phase 2：武器实例改造（核心）

**目标**：将武器实例从 Actor 改为 UObject

1. **创建新的武器实例基类**：
   ```cpp
   // ShootWeaponInstance.h
   UCLASS(MinimalAPI)
   class UShootWeaponInstance : public UShootEquipmentInstance
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
       FShootAnimLayerSelectionSet EquippedAnimSet;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
       FShootAnimLayerSelectionSet UnequippedAnimSet;

       double TimeLastEquipped = 0.0;
       double TimeLastFired = 0.0;

       virtual void OnEquipped() override;
       virtual void OnUnequipped() override;
   };

   // ShootRangedWeaponInstance.h
   UCLASS()
   class UShootRangedWeaponInstance : public UShootWeaponInstance
   {
       GENERATED_BODY()
   public:
       // 从 ARangedWeaponInstance 迁移射击参数
       UPROPERTY(EditAnywhere, Category="Weapon Config")
       int32 BulletsPerCartridge = 1;

       UPROPERTY(EditAnywhere, Category="Weapon Config")
       float MaxDamageRange = 25000.0f;

       // 扩散系统
       float CurrentHeat = 0.0f;
       float CurrentSpreadAngle = 0.0f;

       void Tick(float DeltaSeconds);
       void AddSpread();
   };
   ```

2. **迁移现有功能**：
   - 将 `ARangedWeaponInstance` 的所有逻辑迁移到 `UShootRangedWeaponInstance`
   - 保留 Mesh Actor 作为 SpawnedActors
   - 更新所有引用

### Phase 3：弹药系统改造

**目标**：使用 StatTags 存储弹药，实现预测机制

1. **定义弹药 GameplayTags**：
   ```cpp
   // ShootGameplayTags.h
   FGameplayTag Weapon_Ammo_Magazine;     // 弹夹弹药
   FGameplayTag Weapon_Ammo_Reserve;      // 后备弹药
   ```

2. **创建弹药 SetStats Fragment**：
   ```cpp
   // 在 ItemDefinition 中配置
   UShootInventoryFragment_SetStats:
       InitialItemStats:
           Weapon.Ammo.Magazine: 30
           Weapon.Ammo.Reserve: 120
   ```

3. **修改开火 Ability**：
   ```cpp
   // ShootGameplayAbility_Weapon_Fire.cpp
   void UShootGameplayAbility_Weapon_Fire::ActivateAbility(...)
   {
       // 从 ItemInstance 获取弹药
       UShootInventoryItemInstance* ItemInstance = GetAssociatedItemInstance();
       int32 CurrentAmmo = ItemInstance->GetStatTagStackCount(Tag_Weapon_Ammo_Magazine);

       if (CurrentAmmo <= 0)
       {
           CancelAbility(...);
           return;
       }

       // 消耗弹药
       ItemInstance->RemoveStatTagStack(Tag_Weapon_Ammo_Magazine, 1);

       // 执行射击逻辑
       PerformLocalTargeting(...);
   }
   ```

4. **实现弹药预测**：
   ```cpp
   // ShootInventoryItemInstance.cpp
   void UShootInventoryItemInstance::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
   {
       Super::PreReplication(ChangedPropertyTracker);

       // 如果拥有者正在开火，禁用 StatTags 复制
       APawn* OwnerPawn = Cast<APawn>(GetOuter());
       if (OwnerPawn && OwnerPawn->GetAbilitySystemComponent())
       {
           if (OwnerPawn->GetAbilitySystemComponent()->HasMatchingGameplayTag(Tag_State_Weapon_IsFiring))
           {
               DOREPLIFETIME_ACTIVE_OVERRIDE(UShootInventoryItemInstance, StatTags, false);
               return;
           }
       }

       // 默认启用复制
       DOREPLIFETIME_ACTIVE_OVERRIDE(UShootInventoryItemInstance, StatTags, true);
   }
   ```

### Phase 4：快捷栏集成

**目标**：将快捷栏与新的装备系统集成

1. **改造 QuickbarComponent**：
   ```cpp
   UCLASS()
   class UShootQuickBarComponent : public UControllerComponent
   {
       GENERATED_BODY()
   public:
       UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
       void AddItemToSlot(int32 SlotIndex, UShootInventoryItemInstance* Item);

       UFUNCTION(Server, Reliable, BlueprintCallable)
       void SetActiveSlotIndex(int32 NewIndex);

   private:
       void UnequipItemInSlot();
       void EquipItemInSlot();

       UShootEquipmentManagerComponent* FindEquipmentManager() const;

       UPROPERTY(ReplicatedUsing=OnRep_Slots)
       TArray<TObjectPtr<UShootInventoryItemInstance>> Slots;

       UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
       int32 ActiveSlotIndex = -1;

       UPROPERTY()
       TObjectPtr<UShootEquipmentInstance> EquippedItem;
   };
   ```

2. **实现装备逻辑**：
   ```cpp
   void UShootQuickBarComponent::EquipItemInSlot()
   {
       if (Slots.IsValidIndex(ActiveSlotIndex))
       {
           UShootInventoryItemInstance* ItemInstance = Slots[ActiveSlotIndex];
           if (ItemInstance)
           {
               // 获取 EquippableItem Fragment
               const UShootInventoryFragment_EquippableItem* EquipFragment =
                   ItemInstance->FindFragmentByClass<UShootInventoryFragment_EquippableItem>();

               if (EquipFragment && EquipFragment->EquipmentDefinition)
               {
                   // 通过装备管理器装备
                   UShootEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
                   EquippedItem = EquipmentManager->EquipItem(EquipFragment->EquipmentDefinition);
               }
           }
       }
   }
   ```

### Phase 5：CombatComponent 重构

**目标**：简化 CombatComponent，移除重复功能

1. **移除武器管理逻辑**：
   - 移除 `ActiveWeapon`、`WeaponSlots` 等
   - 保留 GAS Ability 授予逻辑（或移至 EquipmentManager）

2. **保留必要功能**：
   - 生命周期管理
   - Ability 触发接口
   - UI 数据访问接口

### Phase 6：测试与优化

1. **功能测试**：
   - 武器装备/卸载
   - 弹药消耗与装填
   - 网络复制验证
   - 客户端预测验证

2. **性能优化**：
   - 复制带宽优化
   - SubObject 管理优化

---

## 实现优先级

### 高优先级（必须）
1. **武器实例改造**（Phase 2）：核心架构变更
2. **弹药系统改造**（Phase 3）：解决预测问题
3. **装备系统基础设施**（Phase 1）：新架构基础

### 中优先级（重要）
4. **快捷栏集成**（Phase 4）：完善用户体验
5. **CombatComponent 重构**（Phase 5）：清理冗余代码

### 低优先级（可选）
6. **性能优化**（Phase 6）：后续迭代

---

## 风险与注意事项

### 1. 破坏性变更
- **武器实例从 Actor 到 UObject**：影响所有相关代码
- **建议**：创建新的类，渐进式迁移

### 2. 网络复制复杂度
- **SubObject 复制**：需要正确处理 Outer、注册、销毁
- **建议**：参考 Lyra 完整实现，避免踩坑（UE-127172）

### 3. 弹药预测
- **复制控制**：PreReplication 逻辑复杂
- **建议**：先实现基础功能，后续优化预测

### 4. 向后兼容性
- **现有内容**：大量蓝图和数据资产需要迁移
- **建议**：提供迁移工具或临时兼容层

---

## 参考资料

### Lyra 源码文件
- `LyraWeaponInstance.h/cpp`
- `LyraRangedWeaponInstance.h/cpp`
- `LyraEquipmentInstance.h/cpp`
- `LyraEquipmentDefinition.h/cpp`
- `LyraEquipmentManagerComponent.h/cpp`
- `LyraQuickBarComponent.h/cpp`
- `InventoryFragment_EquippableItem.h`
- `InventoryFragment_SetStats.h/cpp`

### GAS Documentation
- Weapon Ammo Prediction Techniques
- Client Prediction Best Practices

### UE 网络系统
- SubObject Replication
- FastArraySerializer
- RegisteredSubObjectList (UE5)

---

## 下一步行动

1. **与团队讨论**：确认改造方案的可行性和优先级
2. **创建改造任务**：分解为可执行的子任务
3. **建立测试环境**：确保改造过程中功能可验证
4. **渐进式迁移**：避免一次性大规模重构

---

**文档修订历史**：
- 2025-11-14 v2：补充"为什么武器实例是 UObject"五大优势
- 2025-11-14 v2：补充 Instigator 连接机制详解
- 2025-11-14 v2：补充"取出/放回库存"正确理解
- 2025-11-14 v1：初始版本，调研完成

**创建日期**：2025-11-14
**作者**：Claude
**状态**：架构分析完成，包含实现指导
