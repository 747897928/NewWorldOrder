// Copyright ZhaoYiJie

#pragma once

#include "Feedback/NumberPops/ShootNumberPopComponent.h"

#include "ShootNumberPopComponent_NiagaraText.generated.h"

class UNiagaraComponent;
class UShootDamagePopStyleNiagara;

/**
 * Lyra Niagara 数字方案的项目层实现。
 * 每个本地 PlayerController 只创建一个长期 NiagaraActor/Component，命中时仅追加数组数据；
 * 独立 Actor 让分屏视口可以通过 HiddenActors 只隐藏其他玩家的数字，而不隐藏角色本体。
 */
UCLASS(Blueprintable, ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootNumberPopComponent_NiagaraText : public UShootNumberPopComponent
{
	GENERATED_BODY()

public:
	virtual void AddNumberPop(const FShootNumberPopRequest& NewRequest) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** BP_ShootPlayerController 的继承组件必须配置项目 DataAsset。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Number Pop|Style")
	TObjectPtr<UShootDamagePopStyleNiagara> Style;

private:
	/** 运行时为当前 LocalPlayer 懒创建一个专属 NiagaraActor；同一玩家的命中持续复用。 */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
};
