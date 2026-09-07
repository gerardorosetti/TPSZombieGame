// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZombieAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class AZombieEnemyBase;

/**
 * High-level AI state for the zombie finite state machine.
 */
UENUM(BlueprintType)
enum class EZombieAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Wander UMETA(DisplayName = "Wander"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Dead UMETA(DisplayName = "Dead")
};

/**
 * AI Controller managing perception, state transitions, and navigation for enemy characters.
 */
UCLASS()
class ISPPV1_API AZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZombieAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	// ----------------------------------------------------------------------------------
	// AI Perception Subsystem
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAISenseConfig_Damage> DamageSenseConfig;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// ----------------------------------------------------------------------------------
	// State & Target Tracking
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="AI|State")
	EZombieAIState CurrentState = EZombieAIState::Idle;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="AI|State")
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="AI|State")
	TWeakObjectPtr<AZombieEnemyBase> ControlledZombie;

	/** Acceptance radius in cm for reaching the player during chase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Navigation")
	float AcceptanceRadius = 15.0f;

	/** Wander radius in cm around the zombie when patrolling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Navigation")
	float WanderRadius = 800.0f;

	/** Minimum distance a wander point must be from current position to avoid tiny twitches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Navigation")
	float MinWanderDistance = 350.0f;

	/** Distance threshold in cm for proximity detection (sneaking up close triggers awareness). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Perception")
	float ProximityAggroRadius = 280.0f;

	FTimerHandle WanderTimerHandle;

	// ----------------------------------------------------------------------------------
	// State Machine Logic
	// ----------------------------------------------------------------------------------

	void SetAIState(EZombieAIState NewState);
	void UpdateChaseLogic();
	void UpdateAttackLogic();
	void PickRandomWanderPoint();

public:
	/** Notified directly by the controlled zombie when damage is taken to trigger immediate aggro. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	void NotifyDamageReceived(AActor* Attacker);

	/** Notified when the target player dies so this zombie immediately stops attacking and returns to wander. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	void NotifyPlayerDied(AActor* DeadPlayer);

	/** Forces an immediate path re-evaluation toward target (e.g. when an obstacle door opens). */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	void ForceRepath();

	UFUNCTION(BlueprintPure, Category="AI|State")
	EZombieAIState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="AI|State")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

protected:
	float TimeSinceLastRepath = 0.0f;
	float RepathInterval = 0.25f;
	FVector LastTargetLocation = FVector::ZeroVector;
};
