// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerUpBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class APlayerCharacter;

/**
 * Enumeration of active survival power-up archetypes.
 */
UENUM(BlueprintType)
enum class EPowerUpType : uint8
{
	MaxAmmo      UMETA(DisplayName="Max Ammo"),
	InstaKill    UMETA(DisplayName="Insta-Kill"),
	DoublePoints UMETA(DisplayName="Double Points"),
	Nuke         UMETA(DisplayName="Nuke")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPowerUpCollectedSignature,
	EPowerUpType, PowerUpType,
	APlayerCharacter*, Collector
);

/**
 * Interactive 3D pickup actor applying temporary buffs or tactical bonuses to the player.
 */
UCLASS()
class ISPPV1_API APowerUpBase : public AActor
{
	GENERATED_BODY()

public:
	APowerUpBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ----------------------------------------------------------------------------------
	// Components
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Overlap trigger detecting player collection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** 3D mesh representation (e.g. ammo box, skull, medallion). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	// ----------------------------------------------------------------------------------
	// Configuration
	// ----------------------------------------------------------------------------------

	/** Power-up type governing the gameplay effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Config")
	EPowerUpType PowerUpType = EPowerUpType::MaxAmmo;

	/** Duration in seconds this pickup remains in the world before expiring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Config", meta=(ClampMin="5.0"))
	float TotalLifetime = 30.0f;

	/** Seconds remaining when blinking begins to warn players of expiration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Config", meta=(ClampMin="1.0"))
	float FlashWarningTime = 5.0f;

	/** Continuous rotation rate around Z axis in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Motion")
	float RotationSpeed = 90.0f;

	/** Oscillation frequency for vertical bobbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Motion")
	float BobbingSpeed = 3.0f;

	/** Oscillation distance in cm for vertical bobbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PowerUp|Motion")
	float BobbingAmplitude = 12.0f;

	// ----------------------------------------------------------------------------------
	// Audio & Spatial Sound
	// ----------------------------------------------------------------------------------

	/** Spatial attenuation asset for 3D spawn audio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PowerUp|Audio")
	TObjectPtr<class USoundAttenuation> SpatialAttenuation;

	/** Sound played when the power-up spawns into the world. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PowerUp|Audio")
	TObjectPtr<class USoundBase> SpawnSound;

	/** Sound / Voice line played when the power-up is picked up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PowerUp|Audio")
	TObjectPtr<class USoundBase> CollectSound;

	// ----------------------------------------------------------------------------------
	// Observer Events
	// ----------------------------------------------------------------------------------

	/** Broadcast when a player collects this power-up. */
	UPROPERTY(BlueprintAssignable, Category="PowerUp|Events")
	FOnPowerUpCollectedSignature OnPowerUpCollected;

public:
	/** Applies the power-up effect to the collecting player or match state. */
	virtual void ApplyPowerUpEffect(APlayerCharacter* Player);

protected:
	/** Internal overlap event handler. */
	UFUNCTION()
	virtual void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/** Seconds remaining before this power-up despawns. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="PowerUp|Runtime")
	float RemainingLifetime = 30.0f;

	/** Accumulated running time used for smooth sine-wave bobbing. */
	float RunningTime = 0.0f;

	/** Baseline relative location of the mesh. */
	FVector BaseMeshRelativeLocation = FVector::ZeroVector;

	/** Prevents double collection. */
	bool bIsCollected = false;

public:
	UFUNCTION(BlueprintPure, Category="PowerUp|Queries")
	FORCEINLINE EPowerUpType GetPowerUpType() const { return PowerUpType; }

	UFUNCTION(BlueprintPure, Category="PowerUp|Queries")
	FORCEINLINE float GetRemainingLifetime() const { return RemainingLifetime; }
};
