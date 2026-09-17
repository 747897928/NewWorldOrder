// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayCues/ShootGameplayCueNotify_WeaponFire.h"

#include "Character/CombatComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayCueNotifyTypes.h"
#include "Sound/SoundBase.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "Weapons/ShootWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGameplayCueNotify_WeaponFire)

DEFINE_LOG_CATEGORY_STATIC(LogShootWeaponFireCue, Log, All);

namespace ShootWeaponFireCue
{
	/**
	 * 参数布局与 Lyra /Game/Weapons/B_Weapon 的 Fire 自定义事件保持一致。
	 * 迁入资产仍使用原事件名和参数，避免复制枪口、弹壳、Tracer、Impact 与 Decal 图表。
	 */
	struct FLyraWeaponFireParameters
	{
		TArray<FVector> ImpactPositions;
		TArray<FVector> ImpactNormals;
		TArray<TEnumAsByte<EPhysicalSurface>> ImpactSurfaceTypes;
	};
}

bool UShootGameplayCueNotify_WeaponFire::OnExecute_Implementation(
	AActor* Target,
	const FGameplayCueParameters& Parameters) const
{
	// 这段与 UGameplayCueNotify_Burst::OnExecute_Implementation 的生命周期一致，但需要
	// SpawnResult 继续执行 Lyra SetWeaponSoundParams。不能先调 Super 再补一次，否则单发声音会重复。
	UWorld* World = Target ? Target->GetWorld() : GetWorld();
	FGameplayCueNotify_SpawnContext SpawnContext(World, Target, Parameters);
	SpawnContext.SetDefaultSpawnCondition(&DefaultSpawnCondition);
	SpawnContext.SetDefaultPlacementInfo(&DefaultPlacementInfo);
	if (!DefaultSpawnCondition.ShouldSpawn(SpawnContext))
	{
		return false;
	}

	FGameplayCueNotify_SpawnResult SpawnResult;
	BurstEffects.ExecuteEffects(SpawnContext, SpawnResult);
	OnBurst(Target, Parameters, SpawnResult);

	const UShootWeaponInstance* WeaponInstance = ResolveWeaponInstance(Target, Parameters);
	const UShootRangedWeaponInstance* RangedWeapon = Cast<UShootRangedWeaponInstance>(WeaponInstance);
	if (RangedWeapon && RangedWeapon->IsProjectileWeapon())
	{
		// B_Weapon.Fire 是 Lyra 命中扫描枪的弹壳、枪口焰、Tracer 与 Impact 表现图表。
		// 火箭筒和榴弹发射器由武器动画 Notify 播放专用枪口焰/音效，投射物负责轨迹与爆炸；
		// 继续调用该图表既会重复表现，也会因特殊武器没有通用 Niagara System 而访问空组件。
		return false;
	}

	AActor* WeaponActor = ResolveWeaponPresentationActor(WeaponInstance);
	if (bUsePersistentFireAudio)
	{
		ExecuteLyraFireAudio(WeaponActor, Target);
	}
	ExecuteLyraWeaponFire(WeaponActor, Parameters);

	// 不从 C++ 反射调用 BlueprintFunctionLibrary 的生成函数。蓝图重编译/reinstance 后，
	// 缓存的 UFunction 参数链可能失效；此前通用遍历参数在 Shotgun Cue 中造成了 PIE 空指针崩溃。
	// 当前正式枪声不依赖这些辅助函数。若以后恢复 EarlyReflections/WhizBy/声音参数，必须在
	// GCN 蓝图用类型安全的节点显式连接，或写项目原生 API，不能恢复通用 ProcessEvent 适配器。

	// 与 UGameplayCueNotify_Burst 一致：这是一次性表现，不保留持续 Cue 实例。
	return false;
}

bool UShootGameplayCueNotify_WeaponFire::ExecuteLyraFireAudio(AActor* WeaponActor, AActor* Target) const
{
	if (!FireSound || !WeaponActor || !Target)
	{
		return false;
	}

	UFunction* AudioFunction = WeaponActor->FindFunction(TEXT("TriggerFireAudio"));
	if (!AudioFunction)
	{
		UE_LOG(LogShootWeaponFireCue, Warning,
			TEXT("%s has FireSound %s but no TriggerFireAudio function."),
			*GetNameSafe(WeaponActor), *GetNameSafe(FireSound));
		return false;
	}

	FObjectPropertyBase* SoundProperty = FindFProperty<FObjectPropertyBase>(AudioFunction, TEXT("Sound"));
	FObjectPropertyBase* ActorProperty = FindFProperty<FObjectPropertyBase>(AudioFunction, TEXT("Actor"));
	if (!SoundProperty || !ActorProperty)
	{
		UE_LOG(LogShootWeaponFireCue, Error,
			TEXT("%s.TriggerFireAudio parameter names changed; expected Sound and Actor."),
			*GetNameSafe(WeaponActor));
		return false;
	}

	// 使用反射属性写入参数缓冲，而不是假设蓝图函数的原生结构体布局；资产改签名时会明确报错。
	FStructOnScope FunctionParameters(AudioFunction);
	SoundProperty->SetObjectPropertyValue_InContainer(FunctionParameters.GetStructMemory(), FireSound);
	ActorProperty->SetObjectPropertyValue_InContainer(FunctionParameters.GetStructMemory(), Target);
	WeaponActor->ProcessEvent(AudioFunction, FunctionParameters.GetStructMemory());
	return true;
}

const UShootWeaponInstance* UShootGameplayCueNotify_WeaponFire::ResolveWeaponInstance(
	AActor* Target,
	const FGameplayCueParameters& Parameters) const
{
	const UShootWeaponInstance* WeaponInstance = Cast<UShootWeaponInstance>(Parameters.GetSourceObject());
	if (!WeaponInstance && Target)
	{
		// SourceObject 在远端尚未解析时，只从本次 Cue 的目标 Pawn 查询，不能使用全局 Controller。
		if (const UCombatComponent* CombatComponent = Target->FindComponentByClass<UCombatComponent>())
		{
			WeaponInstance = CombatComponent->FindActiveWeaponInstance();
		}
	}

	return WeaponInstance;
}

AActor* UShootGameplayCueNotify_WeaponFire::ResolveWeaponPresentationActor(
	const UShootWeaponInstance* WeaponInstance) const
{
	if (!WeaponInstance)
	{
		return nullptr;
	}

	for (AActor* SpawnedActor : WeaponInstance->GetSpawnedActors())
	{
		if (SpawnedActor && SpawnedActor->FindFunction(TEXT("Fire")))
		{
			return SpawnedActor;
		}
	}

	return nullptr;
}

bool UShootGameplayCueNotify_WeaponFire::ExecuteLyraWeaponFire(
	AActor* WeaponActor,
	const FGameplayCueParameters& Parameters) const
{
	if (!WeaponActor)
	{
		UE_LOG(LogShootWeaponFireCue, Verbose, TEXT("Weapon Fire Cue could not resolve a presentation actor."));
		return false;
	}

	UFunction* FireFunction = WeaponActor->FindFunction(TEXT("Fire"));
	if (!FireFunction)
	{
		return false;
	}

	if (FireFunction->ParmsSize != sizeof(ShootWeaponFireCue::FLyraWeaponFireParameters))
	{
		UE_LOG(LogShootWeaponFireCue, Error,
			TEXT("%s.Fire parameter layout changed: expected %d bytes, reflected %d bytes."),
			*GetNameSafe(WeaponActor),
			sizeof(ShootWeaponFireCue::FLyraWeaponFireParameters),
			FireFunction->ParmsSize);
		return false;
	}

	ShootWeaponFireCue::FLyraWeaponFireParameters FireParameters;
	FireParameters.ImpactPositions.Add(FVector(Parameters.Location));
	FireParameters.ImpactNormals.Add(FVector(Parameters.Normal));
	FireParameters.ImpactSurfaceTypes.Add(UPhysicalMaterial::DetermineSurfaceType(Parameters.PhysicalMaterial.Get()));

	WeaponActor->ProcessEvent(FireFunction, &FireParameters);
	return true;
}
