// Copyright ZhaoYiJie

#include "AI/Zombie/ShootMeleeEngagementSubsystem.h"

#include "GameFramework/Actor.h"

void UShootMeleeEngagementSubsystem::RemoveInvalidReservations()
{
	for (auto It = Reservations.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		TArray<FReservation>& TargetReservations = It.Value();
		TargetReservations.RemoveAll([](const FReservation& Reservation)
		{
			return !Reservation.Attacker.IsValid() || Reservation.SlotIndex == INDEX_NONE;
		});
		if (TargetReservations.IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
}

int32 UShootMeleeEngagementSubsystem::AcquireSlot(
	AActor* Target, AActor* Attacker, const int32 SlotCount, const FVector& PreferredDirection)
{
	if (!IsValid(Target) || !IsValid(Attacker) || Target == Attacker || SlotCount <= 0)
	{
		return INDEX_NONE;
	}

	RemoveInvalidReservations();

	const int32 SafeSlotCount = FMath::Max(SlotCount, 1);
	const TWeakObjectPtr<AActor> TargetKey(Target);
	TArray<FReservation>& TargetReservations = Reservations.FindOrAdd(TargetKey);
	for (const FReservation& Reservation : TargetReservations)
	{
		if (Reservation.Attacker.Get() == Attacker && Reservation.SlotIndex >= 0
			&& Reservation.SlotIndex < SafeSlotCount)
		{
			return Reservation.SlotIndex;
		}
	}

	TBitArray<> Occupied;
	Occupied.Init(false, SafeSlotCount);
	for (const FReservation& Reservation : TargetReservations)
	{
		if (Reservation.SlotIndex >= 0 && Reservation.SlotIndex < SafeSlotCount)
		{
			Occupied[Reservation.SlotIndex] = true;
		}
	}

	FVector DirectionToAttacker = PreferredDirection.GetSafeNormal2D();
	if (DirectionToAttacker.IsNearlyZero())
	{
		DirectionToAttacker = (Attacker->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	}

	int32 BestSlotIndex = INDEX_NONE;
	float BestScore = -TNumericLimits<float>::Max();
	for (int32 SlotIndex = 0; SlotIndex < SafeSlotCount; ++SlotIndex)
	{
		if (Occupied[SlotIndex])
		{
			continue;
		}

		const float Angle = FMath::DegreesToRadians(360.0f * static_cast<float>(SlotIndex)
			/ static_cast<float>(SafeSlotCount));
		const FVector SlotDirection(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		const float Score = FVector::DotProduct(SlotDirection, DirectionToAttacker);
		if (BestSlotIndex == INDEX_NONE || Score > BestScore)
		{
			BestSlotIndex = SlotIndex;
			BestScore = Score;
		}
	}

	if (BestSlotIndex != INDEX_NONE)
	{
		FReservation& NewReservation = TargetReservations.AddDefaulted_GetRef();
		NewReservation.Attacker = Attacker;
		NewReservation.SlotIndex = BestSlotIndex;
	}

	return BestSlotIndex;
}

void UShootMeleeEngagementSubsystem::ReleaseSlot(AActor* Target, AActor* Attacker)
{
	if (!Target || !Attacker)
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(Target);
	if (TArray<FReservation>* TargetReservations = Reservations.Find(TargetKey))
	{
		TargetReservations->RemoveAll([Attacker](const FReservation& Reservation)
		{
			return Reservation.Attacker.Get() == Attacker;
		});
		if (TargetReservations->IsEmpty())
		{
			Reservations.Remove(TargetKey);
		}
	}
}

void UShootMeleeEngagementSubsystem::ReleaseAllForAttacker(AActor* Attacker)
{
	if (!Attacker)
	{
		return;
	}

	for (auto It = Reservations.CreateIterator(); It; ++It)
	{
		It.Value().RemoveAll([Attacker](const FReservation& Reservation)
		{
			return Reservation.Attacker.Get() == Attacker;
		});
		if (It.Value().IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
}

bool UShootMeleeEngagementSubsystem::HasSlot(AActor* Target, AActor* Attacker) const
{
	if (!Target || !Attacker)
	{
		return false;
	}

	const TArray<FReservation>* TargetReservations = Reservations.Find(TWeakObjectPtr<AActor>(Target));
	if (!TargetReservations)
	{
		return false;
	}

	for (const FReservation& Reservation : *TargetReservations)
	{
		if (Reservation.Attacker.Get() == Attacker && Reservation.SlotIndex != INDEX_NONE)
		{
			return true;
		}
	}
	return false;
}

FVector UShootMeleeEngagementSubsystem::GetSlotOffset(
	AActor* Target, const int32 SlotIndex, const int32 SlotCount, const float Radius) const
{
	if (!Target || SlotIndex < 0 || SlotCount <= 0 || Radius <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	const float Angle = FMath::DegreesToRadians(360.0f * static_cast<float>(SlotIndex)
		/ static_cast<float>(SlotCount));
	return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;
}

void UShootMeleeEngagementSubsystem::Deinitialize()
{
	Reservations.Empty();
	Super::Deinitialize();
}
