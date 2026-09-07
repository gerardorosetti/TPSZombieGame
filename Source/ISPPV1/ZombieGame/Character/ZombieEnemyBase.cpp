// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/ZombieEnemyBase.h"
#include "ZombieGame/Core/ZombieLog.h"
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
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Gameplay/PowerUpBase.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

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
		MoveComp->MaxWalkSpeed = BaseLocomotionSpeed; // Initial pace in cm/s (configured in Blueprint)
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

	// Apply initial baseline anim rate scale and walk speed from Blueprint defaults
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = BaseAnimRateScale;
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = BaseLocomotionSpeed;
	}

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

	ScheduleNextGroan();
}

void AZombieEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		World->GetTimerManager().ClearTimer(GroanTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AZombieEnemyBase::ScheduleNextGroan()
{
	if (!IsZombieAlive_Implementation())
	{
		return;
	}

	const float MinVal = FMath::Max(1.0f, MinGroanInterval);
	const float MaxVal = FMath::Max(MinVal + 0.5f, MaxGroanInterval);
	const float Interval = FMath::RandRange(MinVal, MaxVal);

	GetWorldTimerManager().SetTimer(GroanTimerHandle, this, &AZombieEnemyBase::PlayAmbientGroan, Interval, false);
}

void AZombieEnemyBase::PlayAmbientGroan()
{
	if (!IsZombieAlive_Implementation())
	{
		return;
	}

	if (GroanSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			GroanSound,
			GetActorLocation(),
			FRotator::ZeroRotator,
			1.0f,
			1.0f,
			0.0f,
			SpatialAttenuation
		);
	}

	ScheduleNextGroan();
}

float AZombieEnemyBase::TakeZombieDamage_Implementation(const FZombieDamageData& DamageData)
{
	// Cache incoming damage data to preserve directional impulse for ragdoll physics
	LastDamageReceived = DamageData;

	const float ActualDamage = Super::TakeZombieDamage_Implementation(DamageData);

	// Award hit points reward directly to the attacking player only if the target survived the shot and not killed by Nuke
	if (ActualDamage > 0.0f && IsZombieAlive_Implementation() && DamageData.InstigatedBy.IsValid() && !DamageData.bIsNuke)
	{
		if (APlayerStateBase* PS = DamageData.InstigatedBy->GetPlayerState<APlayerStateBase>())
		{
			PS->AddPoints(HitPointsReward);
		}
	}

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
	if (IsZombieAlive_Implementation())
	{
		if (HurtSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HurtSound,
				GetActorLocation(),
				FRotator::ZeroRotator,
				1.0f,
				1.0f,
				0.0f,
				SpatialAttenuation
			);
		}

		if (HitReactMontage && !bIsAttacking)
		{
			const float CurrentGlobalScale = (GetMesh() && GetMesh()->GlobalAnimRateScale > 0.01f) ? GetMesh()->GlobalAnimRateScale : 1.0f;
			PlayAnimMontage(HitReactMontage, 1.0f / CurrentGlobalScale);
		}
	}

	return ActualDamage;
}

void AZombieEnemyBase::HandleDeath(AActor* DeadActor, AActor* KillerActor)
{
	// Base class disables capsule collision, movement, and activates ragdoll physics
	Super::HandleDeath(DeadActor, KillerActor);

	// Restore normal anim rate scale for ragdoll/death state
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 1.0f;
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
	GetWorldTimerManager().ClearTimer(AttackDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackSwingFinishTimerHandle);
	GetWorldTimerManager().ClearTimer(GroanTimerHandle);
	bIsAttacking = false;

	// 3.1 Play 3D death screech
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			DeathSound,
			GetActorLocation(),
			FRotator::ZeroRotator,
			1.0f,
			1.0f,
			0.0f,
			SpatialAttenuation
		);
	}

	// 4. Award points to the killer (standard: +100 for headshot, +60 for body kill)
	AController* KillerController = LastDamageReceived.InstigatedBy.IsValid() ? LastDamageReceived.InstigatedBy.Get() : nullptr;
	if (!KillerController && KillerActor)
	{
		if (APawn* KillerPawn = Cast<APawn>(KillerActor))
		{
			KillerController = KillerPawn->GetController();
		}
	}

	if (KillerController)
	{
		if (APlayerStateBase* PS = KillerController->GetPlayerState<APlayerStateBase>())
		{
			if (LastDamageReceived.bIsNuke)
			{
				// Nuke eliminations award match bonus through GameMode, skipping per-zombie kill rewards
				PS->RecordKill(false);
			}
			else if (LastDamageReceived.bIsHeadshot)
			{
				PS->AddPoints(HeadshotPointsReward);
				PS->RecordKill(true);
			}
			else
			{
				PS->AddPoints(KillPointsReward);
				PS->RecordKill(false);
			}
		}
	}

	// 5. Spawn tactical power-up if drop chance succeeds (Weighted drop table, suppressed on Nuke)
	if (!LastDamageReceived.bIsNuke && PowerUpDropTable.Num() > 0 && FMath::FRand() <= PowerUpDropChance)
	{
		float TotalWeight = 0.0f;
		for (const FPowerUpDropEntry& Entry : PowerUpDropTable)
		{
			if (Entry.PowerUpClass && Entry.Weight > 0.0f)
			{
				TotalWeight += Entry.Weight;
			}
		}

		if (TotalWeight > 0.0f)
		{
			const float RolledWeight = FMath::FRandRange(0.0f, TotalWeight);
			float AccumulatedWeight = 0.0f;
			TSubclassOf<APowerUpBase> SelectedPowerUpClass = nullptr;

			for (const FPowerUpDropEntry& Entry : PowerUpDropTable)
			{
				if (Entry.PowerUpClass && Entry.Weight > 0.0f)
				{
					AccumulatedWeight += Entry.Weight;
					if (RolledWeight <= AccumulatedWeight)
					{
						SelectedPowerUpClass = Entry.PowerUpClass;
						break;
					}
				}
			}

			if (SelectedPowerUpClass && GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				const FVector DropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
				GetWorld()->SpawnActor<APowerUpBase>(SelectedPowerUpClass, DropLocation, FRotator::ZeroRotator, SpawnParams);

				ZOMBIE_LOG(Log, TEXT("[%s] Dropped power-up '%s' (Rolled weight %0.2f / %0.2f)"),
					*GetName(), *SelectedPowerUpClass->GetName(), RolledWeight, TotalWeight);
			}
		}
	}

	// 6. Notify observers (Wave manager, sound, score subsystems)
	OnZombieDeath.Broadcast(this);

	// 7. Schedule corpse destruction to free memory
	SetLifeSpan(CorpseLifespan);
}

void AZombieEnemyBase::InitializeZombieRoundStats(int32 RoundNumber)
{
	const int32 EffectiveRound = FMath::Max(1, RoundNumber);

	// 1. Health Scaling Formula
	// Round 1 = 100 HP, increases by 100 HP per round up to Round 9 (900 HP)
	// Round 10+ scales exponentially at 1.1x per round
	float ScaledHealth = 100.0f;
	if (EffectiveRound <= 9)
	{
		ScaledHealth = 100.0f + (EffectiveRound - 1) * 100.0f;
	}
	else
	{
		ScaledHealth = 900.0f * FMath::Pow(1.1f, static_cast<float>(EffectiveRound - 9));
	}

	if (HealthComponent)
	{
		HealthComponent->SetMaxHealth(ScaledHealth, true);
	}

	// 2. Locomotion Speed & Animation Scaling (Data-Driven from Blueprint Base Values)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MoveComp)
	{
		const float EffectiveBaseSpeed = FMath::Max(10.0f, BaseLocomotionSpeed);
		const float EffectiveBaseAnimRate = FMath::Max(0.1f, BaseAnimRateScale);

		// Calculate speed multiplier based on round progression
		float SpeedMultiplier = 1.0f;
		if (EffectiveRound <= 1)
		{
			SpeedMultiplier = 1.0f; // Round 1: Exact baseline from Blueprint
		}
		else if (EffectiveRound == 2)
		{
			SpeedMultiplier = 1.2f;
		}
		else if (EffectiveRound <= 4)
		{
			// Mix of baseline walkers and brisk trotters
			SpeedMultiplier = (FMath::FRand() < 0.6f) ? 1.25f : 1.75f;
		}
		else if (EffectiveRound <= 7)
		{
			// Mix of brisk trotters and runners
			SpeedMultiplier = (FMath::FRand() < 0.4f) ? 1.75f : 2.5f;
		}
		else
		{
			// High rounds: sprinters up to max multiplier/cap
			SpeedMultiplier = (FMath::FRand() < 0.3f) ? 2.2f : MaxSpeedMultiplier;
		}

		// Enforce multiplier clamp and absolute speed cap
		SpeedMultiplier = FMath::Clamp(SpeedMultiplier, 1.0f, MaxSpeedMultiplier);
		float NewSpeed = EffectiveBaseSpeed * SpeedMultiplier;
		if (MaxSpeedCap > 0.0f && NewSpeed > MaxSpeedCap)
		{
			NewSpeed = MaxSpeedCap;
			SpeedMultiplier = NewSpeed / EffectiveBaseSpeed;
		}

		MoveComp->MaxWalkSpeed = NewSpeed;

		// Proportionally scale animation playback rate from the Blueprint's BaseAnimRateScale
		const float ProportionalAnimRate = FMath::Clamp(EffectiveBaseAnimRate * SpeedMultiplier, 0.5f, MaxAnimRateCap);
		if (MeshComp)
		{
			MeshComp->GlobalAnimRateScale = ProportionalAnimRate;
		}

		ZOMBIE_LOG(Log, TEXT("[%s] Scaled for Round %d: BaseSpeed=%0.1f -> NewSpeed=%0.1f (%0.2fx), BaseAnimRate=%0.2fx -> NewAnimRate=%0.2fx (Cap=%0.1f, MaxAnimCap=%0.1f)"),
			*GetName(), EffectiveRound, EffectiveBaseSpeed, NewSpeed, SpeedMultiplier, EffectiveBaseAnimRate, ProportionalAnimRate, MaxSpeedCap, MaxAnimRateCap);
	}
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

void AZombieEnemyBase::StopAttackAndReset()
{
	bIsAttacking = false;

	if (AttackMontage)
	{
		StopAnimMontage(AttackMontage);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		World->GetTimerManager().ClearTimer(AttackDamageTimerHandle);
		World->GetTimerManager().ClearTimer(AttackSwingFinishTimerHandle);
	}
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

	// Play attack vocalization and swipe whoosh with 3D attenuation
	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
	}
	if (AttackWhooshSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackWhooshSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
	}

	float AttackAnimDuration = 1.2f;
	if (AttackMontage)
	{
		const float CurrentGlobalScale = (GetMesh() && GetMesh()->GlobalAnimRateScale > 0.01f) ? GetMesh()->GlobalAnimRateScale : 1.0f;
		// Cancel global anim rate scale on the attack montage so attack swing remains standard 1.0x rate
		const float NormalizedMontageRate = 1.0f / CurrentGlobalScale;
		AttackAnimDuration = PlayAnimMontage(AttackMontage, NormalizedMontageRate);
	}

	// Schedule damage delivery mid-swing when attack reaches apex
	const float DamageDelay = AttackAnimDuration * FMath::Clamp(AttackDamageFraction, 0.1f, 0.9f);
	GetWorldTimerManager().SetTimer(
		AttackDamageTimerHandle,
		this,
		&AZombieEnemyBase::ApplyMeleeDamage,
		DamageDelay,
		false
	);

	// Reset bIsAttacking as soon as the swing montage finishes
	GetWorldTimerManager().SetTimer(
		AttackSwingFinishTimerHandle,
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
	TArray<FHitResult> HitResults;

	const bool bHit = World->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Pawn,
		AttackSphere,
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* TargetActor = Hit.GetActor();
			// STRICT FILTER: Zero friendly fire! Only damage APlayerCharacter, never fellow zombies or power-ups!
			if (TargetActor && TargetActor != this && TargetActor->IsA<APlayerCharacter>() && TargetActor->Implements<UZombieDamageableInterface>())
			{
				// Target must be alive: Never hit or damage dead player corpses!
				if (!IZombieDamageableInterface::Execute_IsZombieAlive(TargetActor))
				{
					continue;
				}

				FZombieDamageData DamageData;
				DamageData.BaseDamage = AttackDamage;
				DamageData.HitLocation = Hit.ImpactPoint;
				DamageData.HitBoneName = Hit.BoneName;
				DamageData.HitImpulse = GetActorForwardVector() * 1000.0f;
				DamageData.DamageCauser = this;
				DamageData.InstigatedBy = GetController();
				DamageData.bIsHeadshot = false;

				const float DealtDamage = IZombieDamageableInterface::Execute_TakeZombieDamage(TargetActor, DamageData);
				ZOMBIE_LOG(Log, TEXT("[%s] Melee hit player %s for %f damage!"),
					*GetName(), *TargetActor->GetName(), DealtDamage);
				break; // Deliver damage to player once per swing
			}
		}
	}
}
