// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZombieGame/Interfaces/ZombieDamageableInterface.h"
#include "ZombieHealthComponent.generated.h"

/**
 * Multicast Delegate triggered whenever the health value changes.
 * Used by UI HUD widgets, floating health bars, and camera damage vignette effects.
 * 
 * Pattern: Observer Pattern (Push model).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnZombieHealthChangedSignature,
	float, CurrentHealth,
	float, MaxHealth,
	float, HealthDelta,
	const FZombieDamageData&, DamageData
);

/**
 * Multicast Delegate triggered whenever the shield value changes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnZombieShieldChangedSignature,
	float, CurrentShield,
	float, MaxShield,
	float, ShieldDelta
);

/**
 * Multicast Delegate triggered when health drops to zero or below.
 * Parameters: The actor that died, and the actor responsible for the fatal hit.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnZombieDeathSignature,
	AActor*, DeadActor,
	AActor*, KillerActor
);

/**
 * Reusable Actor Component managing health, shield, damage absorption, and death states.
 * 
 * Pedagogical Rationale:
 * - Demonstrates the Component Pattern: Decouples health calculation from Character rendering and movement.
 * - Adheres to Single Responsibility Principle (SRP): Only handles vital statistics and damage arithmetic.
 * - Implements Observer Pattern: Emits events without knowing what systems (audio, HUD, animation) are listening.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class ISPPV1_API UZombieHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZombieHealthComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ----------------------------------------------------------------------------------
	// Configuration Properties
	// ----------------------------------------------------------------------------------

	/** Maximum health pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Config", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	/** Maximum shield pool. Damage is deducted from shields before reaching health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Config", meta=(ClampMin="0.0"))
	float MaxShield = 0.0f;

	/** Damage multiplier applied when a hit is flagged as a headshot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Config", meta=(ClampMin="1.0"))
	float HeadshotMultiplier = 2.0f;

	/** If true, incoming damage will not reduce health or shields (useful for debug/god-mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Debug")
	bool bIsInvulnerable = false;

protected:
	/** Current health value, clamped between 0 and MaxHealth. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Runtime")
	float CurrentHealth = 100.0f;

	/** Current shield value, clamped between 0 and MaxShield. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Runtime")
	float CurrentShield = 0.0f;

	/** Guard flag preventing multiple death triggers on subsequent hits after dying. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health|Runtime")
	bool bIsDead = false;

public:
	// ----------------------------------------------------------------------------------
	// Event Delegates (Observer Pattern)
	// ----------------------------------------------------------------------------------

	/** Broadcast when health is deducted or restored. */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnZombieHealthChangedSignature OnHealthChanged;

	/** Broadcast when shields are deducted or recharged. */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnZombieShieldChangedSignature OnShieldChanged;

	/** Broadcast once when the actor's health reaches zero. */
	UPROPERTY(BlueprintAssignable, Category="Health|Events")
	FOnZombieDeathSignature OnDeath;

public:
	// ----------------------------------------------------------------------------------
	// Core Gameplay Operations
	// ----------------------------------------------------------------------------------

	/**
	 * Takes structured damage, computes shields and multipliers, and updates vital stats.
	 * @param DamageData Payload detailing base damage, headshot status, bone, and instigator.
	 * @return Actual net damage applied across shields and health.
	 */
	UFUNCTION(BlueprintCallable, Category="Health|Operations")
	float ProcessDamage(const FZombieDamageData& DamageData);

	/**
	 * Restores health up to MaxHealth.
	 * @param HealAmount Points of health to restore.
	 */
	UFUNCTION(BlueprintCallable, Category="Health|Operations")
	void Heal(float HealAmount);

	/**
	 * Restores shield points up to MaxShield.
	 * @param ShieldAmount Points of shield to restore.
	 */
	UFUNCTION(BlueprintCallable, Category="Health|Operations")
	void AddShield(float ShieldAmount);

	/**
	 * Dynamically alters MaxHealth, useful for scaling enemy health per wave.
	 * @param NewMaxHealth The new upper bound.
	 * @param bAdjustCurrentHealth If true, current health is scaled proportionally or reset to new max.
	 */
	UFUNCTION(BlueprintCallable, Category="Health|Operations")
	void SetMaxHealth(float NewMaxHealth, bool bAdjustCurrentHealth = true);

	// ----------------------------------------------------------------------------------
	// Getters / Query Methods
	// ----------------------------------------------------------------------------------

	/** Returns normalized health ratio (0.0 to 1.0). */
	UFUNCTION(BlueprintPure, Category="Health|Queries")
	FORCEINLINE float GetHealthPercent() const { return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f; }

	/** Returns normalized shield ratio (0.0 to 1.0). */
	UFUNCTION(BlueprintPure, Category="Health|Queries")
	FORCEINLINE float GetShieldPercent() const { return (MaxShield > 0.0f) ? (CurrentShield / MaxShield) : 0.0f; }

	/** Returns current health value. */
	UFUNCTION(BlueprintPure, Category="Health|Queries")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Returns max health value. */
	UFUNCTION(BlueprintPure, Category="Health|Queries")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Returns true if the owner is still alive. */
	UFUNCTION(BlueprintPure, Category="Health|Queries")
	FORCEINLINE bool IsAlive() const { return !bIsDead && CurrentHealth > 0.0f; }
};
