// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Character/ZombieEnemyBase.h"
#include "ZombieGame/AI/ZombieAIController.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "ZombieGame/UI/CombatHUDWidget.h"
#include "ZombieGame/UI/GameOverWidget.h"
#include "Blueprint/UserWidget.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AZombieGameModeBase::AZombieGameModeBase()
{
	DefaultPawnClass = APlayerCharacter::StaticClass();
	PlayerStateClass = APlayerStateBase::StaticClass();
	WaveManagerClass = AZombieWaveManager::StaticClass();
	HUDWidgetClass = nullptr;
	GameOverWidgetClass = nullptr;
	RoundStartSound = nullptr;
	RoundEndSound = nullptr;
	NukeDetonationSound = nullptr;
	GameOverSound = nullptr;
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
		ZOMBIE_LOG(Log, TEXT("[ZombieGameModeBase] Auto-spawned active Wave Manager instance."));
	}

	if (ActiveWaveManager)
	{
		ActiveWaveManager->OnWaveStarted.AddDynamic(this, &AZombieGameModeBase::HandleWaveStarted);
		ActiveWaveManager->OnWaveStateChanged.AddDynamic(this, &AZombieGameModeBase::HandleWaveStateChanged);
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

	// 3. Create and mount Combat HUD
	if (!HUDWidgetClass)
	{
		if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			if (PlayerChar->GetHUDWidgetClass())
			{
				HUDWidgetClass = PlayerChar->GetHUDWidgetClass();
			}
		}
	}

	if (HUDWidgetClass && !HUDWidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (!ActiveHUDWidget)
			{
				ActiveHUDWidget = CreateWidget<UCombatHUDWidget>(PC, HUDWidgetClass);
				if (ActiveHUDWidget)
				{
					ActiveHUDWidget->AddToViewport(0);
					ZOMBIE_LOG(Log, TEXT("[ZombieGameModeBase] Mounted Combat HUD to viewport."));
				}
			}
		}
	}
	else
	{
		ZOMBIE_LOG(Warning, TEXT("[ZombieGameModeBase] No valid Combat HUD widget class available to mount."));
	}

	// 4. Ensure local player controller is in Game-Only input mode with cursor hidden
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AZombieGameModeBase::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AZombieGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstaKillTimerHandle);
	}

	if (ActiveWaveManager)
	{
		ActiveWaveManager->OnWaveStarted.RemoveDynamic(this, &AZombieGameModeBase::HandleWaveStarted);
		ActiveWaveManager->OnWaveStateChanged.RemoveDynamic(this, &AZombieGameModeBase::HandleWaveStateChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void AZombieGameModeBase::ActivateInstaKill(float Duration)
{
	bIsInstaKillActive = true;
	InstaKillDuration = Duration;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstaKillTimerHandle);
		World->GetTimerManager().SetTimer(
			InstaKillTimerHandle,
			this,
			&AZombieGameModeBase::DeactivateInstaKill,
			Duration,
			false
		);
	}

	OnInstaKillStateChanged.Broadcast(true, Duration);
	ZOMBIE_LOG(Log, TEXT("[ZombieGameModeBase] Insta-Kill activated for %f seconds!"), Duration);
}

void AZombieGameModeBase::DeactivateInstaKill()
{
	bIsInstaKillActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstaKillTimerHandle);
	}

	OnInstaKillStateChanged.Broadcast(false, 0.0f);
	ZOMBIE_LOG(Log, TEXT("[ZombieGameModeBase] Insta-Kill expired."));
}

float AZombieGameModeBase::GetInstaKillTimeRemaining() const
{
	if (!bIsInstaKillActive)
	{
		return 0.0f;
	}

	if (const UWorld* World = GetWorld())
	{
		return World->GetTimerManager().GetTimerRemaining(InstaKillTimerHandle);
	}

	return 0.0f;
}

void AZombieGameModeBase::TriggerNuke(APlayerCharacter* InstigatorPlayer)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AZombieEnemyBase*> LivingZombies;
	for (TActorIterator<AZombieEnemyBase> It(World); It; ++It)
	{
		if (IsValid(*It) && It->IsZombieAlive_Implementation())
		{
			LivingZombies.Add(*It);
		}
	}

	for (AZombieEnemyBase* Zombie : LivingZombies)
	{
		if (IsValid(Zombie) && Zombie->IsZombieAlive_Implementation())
		{
			FZombieDamageData NukeDamage;
			NukeDamage.BaseDamage = 99999.0f;
			NukeDamage.bIsHeadshot = false;
			NukeDamage.bIsNuke = true;
			NukeDamage.HitLocation = Zombie->GetActorLocation();
			NukeDamage.HitImpulse = FVector(0.0f, 0.0f, -600.0f);
			NukeDamage.DamageCauser = InstigatorPlayer;
			NukeDamage.InstigatedBy = InstigatorPlayer ? InstigatorPlayer->GetController() : nullptr;

			Zombie->TakeZombieDamage_Implementation(NukeDamage);
		}
	}

	// Award standard 400 pts bonus
	if (InstigatorPlayer)
	{
		if (APlayerStateBase* PS = InstigatorPlayer->GetPlayerState<APlayerStateBase>())
		{
			PS->AddPoints(400);
		}
	}
	else if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>())
		{
			PS->AddPoints(400);
		}
	}

	if (NukeDetonationSound)
	{
		UGameplayStatics::PlaySound2D(this, NukeDetonationSound);
	}

	ZOMBIE_LOG(Warning, TEXT("[ZombieGameModeBase] NUKE ACTIVATED! Eliminated %d zombies and awarded 400 pts."), LivingZombies.Num());
}

void AZombieGameModeBase::HandlePlayerDeath(AActor* DeadActor, AActor* KillerActor)
{
	ZOMBIE_LOG(Warning, TEXT("[ZombieGameModeBase] Player eliminated! Ending match."));

	// Notify all active zombie AI controllers that the player died to immediately cease attacks
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AZombieAIController> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				It->NotifyPlayerDied(DeadActor);
			}
		}
	}

	if (ActiveWaveManager)
	{
		ActiveWaveManager->TriggerGameOver();
	}

	if (GameOverSound)
	{
		UGameplayStatics::PlaySound2D(this, GameOverSound);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	if (GameOverWidgetClass && !ActiveGameOverWidget)
	{
		ActiveGameOverWidget = CreateWidget<UGameOverWidget>(PC, GameOverWidgetClass);
		if (ActiveGameOverWidget)
		{
			ActiveGameOverWidget->AddToViewport(100);

			if (APlayerStateBase* PS = PC->GetPlayerState<APlayerStateBase>())
			{
				const int32 Rounds = ActiveWaveManager ? ActiveWaveManager->GetCurrentWaveNumber() : PS->GetRoundsSurvived();
				ActiveGameOverWidget->SetupGameOverStats(
					Rounds,
					PS->GetTotalKills(),
					PS->GetTotalHeadshots(),
					PS->GetTotalScore()
				);
			}
		}
	}
}

void AZombieGameModeBase::HandleWaveStarted(int32 WaveNumber)
{
	if (RoundStartSound)
	{
		UGameplayStatics::PlaySound2D(this, RoundStartSound);
	}
}

void AZombieGameModeBase::HandleWaveStateChanged(EWaveState NewState)
{
	if (NewState == EWaveState::Intermission && RoundEndSound)
	{
		UGameplayStatics::PlaySound2D(this, RoundEndSound);
	}
}
