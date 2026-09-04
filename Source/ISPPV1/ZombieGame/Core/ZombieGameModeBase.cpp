// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "UObject/ConstructorHelpers.h"

AZombieGameModeBase::AZombieGameModeBase()
{
	// 1. Set default player pawn class to BP_PlayerCharacter (falling back to APlayerCharacter)
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBP(
		TEXT("/Game/ZombieGame/Blueprints/Characters/Player/BP_PlayerCharacter")
	);
	if (PlayerPawnBP.Succeeded() && PlayerPawnBP.Class)
	{
		DefaultPawnClass = PlayerPawnBP.Class;
	}
	else
	{
		DefaultPawnClass = APlayerCharacter::StaticClass();
	}

	// 2. Set default player controller to BP_ThirdPersonPlayerController
	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController")
	);
	if (ControllerBP.Succeeded() && ControllerBP.Class)
	{
		PlayerControllerClass = ControllerBP.Class;
	}

	// 3. Set default player state class for points economy and survival stats
	PlayerStateClass = APlayerStateBase::StaticClass();

	// 4. Default wave manager class
	WaveManagerClass = AZombieWaveManager::StaticClass();
}

void AZombieGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Locate existing or instantiate new active Wave Manager
	ActiveWaveManager = nullptr;
	for (TActorIterator<AZombieWaveManager> It(World); It; ++It)
	{
		ActiveWaveManager = *It;
		break;
	}

	if (!ActiveWaveManager && WaveManagerClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveWaveManager = World->SpawnActor<AZombieWaveManager>(WaveManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		UE_LOG(LogTemp, Log, TEXT("[ZombieGameModeBase] Auto-spawned active Wave Manager instance."));
	}

	// 2. Bind to local player pawn death to trigger match GameOver
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PlayerPawn))
		{
			if (UZombieHealthComponent* HC = PlayerChar->GetHealthComponent())
			{
				HC->OnDeath.AddDynamic(this, &AZombieGameModeBase::HandlePlayerDeath);
			}
		}
	}
}

void AZombieGameModeBase::HandlePlayerDeath(AActor* DeadActor, AActor* KillerActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[ZombieGameModeBase] Player eliminated! Ending match."));

	if (ActiveWaveManager)
	{
		ActiveWaveManager->TriggerGameOver();
	}
}
