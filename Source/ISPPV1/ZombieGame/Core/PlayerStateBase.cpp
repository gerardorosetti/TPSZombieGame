// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/PlayerStateBase.h"

APlayerStateBase::APlayerStateBase()
{
	CurrentPoints = 500;
	TotalScoreEarned = 500;
	TotalKills = 0;
	TotalHeadshots = 0;
	RoundsSurvived = 0;
}

void APlayerStateBase::AddPoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentPoints += Amount;
	TotalScoreEarned += Amount;

	OnPointsChanged.Broadcast(CurrentPoints, Amount);
	UE_LOG(LogTemp, Verbose, TEXT("[%s] +%d Points (Total: %d)"), *GetName(), Amount, CurrentPoints);
}

bool APlayerStateBase::SpendPoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}

	if (CurrentPoints < Amount)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Insufficient points to spend %d (Have: %d)"), *GetName(), Amount, CurrentPoints);
		return false;
	}

	CurrentPoints -= Amount;
	OnPointsChanged.Broadcast(CurrentPoints, -Amount);
	UE_LOG(LogTemp, Log, TEXT("[%s] Spent %d points (Remaining: %d)"), *GetName(), Amount, CurrentPoints);
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
