// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "TimerManager.h"
#include "Engine/World.h"

APlayerStateBase::APlayerStateBase()
{
	CurrentPoints = 500;
	TotalScoreEarned = 500;
	TotalKills = 0;
	TotalHeadshots = 0;
	RoundsSurvived = 0;
	bIsDoublePointsActive = false;
}

void APlayerStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoublePointsTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APlayerStateBase::AddPoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	if (bIsDoublePointsActive)
	{
		Amount *= 2;
	}

	CurrentPoints += Amount;
	TotalScoreEarned += Amount;

	OnPointsChanged.Broadcast(CurrentPoints, Amount);
	ZOMBIE_LOG(Verbose, TEXT("[%s] +%d Points (Total: %d, DoublePoints=%s)"),
		*GetName(), Amount, CurrentPoints, bIsDoublePointsActive ? TEXT("TRUE") : TEXT("FALSE"));
}

void APlayerStateBase::ActivateDoublePoints(float Duration)
{
	bIsDoublePointsActive = true;
	DoublePointsDuration = Duration;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoublePointsTimerHandle);
		World->GetTimerManager().SetTimer(
			DoublePointsTimerHandle,
			this,
			&APlayerStateBase::DeactivateDoublePoints,
			Duration,
			false
		);
	}

	OnDoublePointsStateChanged.Broadcast(true, Duration);
	ZOMBIE_LOG(Log, TEXT("[%s] Double Points activated/reset for %f seconds!"), *GetName(), Duration);
}

void APlayerStateBase::DeactivateDoublePoints()
{
	bIsDoublePointsActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoublePointsTimerHandle);
	}

	OnDoublePointsStateChanged.Broadcast(false, 0.0f);
	ZOMBIE_LOG(Log, TEXT("[%s] Double Points expired."), *GetName());
}

float APlayerStateBase::GetDoublePointsTimeRemaining() const
{
	if (!bIsDoublePointsActive)
	{
		return 0.0f;
	}

	if (const UWorld* World = GetWorld())
	{
		return World->GetTimerManager().GetTimerRemaining(DoublePointsTimerHandle);
	}

	return 0.0f;
}

bool APlayerStateBase::SpendPoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}

	if (CurrentPoints < Amount)
	{
		ZOMBIE_LOG(Log, TEXT("[%s] Insufficient points to spend %d (Have: %d)"), *GetName(), Amount, CurrentPoints);
		return false;
	}

	CurrentPoints -= Amount;
	OnPointsChanged.Broadcast(CurrentPoints, -Amount);
	ZOMBIE_LOG(Log, TEXT("[%s] Spent %d points (Remaining: %d)"), *GetName(), Amount, CurrentPoints);
	return true;
}

void APlayerStateBase::RecordKill(bool bIsHeadshot)
{
	TotalKills++;
	if (bIsHeadshot)
	{
		TotalHeadshots++;
	}
}
