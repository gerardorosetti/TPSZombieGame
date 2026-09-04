// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZombieGame/Character/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/**
 * The player-controlled character in the Zombie Game.
 * 
 * Specializations over ABaseCharacter:
 * - TPS camera boom (SpringArm) positioned over the right shoulder.
 * - Enhanced Input bindings for fluid movement, jumping, and weapon testing.
 * - Test hitscan raycast method to verify damage and headshot detection on targets in real time.
 */
UCLASS()
class ISPPV1_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ----------------------------------------------------------------------------------
	// Camera Components
	// ----------------------------------------------------------------------------------

	/** Spring arm component positioning the camera over the shoulder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera for the third person perspective. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	UCameraComponent* FollowCamera;

	// ----------------------------------------------------------------------------------
	// Enhanced Input Actions
	// ----------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* JumpAction;

	/** Primary fire / test attack input action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* FireAction;

	// ----------------------------------------------------------------------------------
	// Input Handlers
	// ----------------------------------------------------------------------------------

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

public:
	/**
	 * Performs a hitscan line trace from the camera forward.
	 * If it strikes an actor implementing IZombieDamageableInterface, damage is applied.
	 * Can be bound to input or called from Blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category="Combat|Test")
	void FireTestHitscan();

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
