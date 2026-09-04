// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "Zombies/Gameplay/ZombieDamageableDummy.h"
#include "Zombies/Character/ZombieHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AZombieDamageableDummy::AZombieDamageableDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Root mesh setup
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	RootComponent = MeshComponent;

	// Set a default basic cube or cylinder for rapid visual feedback
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DefaultMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMesh.Object);
	}
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	MeshComponent->SetGenerateOverlapEvents(true);

	// 2. Attach reusable Health Component
	HealthComponent = CreateDefaultSubobject<UZombieHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 150.0f;
	HealthComponent->MaxShield = 50.0f;
	HealthComponent->HeadshotMultiplier = 2.5f;
}

void AZombieDamageableDummy::BeginPlay()
{
	Super::BeginPlay();

	// Bind Observer delegates
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AZombieDamageableDummy::HandleHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &AZombieDamageableDummy::HandleDeath);
	}
}

void AZombieDamageableDummy::HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData)
{
	UE_LOG(LogTemp, Log, TEXT("[Dummy Target] Hit registered! Delta: %f | Current Health: %f/%f (Headshot: %s)"),
		HealthDelta, CurrentHealth, MaxHealth, DamageData.bIsHeadshot ? TEXT("YES") : TEXT("NO"));

	// Quick visual feedback: slight impulse or bounce
	if (MeshComponent && !DamageData.HitImpulse.IsNearlyZero())
	{
		MeshComponent->AddImpulse(DamageData.HitImpulse * 0.5f, NAME_None, true);
	}
}

void AZombieDamageableDummy::HandleDeath(AActor* DeadActor, AActor* KillerActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[Dummy Target] DESTROYED by: %s"), KillerActor ? *KillerActor->GetName() : TEXT("Unknown"));

	// Enable physics to collapse on death
	if (MeshComponent)
	{
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->AddImpulse(FVector(0.0f, 0.0f, 300.0f), NAME_None, true);
	}

	// Destroy after a short delay to allow physics reaction
	SetLifeSpan(3.0f);
}

float AZombieDamageableDummy::TakeZombieDamage_Implementation(const FZombieDamageData& DamageData)
{
	if (HealthComponent)
	{
		return HealthComponent->ProcessDamage(DamageData);
	}
	return 0.0f;
}

void AZombieDamageableDummy::ApplyZombieHealing_Implementation(float HealAmount, AActor* Healer)
{
	if (HealthComponent)
	{
		HealthComponent->Heal(HealAmount);
	}
}

bool AZombieDamageableDummy::IsZombieAlive_Implementation() const
{
	return HealthComponent ? HealthComponent->IsAlive() : false;
}

float AZombieDamageableDummy::GetHealthPercent_Implementation() const
{
	return HealthComponent ? HealthComponent->GetHealthPercent() : 0.0f;
}
