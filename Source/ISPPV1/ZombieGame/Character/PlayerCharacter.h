// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZombieGame/Character/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UCombatComponent;
struct FInputActionValue;

/**
 * The player-controlled character in the Zombie Game.
 * 
 * Specializations over ABaseCharacter:
 * - Over-the-shoulder TPS camera boom (SpringArm) & FollowCamera.
 * - Modular combat component (UCombatComponent) managing equipped weapons and ballistics.
 * - Enhanced Input bindings for movement, jumping, mouse look, shooting, aiming, and reloading.
 */
UCLASS()
class ISPPV1_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Registers the default Input Mapping Context to the local player subsystem. */
	void RegisterInputMappingContext();

	// ----------------------------------------------------------------------------------
	// Camera Components
	// ----------------------------------------------------------------------------------

	/** Spring arm component positioning the camera tight over the right shoulder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera for the over-the-shoulder perspective. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// ----------------------------------------------------------------------------------
	// Combat Subsystem
	// ----------------------------------------------------------------------------------

	/** Modular combat component orchestrating equipped weapons, firing, and ADS zoom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCombatComponent> CombatComponent;

	// ----------------------------------------------------------------------------------
	// AI Perception Subsystem
	// ----------------------------------------------------------------------------------

	/** Stimuli source component registering player for AI sight and damage detection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

	// ----------------------------------------------------------------------------------
	// Enhanced Input
	// ----------------------------------------------------------------------------------

	/** Default mapping context activating mouse look, movement, and firing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Primary fire input action (Full-auto or semi-auto trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	/** Reload input action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> ReloadAction;

	/** Aim Down Sights (ADS) input action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> AimAction;

	// ----------------------------------------------------------------------------------
	// Input Handlers
	// ----------------------------------------------------------------------------------

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartFire();
	void StopFire();
	void ReloadWeapon();
	void StartAiming();
	void StopAiming();

public:
	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }
};
