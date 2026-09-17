// Copyright ZhaoYiJie

#include "Equipment/ShootEquipmentInstance.h"
#include "Equipment/ShootEquipmentDefinition.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEquipmentInstance)

// ============================================================================
// UShootEquipmentInstance
// ============================================================================

UShootEquipmentInstance::UShootEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ============================================================================
// UObject Interface
// ============================================================================

UWorld* UShootEquipmentInstance::GetWorld() const
{
	// EquipmentInstance 的 Outer 是 Pawn
	// 从 Pawn 获取 World
	if (APawn* OwnerPawn = GetPawn())
	{
		return OwnerPawn->GetWorld();
	}
	return nullptr;
}

// ============================================================================
// Pawn Access
// ============================================================================

APawn* UShootEquipmentInstance::GetPawn() const
{
	// EquipmentInstance 创建时，Outer 设置为 Pawn
	// 示例：NewObject<UShootEquipmentInstance>(Pawn, ...)
	return Cast<APawn>(GetOuter());
}

APawn* UShootEquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}
	return Result;
}

// ============================================================================
// SpawnedActors 管理
// ============================================================================

void UShootEquipmentInstance::SpawnEquipmentActors(const TArray<FShootEquipmentActorToSpawn>& ActorsToSpawn)
{
	// 只在服务器上生成 Actor
	if (APawn* OwnerPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwnerPawn->GetRootComponent();

		// 如果是 Character，附加到 Mesh
		if (ACharacter* Char = Cast<ACharacter>(OwnerPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		// 遍历配置，生成所有 Actor
		for (const FShootEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			// 生成 Actor
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(
				SpawnInfo.ActorToSpawn,
				FTransform::Identity,
				OwnerPawn
			);

			if (NewActor)
			{
				// 完成生成
				NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);

				// 附加到 Pawn
				NewActor->AttachToComponent(
					AttachTarget,
					FAttachmentTransformRules::KeepRelativeTransform,
					SpawnInfo.AttachSocket
				);

				// 设置相对变换
				NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);

				// 记录生成的 Actor
				SpawnedActors.Add(NewActor);
			}
		}
	}
}

void UShootEquipmentInstance::DestroyEquipmentActors()
{
	// 销毁所有生成的 Actor
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}

	// 清空数组
	SpawnedActors.Empty();
}

// ============================================================================
// 装备生命周期回调
// ============================================================================

void UShootEquipmentInstance::OnEquipped()
{
	// 调用蓝图事件
	K2_OnEquipped();
}

void UShootEquipmentInstance::OnUnequipped()
{
	// 调用蓝图事件
	K2_OnUnequipped();
}

// ============================================================================
// 网络复制
// ============================================================================

void UShootEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UShootEquipmentInstance, Instigator);
	DOREPLIFETIME(UShootEquipmentInstance, SpawnedActors);
}

void UShootEquipmentInstance::OnRep_Instigator()
{
	OnInstigatorReplicated();
}

void UShootEquipmentInstance::OnInstigatorReplicated()
{
	// 基类不依赖 Instigator 生成额外表现；武器等子类按需补齐。
}

#if UE_WITH_IRIS
void UShootEquipmentInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// 构建 NetToken
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS
