// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zombies/Interfaces/ZombieDamageableInterface.h"
#include "ZombieDamageableDummy.generated.h"

class UZombieHealthComponent;
class UStaticMeshComponent;

/**
 * Educational dummy actor used to verify and demonstrate the Health Component
 * and Damageable Interface integration in the editor without requiring full AI.
 */
UCLASS()
class ISPPV1_API AZombieDamageableDummy : public AActor, public IZombieDamageableInterface
{
	GENERATED_BODY()

public:
	AZombieDamageableDummy();

protected:
	virtual void BeginPlay() override;

	/** Root static mesh representing the physical target. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* MeshComponent;

	/** Reusable health component handling vital statistics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UZombieHealthComponent* HealthComponent;

	/** Visual feedback: flashes material or changes scale on hit. */
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData);

	/** Triggered when health reaches zero. */
	UFUNCTION()
	void HandleDeath(AActor* DeadActor, AActor* KillerActor);

public:
	// ----------------------------------------------------------------------------------
	// IZombieDamageableInterface Implementation
	// ----------------------------------------------------------------------------------

	virtual float TakeZombieDamage_Implementation(const FZombieDamageData& DamageData) override;
	virtual void ApplyZombieHealing_Implementation(float HealAmount, AActor* Healer = nullptr) override;
	virtual bool IsZombieAlive_Implementation() const override;
	virtual float GetHealthPercent_Implementation() const override;
};
