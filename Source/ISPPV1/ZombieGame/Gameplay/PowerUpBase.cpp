// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/PowerUpBase.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

APowerUpBase::APowerUpBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Scene Root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 2. Overlap Trigger Sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(65.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);

	// 3. 3D Visual Mesh (Configured in Blueprint Details)
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
}

void APowerUpBase::BeginPlay()
{
	Super::BeginPlay();

	RemainingLifetime = TotalLifetime;
	RunningTime = 0.0f;
	bIsCollected = false;

	if (PickupMesh)
	{
		BaseMeshRelativeLocation = PickupMesh->GetRelativeLocation();
	}

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &APowerUpBase::OnOverlapBegin);
	}

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
	}
}

void APowerUpBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RunningTime += DeltaTime;
	RemainingLifetime -= DeltaTime;

	// 1. 3D Rotation and Bobbing animation
	if (PickupMesh)
	{
		PickupMesh->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

		FVector BobbingLoc = BaseMeshRelativeLocation;
		BobbingLoc.Z += FMath::Sin(RunningTime * BobbingSpeed) * BobbingAmplitude;
		PickupMesh->SetRelativeLocation(BobbingLoc);

		// 2. Expiration Flash Warning (rapidly toggle visibility in the last seconds)
		if (RemainingLifetime <= FlashWarningTime && RemainingLifetime > 0.0f)
		{
			const bool bBlinkVisible = (FMath::Fmod(RemainingLifetime, 0.25f) > 0.12f);
			PickupMesh->SetVisibility(bBlinkVisible);
		}
	}

	// 3. Despawn when lifetime runs out
	if (RemainingLifetime <= 0.0f && !bIsCollected)
	{
		Destroy();
	}
}

void APowerUpBase::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bIsCollected || !OtherActor)
	{
		return;
	}

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		bIsCollected = true;
		ApplyPowerUpEffect(Player);
	}
}

void APowerUpBase::ApplyPowerUpEffect(APlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	if (CollectSound)
	{
		if (SpatialAttenuation)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CollectSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
		}
		else
		{
			UGameplayStatics::PlaySound2D(this, CollectSound);
		}
	}

	UWorld* World = GetWorld();

	switch (PowerUpType)
	{
	case EPowerUpType::MaxAmmo:
		if (UCombatComponent* CombatComp = Player->GetCombatComponent())
		{
			CombatComp->RefillAllWeaponsAmmo();
			ZOMBIE_LOG(Log, TEXT("[PowerUp] MAX AMMO applied to player!"));
		}
		break;

	case EPowerUpType::InstaKill:
		if (World)
		{
			if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
			{
				GM->ActivateInstaKill(30.0f);
				ZOMBIE_LOG(Log, TEXT("[PowerUp] INSTA-KILL activated for 30 seconds!"));
			}
		}
		break;

	case EPowerUpType::DoublePoints:
		if (APlayerStateBase* PS = Player->GetPlayerState<APlayerStateBase>())
		{
			PS->ActivateDoublePoints(30.0f);
			ZOMBIE_LOG(Log, TEXT("[PowerUp] DOUBLE POINTS activated for 30 seconds!"));
		}
		break;

	case EPowerUpType::Nuke:
		if (World)
		{
			if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
			{
				GM->TriggerNuke(Player);
				ZOMBIE_LOG(Log, TEXT("[PowerUp] NUKE detonated!"));
			}
		}
		break;

	default:
		break;
	}

	OnPowerUpCollected.Broadcast(PowerUpType, Player);
	Destroy();
}
