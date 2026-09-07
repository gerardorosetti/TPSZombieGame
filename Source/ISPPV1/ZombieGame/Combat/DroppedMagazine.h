// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroppedMagazine.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Physical discarded magazine spawned during weapon reload.
 * Simulates rigid body physics and world collision.
 */
UCLASS()
class ISPPV1_API ADroppedMagazine : public AActor
{
	GENERATED_BODY()

public:
	ADroppedMagazine();

	/** Initializes the mesh and applies initial toss velocity. */
	UFUNCTION(BlueprintCallable, Category="Combat|Visuals")
	void InitializeDroppedMagazine(UStaticMesh* InMesh, const FVector& InitialImpulse);

protected:
	/** Box root component simulating physics and collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** Visual representation of the discarded magazine attached to the physics box. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	/** Duration in seconds before the physical magazine is cleaned up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Config")
	float DespawnLifespan = 8.0f;
};
