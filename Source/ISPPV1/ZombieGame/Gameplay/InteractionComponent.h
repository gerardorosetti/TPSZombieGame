// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZombieGame/Interfaces/InteractableInterface.h"
#include "InteractionComponent.generated.h"

class APlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnInteractableFoundSignature,
	TScriptInterface<IInteractableInterface>, Interactable,
	const FText&, Prompt,
	int32, Cost
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractableLostSignature);

/**
 * Player component managing line-of-sight traces and input execution for world interactables.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class ISPPV1_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ----------------------------------------------------------------------------------
	// Configuration
	// ----------------------------------------------------------------------------------

	/** Maximum distance in cm within which the player can detect and trigger interactables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float InteractionDistance = 250.0f;

	/** Radius in cm for sphere sweep detection (makes targeting small interactables much more forgiving). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0", ClampMax="50.0"))
	float InteractionTraceRadius = 15.0f;

	/** Collision channel queried for interactive actors. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction")
	TEnumAsByte<ECollisionChannel> InteractionChannel = ECC_Visibility;

	// ----------------------------------------------------------------------------------
	// Event Delegates (Observer Pattern)
	// ----------------------------------------------------------------------------------

	/** Broadcast when the player looks at a valid interactable within range. */
	UPROPERTY(BlueprintAssignable, Category="Interaction|Events")
	FOnInteractableFoundSignature OnInteractableFound;

	/** Broadcast when the player looks away or moves out of range of an interactable. */
	UPROPERTY(BlueprintAssignable, Category="Interaction|Events")
	FOnInteractableLostSignature OnInteractableLost;

	// ----------------------------------------------------------------------------------
	// Core Operations
	// ----------------------------------------------------------------------------------

	/**
	 * Executes interaction on the currently focused interactable if eligible.
	 * @return True if interaction succeeded, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool TryInteract();

	/** Returns the currently focused interactable, or nullptr if none. */
	UFUNCTION(BlueprintPure, Category="Interaction")
	TScriptInterface<IInteractableInterface> GetCurrentInteractable() const { return CurrentInteractable; }

	/** Returns true if the player is currently focusing on a valid interactable. */
	UFUNCTION(BlueprintPure, Category="Interaction")
	bool HasFocusedInteractable() const { return CurrentInteractable.GetObject() != nullptr; }

protected:
	/** Performs the forward spatial sweep and updates the focused interactable. */
	void PerformInteractionTrace();

	/** Currently focused interactive actor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Interaction|Runtime")
	TScriptInterface<IInteractableInterface> CurrentInteractable;

	/** Cached owning character. */
	TWeakObjectPtr<APlayerCharacter> OwningPlayer;
};
