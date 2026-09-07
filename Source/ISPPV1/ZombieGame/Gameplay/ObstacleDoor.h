// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieGame/Interfaces/InteractableInterface.h"
#include "ObstacleDoor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorOpenedSignature, AObstacleDoor*, OpenedDoor);

/**
 * World interactable barrier (debris, gates, barricades) blocking player and AI navigation until purchased with points.
 */
UCLASS()
class ISPPV1_API AObstacleDoor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AObstacleDoor();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------------------------------------
	// Components
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Visual representation of the physical barrier (debris pile, security door, wooden barricade). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Trigger volume detecting player proximity for interaction traces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> InteractionTrigger;

	// ----------------------------------------------------------------------------------
	// Configuration
	// ----------------------------------------------------------------------------------

	/** Point cost required to clear/open this obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Config", meta=(ClampMin="0"))
	int32 DoorCost = 750;

	/** Localized text displayed on HUD prompt (e.g. "Clear Debris", "Open Security Door"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Config")
	FText PromptText;

	/** Name of the spawner zone to activate once this door is unlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Config")
	FName ZoneToUnlock = NAME_None;

	// ----------------------------------------------------------------------------------
	// Audio & Spatial Sound (3D Attenuation)
	// ----------------------------------------------------------------------------------

	/** Spatial attenuation asset for 3D door audio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Obstacle|Audio")
	TObjectPtr<class USoundAttenuation> SpatialAttenuation;

	/** Sound played when the door/obstacle is opened. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Obstacle|Audio")
	TObjectPtr<class USoundBase> DoorOpenSound;

	/** True if this barrier has already been purchased and unlocked. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Obstacle|State")
	bool bIsOpened = false;

public:
	// ----------------------------------------------------------------------------------
	// Observer Events
	// ----------------------------------------------------------------------------------

	/** Broadcast when the door is purchased and successfully opened. */
	UPROPERTY(BlueprintAssignable, Category="Obstacle|Events")
	FOnDoorOpenedSignature OnDoorOpened;

	// ----------------------------------------------------------------------------------
	// IInteractableInterface Implementation
	// ----------------------------------------------------------------------------------

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual int32 GetInteractionCost_Implementation() const override;
	virtual bool CanInteract_Implementation(const APlayerCharacter* InstigatorPlayer) const override;
	virtual bool Interact_Implementation(APlayerCharacter* InstigatorPlayer) override;

	// ----------------------------------------------------------------------------------
	// Operations & Queries
	// ----------------------------------------------------------------------------------

	/** Executes the opening sequence: disables collisions, hides mesh, and wakes spawner zone. */
	UFUNCTION(BlueprintCallable, Category="Obstacle|Operations")
	void OpenDoor(APlayerCharacter* InstigatorPlayer = nullptr);

	UFUNCTION(BlueprintPure, Category="Obstacle|Queries")
	FORCEINLINE bool IsOpened() const { return bIsOpened; }

	UFUNCTION(BlueprintPure, Category="Obstacle|Queries")
	FORCEINLINE FName GetZoneToUnlock() const { return ZoneToUnlock; }
};
