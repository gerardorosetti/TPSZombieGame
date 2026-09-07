// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Combat/DroppedMagazine.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ADroppedMagazine::ADroppedMagazine()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Root collision box component for physics simulation
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	CollisionBox->InitBoxExtent(FVector(4.0f, 10.0f, 16.0f));
	CollisionBox->SetSimulatePhysics(true);
	CollisionBox->SetEnableGravity(true);
	CollisionBox->SetCollisionProfileName(TEXT("PhysicsActor"));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Ensure physics magazine never blocks player movement or camera/visibility raycasts
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// 2. Visual Magazine Mesh attached to physics box with NoCollision
	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	MagazineMesh->SetupAttachment(RootComponent);
	MagazineMesh->SetSimulatePhysics(false);
	MagazineMesh->SetCollisionProfileName(TEXT("NoCollision"));
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DespawnLifespan = 8.0f;
	InitialLifeSpan = 8.0f;
}

void ADroppedMagazine::InitializeDroppedMagazine(UStaticMesh* InMesh, const FVector& InitialImpulse)
{
	if (InMesh && MagazineMesh && CollisionBox)
	{
		MagazineMesh->SetStaticMesh(InMesh);

		const FBoxSphereBounds Bounds = InMesh->GetBounds();
		const FVector Extent = Bounds.BoxExtent;
		if (Extent.X > 0.5f && Extent.Y > 0.5f && Extent.Z > 0.5f)
		{
			CollisionBox->SetBoxExtent(Extent, true);
		}

		CollisionBox->AddImpulse(InitialImpulse, NAME_None, true);
		const FVector RandomSpin = FVector(
			FMath::RandRange(-90.0f, 90.0f),
			FMath::RandRange(-90.0f, 90.0f),
			FMath::RandRange(-90.0f, 90.0f)
		);
		CollisionBox->AddAngularImpulseInDegrees(RandomSpin, NAME_None, true);
	}

	SetLifeSpan(DespawnLifespan);
}
