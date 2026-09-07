// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/WallBuyStation.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Gameplay/PowerUpBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

AWallBuyStation::AWallBuyStation()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Root Scene
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 2. Wall Mount Frame / Plaque (Configured in Blueprint Details)
	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetupAttachment(RootComponent);
	StationMesh->SetCollisionProfileName(TEXT("BlockAll"));
	StationMesh->SetRelativeScale3D(FVector(0.1f, 1.2f, 0.8f)); // Wall plaque scale

	// 3. Item 3D Preview (Configured in Blueprint Details)
	ItemPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemPreviewMesh"));
	ItemPreviewMesh->SetupAttachment(RootComponent);
	ItemPreviewMesh->SetRelativeLocation(FVector(15.0f, 0.0f, 0.0f));
	ItemPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemPreviewMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.35f));
	ItemPreviewMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));

	// 4. Interaction Trigger Box
	InteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(RootComponent);
	InteractionTrigger->SetBoxExtent(FVector(80.0f, 100.0f, 80.0f));
	InteractionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionTrigger->SetGenerateOverlapEvents(true);

	// Default fallback power up class
	PowerUpClass = APowerUpBase::StaticClass();
	PromptText = FText::FromString(TEXT("Buy Ammo"));
}

void AWallBuyStation::BeginPlay()
{
	Super::BeginPlay();

	// Set dynamic defaults based on purchase type if not overridden in instance
	if (PromptText.IsEmpty())
	{
		switch (BuyType)
		{
		case EWallBuyType::AmmoRestock:
			PromptText = FText::FromString(TEXT("Buy Ammo"));
			break;
		case EWallBuyType::PowerUpMaxAmmo:
			PromptText = FText::FromString(TEXT("Buy Max Ammo"));
			break;
		case EWallBuyType::PowerUpInstaKill:
			PromptText = FText::FromString(TEXT("Buy Insta-Kill"));
			break;
		case EWallBuyType::PowerUpDoublePoints:
			PromptText = FText::FromString(TEXT("Buy Double Points"));
			break;
		case EWallBuyType::PowerUpNuke:
			PromptText = FText::FromString(TEXT("Buy Nuke"));
			break;
		case EWallBuyType::CustomPowerUpClass:
			PromptText = FText::FromString(TEXT("Buy Power-Up"));
			break;
		}
	}
}

FText AWallBuyStation::GetInteractionPrompt_Implementation() const
{
	return PromptText;
}

int32 AWallBuyStation::GetInteractionCost_Implementation() const
{
	return Cost;
}

bool AWallBuyStation::CanInteract_Implementation(const APlayerCharacter* InstigatorPlayer) const
{
	if (!InstigatorPlayer)
	{
		return false;
	}

	if (const APlayerStateBase* PS = InstigatorPlayer->GetPlayerState<APlayerStateBase>())
	{
		return PS->GetCurrentPoints() >= Cost;
	}

	return false;
}

bool AWallBuyStation::Interact_Implementation(APlayerCharacter* InstigatorPlayer)
{
	if (!InstigatorPlayer)
	{
		return false;
	}

	APlayerStateBase* PS = InstigatorPlayer->GetPlayerState<APlayerStateBase>();
	if (!PS || !PS->SpendPoints(Cost))
	{
		if (PurchaseFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PurchaseFailSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
		}
		return false;
	}

	if (PurchaseSuccessSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PurchaseSuccessSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
	}

	UWorld* World = GetWorld();

	switch (BuyType)
	{
	case EWallBuyType::AmmoRestock:
		if (UCombatComponent* CombatComp = InstigatorPlayer->GetCombatComponent())
		{
			if (AWeaponBase* Weapon = CombatComp->GetCurrentWeapon())
			{
				Weapon->RefillAmmo(false, true); // Restock reserve ammo
				ZOMBIE_LOG(Log, TEXT("[WallBuy] Restocked reserve ammo for player."));
			}
		}
		break;

	case EWallBuyType::PowerUpMaxAmmo:
		if (bActivateImmediately)
		{
			if (UCombatComponent* CombatComp = InstigatorPlayer->GetCombatComponent())
			{
				CombatComp->RefillAllWeaponsAmmo();
				ZOMBIE_LOG(Log, TEXT("[WallBuy] Instantly activated Max Ammo for player."));
			}
		}
		else
		{
			SpawnPowerUpPickup(EPowerUpType::MaxAmmo);
		}
		break;

	case EWallBuyType::PowerUpInstaKill:
		if (bActivateImmediately)
		{
			if (World)
			{
				if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
				{
					GM->ActivateInstaKill(30.0f);
					ZOMBIE_LOG(Log, TEXT("[WallBuy] Instantly activated Insta-Kill mode for 30 seconds."));
				}
			}
		}
		else
		{
			SpawnPowerUpPickup(EPowerUpType::InstaKill);
		}
		break;

	case EWallBuyType::PowerUpDoublePoints:
		if (bActivateImmediately)
		{
			PS->ActivateDoublePoints(30.0f);
			ZOMBIE_LOG(Log, TEXT("[WallBuy] Instantly activated Double Points for 30 seconds."));
		}
		else
		{
			SpawnPowerUpPickup(EPowerUpType::DoublePoints);
		}
		break;

	case EWallBuyType::PowerUpNuke:
		if (bActivateImmediately)
		{
			if (World)
			{
				if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
				{
					GM->TriggerNuke(InstigatorPlayer);
					ZOMBIE_LOG(Log, TEXT("[WallBuy] Instantly detonated Nuke."));
				}
			}
		}
		else
		{
			SpawnPowerUpPickup(EPowerUpType::Nuke);
		}
		break;

	case EWallBuyType::CustomPowerUpClass:
		if (World && PowerUpClass)
		{
			if (bActivateImmediately)
			{
				const FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * 50.0f) + FVector(0.0f, 0.0f, -20.0f);
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				if (APowerUpBase* SpawnedPU = World->SpawnActor<APowerUpBase>(PowerUpClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams))
				{
					SpawnedPU->ApplyPowerUpEffect(InstigatorPlayer);
					ZOMBIE_LOG(Log, TEXT("[WallBuy] Instantly activated custom power-up effect."));
				}
			}
			else
			{
				SpawnPowerUpPickup(EPowerUpType::MaxAmmo);
			}
		}
		break;
	}

	OnWallBuyPurchased.Broadcast(this, InstigatorPlayer);
	return true;
}

void AWallBuyStation::SpawnPowerUpPickup(EPowerUpType InType)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<APowerUpBase> ClassToSpawn = PowerUpClass ? PowerUpClass : TSubclassOf<APowerUpBase>(APowerUpBase::StaticClass());
	const FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * 50.0f) + FVector(0.0f, 0.0f, -20.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (APowerUpBase* SpawnedPU = World->SpawnActor<APowerUpBase>(ClassToSpawn, SpawnLoc, FRotator::ZeroRotator, SpawnParams))
	{
		SpawnedPU->PowerUpType = InType;
		ZOMBIE_LOG(Log, TEXT("[WallBuy] Dispensed physical power-up pickup from station."));
	}
}
