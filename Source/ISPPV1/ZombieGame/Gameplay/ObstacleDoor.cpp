// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/ObstacleDoor.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Gameplay/ZombieSpawnPoint.h"
#include "ZombieGame/AI/ZombieAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

AObstacleDoor::AObstacleDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Root Scene Component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 2. Physical Barrier Mesh (Assigned in Blueprint or level details)
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(RootComponent);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	DoorMesh->SetMobility(EComponentMobility::Movable);
	DoorMesh->SetRelativeScale3D(FVector(0.3f, 2.5f, 2.5f)); // Standard doorway proportions

	// 3. Interaction Trigger Volume
	InteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(RootComponent);
	InteractionTrigger->SetBoxExtent(FVector(100.0f, 150.0f, 150.0f));
	InteractionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionTrigger->SetGenerateOverlapEvents(true);

	// Default text prompt
	PromptText = FText::FromString(TEXT("Clear Debris"));
}

void AObstacleDoor::BeginPlay()
{
	Super::BeginPlay();

	// Ensure all child barrier components block interaction traces, player movement, and navigation
	if (!bIsOpened)
	{
		TArray<UPrimitiveComponent*> PrimComps;
		GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim != InteractionTrigger)
			{
				Prim->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				Prim->SetCanEverAffectNavigation(true);
				Prim->bNavigationRelevant = true;
			}
		}
	}
}

FText AObstacleDoor::GetInteractionPrompt_Implementation() const
{
	return PromptText;
}

int32 AObstacleDoor::GetInteractionCost_Implementation() const
{
	return DoorCost;
}

bool AObstacleDoor::CanInteract_Implementation(const APlayerCharacter* InstigatorPlayer) const
{
	if (bIsOpened || !InstigatorPlayer)
	{
		return false;
	}

	if (const APlayerStateBase* PS = InstigatorPlayer->GetPlayerState<APlayerStateBase>())
	{
		return PS->GetCurrentPoints() >= DoorCost;
	}

	return false;
}

bool AObstacleDoor::Interact_Implementation(APlayerCharacter* InstigatorPlayer)
{
	if (bIsOpened || !InstigatorPlayer)
	{
		return false;
	}

	APlayerStateBase* PS = InstigatorPlayer->GetPlayerState<APlayerStateBase>();
	if (!PS || !PS->SpendPoints(DoorCost))
	{
		return false;
	}

	OpenDoor(InstigatorPlayer);
	return true;
}

void AObstacleDoor::OpenDoor(APlayerCharacter* InstigatorPlayer)
{
	if (bIsOpened)
	{
		return;
	}

	bIsOpened = true;

	// 1. Calculate combined bounding box of all barrier components BEFORE disabling collision/hiding
	const FBox TotalDirtyBounds = GetComponentsBoundingBox(true).ExpandBy(300.0f);

	// 2. Iterate through all primitive components (supports multi-mesh doors, barrel stacks, debris)
	TArray<UPrimitiveComponent*> PrimComps;
	GetComponents<UPrimitiveComponent>(PrimComps);
	for (UPrimitiveComponent* Prim : PrimComps)
	{
		if (Prim && Prim != InteractionTrigger)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Prim->SetCollisionResponseToAllChannels(ECR_Ignore);
			Prim->SetCanEverAffectNavigation(false);
			Prim->bFillCollisionUnderneathForNavmesh = false;
			Prim->SetVisibility(false);
			if (Prim != RootComponent)
			{
				Prim->UnregisterComponent();
			}
		}
	}

	// Disable interaction trigger volume as well since barrier is opened
	if (InteractionTrigger)
	{
		InteractionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractionTrigger->SetGenerateOverlapEvents(false);
	}

	SetActorEnableCollision(false);

	// 3. Force rebuild and dirtying of NavMesh tiles across the entire combined barrier bounding box
	UWorld* World = GetWorld();
	if (World)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			NavSys->AddDirtyArea(TotalDirtyBounds, ENavigationDirtyFlag::All);
			NavSys->UpdateActorInNavOctree(*this);
		}
	}

	// 4. Unlock corresponding spawn point zone for dynamic wave escalation
	if (!ZoneToUnlock.IsNone())
	{
		if (World)
		{
			int32 ActivatedCount = 0;
			for (TActorIterator<AZombieSpawnPoint> It(World); It; ++It)
			{
				if (It->MatchesZone(ZoneToUnlock))
				{
					It->SetSpawnPointActive(true);
					ActivatedCount++;
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[ObstacleDoor] Unlocked zone '%s' - activated %d spawn points."),
				*ZoneToUnlock.ToString(), ActivatedCount);
		}
	}

	// 5. Force all active zombies in the world to immediately recalculate navigation path through the open doorway
	if (World)
	{
		for (TActorIterator<AZombieAIController> It(World); It; ++It)
		{
			It->ForceRepath();
		}
	}

	// Dispatch notification to listeners (Audio, HUD, Match State)
	OnDoorOpened.Broadcast(this);
}
