// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "Interface/CombatInterface.h"
#include "ShootCharacterBase.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayAbility;
class UShootReviveInteractableComponent;

/**
 * 终结死亡的复制状态，语义与 Lyra HealthComponent 一致。
 * “倒地等待救援”不是 DeathStarted；需要该玩法的 GameMode 应由模式 AbilitySet 在终结死亡前接管。
 */
UENUM(BlueprintType)
enum class EShootDeathState : uint8
{
	NotDead,
	DeathStarted,
	DeathFinished
};

//UCLASS(Abstract),这是个抽象类，加Abstract阻止这个类拖入到关卡中
UCLASS(Abstract)
class NEWWORLDORDER_API AShootCharacterBase : public AModularCharacter, public IAbilitySystemInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	AShootCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 回合/副本重置属性：移除本角色施放的默认属性 GE(SourceObject=this) 后重新初始化。
	 * 服务器调用；玩家与敌人在回合开始时由 GameMode::InitializeRoundForAll 调用。
	 */
	void ResetAndInitializeDefaultAttributes() const;

	//无论是`OwnerActor`还是`AvatarActor`（如果它们是不同的`Actors`），都应该实现`IAbilitySystemInterface`接口。
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Combat Interface */
	virtual UAnimMontage* GetHitReactMontage_Implementation() override;	
	virtual void Die(const FVector& DeathImpulse) override;
	virtual FOnDeathSignature& GetOnDeathDelegate() override { return OnDeathDelegate; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** GameMode 完成尸体阶段时调用；新 Pawn 重生后从 NotDead 开始。 */
	void FinishDeath();

	UFUNCTION(BlueprintPure, Category="Combat|Death")
	EShootDeathState GetDeathState() const { return DeathState; }
	//virtual FOnDeathSignature& GetOnDeathDelegate() override;
	//virtual FVector GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag) override;
	virtual bool IsDead_Implementation() const override;
	//virtual AActor* GetAvatar_Implementation() override;
	// virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() override;
	// virtual UNiagaraSystem* GetBloodEffect_Implementation() override;
	// virtual FTaggedMontage GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) override;
	// virtual int32 GetMinionCount_Implementation() override;
	// virtual void IncremenetMinionCount_Implementation(int32 Amount) override;
	// virtual ECharacterClass GetCharacterClass_Implementation() override;
	// virtual FOnASCRegistered& GetOnASCRegisteredDelegate() override;
	// virtual USkeletalMeshComponent* GetWeapon_Implementation() override;
	// virtual void SetIsBeingShocked_Implementation(bool bInShock) override;
	// virtual bool IsBeingShocked_Implementation() const override;
	//virtual FOnDamageSignature& GetOnDamageSignature() override;
	/** end Combat Interface */
	
protected:

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
	virtual void BeginPlay() override;

	virtual void InitAbilityActorInfo();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;

	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	virtual void InitializeDefaultAttributes() const;

	
	void AddCharacterAbilities();

	// 倒地救援交互开关；只供启用该玩法的模式 AbilitySet/GA 驱动
	void SetReviveInteractableEnabled(bool bEnabled);

	UPROPERTY(ReplicatedUsing=OnRep_DeathState, BlueprintReadOnly, Category="Combat|Death")
	EShootDeathState DeathState = EShootDeathState::NotDead;

	UFUNCTION()
	void OnRep_DeathState(EShootDeathState OldDeathState);

	void StartDeath();

	/** StartDeath 首次进入时广播；服务器所有权组件和蓝图表现可以共用同一终结死亡信号。 */
	UPROPERTY(BlueprintAssignable, Category="Combat|Death")
	FOnDeathSignature OnDeathDelegate;
private:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupPassiveAbilities;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(Category="Interaction|Revive", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootReviveInteractableComponent> ReviveInteractableComponent;
};
