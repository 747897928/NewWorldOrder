// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "CommonActionWidget.h"
#include "LyraActionWidget.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;

/** An action widget that will get the icon of key that is currently assigned to the common input action on this widget */
UCLASS(BlueprintType, Blueprintable)
class NEWWORLDORDER_API ULyraActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:

	//~ Begin UCommonActionWidget interface
	virtual FSlateBrush GetIcon() const override;
	//~ End of UCommonActionWidget interface

	/** 运行时技能槽按 Experience 配置切换 IA 后，立即刷新 CommonUI 的当前设备按键图标。 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void SetAssociatedInputAction(UInputAction* InInputAction);

	/** The Enhanced Input Action that is associated with this Common Input action. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TObjectPtr<UInputAction> AssociatedInputAction;

private:

	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
};
