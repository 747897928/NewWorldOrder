#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "ShootLocalInteractionPrompt.generated.h"

class ULocalPlayer;

/** 蓝图中本组件是可调样式模板；运行时为每个 LocalPlayer 创建独立提示。 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootLocalInteractionPrompt : public UWidgetComponent
{
	GENERATED_BODY()
public:
	UShootLocalInteractionPrompt();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin="0"))
	float DisplayDistance = 160.0f;
private:
	void RefreshLocalPrompts();
	FTimerHandle RefreshTimer;
	UPROPERTY(Transient)
	TMap<TObjectPtr<ULocalPlayer>, TObjectPtr<UWidgetComponent>> LocalPrompts;
};
