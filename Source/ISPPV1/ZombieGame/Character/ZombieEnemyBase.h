// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZombieGame/Character/BaseCharacter.h"
#include "ZombieEnemyBase.generated.h"

class UAnimMontage;
class AZombieAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZombieDeathDelegate, AZombieEnemyBase*, DeadZombie);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZombieAttackDelegate);

/**
 * Base AI-controlled enemy character class for zombies.
 * 
 * Pedagogical Architecture:
 * - Inherits vital systems (HealthComponent, IZombieDamageableInterface) from ABaseCharacter (LSP).
 * - Implements reactive ragdoll physics inheriting directional bullet impulse upon death.
 * - Manages melee attack windows, damage delivery to the player, and combat cooldowns.
 * - Broadcasts decoupled observer events for wave tracking, audio, and UI counters.
 */
UCLASS()
class ISPPV1_API AZombieEnemyBase : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AZombieEnemyBase();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------------------------------------
	// Combat & Attack Configuration
	// ----------------------------------------------------------------------------------

	/** Base melee damage dealt to player on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Combat", meta=(ClampMin="1.0"))
	float AttackDamage = 20.0f;

	/** Maximum distance in cm within which the zombie can strike the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Combat", meta=(ClampMin="50.0"))
	float AttackRange = 115.0f;

	/** Cooldown in seconds between consecutive melee strikes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Combat", meta=(ClampMin="0.5"))
	float AttackCooldown = 0.8f;

	/** Radius of the sphere trace used to detect player overlap during attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Combat", meta=(ClampMin="10.0"))
	float AttackRadius = 40.0f;

	/** Turn rate in degrees per second for smooth, realistic zombie rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Movement", meta=(ClampMin="30.0", ClampMax="720.0"))
	float TurnRate = 180.0f;

	/** Time in seconds before a dead ragdoll corpse is destroyed and cleaned from memory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Death", meta=(ClampMin="1.0"))
	float CorpseLifespan = 10.0f;

	// ----------------------------------------------------------------------------------
	// Locomotion & Animation Scaling (Data-Driven from Blueprint Defaults)
	// ----------------------------------------------------------------------------------

	/** 
	 * Base movement speed (cm/s). 
	 * Designer baseline: Fully customizable in BP_Zombie_Nurse.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Locomotion", meta=(ClampMin="10.0"))
	float BaseLocomotionSpeed = 50.0f;

	/** 
	 * Base animation playback rate scale for locomotion. 
	 * Designer baseline: Fully customizable in BP_Zombie_Nurse.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Locomotion", meta=(ClampMin="0.1", ClampMax="5.0"))
	float BaseAnimRateScale = 1.0f;

	/** Maximum speed multiplier over base speed in high rounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Locomotion", meta=(ClampMin="1.0", ClampMax="10.0"))
	float MaxSpeedMultiplier = 4.0f;

	/** Absolute hard cap for zombie movement speed in cm/s (e.g. 250 cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Locomotion", meta=(ClampMin="50.0", ClampMax="600.0"))
	float MaxSpeedCap = 250.0f;

	/** Maximum cap for locomotion animation playback rate so animation never appears absurdly accelerated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Locomotion", meta=(ClampMin="1.0", ClampMax="6.0"))
	float MaxAnimRateCap = 3.5f;

	// ----------------------------------------------------------------------------------
	// Economy & Bounty Rewards (Data-Driven Bounty System)
	// ----------------------------------------------------------------------------------

	/** Points awarded to the attacker on each verified bullet hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Economy", meta=(ClampMin="0"))
	int32 HitPointsReward = 10;

	/** Points awarded to the killer on standard body elimination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Economy", meta=(ClampMin="0"))
	int32 KillPointsReward = 60;

	/** Points awarded to the killer on lethal headshot elimination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Economy", meta=(ClampMin="0"))
	int32 HeadshotPointsReward = 100;

	// ----------------------------------------------------------------------------------
	// Animations
	// ----------------------------------------------------------------------------------

	/** Melee attack animation montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Flinch / hit reaction animation montage played on non-lethal damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	// ----------------------------------------------------------------------------------
	// State Tracking
	// ----------------------------------------------------------------------------------

	/** True if zombie is actively executing an attack swing. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|State")
	bool bIsAttacking = false;

	/** Caches the last damage data received to apply directional ballistic impulse on death. */
	FZombieDamageData LastDamageReceived;

	FTimerHandle AttackCooldownTimerHandle;

public:
	// ----------------------------------------------------------------------------------
	// Observer Events
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category="Zombie|Events")
	FOnZombieDeathDelegate OnZombieDeath;

	UPROPERTY(BlueprintAssignable, Category="Zombie|Events")
	FOnZombieAttackDelegate OnZombieAttack;

	// ----------------------------------------------------------------------------------
	// Combat Interface
	// ----------------------------------------------------------------------------------

	/** Executes a melee strike against the player. Plays montage and delivers damage. */
	UFUNCTION(BlueprintCallable, Category="Zombie|Combat")
	virtual void PerformAttack();

	/** Called by AnimNotify or timer to deliver damage to player in front of zombie. */
	UFUNCTION(BlueprintCallable, Category="Zombie|Combat")
	virtual void ApplyMeleeDamage();

	/** Returns true if the zombie is alive, not attacking, and cooldown has elapsed. */
	UFUNCTION(BlueprintPure, Category="Zombie|Combat")
	virtual bool CanAttack() const;

	/**
	 * Dynamically scales health, speed, and attributes based on wave number.
	 * @param RoundNumber The active match round (1, 2, 3...).
	 */
	UFUNCTION(BlueprintCallable, Category="Zombie|Scaling")
	virtual void InitializeZombieRoundStats(int32 RoundNumber);

	// ----------------------------------------------------------------------------------
	// Overrides from ABaseCharacter & IZombieDamageableInterface
	// ----------------------------------------------------------------------------------

	virtual float TakeZombieDamage_Implementation(const FZombieDamageData& DamageData) override;
	virtual void HandleDeath(AActor* DeadActor, AActor* KillerActor) override;
	virtual void OnDeathStarted(AActor* Killer) override;

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	FORCEINLINE float GetAttackRange() const { return AttackRange; }
	FORCEINLINE float GetAttackDamage() const { return AttackDamage; }
	FORCEINLINE float GetTurnRate() const { return TurnRate; }
	FORCEINLINE bool IsAttacking() const { return bIsAttacking; }
};
