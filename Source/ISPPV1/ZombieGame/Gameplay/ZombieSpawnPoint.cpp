// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/ZombieSpawnPoint.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "NavigationSystem.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
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

	bIsDormant = false;
	bIsActive = true;
	SpawnRadius = 200.0f;
}

void AZombieSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	// Check if this spawner is assigned to an unlockable zone (e.g. "GreenBlock")
	bool bHasZone = !ZoneName.IsNone();
#if WITH_EDITOR
	if (!bHasZone)
	{
		const FString Label = GetActorLabel().TrimStartAndEnd();
		// Only inherit custom designer labels (e.g. "GreenBlock") and NEVER standard blueprint labels
		if (!Label.IsEmpty() 
			&& !Label.StartsWith(TEXT("BP_ZombieSpawnPoint"), ESearchCase::IgnoreCase)
			&& !Label.StartsWith(TEXT("ZombieSpawnPoint"), ESearchCase::IgnoreCase))
		{
			ZoneName = FName(*Label);
			bHasZone = true;
		}
	}
#endif

	// If assigned to an unlockable zone or explicitly marked as dormant, start inactive
	if (bIsDormant || bHasZone)
	{
		bIsActive = false;
	}
	else
	{
		bIsActive = true;
	}

	// Auto-register with active WaveManager
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AZombieWaveManager> It(World); It; ++It)
		{
			It->RegisterSpawnPoint(this);
		}
	}

	ZOMBIE_LOG(Log, TEXT("[ZombieSpawnPoint] Spawner '%s' (Zone: '%s') initialized. bIsDormant=%s, bHasZone=%s -> Active=%s"),
		*GetName(), *ZoneName.ToString(),
		bIsDormant ? TEXT("TRUE") : TEXT("FALSE"),
		bHasZone ? TEXT("TRUE") : TEXT("FALSE"),
		bIsActive ? TEXT("TRUE") : TEXT("FALSE"));
}

bool AZombieSpawnPoint::MatchesZone(const FName& InZone) const
{
	if (InZone.IsNone())
	{
		return false;
	}

	const FString QueryStr = InZone.ToString().TrimStartAndEnd();

	if (!ZoneName.IsNone())
	{
		const FString MyZoneStr = ZoneName.ToString().TrimStartAndEnd();
		if (MyZoneStr.Equals(QueryStr, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

#if WITH_EDITOR
	const FString LabelStr = GetActorLabel().TrimStartAndEnd();
	if (!LabelStr.StartsWith(TEXT("BP_ZombieSpawnPoint"), ESearchCase::IgnoreCase)
		&& !LabelStr.StartsWith(TEXT("ZombieSpawnPoint"), ESearchCase::IgnoreCase))
	{
		if (LabelStr.Equals(QueryStr, ESearchCase::IgnoreCase) || LabelStr.Contains(QueryStr, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
#endif

	return false;
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
