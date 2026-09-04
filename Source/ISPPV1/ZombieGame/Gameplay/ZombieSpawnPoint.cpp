// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/ZombieSpawnPoint.h"
#include "NavigationSystem.h"
#include "Components/SceneComponent.h"
#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#endif

AZombieSpawnPoint::AZombieSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(RootComponent);
		EditorBillboard->bIsScreenSizeScaled = true;
	}

	DirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	if (DirectionArrow)
	{
		DirectionArrow->SetupAttachment(RootComponent);
		DirectionArrow->ArrowColor = FColor::Red;
		DirectionArrow->ArrowSize = 1.0f;
	}
#endif

	bIsActive = true;
	SpawnRadius = 100.0f;
}

bool AZombieSpawnPoint::GetValidSpawnLocation(FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutLocation = GetActorLocation();
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		OutLocation = GetActorLocation();
		return true;
	}

	FNavLocation NavLocation;
	const bool bProjected = NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation);
	if (bProjected)
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	// Fallback to actor origin if query failed
	OutLocation = GetActorLocation();
	return false;
}
