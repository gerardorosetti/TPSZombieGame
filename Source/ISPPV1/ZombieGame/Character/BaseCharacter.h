// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieGame/Interfaces/ZombieDamageableInterface.h"
#include "BaseCharacter.generated.h"

class UZombieHealthComponent;

/**
 * Common base entity for living characters, centralizing health, damage handling, and death events.
 */
UCLASS(Abstract)
class ISPPV1_API ABaseCharacter : public ACharacter, public IZombieDamageableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------------------------------------
	// Core Components
	// ----------------------------------------------------------------------------------

	/** Reusable health component managing vitals, shields, and death notification. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Health", meta=(AllowPrivateAccess="true"))
	UZombieHealthComponent* HealthComponent;

	// ----------------------------------------------------------------------------------
	// Event Handlers & Virtual Hooks
	// ----------------------------------------------------------------------------------

	/** Called when health changes on the HealthComponent. */
	UFUNCTION()
	virtual void HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData);

	/** Called when the character's health reaches zero. */
	UFUNCTION()
	virtual void HandleDeath(AActor* DeadActor, AActor* KillerActor);

	/** Virtual hook for subclasses to implement custom death behaviors (e.g. drop loot, stop AI, play sound). */
	virtual void OnDeathStarted(AActor* Killer);

public:
	// ----------------------------------------------------------------------------------
	// IZombieDamageableInterface Implementation
	// ----------------------------------------------------------------------------------

	virtual float TakeZombieDamage_Implementation(const FZombieDamageData& DamageData) override;
	virtual void ApplyZombieHealing_Implementation(float HealAmount, AActor* Healer = nullptr) override;
	virtual bool IsZombieAlive_Implementation() const override;
	virtual float GetHealthPercent_Implementation() const override;

	/** Direct getter for the health component. */
	UFUNCTION(BlueprintPure, Category="Character|Health")
	FORCEINLINE UZombieHealthComponent* GetHealthComponent() const { return HealthComponent; }
};
