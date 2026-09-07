// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class APlayerCharacter;

/**
 * UInterface declaration required by Unreal Header Tool for reflection and Blueprint support.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface implemented by interactive world actors (Doors, Wall Buys, Switches).
 */
class ISPPV1_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Returns localized display prompt shown on the player's HUD (e.g. "Clear Debris", "Buy Ammo").
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt() const;

	/**
	 * Returns the cost in currency/points required to interact (0 for free interactables).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	int32 GetInteractionCost() const;

	/**
	 * Validates whether the instigating player is eligible to perform the interaction.
	 * (e.g., has enough points, door is not already opened, weapon can receive ammo).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(const APlayerCharacter* InstigatorPlayer) const;

	/**
	 * Executes the interaction logic (deducts points, opens barrier, grants ammo, etc.).
	 * @param InstigatorPlayer The player pawn performing the interaction.
	 * @return True if the interaction was successfully executed, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool Interact(APlayerCharacter* InstigatorPlayer);
};
