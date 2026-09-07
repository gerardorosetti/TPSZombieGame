// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZombieDamageableInterface.generated.h"

/**
 * Information payload describing a damage event.
 * Using a struct instead of multiple loose parameters adheres to the Parameter Object pattern,
 * making future extensions (e.g. elemental damage, armor piercing) backwards-compatible.
 */
USTRUCT(BlueprintType)
struct FZombieDamageData
{
	GENERATED_BODY()

	/** Base damage value inflicted before multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(ClampMin="0.0"))
	float BaseDamage = 20.0f;

	/** If true, the hit occurred on a critical hitbox (e.g., zombie head). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bIsHeadshot = false;

	/** World location where the projectile/raycast struck the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	FVector HitLocation = FVector::ZeroVector;

	/** Directional impulse vector applied to physics/ragdoll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	FVector HitImpulse = FVector::ZeroVector;

	/** Exact skeletal bone struck by the trace, useful for headshots and dismemberment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	FName HitBoneName = NAME_None;

	/** Actor responsible for instigating the damage (e.g. Weapon or Projectile). */
	UPROPERTY(BlueprintReadWrite, Category="Damage")
	TWeakObjectPtr<AActor> DamageCauser = nullptr;

	/** Controller responsible for instigating the damage (e.g. Player Controller). */
	UPROPERTY(BlueprintReadWrite, Category="Damage")
	TWeakObjectPtr<AController> InstigatedBy = nullptr;

	/** If true, damage originates from a tactical Nuke, suppressing individual kill points and power-up drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bIsNuke = false;
};

/**
 * UInterface reflection wrapper required by Unreal Engine's garbage collection and UHT.
 * Do NOT add game logic inside this class.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UZombieDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for entities capable of receiving damage or healing.
 */
class ISPPV1_API IZombieDamageableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Processes incoming damage on the implementing actor.
	 * @param DamageData The structured payload containing damage amounts, bone names, and instigators.
	 * @return The actual amount of damage successfully applied after shields and multipliers.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Damage")
	float TakeZombieDamage(const FZombieDamageData& DamageData);

	/**
	 * Restores health/shields to the implementing actor.
	 * @param HealAmount Amount of health points to restore.
	 * @param Healer Optional actor responsible for healing (e.g. Perk dispenser).
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Damage")
	void ApplyZombieHealing(float HealAmount, AActor* Healer = nullptr);

	/**
	 * Queries whether the actor is currently alive.
	 * @return True if alive, false if dead or in a downed state.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Damage")
	bool IsZombieAlive() const;

	/**
	 * Queries normalized health percentage (0.0f to 1.0f) for UI health bars.
	 * @return Health ratio between 0.0 (dead) and 1.0 (full health).
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Damage")
	float GetHealthPercent() const;
};
