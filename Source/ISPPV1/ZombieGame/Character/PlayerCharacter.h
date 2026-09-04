// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZombieGame/Character/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * The player-controlled character in the Zombie Game.
 * 
 * Specializations over ABaseCharacter:
 * - Over-the-shoulder TPS camera boom (SpringArm) & FollowCamera.
 * - Enhanced Input bindings for movement, jumping, mouse look, and shooting.
 * - Prototype weapon mesh attached to hand_r socket.
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
	// Prototype Weapon Visual (Milestone 1)
	// ----------------------------------------------------------------------------------

	/** Prototype weapon mesh attached to the character's right hand. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

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

	/** Primary fire / attack input action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FireAction;

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
	FORCEINLINE UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
};
