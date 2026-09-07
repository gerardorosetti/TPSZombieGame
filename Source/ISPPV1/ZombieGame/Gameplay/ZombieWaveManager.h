// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieWaveManager.generated.h"

class AZombieEnemyBase;
class AZombieSpawnPoint;

/**
 * State lifecycle of the wave manager finite state machine.
 */
UENUM(BlueprintType)
enum class EWaveState : uint8
{
	WaitingToStart UMETA(DisplayName = "Waiting to Start"),
	WaveActive     UMETA(DisplayName = "Wave Active"),
	Intermission   UMETA(DisplayName = "Intermission"),
	WaveCompleted  UMETA(DisplayName = "Wave Completed"),
	GameOver       UMETA(DisplayName = "Game Over")
};

// ----------------------------------------------------------------------------------
// Observer Pattern Delegates (HUD & Audio Subsystems)
// ----------------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStartedSignature, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZombiesRemainingSignature, int32, RemainingCount, int32, TotalWaveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntermissionCountdownSignature, float, SecondsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStateChangedSignature, EWaveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverSignature);

/**
 * Orchestrates wave generation, enemy instantiation, round difficulty scaling, and match lifecycle.
 */
UCLASS()
class ISPPV1_API AZombieWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AZombieWaveManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ----------------------------------------------------------------------------------
	// Configuration Properties
	// ----------------------------------------------------------------------------------

	/** Zombie enemy Blueprint or C++ class to spawn. Defaults to BP_Zombie_Nurse or AZombieEnemyBase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config")
	TSubclassOf<AZombieEnemyBase> ZombieClass;

	/** Number of enemies spawned in Wave 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="1"))
	int32 BaseZombiesPerWave = 6;

	/** Multiplier added to zombie count for each subsequent round. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="1.0"))
	float ZombiesPerWaveMultiplier = 4.0f;

	/** Maximum active zombies allowed on the map at the same time to prevent performance drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="1", ClampMax="64"))
	int32 MaxSimultaneousZombies = 16;

	/** Seconds between each individual zombie spawn during an active wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="0.2", ClampMax="10.0"))
	float SpawnInterval = 1.2f;

	/** Rest duration in seconds between rounds (intermission countdown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="1.0"))
	float IntermissionDuration = 10.0f;

	/** Delay in seconds before Wave 1 begins after map load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves|Config", meta=(ClampMin="0.0"))
	float InitialWarmupDuration = 3.0f;

	// ----------------------------------------------------------------------------------
	// Runtime State Tracking
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	EWaveState CurrentWaveState = EWaveState::WaitingToStart;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	int32 TotalZombiesForWave = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	int32 ZombiesSpawnedThisWave = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	int32 ZombiesAliveCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	float IntermissionTimeRemaining = 0.0f;

	/** Cached list of all registered spawn point actors placed across map zones. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves|Runtime")
	TArray<TObjectPtr<AZombieSpawnPoint>> SpawnPoints;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle IntermissionTimerHandle;
	FTimerHandle CountdownTickTimerHandle;

public:
	// ----------------------------------------------------------------------------------
	// Observer Event Delegates
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category="Zombie|Waves|Events")
	FOnWaveStartedSignature OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category="Zombie|Waves|Events")
	FOnZombiesRemainingSignature OnZombiesRemainingChanged;

	UPROPERTY(BlueprintAssignable, Category="Zombie|Waves|Events")
	FOnIntermissionCountdownSignature OnIntermissionCountdown;

	UPROPERTY(BlueprintAssignable, Category="Zombie|Waves|Events")
	FOnWaveStateChangedSignature OnWaveStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Zombie|Waves|Events")
	FOnGameOverSignature OnGameOver;

public:
	// ----------------------------------------------------------------------------------
	// Core Wave Operations
	// ----------------------------------------------------------------------------------

	/** Starts the next consecutive wave, calculating enemy count and difficulty scaling. */
	UFUNCTION(BlueprintCallable, Category="Zombie|Waves|Operations")
	void StartNextWave();

	/** Instantiates a single zombie at a valid NavMesh spawn point (Factory Pattern). */
	UFUNCTION(BlueprintCallable, Category="Zombie|Waves|Operations")
	void SpawnSingleZombie();

	/** Triggered via dynamic delegate when an active zombie enemy dies. */
	UFUNCTION()
	void HandleZombieDeath(AZombieEnemyBase* DeadZombie);

	/** Manually triggers game over state (e.g. player died). */
	UFUNCTION(BlueprintCallable, Category="Zombie|Waves|Operations")
	void TriggerGameOver();

	/** Registers an active spawn point (called automatically on BeginPlay or when a door opens). */
	UFUNCTION(BlueprintCallable, Category="Zombie|Waves|Setup")
	void RegisterSpawnPoint(AZombieSpawnPoint* NewSpawnPoint);

	/** Collects all active spawn points in the map. */
	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Setup")
	TArray<AZombieSpawnPoint*> GetActiveSpawnPoints() const;

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Queries")
	int32 GetCurrentWaveNumber() const { return CurrentWaveNumber; }

	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Queries")
	int32 GetRemainingZombiesCount() const { return (TotalZombiesForWave - ZombiesSpawnedThisWave) + ZombiesAliveCount; }

	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Queries")
	int32 GetTotalZombiesForWave() const { return TotalZombiesForWave; }

	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Queries")
	EWaveState GetCurrentWaveState() const { return CurrentWaveState; }

	UFUNCTION(BlueprintPure, Category="Zombie|Waves|Queries")
	float GetIntermissionTimeRemaining() const { return IntermissionTimeRemaining; }

protected:
	void SetWaveState(EWaveState NewState);
	void StartIntermission();
	void TickIntermissionCountdown();
	int32 CalculateZombiesForWave(int32 WaveNum) const;
	AZombieSpawnPoint* SelectRandomSpawnPoint() const;
};
