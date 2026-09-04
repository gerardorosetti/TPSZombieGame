// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawnPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;

/**
 * Designated spawn point actor for zombie enemy generation.
 * 
 * Pedagogical Architecture:
 * - Level designers place these in rooms, windows, or behind barriers.
 * - Supports Zone tagging to enable spawners only when specific doors/areas are unlocked.
 * - Validates NavMesh projection to ensure spawned zombies are placed safely on walkable surfaces.
 */
UCLASS()
class ISPPV1_API AZombieSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AZombieSpawnPoint();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBillboardComponent> EditorBillboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> DirectionArrow;
#endif

	/** If false, the wave manager will not select this spawn point (e.g. area locked by door). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn")
	bool bIsActive = true;

	/** Optional zone name identifier (e.g. "StartingRoom", "Courtyard") unlocked by doors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn")
	FName ZoneName = NAME_None;

	/** Radius in cm around this actor to project onto the NavMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Spawn", meta=(ClampMin="50.0", ClampMax="500.0"))
	float SpawnRadius = 100.0f;

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
};
