// QuickbarMessageTypes.h
#pragma once
#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Styling/SlateBrush.h"
#include "QuickbarMessageTypes.generated.h"

USTRUCT(BlueprintType)
struct FQuickbarSlotData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid ItemInstanceId;
    UPROPERTY(BlueprintReadOnly) EShootItemLifetime Lifetime = EShootItemLifetime::Persistent;
    UPROPERTY(BlueprintReadOnly) TSubclassOf<UShootInventoryItemDefinition> ItemDefinition;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) FName WeaponId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;
    UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
    UPROPERTY(BlueprintReadOnly) FSlateBrush Icon;
    UPROPERTY(BlueprintReadOnly) FSlateBrush AmmoIcon;
};

USTRUCT(BlueprintType)
struct FQuickbarSlotsChangedMessage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Owner;
    UPROPERTY(BlueprintReadOnly) TArray<FQuickbarSlotData> Slots;
};

USTRUCT(BlueprintType)
struct FQuickbarActiveIndexChangedMessage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Owner;
    UPROPERTY(BlueprintReadOnly) int32 ActiveIndex = -1;
};

USTRUCT(BlueprintType)
struct FWeaponAmmoChangedMessage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Owner;
    UPROPERTY(BlueprintReadOnly) FName WeaponId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;
    UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
};
