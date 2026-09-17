# Lyra 库存系统架构解析

日期：2025-11-14
状态：[可用]
来源：Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Inventory & System

## 2025-11-15 补充
- PlayerState 必须同时持有 `UResourceInventoryComponent`（堆叠资源仓库）与 `UShootInventoryManagerComponent`（有身份背包）。前者只存材料/货币/徽章/设计图等 Persistent 数据，后者存武器/装备/可实例化消耗品。
- `UShootInventoryItemInstance` 需要 `EShootItemLifetime`，InventoryManager 提供 `AddPersistentItem` 与 `AddRuntimeItem`；副本拾取武器一律标记为 RuntimeOnly，退出副本清空且不写存档、不改 QuickBar。
- QuickBar、EquipmentManager、武器 GA 只能依赖 InventoryManager 返回的 ItemInstance，不得直接引用 SaveGame 或 DataAsset；ResourceInventory 严禁出现 RuntimeOnly 概念。
- 编写库存/武器 C++ 代码时需加入中文注解，明确 ResourceInventory=数量型仓库、InventoryManager=有身份背包、QuickBar/Equipment 仅引用 InventoryManager，方便 AI/同事快速回忆架构。

## 概述

Lyra 库存系统是一个完整的物品管理解决方案，支持网络复制、堆叠、Fragment 扩展。核心设计理念：
1. 定义与实例分离（Definition vs Instance）
2. 组合模式（Fragment 系统）
3. FastArray 增量复制
4. SubObject 复制机制
5. GameplayMessage 消息广播

## 核心架构

```
ULyraInventoryManagerComponent (ActorComponent)
└── FLyraInventoryList (FastArraySerializer)
    └── TArray<FLyraInventoryEntry> (FastArraySerializerItem)
        ├── ULyraInventoryItemInstance (UObject, Replicated)
        │   ├── ItemDef (TSubclassOf<ULyraInventoryItemDefinition>)
        │   └── StatTags (FGameplayTagStackContainer)
        ├── StackCount (int32)
        └── LastObservedCount (int32, NotReplicated)
```

## 第一层：GameplayTagStack（基础数据结构）

### FGameplayTagStack
```cpp
USTRUCT(BlueprintType)
struct FGameplayTagStack : public FFastArraySerializerItem
{
    UPROPERTY()
    FGameplayTag Tag;           // Tag 标识符

    UPROPERTY()
    int32 StackCount = 0;       // 堆栈数量
};
```

### FGameplayTagStackContainer
```cpp
USTRUCT(BlueprintType)
struct FGameplayTagStackContainer : public FFastArraySerializer
{
    // 复制的堆栈数组（网络传输）
    UPROPERTY()
    TArray<FGameplayTagStack> Stacks;

    // 加速查询的 Map（本地缓存，不复制）
    TMap<FGameplayTag, int32> TagToCountMap;
};
```

**关键设计**：
- 双重数据结构：TArray 用于网络复制，TMap 用于 O(1) 查询
- FastArray 增量复制：只传输变化的 Stack
- 自动维护缓存：通过 PreReplicatedRemove/PostReplicatedAdd/PostReplicatedChange 同步 Map

**核心方法**：
```cpp
void AddStack(FGameplayTag Tag, int32 StackCount);     // 添加堆栈
void RemoveStack(FGameplayTag Tag, int32 StackCount);  // 移除堆栈
int32 GetStackCount(FGameplayTag Tag) const;           // 查询数量
bool ContainsTag(FGameplayTag Tag) const;              // 是否包含
```

## 第二层：ItemDefinition & ItemInstance

### ULyraInventoryItemDefinition（物品定义 - 静态数据）
```cpp
UCLASS(Blueprintable, Const, Abstract)
class ULyraInventoryItemDefinition : public UObject
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
    FText DisplayName;  // 显示名称

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
    TArray<TObjectPtr<ULyraInventoryItemFragment>> Fragments;  // 功能片段数组
};
```

**特点**：
- DataAsset 形式，在编辑器中创建
- Const 标记，运行时不可修改
- Abstract 抽象类，必须继承使用
- 使用 Fragment 组合模式实现功能扩展

### ULyraInventoryItemFragment（功能片段）
```cpp
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class ULyraInventoryItemFragment : public UObject
{
    virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const {}
};
```

**作用**：
- 为物品添加各种功能（装备、消耗、制作等）
- EditInlineNew：可以在 Definition 中内联编辑
- OnInstanceCreated：实例创建时的初始化钩子

### ULyraInventoryItemInstance（物品实例 - 运行时状态）
```cpp
UCLASS(BlueprintType)
class ULyraInventoryItemInstance : public UObject
{
    // 复制的 StatTags（材料、货币等堆栈数据）
    UPROPERTY(Replicated)
    FGameplayTagStackContainer StatTags;

    // 指向定义
    UPROPERTY(Replicated)
    TSubclassOf<ULyraInventoryItemDefinition> ItemDef;

    virtual bool IsSupportedForNetworking() const override { return true; }
};
```

**特点**：
- UObject 形式，支持网络复制
- 包含 StatTags 用于存储动态数据（弹药、材料数量等）
- 提供 AddStatTagStack/RemoveStatTagStack/GetStatTagStackCount/HasStatTag
- 提供 FindFragmentByClass 查找功能片段

## 第三层：InventoryList & InventoryEntry

### FLyraInventoryEntry（库存条目）
```cpp
USTRUCT(BlueprintType)
struct FLyraInventoryEntry : public FFastArraySerializerItem
{
    UPROPERTY()
    TObjectPtr<ULyraInventoryItemInstance> Instance = nullptr;  // 物品实例

    UPROPERTY()
    int32 StackCount = 0;  // 堆栈数量（注意：与 StatTags 不同！）

    UPROPERTY(NotReplicated)
    int32 LastObservedCount = INDEX_NONE;  // 用于跟踪变化
};
```

**设计要点**：
- StackCount 表示"这个 Entry 的数量"（通常为 1）
- LastObservedCount 用于在复制钩子中检测变化
- NotReplicated：只在本地使用，不网络传输

### FLyraInventoryList（库存列表）
```cpp
USTRUCT(BlueprintType)
struct FLyraInventoryList : public FFastArraySerializer
{
    // 复制的条目数组
    UPROPERTY()
    TArray<FLyraInventoryEntry> Entries;

    // 拥有组件（不复制）
    UPROPERTY(NotReplicated)
    TObjectPtr<UActorComponent> OwnerComponent;
};
```

**核心方法**：
```cpp
// 添加物品定义（创建实例）
ULyraInventoryItemInstance* AddEntry(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount);

// 添加物品实例
void AddEntry(ULyraInventoryItemInstance* Instance);

// 移除物品实例
void RemoveEntry(ULyraInventoryItemInstance* Instance);

// 获取所有物品
TArray<ULyraInventoryItemInstance*> GetAllItems() const;

// 广播变化消息
void BroadcastChangeMessage(FLyraInventoryEntry& Entry, int32 OldCount, int32 NewCount);
```

**AddEntry 详细流程**：
1. 检查权威性（HasAuthority）
2. 创建 ItemInstance（Outer 为 Actor，非 Component）
3. 设置 ItemDef
4. 遍历所有 Fragment，调用 OnInstanceCreated
5. 设置 StackCount
6. MarkItemDirty 通知网络复制
7. 返回 Instance

**复制钩子**：
```cpp
void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    // 遍历被移除的索引
    for (int32 Index : RemovedIndices)
    {
        FLyraInventoryEntry& Stack = Entries[Index];
        // 广播变化：OldCount -> 0
        BroadcastChangeMessage(Stack, Stack.StackCount, 0);
        Stack.LastObservedCount = 0;
    }
}

void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    // 遍历新添加的索引
    for (int32 Index : AddedIndices)
    {
        FLyraInventoryEntry& Stack = Entries[Index];
        // 广播变化：0 -> NewCount
        BroadcastChangeMessage(Stack, 0, Stack.StackCount);
        Stack.LastObservedCount = Stack.StackCount;
    }
}

void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    // 遍历改变的索引
    for (int32 Index : ChangedIndices)
    {
        FLyraInventoryEntry& Stack = Entries[Index];
        // 广播变化：LastObserved -> NewCount
        BroadcastChangeMessage(Stack, Stack.LastObservedCount, Stack.StackCount);
        Stack.LastObservedCount = Stack.StackCount;
    }
}
```

## 第四层：InventoryManagerComponent

### ULyraInventoryManagerComponent（库存管理组件）
```cpp
UCLASS(MinimalAPI, BlueprintType)
class ULyraInventoryManagerComponent : public UActorComponent
{
    UPROPERTY(Replicated)
    FLyraInventoryList InventoryList;  // 库存列表
};
```

**构造函数**：
```cpp
ULyraInventoryManagerComponent::ULyraInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , InventoryList(this)  // 传递 this 作为 OwnerComponent
{
    SetIsReplicatedByDefault(true);
}
```

**复制设置**：
```cpp
void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ThisClass, InventoryList);
}
```

**公共 API**：
```cpp
// 检查是否可以添加
bool CanAddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount);

// 添加物品定义（创建实例）
ULyraInventoryItemInstance* AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount);

// 添加物品实例
void AddItemInstance(ULyraInventoryItemInstance* ItemInstance);

// 移除物品实例
void RemoveItemInstance(ULyraInventoryItemInstance* ItemInstance);

// 获取所有物品
TArray<ULyraInventoryItemInstance*> GetAllItems() const;

// 查找第一个匹配的物品
ULyraInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const;

// 获取物品总数（Entry 数量，非 StackCount）
int32 GetTotalItemCountByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const;

// 消耗物品
bool ConsumeItemsByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 NumToConsume);
```

## 关键：SubObject 复制机制

UE5 引入了新的 RegisteredSubObjectList 机制，用于复制 UObject 子对象。

### ReadyForReplication
```cpp
void ULyraInventoryManagerComponent::ReadyForReplication()
{
    Super::ReadyForReplication();

    // 注册所有已存在的 ItemInstance
    if (IsUsingRegisteredSubObjectList())
    {
        for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
        {
            ULyraInventoryItemInstance* Instance = Entry.Instance;
            if (IsValid(Instance))
            {
                AddReplicatedSubObject(Instance);  // 新机制
            }
        }
    }
}
```

### AddItemDefinition
```cpp
ULyraInventoryItemInstance* ULyraInventoryManagerComponent::AddItemDefinition(
    TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
    ULyraInventoryItemInstance* Result = nullptr;
    if (ItemDef != nullptr)
    {
        Result = InventoryList.AddEntry(ItemDef, StackCount);

        // 注册子对象复制（新机制）
        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
        {
            AddReplicatedSubObject(Result);
        }
    }
    return Result;
}
```

### RemoveItemInstance
```cpp
void ULyraInventoryManagerComponent::RemoveItemInstance(ULyraInventoryItemInstance* ItemInstance)
{
    InventoryList.RemoveEntry(ItemInstance);

    // 注销子对象复制（新机制）
    if (ItemInstance && IsUsingRegisteredSubObjectList())
    {
        RemoveReplicatedSubObject(ItemInstance);
    }
}
```

### ReplicateSubobjects（旧机制，作为后备）
```cpp
bool ULyraInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    // 手动复制所有 ItemInstance
    for (FLyraInventoryEntry& Entry : InventoryList.Entries)
    {
        ULyraInventoryItemInstance* Instance = Entry.Instance;
        if (Instance && IsValid(Instance))
        {
            WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
        }
    }

    return WroteSomething;
}
```

## GameplayMessage 广播

### TAG 定义
```cpp
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Lyra_Inventory_Message_StackChanged,
                              "Lyra.Inventory.Message.StackChanged");
```

### 消息结构
```cpp
USTRUCT(BlueprintType)
struct FLyraInventoryChangeMessage
{
    UPROPERTY(BlueprintReadOnly, Category=Inventory)
    TObjectPtr<UActorComponent> InventoryOwner = nullptr;  // 拥有组件

    UPROPERTY(BlueprintReadOnly, Category=Inventory)
    TObjectPtr<ULyraInventoryItemInstance> Instance = nullptr;  // 物品实例

    UPROPERTY(BlueprintReadOnly, Category=Inventory)
    int32 NewCount = 0;  // 新数量

    UPROPERTY(BlueprintReadOnly, Category=Inventory)
    int32 Delta = 0;  // 变化量
};
```

### 广播实现
```cpp
void FLyraInventoryList::BroadcastChangeMessage(FLyraInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
    FLyraInventoryChangeMessage Message;
    Message.InventoryOwner = OwnerComponent;
    Message.Instance = Entry.Instance;
    Message.NewCount = NewCount;
    Message.Delta = NewCount - OldCount;

    UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
    MessageSystem.BroadcastMessage(TAG_Lyra_Inventory_Message_StackChanged, Message);
}
```

## 核心优势

1. **增量复制**：FastArray 只传输变化的数据，节省带宽
2. **SubObject 复制**：UObject Instance 自动处理复制，无需手动管理
3. **消息驱动**：通过 GameplayMessage 解耦 UI 和数据层
4. **扩展性**：Fragment 模式允许组合各种功能
5. **性能优化**：TagToCountMap 提供 O(1) 查询

## 移植注意事项

1. **命名空间**：所有 Lyra 前缀改为 Shoot
2. **依赖清理**：移除对 Lyra AbilitySet/Equipment 的依赖
3. **Outer 对象**：ItemInstance 的 Outer 必须是 Actor，非 Component
4. **GameplayTag**：定义项目专用的消息 Tag
5. **SubObject 注册**：确保正确调用 AddReplicatedSubObject/RemoveReplicatedSubObject
6. **复制钩子**：必须实现 Pre/Post ReplicatedRemove/Add/Change

## 扩展点

1. **Fragment 系统**：创建 WeaponFragment、ConsumableFragment、CraftingRecipeFragment
2. **过滤器**：实现物品过滤器系统（按类型、Tag 等）
3. **堆叠限制**：在 CanAddItemDefinition 中实现堆叠上限检查
4. **唯一性检查**：防止重复添加唯一物品（徽章、设计图）
5. **UI 数据绑定**：通过 MVVM 模式绑定库存数据到 UI

## 参考文件

- Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/System/GameplayTagStack.*
- Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Inventory/LyraInventoryItemDefinition.*
- Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Inventory/LyraInventoryItemInstance.*
- Docs/ExampleProjectCode/LyraStarterGame/Source/LyraGame/Inventory/LyraInventoryManagerComponent.*

## 相关文档

- Docs/Tasks/InventorySystem/DesignSpec_设计规格.md
- Docs/Tasks/InventorySystem/Implementation_实现指南.md

最后更新：2025-11-14
