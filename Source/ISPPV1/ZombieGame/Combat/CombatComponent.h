// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AWeaponBase;
class ACharacter;
class UCameraComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimStateChanged, bool, bIsAiming);

/**
 * Combat and weapon management component attached to the player character.
 * 
 * Responsibilities:
 * - Spawns, equips, and holds weapon actors in character sockets.
 * - Routes player input commands (StartFire, StopFire, Reload, Aim) to the active weapon.
 * - Manages smooth Aim Down Sights (ADS) camera transitions (FOV and arm length).
 * - Broadcasts combat state changes via Observer delegates.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class ISPPV1_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ----------------------------------------------------------------------------------
	// Weapon Management
	// ----------------------------------------------------------------------------------

	/** Weapon class to automatically spawn and equip upon spawning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Weapons")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	/** Active equipped weapon instance. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|Weapons")
	TObjectPtr<AWeaponBase> CurrentWeapon;

	/** Character skeletal mesh socket where the primary weapon attaches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Sockets")
	FName WeaponAttachSocket = TEXT("hand_r");

	// ----------------------------------------------------------------------------------
	// Aim Down Sights (ADS) Settings
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|Aim")
	bool bIsAiming = false;

	/** Interpolation speed for smooth aiming zoom transitions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Aim", meta=(ClampMin="1.0"))
	float AimInterpSpeed = 15.0f;

	/** Default camera FOV when hip firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Aim")
	float DefaultFOV = 85.0f;

	/** Camera arm length when hip firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Aim")
	float DefaultArmLength = 220.0f;

	/** Camera arm length when fully aimed in (tighter over-the-shoulder framing). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Aim")
	float AimArmLength = 160.0f;

	// ----------------------------------------------------------------------------------
	// Observer Delegates
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category="Combat|Events")
	FOnAimStateChanged OnAimStateChanged;

	// ----------------------------------------------------------------------------------
	// Input Routing & Combat Actions
	// ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category="Combat|Actions")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Combat|Actions")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Combat|Actions")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="Combat|Actions")
	void SetAiming(bool bNewAiming);

	/** Spawns and equips a new weapon, replacing the current one if present. */
	UFUNCTION(BlueprintCallable, Category="Combat|Weapons")
	void EquipWeapon(TSubclassOf<AWeaponBase> NewWeaponClass);

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Combat|State")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Combat|Weapons")
	FORCEINLINE AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

private:
	/** Cached reference to character's camera and spring arm for smooth FOV interpolation. */
	TWeakObjectPtr<UCameraComponent> CachedCamera;
	TWeakObjectPtr<USpringArmComponent> CachedSpringArm;

	void CacheCameraReferences();
};
