// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerStateBase.generated.h"

/**
 * Multicast delegate notifying listeners when player points change.
 * Parameters: New total points, Delta points (positive on gain, negative on spend).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPointsChangedSignature, int32, NewPoints, int32, DeltaPoints);

/**
 * Player State managing economy (Points), combat statistics (Kills, Headshots),
 * and survival scoring for the player.
 * 
 * Pedagogical Rationale:
 * - Extends Unreal Engine's native APlayerState to hold persistent match statistics.
 * - Adheres to generic player domain naming (no redundant Zombie prefix on player subsystems).
 * - Implements the Observer pattern via FOnPointsChangedSignature for decoupled HUD integration.
 */
UCLASS()
class ISPPV1_API APlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	APlayerStateBase();

protected:
	/** Current spendable points (currency for doors, wall buys, power-ups). Starting points: 500. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Economy")
	int32 CurrentPoints = 500;

	/** Cumulative score earned during the match (never decreases on purchase). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	int32 TotalScoreEarned = 0;

	/** Total enemies killed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	int32 TotalKills = 0;

	/** Total headshot kills. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	int32 TotalHeadshots = 0;

	/** Total rounds survived. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	int32 RoundsSurvived = 0;

public:
	/** Broadcast whenever points are gained or spent. */
	UPROPERTY(BlueprintAssignable, Category="Economy")
	FOnPointsChangedSignature OnPointsChanged;

	/** Adds points to the player's balance and increases total score. */
	UFUNCTION(BlueprintCallable, Category="Economy")
	void AddPoints(int32 Amount);

	/**
	 * Deducts points if player has sufficient funds.
	 * @param Amount Cost in points.
	 * @return True if transaction succeeded, false if insufficient points.
	 */
	UFUNCTION(BlueprintCallable, Category="Economy")
	bool SpendPoints(int32 Amount);

	/** Records a confirmed enemy kill. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void RecordKill(bool bIsHeadshot);

	/** Sets the number of rounds survived. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetRoundsSurvived(int32 Rounds) { RoundsSurvived = Rounds; }

	// Getters
	UFUNCTION(BlueprintPure, Category="Economy")
	int32 GetCurrentPoints() const { return CurrentPoints; }

	UFUNCTION(BlueprintPure, Category="Stats")
	int32 GetTotalScore() const { return TotalScoreEarned; }

	UFUNCTION(BlueprintPure, Category="Stats")
	int32 GetTotalKills() const { return TotalKills; }

	UFUNCTION(BlueprintPure, Category="Stats")
	int32 GetTotalHeadshots() const { return TotalHeadshots; }

	UFUNCTION(BlueprintPure, Category="Stats")
	int32 GetRoundsSurvived() const { return RoundsSurvived; }
};
