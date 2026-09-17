# CSV结构定义 CSV Structure

文档版本: 1.0
最后更新: 2025-11-01

---

## CSV数据管道

CSV文件 → DataTable资产 → 游戏运行时数据

原始CSV存放位置：
- 剧情和策划/NewOrder_UE5_CSVs_vFinal/

修改流程：
1. 编辑CSV文件
2. UE5中导入/重新生成DataTable
3. 验证数据正确性
4. 测试游戏中效果

---

## WeaponStats.csv（主表）

### CSV结构

```csv
WeaponID,WeaponName,WeaponType,BaseDamage,MagazineSize,FireRateRPM,FireDelayTimeSecs,BaseSpreadAngle,SpreadPerShot,MaxSpreadAngle,SpreadRecovery,BulletsPerShot,CritMultiplier,FalloffCurveID,ReloadTime,ReloadType,WeaponTier
```

### 字段说明

| 字段名 | 类型 | 说明 | 示例 |
|-------|------|------|------|
| WeaponID | String | 唯一标识符 | WPN_AK47 |
| WeaponName | String | 武器显示名称 | AK-47 |
| WeaponType | String | 武器类型 | AR/SMG/SG/SR/LMG/Pistol/RL/GL |
| BaseDamage | Float | 基础伤害（每发） | 35.0 |
| MagazineSize | Int32 | 弹夹容量（发） | 30 |
| FireRateRPM | Float | 射速（发/分钟） | 600.0 |
| FireDelayTimeSecs | Float | 开火后延迟时间（秒），影响实际射速 | 0.12 |
| BaseSpreadAngle | Float | 基础扩散角度（度） | 1.5 |
| SpreadPerShot | Float | 每发增加扩散（度） | 0.8 |
| MaxSpreadAngle | Float | 最大扩散角度（度） | 6.0 |
| SpreadRecovery | Float | 扩散恢复速度（度/秒） | 3.0 |
| BulletsPerShot | Int32 | 单次射击子弹数 | 1（霰弹枪=8） |
| CritMultiplier | Float | 弱点伤害倍率 | 2.0 |
| FalloffCurveID | String | 距离衰减曲线ID | Curve_MidRange |
| ReloadTime | Float | 换弹时间（秒） | 2.2 |
| ReloadType | String | 换弹类型 | Magazine/Single |
| WeaponTier | Int32 | 武器品质 | 1-5 |

### 示例数据行

```csv
WPN_AK47,AK-47,AR,35.0,30,600.0,0.12,1.5,0.8,6.0,3.0,1,2.0,Curve_MidRange,2.2,Magazine,3
WPN_M870,M870,SG,15.0,8,60.0,0.20,3.0,0.0,3.0,0.0,8,1.5,Curve_CloseRange,0.6,Single,3
WPN_AWP,AWP,SR,180.0,5,50.0,0.30,0.1,0.0,0.1,0.0,1,2.5,Curve_LongRange,3.0,Magazine,5
```

---

## WeaponFalloffCurves.csv（衰减曲线表）

### CSV结构

```csv
CurveID,CurveName,Point1_Distance,Point1_Mult,Point2_Distance,Point2_Mult,Point3_Distance,Point3_Mult,Point4_Distance,Point4_Mult
```

### 字段说明

- Distance单位：厘米（cm），100cm=1米
- Mult：伤害倍率（1.0=100%伤害，0.5=50%伤害）
- 4个关键点定义曲线，UE5运行时自动插值

### 预设曲线

Curve_CloseRange（近距离 - 霰弹枪）：
```csv
Curve_CloseRange,近距离衰减,0,1.0,500,1.0,1000,0.5,2000,0.1
```
- 0-5米：100%伤害
- 5-10米：快速衰减到50%
- 10-20米：极速衰减到10%

Curve_MidRange（中距离 - 步枪/冲锋枪）：
```csv
Curve_MidRange,中距离衰减,0,1.0,2000,1.0,5000,0.7,8000,0.4
```
- 0-20米：100%伤害
- 20-50米：缓慢衰减到70%
- 50-80米：继续衰减到40%

Curve_LongRange（远距离 - 狙击枪）：
```csv
Curve_LongRange,远距离衰减,0,1.0,3000,1.0,6000,0.9,10000,0.75
```
- 0-30米：100%伤害
- 30-60米：保持90%
- 60-100米：衰减到75%

Curve_NoFalloff（无衰减 - 爆炸武器）：
```csv
Curve_NoFalloff,无距离衰减,0,1.0,5000,1.0,10000,1.0,15000,1.0
```
- 全距离100%伤害

---

## EnemyStatsPerChapter.csv（敌人血量配置）

### CSV结构

```csv
ChapterID,EnemyType,EnemyTier,BaseHP,Diff_Normal,Diff_Hard,Diff_Master,Diff_Nightmare
```

### 字段说明

| 字段名 | 类型 | 说明 |
|-------|------|------|
| ChapterID | String | 章节ID | Chapter_1 |
| EnemyType | String | 敌人类型 | Wanderer |
| EnemyTier | String | 敌人分级 | Common/Special/Elite/Boss |
| BaseHP | Float | 基础血量 | 150.0 |
| Diff_Normal | Float | 普通难度倍率 | 1.0 |
| Diff_Hard | Float | 困难难度倍率 | 1.3 |
| Diff_Master | Float | 大师难度倍率 | 1.6 |
| Diff_Nightmare | Float | 噩梦难度倍率 | 2.0 |

### 示例数据

```csv
Chapter_1,Wanderer,Common,150,1.0,1.3,1.6,2.0
Chapter_1,Hunter,Special,120,1.0,1.3,1.6,2.0
Chapter_1,Devourer,Elite,600,1.0,1.3,1.6,2.0
Chapter_7,Wanderer,Common,220,1.0,1.3,1.6,2.0
Chapter_10,Wanderer,Common,250,1.0,1.3,1.6,2.0
```

---

## AmmoConfig.csv（弹药配置）

### CSV结构

```csv
WeaponType,MaxAmmo,PickupCommon,PickupSpecial,PickupElite,DropRateCommon,DropRateSpecial,DropRateElite
```

### 字段说明

| 字段名 | 类型 | 说明 |
|-------|------|------|
| WeaponType | String | 武器类型 | AR/SMG/SG/SR/LMG/Pistol/RL/GL |
| MaxAmmo | Int32 | 最大携带弹药（不含弹夹内） | 150 |
| PickupCommon | Int32 | 拾取量（普通敌人） | 20 |
| PickupSpecial | Int32 | 拾取量（特殊敌人） | 40 |
| PickupElite | Int32 | 拾取量（精英敌人） | 40 |
| DropRateCommon | Float | 基础掉落率（普通） | 0.10 |
| DropRateSpecial | Float | 基础掉落率（特殊） | 0.30 |
| DropRateElite | Float | 基础掉落率（精英） | 0.60 |

### 示例数据

```csv
AR,150,20,40,40,0.10,0.30,0.60
SMG,150,20,40,40,0.10,0.30,0.60
SG,40,8,16,16,0.10,0.30,0.60
SR,20,5,10,10,0.10,0.30,0.60
LMG,200,40,80,80,0.10,0.30,0.60
Pistol,51,17,34,34,0.10,0.30,0.60
RL,3,1,1,1,0.00,0.00,1.00
GL,12,3,6,6,0.10,0.30,0.60
```

说明：
- AK47携带150发（弹夹外）+30发（弹夹内）=总180发
- 击杀普通敌人10%概率掉落20发
- 感知30点：掉落概率40%（10%+30%）
- RL弹药极其稀缺，仅精英100%掉落

---

## UE5 DataTable导入指南

### 步骤1：定义C++结构体

```cpp
// WeaponStatsRow.h
USTRUCT(BlueprintType)
struct FWeaponStatsRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FString WeaponID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FString WeaponName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FString WeaponType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float BaseDamage = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
    int32 MagazineSize = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    float FireRateRPM = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
    float BaseSpreadAngle = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
    float SpreadPerShot = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
    float MaxSpreadAngle = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
    float SpreadRecovery = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    int32 BulletsPerShot = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float CritMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    FString FalloffCurveID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
    float ReloadTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
    FString ReloadType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    int32 WeaponTier = 1;
};
```

### 步骤2：创建DataTable

1. Content Browser → 右键 → Miscellaneous → Data Table
2. 选择Row Structure：FWeaponStatsRow
3. 命名：DT_WeaponStats

### 步骤3：导入CSV

1. 打开DataTable
2. 工具栏 → Import → 选择WeaponStats.csv
3. 自动匹配字段（确保CSV列名与结构体属性名一致）

---

## 武器实例化示例

```cpp
// RangedWeaponInstance.cpp
void ARangedWeaponInstance::LoadWeaponData(const FString& WeaponID)
{
    // 从DataTable加载
    UDataTable* WeaponTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_WeaponStats"));

    if (!WeaponTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load WeaponStats DataTable"));
        return;
    }

    FWeaponStatsRow* WeaponData = WeaponTable->FindRow<FWeaponStatsRow>(
        FName(*WeaponID), TEXT("LoadWeaponData"));

    if (WeaponData)
    {
        // 复制基础属性
        BaseDamage = WeaponData->BaseDamage;
        MagazineSize = WeaponData->MagazineSize;
        FireRateRPM = WeaponData->FireRateRPM;

        // 复制扩散属性
        BaseSpreadAngle = WeaponData->BaseSpreadAngle;
        SpreadPerShot = WeaponData->SpreadPerShot;
        MaxSpreadAngle = WeaponData->MaxSpreadAngle;
        SpreadRecovery = WeaponData->SpreadRecovery;

        // 复制其他属性
        BulletsPerShot = WeaponData->BulletsPerShot;
        CritMultiplier = WeaponData->CritMultiplier;
        ReloadTime = WeaponData->ReloadTime;

        // 加载距离衰减曲线
        LoadFalloffCurve(WeaponData->FalloffCurveID);

        UE_LOG(LogTemp, Log, TEXT("Loaded weapon: %s"), *WeaponData->WeaponName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Weapon ID not found: %s"), *WeaponID);
    }
}
```

---

## 衰减曲线加载示例

```cpp
// 从DataTable加载衰减曲线
void ARangedWeaponInstance::LoadFalloffCurve(const FString& CurveID)
{
    UDataTable* CurveTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_FalloffCurves"));

    if (!CurveTable) return;

    FFalloffCurveRow* CurveData = CurveTable->FindRow<FFalloffCurveRow>(
        FName(*CurveID), TEXT("LoadFalloffCurve"));

    if (CurveData)
    {
        // 清空现有曲线
        DistanceFalloff.GetRichCurve()->Reset();

        // 添加4个关键点
        DistanceFalloff.GetRichCurve()->AddKey(
            CurveData->Point1_Distance, CurveData->Point1_Mult);
        DistanceFalloff.GetRichCurve()->AddKey(
            CurveData->Point2_Distance, CurveData->Point2_Mult);
        DistanceFalloff.GetRichCurve()->AddKey(
            CurveData->Point3_Distance, CurveData->Point3_Mult);
        DistanceFalloff.GetRichCurve()->AddKey(
            CurveData->Point4_Distance, CurveData->Point4_Mult);

        // 设置插值方式（线性）
        DistanceFalloff.GetRichCurve()->SetKeyInterpMode(
            DistanceFalloff.GetRichCurve()->GetFirstKeyHandle(),
            ERichCurveInterpMode::RCIM_Linear);
    }
}
```

---

## 伤害计算示例

```cpp
// 距离伤害计算
float ARangedWeaponInstance::CalculateDamageAtDistance(float Distance) const
{
    // 获取距离衰减倍率
    float FalloffMult = DistanceFalloff.GetRichCurveConst()->Eval(Distance);

    // 计算最终伤害
    float FinalDamage = BaseDamage * FalloffMult;

    return FinalDamage;
}

// 弱点伤害计算
float ARangedWeaponInstance::CalculateCriticalDamage(float BaseDamageValue) const
{
    return BaseDamageValue * CritMultiplier;
}
```

---

参考文档：
- Overview_总览.md - 武器系统总览
- WeaponData_武器数值.md - 所有武器数据
- BalanceValidation_平衡验证.md - TTK验证
- ../../claude/新秩序武器系统完整设计文档.md - 原始设计（归档）

最后更新: 2025-11-01
维护者: 项目团队
