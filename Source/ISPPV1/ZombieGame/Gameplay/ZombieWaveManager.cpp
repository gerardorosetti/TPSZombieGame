// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "ZombieGame/Gameplay/ZombieSpawnPoint.h"
#include "ZombieGame/Character/ZombieEnemyBase.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

AZombieWaveManager::AZombieWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default to C++ ZombieEnemyBase, attempt to bind BP_Zombie_Nurse if available
	ZombieClass = AZombieEnemyBase::StaticClass();
	static ConstructorHelpers::FClassFinder<AZombieEnemyBase> ZombieBP(
		TEXT("/Game/ZombieGame/Blueprints/Characters/Enemies/BP_Zombie_Nurse")
	);
	if (ZombieBP.Succeeded() && ZombieBP.Class)
	{
		ZombieClass = ZombieBP.Class;
	}

	BaseZombiesPerWave = 6;
	ZombiesPerWaveMultiplier = 4.0f;
	MaxSimultaneousZombies = 16;
	SpawnInterval = 1.2f;
	IntermissionDuration = 10.0f;
	InitialWarmupDuration = 3.0f;
}

void AZombieWaveManager::BeginPlay()
{
	Super::BeginPlay();

	// 1. Discover all spawn points placed across the map
	SpawnPoints.Empty();
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AZombieSpawnPoint> It(World); It; ++It)
		{
			RegisterSpawnPoint(*It);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ZombieWaveManager] Discovered %d spawn points in level."), SpawnPoints.Num());

	// 2. Schedule initial match warmup before Wave 1 begins
	SetWaveState(EWaveState::WaitingToStart);

	if (InitialWarmupDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			IntermissionTimerHandle,
			this,
			&AZombieWaveManager::StartNextWave,
			InitialWarmupDuration,
			false
		);
	}
	else
	{
		StartNextWave();
	}
}

void AZombieWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTickTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AZombieWaveManager::SetWaveState(EWaveState NewState)
{
	if (CurrentWaveState == NewState)
	{
		return;
	}

	CurrentWaveState = NewState;
	OnWaveStateChanged.Broadcast(CurrentWaveState);
	UE_LOG(LogTemp, Log, TEXT("[ZombieWaveManager] State Transition -> %d"), static_cast<int32>(CurrentWaveState));
}

int32 AZombieWaveManager::CalculateZombiesForWave(int32 WaveNum) const
{
	if (WaveNum <= 1)
	{
		return BaseZombiesPerWave;
	}

	// Formula: Base + (Wave - 1) * Multiplier
	return BaseZombiesPerWave + FMath::RoundToInt32((WaveNum - 1) * ZombiesPerWaveMultiplier);
}

void AZombieWaveManager::StartNextWave()
{
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTickTimerHandle);

	CurrentWaveNumber++;
	TotalZombiesForWave = CalculateZombiesForWave(CurrentWaveNumber);
	ZombiesSpawnedThisWave = 0;
	ZombiesAliveCount = 0;

	SetWaveState(EWaveState::WaveActive);

	// Update player state rounds survived
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>())
		{
			PS->SetRoundsSurvived(CurrentWaveNumber - 1);
		}
	}

	OnWaveStarted.Broadcast(CurrentWaveNumber);
	OnZombiesRemainingChanged.Broadcast(TotalZombiesForWave, TotalZombiesForWave);

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[ZombieWaveManager] >>> STARTING ROUND %d (%d Zombies) <<<"), CurrentWaveNumber, TotalZombiesForWave);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));

	// Start spawning loop
	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AZombieWaveManager::SpawnSingleZombie,
		SpawnInterval,
		true,
		0.5f
	);
}

AZombieSpawnPoint* AZombieWaveManager::SelectRandomSpawnPoint() const
{
	TArray<AZombieSpawnPoint*> ActivePoints = GetActiveSpawnPoints();
	if (ActivePoints.Num() == 0)
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, ActivePoints.Num() - 1);
	return ActivePoints[RandomIndex];
}

void AZombieWaveManager::SpawnSingleZombie()
{
	if (CurrentWaveState != EWaveState::WaveActive)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	// Check if all zombies for this round have been generated
	if (ZombiesSpawnedThisWave >= TotalZombiesForWave)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	// Enforce maximum concurrent zombie limit to protect frame rate
	if (ZombiesAliveCount >= MaxSimultaneousZombies)
	{
		return;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	AZombieSpawnPoint* ChosenSpawnPoint = SelectRandomSpawnPoint();
	if (ChosenSpawnPoint)
	{
		ChosenSpawnPoint->GetValidSpawnLocation(SpawnLocation);
		SpawnRotation = FRotator(0.0f, ChosenSpawnPoint->GetActorRotation().Yaw, 0.0f);
	}
	else
	{
		// Intelligent Fallback: Query NavMesh around the player at a safe distance (900-1500 cm)
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (PlayerPawn && NavSys)
		{
			const FVector PlayerLoc = PlayerPawn->GetActorLocation();
			const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
			const float RandomDist = FMath::FRandRange(900.0f, 1500.0f);
			const FVector TargetCandidate = PlayerLoc + FVector(FMath::Cos(RandomAngle) * RandomDist, FMath::Sin(RandomAngle) * RandomDist, 0.0f);

			FNavLocation NavLoc;
			if (NavSys->GetRandomReachablePointInRadius(TargetCandidate, 350.0f, NavLoc))
			{
				SpawnLocation = NavLoc.Location;
				SpawnRotation = (PlayerLoc - SpawnLocation).Rotation();
				SpawnRotation.Pitch = 0.0f;
				SpawnRotation.Roll = 0.0f;
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World || !ZombieClass)
	{
		return;
	}

	// 1. Compensate for ACharacter capsule origin being at center (Z/2):
	// NavMesh query returns the floor surface. Placing the root component directly at
	// NavLoc buries the lower half of the capsule inside slopes, ramps, or terrain.
	float CapsuleHalfHeight = 88.0f;
	if (const ACharacter* CDO = ZombieClass->GetDefaultObject<ACharacter>())
	{
		if (const UCapsuleComponent* Capsule = CDO->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	// 2. Perform a vertical raycast against physical geometry (ramps, terrain, meshes)
	// to ensure sub-millimeter precision above inclined surfaces.
	FHitResult FloorHit;
	FCollisionQueryParams FloorQueryParams(SCENE_QUERY_STAT(ZombieSpawnFloorTrace), false);
	FloorQueryParams.bTraceComplex = true;
	const FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, 250.0f);
	const FVector TraceEnd = SpawnLocation - FVector(0.0f, 0.0f, 250.0f);

	if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, FloorQueryParams))
	{
		SpawnLocation.Z = FloorHit.ImpactPoint.Z + CapsuleHalfHeight + 4.0f;
	}
	else
	{
		SpawnLocation.Z += (CapsuleHalfHeight + 4.0f);
	}

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AZombieEnemyBase* SpawnedZombie = World->SpawnActor<AZombieEnemyBase>(ZombieClass, SpawnTransform, SpawnParams);
	if (!SpawnedZombie)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZombieWaveManager] Failed to instantiate zombie from class %s"), *ZombieClass->GetName());
		return;
	}

	// Scale zombie attributes dynamically to match the current round
	SpawnedZombie->InitializeZombieRoundStats(CurrentWaveNumber);

	// Bind death notification to maintain remaining enemy tally
	SpawnedZombie->OnZombieDeath.AddDynamic(this, &AZombieWaveManager::HandleZombieDeath);

	ZombiesSpawnedThisWave++;
	ZombiesAliveCount++;

	const int32 Remaining = GetRemainingZombiesCount();
	OnZombiesRemainingChanged.Broadcast(Remaining, TotalZombiesForWave);

	UE_LOG(LogTemp, Verbose, TEXT("[ZombieWaveManager] Spawned zombie #%d/%d (Alive: %d)"),
		ZombiesSpawnedThisWave, TotalZombiesForWave, ZombiesAliveCount);

	// If that was the last zombie of the wave, stop the spawner
	if (ZombiesSpawnedThisWave >= TotalZombiesForWave)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

void AZombieWaveManager::HandleZombieDeath(AZombieEnemyBase* DeadZombie)
{
	ZombiesAliveCount = FMath::Max(0, ZombiesAliveCount - 1);

	const int32 Remaining = GetRemainingZombiesCount();
	OnZombiesRemainingChanged.Broadcast(Remaining, TotalZombiesForWave);

	UE_LOG(LogTemp, Log, TEXT("[ZombieWaveManager] Zombie eliminated. %d remaining in Wave %d."),
		Remaining, CurrentWaveNumber);

	// Check if wave is completed (all spawned and all dead)
	if (Remaining <= 0 && CurrentWaveState == EWaveState::WaveActive)
	{
		SetWaveState(EWaveState::WaveCompleted);
		StartIntermission();
	}
}

void AZombieWaveManager::StartIntermission()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	SetWaveState(EWaveState::Intermission);

	IntermissionTimeRemaining = IntermissionDuration;
	OnIntermissionCountdown.Broadcast(IntermissionTimeRemaining);

	UE_LOG(LogTemp, Warning, TEXT("[ZombieWaveManager] Wave %d Cleared! Rest period: %0.1fs"),
		CurrentWaveNumber, IntermissionDuration);

	// Tick countdown every second
	GetWorldTimerManager().SetTimer(
		CountdownTickTimerHandle,
		this,
		&AZombieWaveManager::TickIntermissionCountdown,
		1.0f,
		true
	);
}

void AZombieWaveManager::TickIntermissionCountdown()
{
	IntermissionTimeRemaining -= 1.0f;

	if (IntermissionTimeRemaining <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(CountdownTickTimerHandle);
		StartNextWave();
	}
	else
	{
		OnIntermissionCountdown.Broadcast(IntermissionTimeRemaining);
	}
}

void AZombieWaveManager::TriggerGameOver()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTickTimerHandle);

	SetWaveState(EWaveState::GameOver);
	OnGameOver.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[ZombieWaveManager] >>> GAME OVER <<< (Survived %d Rounds)"), CurrentWaveNumber - 1);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void AZombieWaveManager::RegisterSpawnPoint(AZombieSpawnPoint* NewSpawnPoint)
{
	if (NewSpawnPoint && !SpawnPoints.Contains(NewSpawnPoint))
	{
		SpawnPoints.Add(NewSpawnPoint);
	}
}

TArray<AZombieSpawnPoint*> AZombieWaveManager::GetActiveSpawnPoints() const
{
	TArray<AZombieSpawnPoint*> Active;
	for (AZombieSpawnPoint* SP : SpawnPoints)
	{
		if (IsValid(SP) && SP->IsSpawnPointActive())
		{
			Active.Add(SP);
		}
	}
	return Active;
}
