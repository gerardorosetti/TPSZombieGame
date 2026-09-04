// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/ZombieEnemyBase.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "ZombieGame/AI/ZombieAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Perception/AISense_Damage.h"

AZombieEnemyBase::AZombieEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Configure AI Controller defaults
	AIControllerClass = AZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 2. Configure Vitals & Health parameters
	if (HealthComponent)
	{
		HealthComponent->MaxHealth = 100.0f;
		HealthComponent->MaxShield = 0.0f;
		HealthComponent->HeadshotMultiplier = 2.5f;
	}

	// Disable controller rotation snapping
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 3. Configure Zombie Movement (Shambling / Walking Pace)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator(0.0f, TurnRate, 0.0f);
		MoveComp->MaxWalkSpeed = 130.0f;       // Realistic zombie shambling pace in cm/s
		MoveComp->MaxAcceleration = 800.0f;
		MoveComp->BrakingDecelerationWalking = 1000.0f;
	}

	// 4. Configure Skeletal Mesh Collision for Ragdoll & Hit Detection
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComp->SetCollisionObjectType(ECC_Pawn);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// 5. Configure Capsule Component for Movement & Navigation (ignores bullets so hits test skeleton)
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->InitCapsuleSize(38.0f, 90.0f);
		CapsuleComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
}

void AZombieEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// Ensure movement rotation settings are applied even if Blueprint had serialized defaults
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
		const float EffectiveTurnRate = (TurnRate > 10.0f) ? TurnRate : 180.0f;
		MoveComp->RotationRate = FRotator(0.0f, EffectiveTurnRate, 0.0f);
	}

	// Ensure bullet collision hits the anatomical skeleton (Physics Asset) rather than the oversized capsule
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionObjectType(ECC_Pawn);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	AttackRange = 115.0f;
	AttackRadius = 40.0f;
	AttackCooldown = 0.8f;
}

float AZombieEnemyBase::TakeZombieDamage_Implementation(const FZombieDamageData& DamageData)
{
	// Cache incoming damage data to preserve directional impulse for ragdoll physics
	LastDamageReceived = DamageData;

	const float ActualDamage = Super::TakeZombieDamage_Implementation(DamageData);

	// Alert AI controller immediately to the attacker (instant aggro when shot)
	if (IsZombieAlive_Implementation())
	{
		AActor* Attacker = DamageData.DamageCauser.IsValid() ? DamageData.DamageCauser.Get() : (DamageData.InstigatedBy.IsValid() ? DamageData.InstigatedBy->GetPawn() : nullptr);
		if (AZombieAIController* AIC = Cast<AZombieAIController>(GetController()))
		{
			AIC->NotifyDamageReceived(Attacker);
		}

		if (Attacker)
		{
			UAISense_Damage::ReportDamageEvent(
				GetWorld(),
				this,
				Attacker,
				ActualDamage,
				DamageData.HitLocation,
				DamageData.HitLocation
			);
		}
	}

	// Play flinch / hit reaction montage if alive and not currently executing an attack swing
	if (IsZombieAlive_Implementation() && HitReactMontage && !bIsAttacking)
	{
		PlayAnimMontage(HitReactMontage);
	}

	return ActualDamage;
}

void AZombieEnemyBase::HandleDeath(AActor* DeadActor, AActor* KillerActor)
{
	// Base class disables capsule collision, movement, and activates ragdoll physics
	Super::HandleDeath(DeadActor, KillerActor);

	// 1. Apply lethal bullet's directional ballistic impulse directly to the struck bone
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (!LastDamageReceived.HitImpulse.IsNearlyZero())
		{
			MeshComp->AddImpulseAtLocation(
				LastDamageReceived.HitImpulse,
				LastDamageReceived.HitLocation,
				LastDamageReceived.HitBoneName
			);
		}
	}

	// 2. Unpossess and detach AI controller
	DetachFromControllerPendingDestroy();

	// 3. Clear active timers
	GetWorldTimerManager().ClearTimer(AttackCooldownTimerHandle);
	bIsAttacking = false;

	// 4. Notify observers (Wave manager, sound, score subsystems)
	OnZombieDeath.Broadcast(this);

	// 5. Schedule corpse destruction to free memory
	SetLifeSpan(CorpseLifespan);
}

void AZombieEnemyBase::OnDeathStarted(AActor* Killer)
{
	Super::OnDeathStarted(Killer);
}

bool AZombieEnemyBase::CanAttack() const
{
	UWorld* World = GetWorld();
	const bool bCooldownActive = World && World->GetTimerManager().IsTimerActive(AttackCooldownTimerHandle);
	return IsZombieAlive_Implementation() && !bIsAttacking && !bCooldownActive;
}

void AZombieEnemyBase::PerformAttack()
{
	if (!CanAttack())
	{
		return;
	}

	bIsAttacking = true;

	// Halt movement immediately so feet stay planted during attack swing
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	OnZombieAttack.Broadcast();

	float AttackAnimDuration = 1.2f;
	if (AttackMontage)
	{
		AttackAnimDuration = PlayAnimMontage(AttackMontage);
	}

	// Schedule damage delivery mid-swing (e.g. 0.4s into animation)
	FTimerHandle DamageTimerHandle;
	GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		this,
		&AZombieEnemyBase::ApplyMeleeDamage,
		FMath::Min(0.45f, AttackAnimDuration * 0.4f),
		false
	);

	// Reset bIsAttacking as soon as the swing montage finishes
	FTimerHandle SwingFinishTimerHandle;
	GetWorldTimerManager().SetTimer(
		SwingFinishTimerHandle,
		[this]()
		{
			bIsAttacking = false;
		},
		AttackAnimDuration,
		false
	);

	// Start cooldown timer (active for AttackAnimDuration + AttackCooldown)
	GetWorldTimerManager().SetTimer(
		AttackCooldownTimerHandle,
		AttackAnimDuration + AttackCooldown,
		false
	);
}

void AZombieEnemyBase::ApplyMeleeDamage()
{
	if (!IsZombieAlive_Implementation())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Sphere sweep in front of the zombie to hit the player
	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * AttackRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionShape AttackSphere = FCollisionShape::MakeSphere(AttackRadius);
	FHitResult HitResult;

	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Pawn,
		AttackSphere,
		QueryParams
	);

	if (bHit && HitResult.GetActor())
	{
		AActor* TargetActor = HitResult.GetActor();
		if (TargetActor != this && TargetActor->Implements<UZombieDamageableInterface>())
		{
			FZombieDamageData DamageData;
			DamageData.BaseDamage = AttackDamage;
			DamageData.HitLocation = HitResult.ImpactPoint;
			DamageData.HitBoneName = HitResult.BoneName;
			DamageData.HitImpulse = GetActorForwardVector() * 1000.0f;
			DamageData.DamageCauser = this;
			DamageData.InstigatedBy = GetController();
			DamageData.bIsHeadshot = false;

			const float DealtDamage = IZombieDamageableInterface::Execute_TakeZombieDamage(TargetActor, DamageData);
			UE_LOG(LogTemp, Log, TEXT("[%s] Melee hit %s for %f damage!"),
				*GetName(), *TargetActor->GetName(), DealtDamage);
		}
	}
}
