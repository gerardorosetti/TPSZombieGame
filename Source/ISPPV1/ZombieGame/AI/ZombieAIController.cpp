// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/AI/ZombieAIController.h"
#include "ZombieGame/Character/ZombieEnemyBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AZombieAIController::AZombieAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = true;

	// 1. Instantiate AI Perception Component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	// 2. Configure Sight Sense (Human visual cone)
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = 1800.0f;                    // 18 meters visual range
		SightConfig->LoseSightRadius = 2200.0f;                // 22 meters to lose track
		SightConfig->PeripheralVisionAngleDegrees = 65.0f;      // 130 degree field of view
		SightConfig->SetMaxAge(5.0f);
		SightConfig->AutoSuccessRangeFromLastSeenLocation = 150.0f;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		AIPerceptionComponent->ConfigureSense(*SightConfig);
		AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	}

	// 3. Configure Damage Sense (Immediate aggro when shot from anywhere)
	DamageSenseConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageSenseConfig"));
	if (DamageSenseConfig)
	{
		DamageSenseConfig->SetMaxAge(5.0f);
		AIPerceptionComponent->ConfigureSense(*DamageSenseConfig);
	}
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledZombie = Cast<AZombieEnemyBase>(InPawn);
	AcceptanceRadius = 15.0f;

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::HandleTargetPerceptionUpdated);
	}

	// In zombie survival, spawned zombies immediately pursue the player
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		TargetActor = PlayerPawn;
		SetAIState(EZombieAIState::Chase);
	}
	else
	{
		SetAIState(EZombieAIState::Wander);
	}
}

void AZombieAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(WanderTimerHandle);
	StopMovement();

	ControlledZombie = nullptr;
	TargetActor = nullptr;

	Super::OnUnPossess();
}

void AZombieAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastRepath += DeltaTime;

	if (!ControlledZombie.IsValid())
	{
		return;
	}

	// Check if controlled zombie has died
	if (!ControlledZombie->IsZombieAlive_Implementation())
	{
		if (CurrentState != EZombieAIState::Dead)
		{
			SetAIState(EZombieAIState::Dead);
		}
		return;
	}

	// Continuous hunt safeguard: ensure zombie is always chasing the living player
	if (!TargetActor.IsValid() || CurrentState == EZombieAIState::Wander || CurrentState == EZombieAIState::Idle)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			TargetActor = PlayerPawn;
			SetAIState(EZombieAIState::Chase);
		}
	}

	switch (CurrentState)
	{
	case EZombieAIState::Chase:
		UpdateChaseLogic();
		break;

	case EZombieAIState::Attack:
		UpdateAttackLogic();
		break;

	case EZombieAIState::Wander:
	case EZombieAIState::Idle:
	default:
		break;
	}
}

void AZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!ControlledZombie.IsValid() || !ControlledZombie->IsZombieAlive_Implementation())
	{
		return;
	}

	// Strictly target living player characters! Never target other zombies or world items
	if (Actor && Actor != GetPawn() && Actor->IsA<APlayerCharacter>())
	{
		TargetActor = Actor;
		SetAIState(EZombieAIState::Chase);
	}
}

void AZombieAIController::SetAIState(EZombieAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	switch (CurrentState)
	{
	case EZombieAIState::Wander:
		GetWorldTimerManager().SetTimer(WanderTimerHandle, this, &AZombieAIController::PickRandomWanderPoint, 5.0f, true, 0.5f);
		break;

	case EZombieAIState::Chase:
		GetWorldTimerManager().ClearTimer(WanderTimerHandle);
		if (TargetActor.IsValid())
		{
			LastTargetLocation = TargetActor->GetActorLocation();
			TimeSinceLastRepath = 0.0f;
			MoveToActor(TargetActor.Get(), AcceptanceRadius, true, true, true, nullptr, true);
		}
		break;

	case EZombieAIState::Attack:
		GetWorldTimerManager().ClearTimer(WanderTimerHandle);
		StopMovement();
		break;

	case EZombieAIState::Dead:
		GetWorldTimerManager().ClearTimer(WanderTimerHandle);
		StopMovement();
		break;

	case EZombieAIState::Idle:
	default:
		StopMovement();
		break;
	}
}

void AZombieAIController::UpdateChaseLogic()
{
	if (!TargetActor.IsValid() || !ControlledZombie.IsValid())
	{
		SetAIState(EZombieAIState::Wander);
		return;
	}

	// If actively executing an attack swing, do not move
	if (ControlledZombie->IsAttacking())
	{
		return;
	}

	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector MyLoc = ControlledZombie->GetActorLocation();
	const float Distance2D = FVector::Dist2D(MyLoc, TargetLoc);

	// Always smoothly rotate toward the target player even while pathfinding or temporarily obstructed
	FVector DirToTarget = TargetLoc - MyLoc;
	DirToTarget.Z = 0.0f;
	if (!DirToTarget.IsNearlyZero())
	{
		const FRotator CurrentRot = ControlledZombie->GetActorRotation();
		const FRotator TargetRot = DirToTarget.Rotation();
		const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
		ControlledZombie->SetActorRotation(FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 6.0f));
	}

	// If within melee strike distance, switch immediately to attack state and strike
	if (Distance2D <= ControlledZombie->GetAttackRange())
	{
		SetAIState(EZombieAIState::Attack);
		if (ControlledZombie->CanAttack())
		{
			ControlledZombie->PerformAttack();
		}
		return;
	}

	// Intelligent rate-limited repath:
	// Only request MoveToActor if not currently moving OR if time interval elapsed and player moved significantly (>80 cm)
	const bool bNotMoving = (GetMoveStatus() != EPathFollowingStatus::Moving);
	const bool bTargetMoved = (FVector::DistSquared(LastTargetLocation, TargetLoc) > 6400.0f); // 80cm
	const bool bTimeElapsed = (TimeSinceLastRepath >= RepathInterval);

	if (bNotMoving || (bTimeElapsed && bTargetMoved))
	{
		TimeSinceLastRepath = 0.0f;
		LastTargetLocation = TargetLoc;
		MoveToActor(TargetActor.Get(), AcceptanceRadius, true, true, true, nullptr, true);
	}
}

void AZombieAIController::ForceRepath()
{
	TimeSinceLastRepath = RepathInterval;
	if (TargetActor.IsValid() && ControlledZombie.IsValid() && ControlledZombie->IsZombieAlive_Implementation())
	{
		LastTargetLocation = TargetActor->GetActorLocation();
		MoveToActor(TargetActor.Get(), AcceptanceRadius, true, true, true, nullptr, true);
	}
}

void AZombieAIController::UpdateAttackLogic()
{
	if (!TargetActor.IsValid() || !ControlledZombie.IsValid())
	{
		SetAIState(EZombieAIState::Wander);
		return;
	}

	// If actively executing an attack swing, stay planted!
	// Never resume chase or slide while the current strike is playing!
	if (ControlledZombie->IsAttacking())
	{
		return;
	}

	const float Distance2D = FVector::Dist2D(ControlledZombie->GetActorLocation(), TargetActor->GetActorLocation());

	// If player escaped beyond attack range + hysteresis buffer, resume chase
	if (Distance2D > ControlledZombie->GetAttackRange() + 15.0f)
	{
		SetAIState(EZombieAIState::Chase);
		return;
	}

	// Smoothly interpolate zombie rotation towards target while stopped (no instant snapping)
	FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledZombie->GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	if (!DirectionToTarget.IsNearlyZero())
	{
		const FRotator CurrentRot = ControlledZombie->GetActorRotation();
		const FRotator DesiredRot = DirectionToTarget.Rotation();
		const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
		const FRotator SmoothedRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaSeconds, 5.0f);
		ControlledZombie->SetActorRotation(SmoothedRot);
	}

	// If attack cooldown has elapsed and zombie is ready, strike again
	if (ControlledZombie->CanAttack())
	{
		ControlledZombie->PerformAttack();
	}
}

void AZombieAIController::NotifyDamageReceived(AActor* Attacker)
{
	if (!ControlledZombie.IsValid() || !ControlledZombie->IsZombieAlive_Implementation())
	{
		return;
	}

	// Zero friendly fire aggro: only aggro on player characters
	if (Attacker && Attacker != GetPawn())
	{
		if (Attacker->IsA<APlayerCharacter>() || (Cast<APawn>(Attacker) && Cast<APawn>(Attacker)->IsPlayerControlled()))
		{
			TargetActor = Attacker;
			SetAIState(EZombieAIState::Chase);
			UE_LOG(LogTemp, Log, TEXT("[%s] Aggroed by damage from player %s!"), *GetName(), *Attacker->GetName());
		}
	}
}

void AZombieAIController::PickRandomWanderPoint()
{
	if (CurrentState != EZombieAIState::Wander || !ControlledZombie.IsValid())
	{
		return;
	}

	// Do not interrupt active wandering if the zombie is currently following a path
	if (GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys)
	{
		FNavLocation ResultLocation;
		const FVector CurrentLoc = ControlledZombie->GetActorLocation();

		// Attempt to find a reachable point with a minimum travel distance (350 cm) to avoid jittery twitches
		bool bFound = false;
		for (int32 Attempt = 0; Attempt < 5; ++Attempt)
		{
			if (NavSys->GetRandomReachablePointInRadius(CurrentLoc, WanderRadius, ResultLocation))
			{
				if (FVector::Dist2D(CurrentLoc, ResultLocation.Location) >= MinWanderDistance)
				{
					bFound = true;
					break;
				}
			}
		}

		if (!bFound)
		{
			bFound = NavSys->GetRandomReachablePointInRadius(CurrentLoc, WanderRadius, ResultLocation);
		}

		if (bFound)
		{
			MoveToLocation(ResultLocation.Location, 50.0f, true, true, true);
		}
	}
}
