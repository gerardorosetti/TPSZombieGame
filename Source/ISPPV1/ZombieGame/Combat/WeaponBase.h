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
class ADroppedMagazine;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHitTargetSignature, bool, bIsHeadshot);

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
	virtual void Tick(float DeltaTime) override;

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

	/** Static mesh for detachable magazine (supports procedural reload animation). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MagazineStaticMesh;

	// ----------------------------------------------------------------------------------
	// Procedural Magazine Reload Animation (Hand-Synchronized)
	// ----------------------------------------------------------------------------------

	/** If true, magazine detaches to the left hand, tosses away, and inserts a fresh mag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	bool bEnableProceduralReload = true;

	/** Skeletal mesh bone/socket on the character to follow during reload (default: hand_l). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	FName ReloadHandSocket = TEXT("hand_l");

	/** Relative location offset when attached to reload hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	FVector MagHandOffset = FVector(0.0f, 4.0f, -2.0f);

	/** Relative rotation offset when attached to reload hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	FRotator MagHandRotation = FRotator(0.0f, 0.0f, 90.0f);

	/** Time in seconds into reload when hand grabs and detaches the empty magazine from the rifle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload", meta=(ClampMin="0.0"))
	float ReloadDetachTime = 0.35f;

	/** Time in seconds into reload when empty magazine is tossed away (hidden). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload", meta=(ClampMin="0.0"))
	float ReloadTossTime = 0.85f;

	/** Time in seconds into reload when fresh magazine is grabbed from the pouch (unhidden in hand). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload", meta=(ClampMin="0.0"))
	float ReloadGrabNewTime = 1.05f;

	/** Time in seconds into reload when fresh magazine is inserted into the rifle magazine well. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload", meta=(ClampMin="0.0"))
	float ReloadInsertTime = 2.05f;

	/** Linear velocity impulse applied to discarded magazine upon toss (X=Forward, Y=Right, Z=Up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	FVector DroppedMagazineImpulse = FVector(0.0f, -140.0f, -60.0f);

	/** Class to spawn for the discarded empty magazine with rigid-body physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|ProceduralReload")
	TSubclassOf<ADroppedMagazine> DroppedMagazineClass;

	FVector InitialMagLocation = FVector::ZeroVector;
	FRotator InitialMagRotation = FRotator::ZeroRotator;
	int32 ReloadPhase = 0;

	// ----------------------------------------------------------------------------------
	// Weapon Configuration & State
	// ----------------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Config")
	FWeaponConfig WeaponConfig;

	/** Name of the socket where muzzle flash, tracers, and traces originate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Config")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Additional offset in cm added to the computed muzzle tip location. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|Config")
	FVector MuzzleOffset = FVector::ZeroVector;

	// ----------------------------------------------------------------------------------
	// Aiming Alignment (ADS Offset Tuning)
	// ----------------------------------------------------------------------------------

	/** Relative location offset applied to weapon when Aiming Down Sights (ADS). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|AimAlignment")
	FVector AimingLocationOffset = FVector::ZeroVector;

	/** Relative rotation offset applied to weapon when Aiming Down Sights (ADS). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|AimAlignment")
	FRotator AimingRotationOffset = FRotator::ZeroRotator;

	/** Interp speed for transitioning weapon between hipfire and aiming transforms. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon|AimAlignment", meta=(ClampMin="1.0", ClampMax="50.0"))
	float AimInterpSpeed = 14.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon|State")
	bool bIsAiming = false;

	FVector DefaultMeshLocation = FVector::ZeroVector;
	FRotator DefaultMeshRotation = FRotator::ZeroRotator;

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

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnWeaponHitTargetSignature OnWeaponHitTarget;

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

	/** Restores ammunition in magazine and/or reserve pools (used by Max Ammo and Wall Buys). */
	UFUNCTION(BlueprintCallable, Category="Weapon|Combat")
	virtual void RefillAmmo(bool bRefillMag = true, bool bRefillReserve = true);

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

	UFUNCTION(BlueprintPure, Category="Weapon|State")
	FORCEINLINE bool IsFiring() const { return bIsFiring; }

	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE FWeaponConfig GetWeaponConfig() const { return WeaponConfig; }

	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE float GetAimFOV() const { return WeaponConfig.AimFOV; }

	/** Calculates time delay between shots based on RPM. */
	UFUNCTION(BlueprintPure, Category="Weapon|Config")
	FORCEINLINE float GetTimeBetweenShots() const { return 60.0f / FMath::Max(WeaponConfig.FireRateRPM, 1.0f); }

	UFUNCTION(BlueprintCallable, Category="Weapon|AimAlignment")
	void SetAiming(bool bNewAiming) { bIsAiming = bNewAiming; }

	UFUNCTION(BlueprintPure, Category="Weapon|AimAlignment")
	bool IsAiming() const { return bIsAiming; }
};
