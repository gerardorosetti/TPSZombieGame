// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawnPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;

/**
 * Designated spawn point actor for enemy generation, with zone tagging and NavMesh projection validation.
 */
UCLASS()
class ISPPV1_API AZombieSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AZombieSpawnPoint();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBillboardComponent> EditorBillboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> DirectionArrow;
#endif

	/** If true, this spawner begins dormant (inactive) until its Zone is unlocked by an ObstacleDoor.
	 * If false and ZoneName is None, it is active immediately from Round 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn", meta=(DisplayName="Is Dormant"))
	bool bIsDormant = false;

	/** Optional zone name identifier (e.g. "StartingRoom", "GreenBlock") unlocked by doors.
	 * Leave None or empty for the starting room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn")
	FName ZoneName = NAME_None;

	/** Radius in cm around this actor to project onto the NavMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float SpawnRadius = 200.0f;

	/** Runtime active state. True when WaveManager can spawn zombies from this location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Zombie|Spawn")
	bool bIsActive = true;

public:
	/**
	 * Queries NavMesh for a valid walkable spawn location near this actor.
	 * @param OutLocation Projected NavMesh coordinate.
	 * @return True if a valid walkable point was found, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Zombie|Spawn")
	bool GetValidSpawnLocation(FVector& OutLocation) const;

	/** Enables or disables this spawn point dynamically (e.g. when an obstacle door opens). */
	UFUNCTION(BlueprintCallable, Category="Zombie|Spawn")
	void SetSpawnPointActive(bool bNewActive) { bIsActive = bNewActive; }

	UFUNCTION(BlueprintPure, Category="Zombie|Spawn")
	bool IsSpawnPointActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category="Zombie|Spawn")
	FName GetZoneName() const { return ZoneName; }

	UFUNCTION(BlueprintPure, Category="Zombie|Spawn")
	bool MatchesZone(const FName& InZone) const;
};
