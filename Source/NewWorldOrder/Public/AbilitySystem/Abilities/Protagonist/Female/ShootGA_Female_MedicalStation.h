/**
 * 女主 X：医疗站（MedicalStation）
 * - 设计：部署医疗站 Actor，周期治疗范围内友军（过滤敌人），治疗量可随队伍/等级缩放，提供死亡保护等附加效果。
 * - 当前实现：服务器生成可复制医疗站 Actor，按 ILyraTeamAgentInterface 过滤友军并周期治疗。
 * - 当前冷却：终极技能充能系统落地前使用 30 秒临时冷却；Definition 与 HUD 读取同一 Cooldown Tag。
 * - 缺失：等级成长、治疗来源归因和完整死亡保护等设计细节。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Female_MedicalStation.generated.h"

class AShootSkillMedicalStation;
class AActor;

UCLASS()
class NEWWORLDORDER_API UShootGA_Female_MedicalStation : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Female_MedicalStation();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * 默认取角色胶囊脚底，避免用胶囊中心生成导致医疗站和光环悬空。
	 * GA_MedicalStation 蓝图可以覆盖此函数，实现投掷部署、吸附 NavMesh 等后续玩法。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="MedicalStation|Placement")
	FTransform ResolveMedicalStationSpawnTransform(AActor* AvatarActor) const;
	virtual FTransform ResolveMedicalStationSpawnTransform_Implementation(AActor* AvatarActor) const;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation")
	TSubclassOf<AShootSkillMedicalStation> MedicalStationClass;

	/** 当前不参与运行时；保留给 cpp 中注释的复杂地面 Visibility Trace 参考实现。 */
	UPROPERTY(EditDefaultsOnly, Category="MedicalStation|Placement|Reference", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="MedicalStation|Placement", meta=(ClampMin="0.0", Units="cm"))
	float GroundClearance = 2.f;

	/** 部署成功的一次性网络表现；持续光环由 MedicalStationClass 蓝图组件承担。 */
	UPROPERTY(EditDefaultsOnly, Category="MedicalStation|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag DeployGameplayCueTag;
};
