# Lyra 武器系统：蓝图配置与射击流程详解

> **目标读者**：需要理解 Lyra 蓝图配置和射击预测实现细节的开发者
> **前置知识**：Lyra武器系统与库存集成_架构分析.md

## 目录
- [四层数据结构](#四层数据结构)
- [各层蓝图配置详解](#各层蓝图配置详解)
- [完整工作流程](#完整工作流程)
- [射击预测完整流程](#射击预测完整流程)
- [实现要点](#实现要点)

---

## 四层数据结构

Lyra 把"武器"拆成四层数据，每层职责明确：

```
ID_XXX         库存物品定义（Inventory Item Definition）
   ↓
WID_XXX        装备定义（Equipment Definition）
   ↓
B_WeaponInstance_XXX   武器逻辑实例（UObject 蓝图）
   ↓
B_Weapon_XXX   武器 Actor（场景中的枪）
```

总结：
- **ID_XXX**：定义"物品是什么"，有一堆 Fragment，其中一个指向 WID_XXX
- **WID_XXX**：定义"装备后要生成什么 WeaponInstance、什么 Weapon Actor、给哪些 GA"
- **B_WeaponInstance_XXX**：武器逻辑数据（散布、伤害、动画配置），是 UObject，不在场景里
- **B_Weapon_XXX**：真正的 3D 枪，只负责特效和动画，由 GA 通过 GameplayCue 驱动

---

## 各层蓝图配置详解

### 1. ID_XXX：物品定义蓝图

**资产类型**：`ULyraInventoryItemDefinition`

**文件位置**：`Plugins/GameFeatures/ShooterCore/Content/Items/ID_*.uasset`

**主要字段**：

```cpp
// DisplayName
FText DisplayName;  // UI 显示名字

// Fragments（真正有用的都在这里）
TArray<ULyraInventoryItemFragment*> Fragments;
```

**核心 Fragments**：

#### 1.1 UInventoryFragment_EquippableItem

最关键的 Fragment，连接库存和装备：

```cpp
UCLASS()
class UInventoryFragment_EquippableItem : public ULyraInventoryItemFragment
{
    GENERATED_BODY()
public:
    // 指向装备定义（WID_XXX）
    UPROPERTY(EditAnywhere, Category=Equipment)
    TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition;
};
```

作用：决定"这个库存物品对应哪种装备/武器"

#### 1.2 UInventoryFragment_QuickBarIcon

快捷栏 UI 配置：

```cpp
UPROPERTY(EditAnywhere)
FSlateBrush Icon;  // 快捷栏图标

UPROPERTY(EditAnywhere)
FText DisplayName;  // 显示名称
```

#### 1.3 UInventoryFragment_PickupIcon

世界拾取物外观：

```cpp
UPROPERTY(EditAnywhere)
USkeletalMesh* PickupMesh;  // 或 StaticMesh

UPROPERTY(EditAnywhere)
FLinearColor PadColor;  // 地上发光小板子颜色
```

#### 1.4 UInventoryFragment_SetStats

初始化物品状态：

```cpp
UPROPERTY(EditAnywhere)
TMap<FGameplayTag, int32> InitialItemStats;

// 例如配置：
// Weapon.Ammo.Magazine: 30
// Weapon.Ammo.Reserve: 120
```

在 `OnInstanceCreated` 时自动添加到 `ItemInstance->StatTags`。

#### 1.5 其他 Fragments

- `InventoryFragment_ReticleConfig`：准星 HUD 类
- `InventoryFragment_*`：可扩展自定义功能

**总结**：
> ID_XXX 就是"武器物品的数据表行"，真正的行为都没有，只有一堆 Fragment 指针。

---

### 2. WID_XXX：装备定义蓝图

**资产类型**：`ULyraEquipmentDefinition`

**文件位置**：`Plugins/GameFeatures/ShooterCore/Content/Weapons/WID_*.uasset`

**关键字段只有三类**：

#### 2.1 InstanceType

```cpp
UPROPERTY(EditDefaultsOnly, Category=Equipment)
TSubclassOf<ULyraEquipmentInstance> InstanceType;
```

对武器来说，通常是：`B_WeaponInstance_AssaultRifle`（蓝图类，继承自 `ULyraRangedWeaponInstance`）

作用："手上这把武器的逻辑对象用什么类来实例化"

#### 2.2 AbilitySetsToGrant

```cpp
UPROPERTY(EditDefaultsOnly, Category=Equipment)
TArray<TObjectPtr<const ULyraAbilitySet>> AbilitySetsToGrant;
```

每套 AbilitySet 包含：
- 当前武器的射击 GA、换弹 GA 等
- 必要时的输入 tag 绑定、GameplayEffect

装备时，这些 AbilitySet 会加到角色的 ASC。

#### 2.3 ActorsToSpawn

```cpp
UPROPERTY(EditDefaultsOnly, Category=Equipment)
TArray<FLyraEquipmentActorToSpawn> ActorsToSpawn;
```

每个元素包含：

```cpp
USTRUCT()
struct FLyraEquipmentActorToSpawn
{
    GENERATED_BODY()

    // Actor 类（通常是 B_Weapon_XXX 蓝图）
    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> ActorClass;

    // 挂到角色 Mesh 的 Socket 名
    UPROPERTY(EditAnywhere)
    FName AttachSocket;  // 例如 "Weapon_R"

    // 附加规则
    UPROPERTY(EditAnywhere)
    FAttachmentTransformRules AttachRules;

    // 相对位置/旋转/缩放
    UPROPERTY(EditAnywhere)
    FTransform RelativeTransform;
};
```

**总结**：
> WID_XXX 就是"把这个物品拿在手上时要生成什么逻辑实例、给什么能力、生成什么 Actor"。

---

### 3. B_WeaponInstance_XXX：武器逻辑实例蓝图

**继承链**：

```
ULyraEquipmentInstance
  → ULyraWeaponInstance
    → ULyraRangedWeaponInstance
      → B_WeaponInstance_Base（蓝图）
        → B_WeaponInstance_AssaultRifle / Shotgun / ...
```

**重要认知**：
> B_WeaponInstance_Base 的子类**基本只提供数据**，不写逻辑。

**在蓝图中主要配置的参数**：

#### 3.1 开火节奏相关

```cpp
// ULyraRangedWeaponInstance 的可编辑属性

// 每秒发射几发
UPROPERTY(EditAnywhere, Category="Weapon Config")
float RoundsPerMinute = 600.0f;

// 是否连发
UPROPERTY(EditAnywhere, Category="Weapon Config")
bool bAutomatic = true;

// 首发精度
UPROPERTY(EditAnywhere, Category="Weapon Config")
bool bHasFirstShotAccuracy = false;
```

#### 3.2 散布 / 精度

```cpp
// 基础散布角度
UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
FRuntimeFloatCurve HeatToSpreadCurve;

// 射击越久越大的热度增加
UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

// 散布分布指数（子弹多集中在圆心还是边缘）
UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
float SpreadExponent = 1.0f;
```

#### 3.3 弹道 / 伤害

```cpp
// 子弹数量（霰弹枪 N 发）
UPROPERTY(EditAnywhere, Category="Weapon Config")
int32 BulletsPerCartridge = 1;

// 最大伤害距离（厘米）
UPROPERTY(EditAnywhere, Category="Weapon Config")
float MaxDamageRange = 25000.0f;

// 伤害衰减曲线
UPROPERTY(EditAnywhere, Category="Weapon Config")
FRuntimeFloatCurve DistanceDamageFalloff;

// Trace 半径（Sweep 的 Radius）
UPROPERTY(EditAnywhere, Category="Weapon Config")
float BulletTraceSweepRadius = 0.0f;
```

#### 3.4 动画相关

```cpp
// 装备时的动画图层
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
FLyraAnimLayerSelectionSet EquippedAnimSet;

// 卸载时的动画图层
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
FLyraAnimLayerSelectionSet UnequippedAnimSet;
```

蓝图中会在 `OnEquipped` 事件里调用 `PickBestAnimLayer` 选择合适的 Anim Layer。

**总结**：
> B_WeaponInstance_XXX 负责"武器逻辑数据"：开火节奏、散布、伤害、动画配置等，是一个纯 UObject，不在场景里。

---

### 4. B_Weapon_XXX：武器 Actor 蓝图

**资产类型**：AActor 蓝图

**命名惯例**：`B_Weapon_AutoRifle` / `B_Weapon_Pistol`

**职责**：
> 武器 Actor 只负责射击的画面表现，会在 GA 触发的 GameplayCue 中被调用。

**里面一般有什么**：

#### 4.1 组件

```cpp
// 枪的模型
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
USkeletalMeshComponent* WeaponMesh;

// 可选的常驻特效
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
UNiagaraComponent* IdleEffect;
```

子弹壳抛出、火光多半通过**动画通知 + GameplayCue** 动态触发。

#### 4.2 变量

```cpp
// Socket 名称
UPROPERTY(EditDefaultsOnly)
FName MuzzleSocketName = "Muzzle";

UPROPERTY(EditDefaultsOnly)
FName EjectSocketName = "ShellEject";

// 特效 / 声音资源
UPROPERTY(EditDefaultsOnly)
UNiagaraSystem* MuzzleFlashEffect;

UPROPERTY(EditDefaultsOnly)
USoundBase* FireSound;
```

#### 4.3 事件图

响应自定义事件：

```cpp
// OnWeaponFire（蓝图事件）
Event OnWeaponFire
{
    // 在枪口 Socket Play Niagara
    Spawn Niagara System at Location(MuzzleSocketName);

    // 播放枪声
    Play Sound at Location(FireSound);

    // 摄像机抖动
    Client Start Camera Shake(FireCameraShake);

    // 抛壳
    Spawn Shell Eject Effect at Socket(EjectSocketName);
}

// OnWeaponReload（蓝图事件）
Event OnWeaponReload
{
    // 播放换弹动画
    Play Montage(ReloadMontage);
}

// OnEquipped（蓝图事件）
Event OnEquipped
{
    // 显示武器
    Set Visibility(true);
}
```

这些事件由 GA 通过 `GameplayCue` 触发。

**总结**：
> B_Weapon_XXX 是"枪的 3D 外壳"，不负责子弹数量、散布、伤害，只负责特效和动画，由武器 GA 通过 GameplayCue 驱动。

---

## 完整工作流程

### 1. 拾取武器时（Inventory 层）

```cpp
// 角色碰到拾取物 Actor
void ALyraWeaponPickup::OnPickedUp(APawn* Pawn)
{
    // 服务器调用
    ULyraInventoryManagerComponent* InventoryManager = Pawn->FindComponentByClass<...>();

    // 添加物品到库存
    ULyraInventoryItemInstance* ItemInstance =
        InventoryManager->AddItemDefinition(ID_AssaultRifle, 1);

    // 这会：
    // 1. 创建 ULyraInventoryItemInstance
    // 2. 保存 ID_XXX 引用
    // 3. 创建 FGameplayTagStackContainer（保存弹药等状态）
    // 4. 遍历 Fragments，调用 OnInstanceCreated
    //    - SetStats Fragment 初始化弹药 StatTags
    //    - QuickBar Fragment 提供 UI 数据
    //    - EquippableItem Fragment 连接到 WID_XXX
}
```

关键点：
- `ID_XXX` 里的多个 `InventoryFragment_*` 在此时初始化
- 最关键：`UInventoryFragment_EquippableItem` 指向 `WID_XXX`

### 2. 放入快捷栏（QuickBar 层）

```cpp
// ULyraQuickBarComponent 维护槽位数组
UPROPERTY(ReplicatedUsing=OnRep_Slots)
TArray<TObjectPtr<ULyraInventoryItemInstance>> Slots;

// 拾取时添加到槽位
void ULyraQuickBarComponent::AddItemToSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item)
{
    Slots[SlotIndex] = Item;
    MarkItemDirty();  // FastArray 复制
}
```

### 3. 从库存"取出"武器（Equip 流程）

```cpp
// 玩家切换快捷栏槽位
void ULyraQuickBarComponent::SetActiveSlotIndex(int32 NewIndex)
{
    // 1. 找到该槽位的 ItemInstance
    ULyraInventoryItemInstance* ItemInstance = Slots[NewIndex];

    // 2. 从 ItemInstance 的 Definition 获取 EquippableItem Fragment
    const UInventoryFragment_EquippableItem* EquipFragment =
        ItemInstance->FindFragmentByClass<UInventoryFragment_EquippableItem>();

    // 3. 取得 WID_XXX
    TSubclassOf<ULyraEquipmentDefinition> EquipDef = EquipFragment->EquipmentDefinition;

    // 4. 交给 EquipmentManagerComponent
    ULyraEquipmentManagerComponent* EquipManager = FindEquipmentManager();
    ULyraEquipmentInstance* EquipInstance = EquipManager->EquipItem(EquipDef);

    // 5. EquipmentManagerComponent 做三件事：
}
```

**EquipmentManagerComponent 的工作**：

```cpp
ULyraEquipmentInstance* ULyraEquipmentManagerComponent::EquipItem(TSubclassOf<ULyraEquipmentDefinition> EquipDef)
{
    // 1. 创建 WeaponInstance（UObject）
    ULyraEquipmentInstance* Instance =
        NewObject<ULyraEquipmentInstance>(GetOwner(), EquipDef->InstanceType);

    // 例如：InstanceType = B_WeaponInstance_AssaultRifle

    // 2. 生成 Weapon Actor（场景物体）
    for (const FLyraEquipmentActorToSpawn& SpawnInfo : EquipDef->ActorsToSpawn)
    {
        AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnInfo.ActorClass);
        SpawnedActor->AttachToComponent(
            Pawn->GetMesh(),
            FAttachmentTransformRules::KeepRelativeTransform,
            SpawnInfo.AttachSocket  // "Weapon_R"
        );
        SpawnedActor->SetRelativeTransform(SpawnInfo.RelativeTransform);

        Instance->SpawnedActors.Add(SpawnedActor);
    }

    // 3. 授予 Abilities
    UAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();
    for (const ULyraAbilitySet* AbilitySet : EquipDef->AbilitySetsToGrant)
    {
        AbilitySet->GiveToAbilitySystem(ASC, &Instance->GrantedHandles);
        // 授予 GA_Weapon_Fire、GA_Weapon_Reload 等
    }

    // 4. 调用 OnEquipped
    Instance->OnEquipped();

    return Instance;
}
```

**此时状态**：
- **库存里仍然有 `ItemInstance`**（你"拥有"这把枪）
- **角色身上多了**：
  - `WeaponInstance`（逻辑 UObject）
  - `Weapon Actor`（表现 AActor）
  - 一堆 Ability（射击、换弹等）

### 4. 切换武器（放回再取出）

```cpp
void ULyraQuickBarComponent::SetActiveSlotIndex(int32 NewIndex)
{
    // 对旧武器：UnequipItemInSlot()
    if (EquippedItem)
    {
        EquipmentManager->UnequipItem(EquippedItem);
        // → EquipInstance->OnUnequipped()
        // → EquipInstance->DestroyEquipmentActors()
        // → RemoveEntry (销毁 UObject)
        // ⚠️ 但 ItemInstance 不动！仍在 InventoryComponent
    }

    // 对新武器：重复步骤 3
    EquipItemInSlot(NewIndex);
}
```

**UnequipItem 的工作**：

```cpp
void ULyraEquipmentManagerComponent::UnequipItem(ULyraEquipmentInstance* Instance)
{
    // 1. 回写状态（可选）
    Instance->OnUnequipped();

    // 2. 销毁 Actor
    for (AActor* Actor : Instance->SpawnedActors)
    {
        Actor->Destroy();
    }
    Instance->SpawnedActors.Empty();

    // 3. 移除 Abilities
    UAbilitySystemComponent* ASC = GetOwnerPawn()->GetAbilitySystemComponent();
    Instance->GrantedHandles.TakeFromAbilitySystem(ASC);

    // 4. 从 EquipmentList 移除（FastArray）
    RemoveEntry(Instance);
}
```

这就是"从库存收回旧枪，再从库存拿出新枪"的实现。

---

## 射击预测完整流程

### 输入如何触发 GA

```
Enhanced Input 绑定鼠标左键/手柄扳机
    ↓
InputTag.Weapon.Fire
    ↓
映射到 GA_Weapon_Fire_Rifle_Auto 的 Activation Owned Tags
    ↓
ASC 收到 GameplayEvent / Input
    ↓
激活 GameplayAbility
```

**配置方式**：
1. Enhanced Input Mapping Context 绑定：`LeftMouseButton` → `InputTag.Weapon.Fire`
2. GA 的 AbilityTags 包含：`InputTag.Weapon.Fire`
3. 按下触发 → ASC 激活匹配的 GA

**重点**：
> 开火输入不是直接调用 C++ 函数，而是触发一个 GameplayAbility。

### 客户端做射线检测 + 生成 TargetData

在 `GA_Weapon_Fire_Rifle_Auto::ActivateAbility` 里调用：

```cpp
void ULyraGameplayAbility_RangedWeapon::StartRangedWeaponTargeting()
{
    // 1. 从 Camera 取视角位置和方向
    APawn* AvatarPawn = GetAvatarActorFromActorInfo();
    APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    FVector AimDir = CamRot.Vector();

    // 2. 获取武器散布
    ULyraRangedWeaponInstance* WeaponInstance = GetWeaponInstance();
    float SpreadAngle = WeaponInstance->GetCalculatedSpreadAngle();

    // 3. 做射线检测（考虑散布）
    TArray<FHitResult> HitResults;
    for (int32 i = 0; i < WeaponInstance->BulletsPerCartridge; ++i)
    {
        // 应用散布
        FVector BulletDir = VRandConeNormalDistribution(AimDir, SpreadAngle, SpreadExponent);

        // LineTrace
        FHitResult Hit;
        GetWorld()->LineTraceSingleByChannel(
            Hit,
            CamLoc,
            CamLoc + BulletDir * WeaponInstance->MaxDamageRange,
            Lyra_TraceChannel_Weapon
        );

        HitResults.Add(Hit);
    }

    // 4. 封装 TargetData
    FGameplayAbilityTargetDataHandle TargetData;
    for (const FHitResult& Hit : HitResults)
    {
        FLyraGameplayAbilityTargetData_SingleTargetHit* NewData =
            new FLyraGameplayAbilityTargetData_SingleTargetHit();
        NewData->HitResult = Hit;
        TargetData.Add(NewData);
    }

    // 5. RPC 给服务器
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    ASC->CallServerSetReplicatedTargetData(
        CurrentSpecHandle,
        CurrentActivationInfo.GetActivationPredictionKey(),
        TargetData,
        FGameplayTag(),
        ASC->ScopedPredictionKey
    );

    // 6. 本地立即处理
    OnTargetDataReadyCallback(TargetData, FGameplayTag());
}
```

**关键点**：
- 射线检测**完全在客户端本地**
- 服务器上**并没有校验**
- 发给服务器的 RPC 是 `CallServerSetReplicatedTargetData`

### 客户端立即 CommitAbility：减子弹、播动画、播特效

```cpp
void ULyraGameplayAbility_RangedWeapon::OnTargetDataReadyCallback(
    const FGameplayAbilityTargetDataHandle& TargetData,
    FGameplayTag ApplicationTag)
{
    // 1. 打开预测窗口
    FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponent());

    // 2. CommitAbility（检查并消耗 Cost）
    if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        // Cost 是 ULyraAbilityCost_ItemTagStack
        // CheckCost: 从 ItemInstance->StatTags 读取弹药数量
        // ApplyCost:
        //   - 服务器：扣减 StatTags（权威）
        //   - 客户端：不执行（!IsNetAuthority）

        // 3. 更新武器状态
        ULyraRangedWeaponInstance* WeaponInstance = GetWeaponInstance();
        WeaponInstance->AddSpread();  // 增加散布热度

        // 4. 蓝图事件：播放特效
        OnRangedWeaponTargetDataReady(TargetData);
        // 蓝图中会：
        // - 播放开火蒙太奇
        // - 触发 GameplayCue（muzzle flash、轨迹、命中特效）
        // - 播放音效
        // - 摄像机抖动
    }
    else
    {
        // CommitAbility 失败（弹药不足）
        K2_EndAbility();
    }
}
```

**对玩家来说**：
> 按下鼠标 → 本地立刻播动画、减子弹（UI）、看到命中特效 → 完全不等服务器

**注意**：
- Lyra 的 `ULyraAbilityCost_ItemTagStack::ApplyCost` 只在服务器执行
- 客户端**不会**立即扣减 StatTags
- 客户端 UI 更新依赖 StatTags 复制（有延迟 50-150ms）

### 服务器收到 TargetData 后做权威伤害结算

```cpp
// 服务器侧的同一个 Ability 收到 TargetData
void ULyraGameplayAbility_RangedWeapon::OnTargetDataReadyCallback_Server(
    const FGameplayAbilityTargetDataHandle& TargetData,
    FGameplayTag ApplicationTag)
{
    // 1. 服务器也调用 CommitAbility
    // ApplyCost 在服务器执行：扣减 ItemInstance->StatTags
    if (CommitAbility(...))
    {
        // 2. 应用伤害 GE
        for (int32 i = 0; i < TargetData.Num(); ++i)
        {
            const FHitResult* HitResult = TargetData.Get(i)->GetHitResult();
            if (HitResult && HitResult->GetActor())
            {
                // 获取伤害 GE
                FGameplayEffectSpecHandle DamageSpec =
                    MakeOutgoingGameplayEffectSpec(GE_Damage_RifleAuto);

                // 设置伤害参数（距离衰减等）
                float Distance = (HitResult->Location - GetAvatarActorFromActorInfo()->GetActorLocation()).Size();
                float DamageMult = WeaponInstance->DistanceDamageFalloff.GetRichCurve()->Eval(Distance);
                DamageSpec.Data->SetSetByCallerMagnitude(DamageTag, BaseDamage * DamageMult);

                // 应用到目标
                ApplyGameplayEffectSpecToTarget(
                    CurrentSpecHandle,
                    CurrentActorInfo,
                    CurrentActivationInfo,
                    DamageSpec,
                    TargetData,
                    i
                );
            }
        }

        // 3. 被击中角色的 AttributeSet 处理
        // ULyraHealthSet::PostGameplayEffectExecute
        // → 扣血
        // → 如果 Health <= 0，触发 GA_Death
    }
}
```

**关键点**：
- 服务器**不会重新做射线检测**
- 完全基于客户端上传的 TargetData
- Lyra"完全相信客户端的命中信息"

**潜在问题**：
- 在某些极端情况（遮挡、延迟）会出问题
- 容易被作弊（修改客户端上传假的 TargetData）
- Epic 论坛有开发者吐槽这一点

---

## 实现要点

### 1. Lyra 的预测策略

**预测部分**（客户端即时完成）：
- 射线检测
- 动画播放
- 粒子特效
- 音效
- 摄像机抖动
- UI 弹药更新（通过 StatTags 复制，有延迟）

**权威部分**（服务器最终结算）：
- 根据 TargetData 应用伤害 GE
- 驱动 Attribute 变化（Health、Stamina 等）
- 触发死亡 Ability

### 2. 为什么客户端弹药 UI 有延迟

Lyra 的 `ULyraAbilityCost_ItemTagStack::ApplyCost` 实现：

```cpp
void ULyraAbilityCost_ItemTagStack::ApplyCost(...)
{
    // 重点：仅在服务器执行
    if (ActorInfo->IsNetAuthority())
    {
        ItemInstance->RemoveStatTagStack(Tag, NumStacks);
    }
    // 客户端不执行任何操作
}
```

流程：
```
T=0.0s    客户端开火
          → CommitAbility
          → ApplyCost 不执行（!IsNetAuthority）
          → UI 仍显示 30 发

T=0.075s  服务器执行 ApplyCost
          → StatTags 扣减 30 → 29
          → 开始复制

T=0.15s   客户端收到 StatTags 复制
          → UI 更新 30 → 29 ✅ 有延迟但正确
```

### 3. 如何实现即时弹药 UI 反馈

参考"弹药预测机制实现指南.md"的方案 B：

在 UShootRangedWeaponInstance 添加：
```cpp
UPROPERTY(Transient)
int32 PredictedAmmoInMag = 0;

void ApplyCost()
{
    if (IsNetAuthority())
    {
        // 服务器：修改 StatTags
        ItemInstance->RemoveStatTagStack(AmmoTag, Cost);
    }
    else
    {
        // 客户端：修改预测值
        WeaponInstance->PredictedAmmoInMag -= Cost;
    }
}

void OnRep_StatTags()
{
    // 同步预测值
    WeaponInstance->PredictedAmmoInMag = GetStatTagStackCount(AmmoTag);
}
```

### 4. Fragment 扩展模式

自定义功能通过 Fragment 扩展：

```cpp
// 例如：物品品质 Fragment
UCLASS()
class UInventoryFragment_ItemQuality : public ULyraInventoryItemFragment
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    EItemQuality Quality;

    virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const override
    {
        // 设置品质相关 StatTag
        Instance->AddStatTagStack(QualityTag, (int32)Quality);
    }
};
```

### 5. 关键 Socket 命名

武器 Actor 的 Socket（SkeletalMesh）：
- `Muzzle`：枪口位置（muzzle flash）
- `ShellEject`：抛壳位置
- `Weapon_R`：角色右手 Socket（Actor 附着点）

---

**文档修订历史**：
- 2025-11-14 v1：基于 GPT5 详解创建，包含蓝图配置和射击流程

**创建日期**：2025-11-14
**作者**：Claude
**状态**：详细实现指南，包含完整代码示例
