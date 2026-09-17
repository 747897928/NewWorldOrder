// Copyright ZhaoYiJie

#include "Feedback/NumberPops/ShootNumberPopComponent_NiagaraText.h"

#include "Feedback/NumberPops/ShootDamagePopStyleNiagara.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

void UShootNumberPopComponent_NiagaraText::AddNumberPop(const FShootNumberPopRequest& NewRequest)
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || !Style || !Style->TextNiagara
		|| Style->NiagaraArrayName.IsNone() || NewRequest.NumberToDisplay <= 0)
	{
		return;
	}

	// 与 Lyra 一致：负 W 表示暴击/弱点，Niagara 材质据此选择强调样式。
	const int32 EncodedDamage = NewRequest.bIsCriticalDamage
		? -NewRequest.NumberToDisplay
		: NewRequest.NumberToDisplay;

	if (!IsValid(NiagaraComponent))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		ANiagaraActor* NiagaraActor = GetWorld()->SpawnActor<ANiagaraActor>(
			ANiagaraActor::StaticClass(), FTransform::Identity, SpawnParameters);
		if (!NiagaraActor)
		{
			return;
		}

		// 独立 Actor 只承载该 LocalPlayer 的伤害数字。不能把粒子挂到 Pawn 后再隐藏 Actor，
		// 否则其他分屏视口会连角色本体一起隐藏。
		NiagaraComponent = NiagaraActor->GetNiagaraComponent();
		NiagaraComponent->SetAsset(Style->TextNiagara);
		NiagaraComponent->bAutoActivate = false;
		NiagaraComponent->SetOnlyOwnerSee(false);
	}

	AActor* NiagaraActor = NiagaraComponent->GetOwner();
	// Lyra 默认一台客户端只有一个 LocalPlayer，因此未处理同世界分屏。Niagara 状态流在 UE 5.8
	// 不可靠地支持 Primitive 的 OnlyOwnerSee；这里改走每个 PlayerController 原生的 HiddenActors。
	// 每次请求刷新列表，以覆盖运行中新增/移除 LocalPlayer 的生命周期。
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* LocalController = It->Get(); LocalController && LocalController->IsLocalController())
		{
			if (LocalController == Controller)
			{
				LocalController->HiddenActors.Remove(NiagaraActor);
			}
			else
			{
				LocalController->HiddenActors.AddUnique(NiagaraActor);
			}
		}
	}

	NiagaraComponent->SetWorldLocation(NewRequest.WorldLocation);
	NiagaraComponent->Activate(false);

	// 一个长期 NiagaraComponent 接收所有请求；NS_DamageNumbers 自己消费 DamageInfo，避免每发创建 UObject。
	TArray<FVector4> DamageList = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector4(
		NiagaraComponent, Style->NiagaraArrayName);
	DamageList.Add(FVector4(NewRequest.WorldLocation.X, NewRequest.WorldLocation.Y,
		NewRequest.WorldLocation.Z, EncodedDamage));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(
		NiagaraComponent, Style->NiagaraArrayName, DamageList);
}

void UShootNumberPopComponent_NiagaraText::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(NiagaraComponent))
	{
		AActor* NiagaraActor = NiagaraComponent->GetOwner();
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* LocalController = It->Get(); LocalController && LocalController->IsLocalController())
			{
				LocalController->HiddenActors.Remove(NiagaraActor);
			}
		}

		NiagaraActor->Destroy();
		NiagaraComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
