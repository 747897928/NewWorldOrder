// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "AttributeViewModel.generated.h"

struct FShootAttributeInfo;
struct FGameplayTag;
struct FGameplayAttribute;
class UAttributeInfo;
class AShootPlayerController;
class UShootAttributeSet;
class UShootAbilitySystemComponent;
class AShootPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelChangedSignature, int32, NewLevel, bool, bLevelUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeInfoSignature, const FShootAttributeInfo&, Info);

/**
 * BlueprintType必须写，VM默认不支持蓝图
 */
UCLASS(BlueprintType, DisplayName = "Attribute ViewModel", Config=Game)
class NEWWORLDORDER_API UAttributeViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    UAttributeViewModel();

    virtual void BeginDestroy() override;
    
    //必须指定FieldNotify说明符，才能向控件广播值更改。 具有此说明符的变量将出现在"视图绑定（View Binding）"菜单中。
    //如果没有FieldNotify，你就只能在OneTime模式下绑定到变量。
    // 基础属性
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float Health;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float MaxHealth;

	/** PlayerState 复制的玩家名；本地 HUD 和世界名牌共用同一 ViewModel 类型，但各自持有独立实例。 */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
	FText PlayerName;

	/** 统一由 Health / MaxHealth 计算，避免各 Widget 重复实现比例和除零规则。 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter)
	float HealthPercent;

	// 当前护盾与最大护盾分别绑定 W_Shieldbar；ShieldCapacity 不是可消耗数值。
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
	float ShieldValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    int32 CurrentLevel;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float CurrentXPBarPercent;

    UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
    FAttributeInfoSignature AttributeInfoDelegate;

    // 属性值字段
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float StrengthValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float VitalityValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float AgilityValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float PerceptionValue;
    
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float ArmorValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float ArmorPenetrationValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float CriticalHitChanceValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float CriticalHitDamageValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float ShieldCapacityValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    float DamageReductionValue;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter)
    int32 AttributePointsValue;

    
public:
    // 基础属性Getter/Setter
    float GetHealth() const;
    void SetHealth(float NewHealth);
    float GetMaxHealth() const;
    void SetMaxHealth(float NewMaxHealth);
	FText GetPlayerName() const;
	void SetPlayerName(FText NewPlayerName);
	float GetHealthPercent() const;
	float GetShieldValue() const;
	void SetShieldValue(float NewShieldValue);

    int32 GetCurrentLevel() const;
    void SetCurrentLevel(int32 NewCurrentLevel);

    float GetCurrentXPBarPercent() const;
    void SetCurrentXPBarPercent(float NewCurrentXPBarPercent);

    // 属性值Getter/Setter
    float GetStrengthValue() const;
    void SetStrengthValue(float NewStrengthValue);

    float GetVitalityValue() const;
    void SetVitalityValue(float NewVitalityValue);

    float GetAgilityValue() const;
    void SetAgilityValue(float NewAgilityValue);

    float GetPerceptionValue() const;
    void SetPerceptionValue(float NewPerceptionValue);
    
    float GetArmorValue() const;
    void SetArmorValue(float NewArmorValue);

    float GetArmorPenetrationValue() const;
    void SetArmorPenetrationValue(float NewArmorPenetrationValue);

    float GetCriticalHitChanceValue() const;
    void SetCriticalHitChanceValue(float NewCriticalHitChanceValue);

    float GetCriticalHitDamageValue() const;
    void SetCriticalHitDamageValue(float NewCriticalHitDamageValue);

    float GetShieldCapacityValue() const;
    void SetShieldCapacityValue(float NewShieldCapacityValue);

    float GetDamageReductionValue() const;
    void SetDamageReductionValue(float NewDamageReductionValue);

    int32 GetAttributePointsValue() const;
    void SetAttributePointsValue(int32 NewAttributePointsValue);


    // 初始化属性映射
    void InitAttributeMappings();
    
    // 设置玩家状态
    void SetPlayerState(AShootPlayerState* InPlayerState);
    AShootPlayerState* GetPlayerState();

    UShootAbilitySystemComponent* GetShootAbilitySystemComponent();
    UShootAttributeSet* GetShootAttributeSet();

    // 初始化和清理
	UFUNCTION(BlueprintCallable, Category="ViewModel")
    void InitializeWithPlayerState(AShootPlayerState* InPlayerState);
    void Cleanup();

    // 绑定和解绑回调
    void BindCallbacksToDependencies();
    void UnBindCallbacksToDependencies();

    void OnXPChanged(int32 NewXP);
    void HandleLevelChanged(int32 NewLevel, bool bLevelUp);
    
    UPROPERTY(BlueprintAssignable, Category="GAS|Level")
    FOnLevelChangedSignature OnPlayerLevelChangedDelegate;

    UFUNCTION(BlueprintCallable)
    void UpgradeAttribute(const FGameplayTag& AttributeTag);
    
protected:
    UPROPERTY(BlueprintReadOnly, Category="ViewModel")
    TWeakObjectPtr<AShootPlayerState> PlayerState;

    UPROPERTY(BlueprintReadOnly, Category="ViewModel")
    TWeakObjectPtr<UShootAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(BlueprintReadOnly, Category="ViewModel")
    TWeakObjectPtr<UShootAttributeSet> AttributeSet;
    
private:
    // 属性标签到setter的映射
    TMap<FGameplayTag, TFunction<void(float)>> AttributeTagToValueSetterMap;
    
    // 属性标签到getter的映射
    TMap<FGameplayTag, TFunction<float()>> AttributeTagToValueGetterMap;
    
    // 属性变化委托句柄
    TMap<FGameplayTag, FDelegateHandle> AttributeDelegateHandles;

    // 玩家状态相关委托句柄
    FDelegateHandle XPChangedHandle;
    FDelegateHandle LevelChangedHandle;
    FDelegateHandle AttributePointsChangedHandle;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle ShieldChangedHandle;

	void RefreshHealthPercent();

private:
	// 更新属性信息
	void UpdateAttributeInfo(const FGameplayTag& AttributeTag, float NewValue);
	bool TryInitializeHealthSnapshot();
    
	// 检查并重新获取依赖
	bool EnsureDependencies();

	/** 首个有效快照前的 Health=0 表示属性 GE 尚未就绪，不能让分屏 HUD 把它当成死亡。 */
	bool bHasValidHealthSnapshot = false;
};
