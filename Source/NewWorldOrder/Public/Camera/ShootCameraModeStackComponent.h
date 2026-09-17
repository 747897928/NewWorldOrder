#pragma once

#include "Components/ActorComponent.h"
#include "Camera/ShootCameraTypes.h"
#include "ShootCameraModeStackComponent.generated.h"

class AShootCharacter;
class UShootExperienceDefinition;
class UShootExperienceManagerComponent;
class UShootRangedWeaponInstance;

/**
 * 项目级双相机模式栈。
 *
 * 基础层在独立 ThirdPerson FollowCamera / FirstPersonCamera 间选择，ADS 作为临时层叠加到当前输出。
 * 两个 CameraComponent 各自保留 FOV、PostProcess 和未来视图专属配置，模式栈不在切回时逐项“猜测恢复”；
 * 组件只操作本地控制 Pawn，因此每个 LocalPlayer 拥有独立模式栈，分屏不会串视角。
 */
UCLASS(ClassGroup=Camera, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootCameraModeStackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootCameraModeStackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="Camera|Perspective")
	EShootCameraPerspective GetPerspective() const;

	UFUNCTION(BlueprintPure, Category="Camera|Perspective")
	bool CanTogglePerspective() const;

	/** 由 Experience 授予的切换 GA 调用，不在 C++ 里判断 V 或手柄按键。选择会保存到本地 PlayerController。 */
	UFUNCTION(BlueprintCallable, Category="Camera|Perspective")
	void TogglePerspective();

	void SetPerspective(EShootCameraPerspective NewPerspective);

	/** ADS GA 只提交武器和过渡权重；最终相机结果由本模式栈求值。 */
	void SetAimModeWeight(UShootRangedWeaponInstance* Weapon, float NewWeight);

protected:
	/** 第一人称控制器仰角下限；值由 BP_ShootCharacter.CameraModeStack 调优。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|First Person", meta=(ClampMin="-89.0", ClampMax="89.0", ForceUnits=deg))
	float FirstPersonViewPitchMin = -75.0f;

	/** 第一人称控制器俯角上限；限制低头进入仍可见的第三人称躯干内部。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|First Person", meta=(ClampMin="-89.0", ClampMax="89.0", ForceUnits=deg))
	float FirstPersonViewPitchMax = 75.0f;

private:
	void BindToExperienceManager();
	void HandleExperienceLoaded(const UShootExperienceDefinition* Experience);
	void ApplyCameraView();
	void ActivatePerspective(EShootCameraPerspective NewPerspective);
	void UpdateFirstPersonPresentation(bool bFirstPersonActive, bool bForceCleanup = false);

	TWeakObjectPtr<AShootCharacter> CharacterOwner;
	TWeakObjectPtr<UShootExperienceManagerComponent> BoundExperienceManager;
	TWeakObjectPtr<UShootRangedWeaponInstance> AimingWeapon;

	float ThirdPersonFieldOfView = 90.0f;
	float ThirdPersonArmLength = 300.0f;
	FVector ThirdPersonTargetOffset = FVector::ZeroVector;
	FVector ThirdPersonSocketOffset = FVector::ZeroVector;
	float FirstPersonFieldOfView = 90.0f;
	float AimModeWeight = 0.0f;
	EShootCameraPerspective CurrentPerspective = EShootCameraPerspective::ThirdPerson;
	bool bAllowPerspectiveSwitch = false;
	bool bExperienceBound = false;
	bool bDefaultPerspectiveApplied = false;
	bool bFirstPersonPresentationActive = false;
	EShootCameraPerspective ExperienceDefaultPerspective = EShootCameraPerspective::ThirdPerson;
};
