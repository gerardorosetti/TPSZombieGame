// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/BaseCharacter.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Instantiate the modular Health Component
	HealthComponent = CreateDefaultSubobject<UZombieHealthComponent>(TEXT("HealthComponent"));

	// 2. Configure mesh collision presets for hit detection
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetGenerateOverlapEvents(true);
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Bind Observer delegates to respond to health & death events
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ABaseCharacter::HandleHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &ABaseCharacter::HandleDeath);
	}
}

void ABaseCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData)
{
	// Log for instructor and student debugging
	UE_LOG(LogTemp, Verbose, TEXT("[%s] Health Changed: %f/%f (Delta: %f)"),
		*GetName(), CurrentHealth, MaxHealth, HealthDelta);
}

void ABaseCharacter::HandleDeath(AActor* DeadActor, AActor* KillerActor)
{
	UE_LOG(LogTemp, Log, TEXT("[%s] Died. Killer: %s"), *GetName(), KillerActor ? *KillerActor->GetName() : TEXT("Environment"));

	// 1. Disable Capsule Component to stop blocking navigation and other players
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// 2. Disable Character Movement
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->SetComponentTickEnabled(false);
	}

	// 3. Activate Ragdoll Physics on Skeletal Mesh
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
		MeshComp->WakeAllRigidBodies();
		MeshComp->bBlendPhysics = true;
	}

	// 4. Trigger virtual hook for subclass-specific cleanup
	OnDeathStarted(KillerActor);
}

void ABaseCharacter::OnDeathStarted(AActor* Killer)
{
	// Base implementation sets a lifespan before despawning the ragdoll corpse
	SetLifeSpan(5.0f);
}

float ABaseCharacter::TakeZombieDamage_Implementation(const FZombieDamageData& DamageData)
{
	if (HealthComponent)
	{
		return HealthComponent->ProcessDamage(DamageData);
	}
	return 0.0f;
}

void ABaseCharacter::ApplyZombieHealing_Implementation(float HealAmount, AActor* Healer)
{
	if (HealthComponent)
	{
		HealthComponent->Heal(HealAmount);
	}
}

bool ABaseCharacter::IsZombieAlive_Implementation() const
{
	return HealthComponent ? HealthComponent->IsAlive() : false;
}

float ABaseCharacter::GetHealthPercent_Implementation() const
{
	return HealthComponent ? HealthComponent->GetHealthPercent() : 0.0f;
}
