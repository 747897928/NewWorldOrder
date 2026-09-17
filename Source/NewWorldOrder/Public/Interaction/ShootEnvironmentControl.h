#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Abilities/GameplayAbility.h"
#include "Interaction/IInteractableTarget.h"
#include "ShootEnvironmentControl.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UShootLocalInteractionPrompt;
class ALevelSequenceActor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShootEnvironmentInteraction, APawn*, InstigatorPawn);

UENUM(BlueprintType)
enum class EShootEnvironmentTimeOfDay : uint8
{
	Day,
	Dusk,
	Night,
};

/** IInteractableTarget 禁止蓝图实现；此桥接让现有 GAS 交互触发蓝图中的环境表现。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API AShootEnvironmentControl : public AActor, public IInteractableTarget
{
	GENERATED_BODY()
public:
	AShootEnvironmentControl();
	virtual void BeginPlay() override;
	virtual void GatherInteractionOptions(const FInteractionQuery& Query, FInteractionOptionBuilder& Builder) override;

	/** 沿“白天 → 黄昏 → 夜晚”循环推进；输入仍由项目已有 IA_Interact 提供。 */
	UFUNCTION(BlueprintCallable, Category="Environment")
	void AdvanceTimeOfDay();

	/** 灯光序列、状态顺序和引用全部由蓝图配置；不占用额外输入映射。 */
	UPROPERTY(BlueprintAssignable, Category="Environment")
	FShootEnvironmentInteraction OnEnvironmentInteraction;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
	TObjectPtr<USphereComponent> InteractionCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
	TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment|Interaction")
	TObjectPtr<UShootLocalInteractionPrompt> InteractionPrompt;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Environment")
	FText InteractionText;

	/** 由备份关卡实例指向 HM_DayNight；不在 C++ 中硬编码 /Game 资产路径。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Environment|Sequence")
	TObjectPtr<ALevelSequenceActor> DayNightSequenceActor;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Environment|Sequence")
	EShootEnvironmentTimeOfDay InitialTimeOfDay = EShootEnvironmentTimeOfDay::Dusk;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Environment|Sequence", meta=(ClampMin="0"))
	int32 DayFrame = 0;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Environment|Sequence", meta=(ClampMin="0"))
	int32 DuskFrame = 60;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Environment|Sequence", meta=(ClampMin="0"))
	int32 NightFrame = 120;

private:
	void ApplyInitialTimeOfDay();
	void ApplyTimeOfDay(EShootEnvironmentTimeOfDay TimeOfDay, bool bPlayToFrame);
	int32 GetFrameForTimeOfDay(EShootEnvironmentTimeOfDay TimeOfDay) const;

	EShootEnvironmentTimeOfDay CurrentTimeOfDay = EShootEnvironmentTimeOfDay::Dusk;
};

/** 只转交当前客户端的环境表现请求，输入与附近能力授予复用现有交互系统。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_Environment : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UShootGA_Interaction_Environment();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
