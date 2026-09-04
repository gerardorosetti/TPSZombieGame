// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

class APlayerCharacter;
class UCombatComponent;
class UCharacterMovementComponent;
class AWeaponBase;

/**
 * C++ base class for the player character's Animation Blueprint.
 * 
 * Responsibilities:
 * - Extracts and caches gameplay state (Speed, Falling, Aiming, Aim Pitch/Yaw) during NativeUpdateAnimation.
 * - Exposes clean, thread-safe properties to the AnimGraph without requiring messy blueprint casts.
 * - Powers Aim Offsets (AO_Rifle) and Layered Blend Per Bone transitions.
 */
UCLASS()
class ISPPV1_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// ----------------------------------------------------------------------------------
	// Cached References
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category="Character")
	TObjectPtr<APlayerCharacter> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category="Character")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category="Character")
	TObjectPtr<UCombatComponent> CombatComponent;

	// ----------------------------------------------------------------------------------
	// Locomotion & State Variables (Consumed by AnimGraph)
	// ----------------------------------------------------------------------------------

	/** Current 2D ground speed of the character in cm/s. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Locomotion")
	float GroundSpeed = 0.0f;

	/** True if the character has significant velocity and input acceleration. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Locomotion")
	bool bShouldMove = false;

	/** True if the character is airborne/falling. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Locomotion")
	bool bIsFalling = false;

	/** True if the character is currently Aiming Down Sights (ADS). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat")
	bool bIsAiming = false;

	/** Normalized pitch angle (-90 to +90 degrees) for vertical Aim Offset blending. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat")
	float AimPitch = 0.0f;

	/** Normalized yaw angle (-90 to +90 degrees) for horizontal Aim Offset blending. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat")
	float AimYaw = 0.0f;

	/** Currently equipped weapon actor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat")
	TObjectPtr<AWeaponBase> CurrentWeapon;
};
