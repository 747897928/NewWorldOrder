// 项目化手雷能力：服务端生成投射物，客户端仅预测冷却 UI。
#include "AbilitySystem/Abilities/ShootGA_ThrowGrenade.h"

#include "AbilitySystem/Effects/ShootEffect_GrenadeCooldown.h"
#include "AbilitySystem/Effects/ShootEffect_ThrowGrenadeDamage.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/LyraInteractionDurationMessage.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Weapons/Projectiles/ShootProjectileGrenade.h"

UShootGA_ThrowGrenade::UShootGA_ThrowGrenade(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Grenade"), false)));
	DurationMessageTag = FGameplayTag::RequestGameplayTag(FName("Ability.Grenade.Duration.Message"), false);
	ProjectileClass = AShootProjectileGrenade::StaticClass();
	DamageEffectClass = UShootEffect_ThrowGrenadeDamage::StaticClass();
	CooldownGameplayEffectClass = UShootEffect_GrenadeCooldown::StaticClass();
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Cooldown.Weapon.Grenade"), false));
	ProjectileConfig.bShouldBounce = true;
	ProjectileConfig.FuseTime = 2.5f;
	ProjectileConfig.DamageInnerRadius = 250.f;
	ProjectileConfig.DamageOuterRadius = 500.f;
	ProjectileConfig.DamageFalloff = 1.f;
	ProjectileConfig.ExplosionCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Grenade.Detonate"), false);
}

void UShootGA_ThrowGrenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DSH] ThrowGrenade COMMIT FAILED"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[DSH] ThrowGrenade activated (hasAuth=%d)"), ActorInfo->IsNetAuthority());

	// 投掷动画仅本地拥有者播放（与 Lyra GA_Grenade 的 PlayMontageAndWait 一致）。
	if (UAnimMontage* ThrowMontage = GetThrowMontageForAvatar())
	{
		if (ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			AvatarCharacter->PlayAnimMontage(ThrowMontage);

			// 投掷音效只在本地控制端播放一次；服务器仍负责生成投射物，
			// 不把同一个本地反馈再从权威端重复播放给 Listen Server。
			if (ThrowSound && ActorInfo->IsLocallyControlled())
			{
				UGameplayStatics::PlaySoundAtLocation(
					AvatarCharacter, ThrowSound, AvatarCharacter->GetActorLocation(),
					AvatarCharacter->GetActorRotation(), 1.0f, 1.0f, 0.0f, ThrowSoundAttenuation);
			}
		}
	}

	BroadcastCooldownDuration();
	if (K2_HasAuthority())
	{
		SpawnGrenade();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UShootGA_ThrowGrenade::SpawnGrenade()
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!SourceASC || !AvatarPawn || !ProjectileClass || !DamageEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DSH] SpawnGrenade early-return: asc=%d pawn=%d proj=%d dmg=%d"),
			SourceASC ? 1 : 0, AvatarPawn ? 1 : 0, ProjectileClass ? 1 : 0, DamageEffectClass ? 1 : 0);
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	AvatarPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	if (const APlayerController* PlayerController = Cast<APlayerController>(AvatarPawn->GetController()))
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	FVector ThrowDirection = ViewRotation.Vector();
	if (!ThrowDirection.Normalize())
	{
		ThrowDirection = AvatarPawn->GetActorForwardVector();
	}
	const FVector SpawnLocation = AvatarPawn->GetActorLocation() + FVector(0.f, 0.f, AvatarPawn->BaseEyeHeight * 0.7f) + ThrowDirection * 50.f;
	const FTransform SpawnTransform(ThrowDirection.Rotation(), SpawnLocation);
	UE_LOG(LogTemp, Warning, TEXT("[DSH] SpawnGrenade: dir=(%f,%f,%f) loc=(%f,%f,%f) world=%s"),
		ThrowDirection.X, ThrowDirection.Y, ThrowDirection.Z, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z, *GetNameSafe(GetWorld()));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarPawn;
	SpawnParams.Instigator = AvatarPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShootProjectileBase* Projectile = GetWorld()->SpawnActor<AShootProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DSH] SpawnGrenade: SpawnActor returned null"));
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(AvatarPawn, AvatarPawn);
	// 同步 SpawnActor 已完成 FinishSpawning；这里不再调用 FinishSpawning(二次调用会触发
	// !bHasFinishedSpawning Ensure)。InitializeProjectile 仅做配置并激活移动，可在 spawn 后执行。
	Projectile->InitializeProjectile(ProjectileConfig, nullptr, SourceASC, DamageEffectClass,
		GetAbilityLevel(), EffectContext, ThrowDirection);
	UE_LOG(LogTemp, Warning, TEXT("[DSH] Grenade spawned: %s"), *GetNameSafe(Projectile));
}

UAnimMontage* UShootGA_ThrowGrenade::GetThrowMontageForAvatar() const
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystem)
	{
		return nullptr;
	}
	const FGameplayTag MaleKit = FGameplayTag::RequestGameplayTag(FName("Abilities.Kit.Protagonist.Male"), false);
	if (MaleKit.IsValid() && AbilitySystem->HasMatchingGameplayTag(MaleKit))
	{
		return ThrowMontageMale;
	}
	return ThrowMontageFemale;
}

void UShootGA_ThrowGrenade::BroadcastCooldownDuration() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !DurationMessageTag.IsValid() || !UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	FLyraInteractionDurationMessage Message;
	Message.Instigator = GetAvatarActorFromActorInfo();
	Message.Duration = GetCooldown(GetAbilityLevel());
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(DurationMessageTag, Message);
}
