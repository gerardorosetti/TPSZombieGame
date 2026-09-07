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
class UInteractionComponent;
class UCombatHUDWidget;
class USoundBase;
class UAudioComponent;
class UCameraShakeBase;
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

	/** Restores GameOnly input mode, reenables pawn input, and hides mouse cursor. */
	UFUNCTION(BlueprintCallable, Category="Zombie|Input")
	void RestorePlayerControl();

	virtual void PossessedBy(AController* NewController) override;

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
	// Interaction Subsystem
	// ----------------------------------------------------------------------------------

	/** Interaction component performing forward line-of-sight raycasts for world interactables. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInteractionComponent> InteractionComponent;

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

	/** World interaction input action (Key [E]). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Combat HUD widget class to instantiate. Can be assigned in BP_PlayerCharacter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="UI")
	TSubclassOf<UCombatHUDWidget> HUDWidgetClass;

	// ----------------------------------------------------------------------------------
	// Feedback, Effects & Audio
	// ----------------------------------------------------------------------------------

	/** Camera shake triggered when player takes damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Effects")
	TSubclassOf<UCameraShakeBase> DamageCameraShakeClass;

	/** Sound played when player takes damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Audio")
	TObjectPtr<USoundBase> HurtSound;

	/** Sound played when player is eliminated. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Audio")
	TObjectPtr<USoundBase> DeathSound;

	// ----------------------------------------------------------------------------------
	// Low Health Audio System
	// ----------------------------------------------------------------------------------

	/** Health ratio threshold (0.0 - 1.0) below which the character enters critical low-health danger state. Defaults to 0.40 (40%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Audio", meta=(ClampMin="0.1", ClampMax="0.9"))
	float LowHealthThreshold = 0.40f;

	/** Stinger sound played once when health drops into the critical red danger threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Audio")
	TObjectPtr<USoundBase> LowHealthEnterSound;

	/** Looping audio (heartbeat, heavy breathing) played continuously while health remains in danger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Audio")
	TObjectPtr<USoundBase> LowHealthLoopSound;

	/** Sound played when health recovers and exits the danger threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Audio")
	TObjectPtr<USoundBase> LowHealthExitSound;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|Audio")
	bool bIsLowHealth = false;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LowHealthAudioComponent;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData) override;
	virtual void OnDeathStarted(AActor* Killer) override;

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
	void Interact();

public:
	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	FORCEINLINE TSubclassOf<UCombatHUDWidget> GetHUDWidgetClass() const { return HUDWidgetClass; }
};
