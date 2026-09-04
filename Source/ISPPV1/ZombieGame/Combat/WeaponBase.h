// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieGame/Interfaces/ZombieDamageableInterface.h"
#include "WeaponBase.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class ACharacter;

/**
 * Firing mode of a weapon.
 */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	SemiAuto UMETA(DisplayName = "Semi-Automatic"),
	FullAuto UMETA(DisplayName = "Full-Automatic"),
	Burst    UMETA(DisplayName = "Burst")
};

/**
 * Configuration parameters defining weapon stats, ballistics, and timing.
 */
USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	/** Display name for UI and inventory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
	FString WeaponName = TEXT("Assault Rifle");

	/** Firing mode determining automatic repeat behavior. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Firing")
	EWeaponFireMode FireMode = EWeaponFireMode::FullAuto;

	/** Base hit damage before headshot/armor multipliers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Firing", meta=(ClampMin="1.0"))
	float BaseDamage = 28.0f;

	/** Rounds per minute (e.g. 600 RPM = 10 rounds/sec, 0.1s delay). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Firing", meta=(ClampMin="60.0", ClampMax="1500.0"))
	float FireRateRPM = 600.0f;

	/** Maximum ammunition held in a single magazine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="1"))
	int32 MagCapacity = 30;

	/** Maximum total spare ammunition carried in reserve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0"))
	int32 MaxReserveAmmo = 120;

	/** Duration in seconds required to complete a full reload. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0.1"))
	float ReloadDuration = 2.2f;

	/** Radius of the ballistic SphereTrace. Prevents zero-thickness ray tunneling through polygon seams. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ballistics", meta=(ClampMin="0.5", ClampMax="20.0"))
	float BulletRadius = 2.5f;

	/** Maximum effective range of the ballistic trace in centimeters (10000 = 100 meters). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ballistics", meta=(ClampMin="500.0"))
	float MaxRange = 10000.0f;

	/** Camera field of view when Aiming Down Sights (ADS) with this weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aiming", meta=(ClampMin="30.0", ClampMax="110.0"))
	float AimFOV = 65.0f;

	/** Recoil impulse applied to controller pitch upon firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Firing", meta=(ClampMin="0.0"))
	float RecoilPitch = 0.35f;
};

// Observer Pattern Multicast Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentMag, int32, CurrentReserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStateChanged, bool, bIsReloading);

/**
 * Autonomous weapon actor in the modular combat architecture.
 * 
 * Responsibilities:
 * - Owns 3D visual representations (Skeletal or Static Mesh).
 * - Manages ammunition state and firing timers (Rate of Fire).
 * - Implements precision SphereTrace ballistics with TPS parallax compensation (Camera to Muzzle).
 * - Applies damage via IZombieDamageableInterface with headshot detection.
 * - Broadcasts state changes via Observer delegates for HUD/UI decoupling.
 */
UCLASS(Blueprintable, ClassGroup=(Combat))
class ISPPV1_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------------------------------------
	// Visual Components
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	/** Skeletal mesh for animated weapons (moving slide, bolt, magazine). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMesh;

	/** Static mesh for simple or prototype weapons. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponStaticMesh;

	// ----------------------------------------------------------------------------------
	// Weapon Configuration & State
	// ----------------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Config")
	FWeaponConfig WeaponConfig;

	/** Name of the socket where muzzle flash, tracers, and traces originate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Config")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	int32 CurrentMagAmmo;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	int32 CurrentReserveAmmo;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	bool bIsReloading = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	bool bIsFiring = false;

	/** Non-owning reference to the character holding this weapon. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	TWeakObjectPtr<ACharacter> OwningCharacter;

	// ----------------------------------------------------------------------------------
	// Animation Montages (Character Body Reactions)
	// ----------------------------------------------------------------------------------

	/** Anim montage to play on the character when this weapon fires (recoil kick). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<class UAnimMontage> FireMontage;

	/** Anim montage to play on the character when this weapon reloads. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TObjectPtr<class UAnimMontage> ReloadMontage;

	// Timer handles for firing cadence and reload
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

public:
	// ----------------------------------------------------------------------------------
	// Observer Delegates (Decoupled UI / Audio / VFX Binding)
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnWeaponAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnWeaponReloadStateChanged OnReloadStateChanged;

	// ----------------------------------------------------------------------------------
	// Combat Interface
	// ----------------------------------------------------------------------------------

	/** Begins firing sequence. Respects fire rate and automatic repetition. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void StartFire();

	/** Stops firing sequence. Clears automatic fire timers. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void StopFire();

	/** Executes a single shot: ballistic SphereTrace, damage delivery, and visual feedback. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void FireShot();

	/** Initiates reload sequence if reserve ammo is available and magazine is not full. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void Reload();

	/** Called when reload timer finishes to replenish magazine. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void FinishReload();

	/** Cancels reload (e.g. on sprint or weapon switch). */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void CancelReload();

	/** Returns true if weapon has ammo and is not currently reloading. */
	UFUNCTION(BlueprintPure, Category="Weapon|Combat")
	virtual bool CanFire() const;

	/** Attaches weapon to character mesh socket and records ownership. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void AttachToCharacter(ACharacter* InCharacter, FName SocketName = TEXT("hand_r"));

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FVector GetMuzzleForwardVector() const;

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FORCEINLINE int32 GetCurrentMagAmmo() const { return CurrentMagAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FORCEINLINE int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FORCEINLINE bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE FWeaponConfig GetWeaponConfig() const { return WeaponConfig; }

	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE float GetAimFOV() const { return WeaponConfig.AimFOV; }

	/** Calculates time delay between shots based on RPM. */
	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE float GetTimeBetweenShots() const { return 60.0f / FMath::Max(WeaponConfig.FireRateRPM, 1.0f); }
};
