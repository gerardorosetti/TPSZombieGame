// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Combat/WeaponBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "CollisionShape.h"
#include "Animation/AnimMontage.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Root Scene Component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	// 2. Visual Meshes (Supports both animated Skeletal Meshes and prototype Static Meshes)
	WeaponSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMesh"));
	WeaponSkeletalMesh->SetupAttachment(RootComponent);
	WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));
	WeaponStaticMesh->SetupAttachment(RootComponent);
	WeaponStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Setup prototype mesh proportions (sleek rifle barrel)
	WeaponStaticMesh->SetRelativeLocation(FVector(2.0f, 6.0f, -1.0f));
	WeaponStaticMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	WeaponStaticMesh->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.40f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (DefaultMeshFinder.Succeeded())
	{
		WeaponStaticMesh->SetStaticMesh(DefaultMeshFinder.Object);
	}
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ammunition counters from configuration
	CurrentMagAmmo = WeaponConfig.MagCapacity;
	CurrentReserveAmmo = WeaponConfig.MaxReserveAmmo;

	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);
}

void AWeaponBase::StartFire()
{
	if (!CanFire())
	{
		if (CurrentMagAmmo == 0 && CurrentReserveAmmo > 0)
		{
			Reload();
		}
		return;
	}

	bIsFiring = true;
	FireShot();

	// If Full-Auto, schedule subsequent shots with a timer
	if (WeaponConfig.FireMode == EWeaponFireMode::FullAuto)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AWeaponBase::FireShot, GetTimeBetweenShots(), true);
	}
}

void AWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AWeaponBase::FireShot()
{
	UWorld* World = GetWorld();
	if (!World || !CanFire())
	{
		StopFire();
		if (CurrentMagAmmo == 0 && CurrentReserveAmmo > 0)
		{
			Reload();
		}
		return;
	}

	// 1. Consume ammunition
	CurrentMagAmmo = FMath::Max(0, CurrentMagAmmo - 1);
	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);

	// 2. Ballistics Pipeline: TPS Parallax-Compensated SphereTrace
	FVector TraceStart = GetMuzzleLocation();
	FVector TargetPoint = TraceStart + (GetMuzzleForwardVector() * WeaponConfig.MaxRange);

	// Determine aim point from player's camera if owned by a character
	if (OwningCharacter.IsValid())
	{
		if (APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				const FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
				const FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
				const FVector CameraEnd = CameraLoc + (CameraForward * WeaponConfig.MaxRange);

				FCollisionQueryParams CameraQueryParams;
				CameraQueryParams.AddIgnoredActor(this);
				CameraQueryParams.AddIgnoredActor(OwningCharacter.Get());

				FHitResult CameraHit;
				if (World->LineTraceSingleByChannel(CameraHit, CameraLoc, CameraEnd, ECC_Visibility, CameraQueryParams))
				{
					TargetPoint = CameraHit.ImpactPoint;
				}
				else
				{
					TargetPoint = CameraEnd;
				}
			}

			// Apply recoil kick to camera pitch
			if (WeaponConfig.RecoilPitch > 0.0f)
			{
				PC->AddPitchInput(-WeaponConfig.RecoilPitch);
			}
		}
	}

	// 3. Physical trace from Muzzle to Target Point with SphereTrace
	const FVector BallisticDirection = (TargetPoint - TraceStart).GetSafeNormal();
	const float DistanceToTarget = FVector::Dist(TraceStart, TargetPoint);
	const FVector BallisticEnd = TraceStart + (BallisticDirection * (DistanceToTarget + 50.0f));

	FCollisionQueryParams BallisticQueryParams;
	BallisticQueryParams.AddIgnoredActor(this);
	if (OwningCharacter.IsValid())
	{
		BallisticQueryParams.AddIgnoredActor(OwningCharacter.Get());
	}
	BallisticQueryParams.bReturnPhysicalMaterial = true;

	FCollisionShape BulletShape = FCollisionShape::MakeSphere(WeaponConfig.BulletRadius);

	FHitResult BallisticHit;
	const bool bHit = World->SweepSingleByChannel(
		BallisticHit,
		TraceStart,
		BallisticEnd,
		FQuat::Identity,
		ECC_Visibility,
		BulletShape,
		BallisticQueryParams
	);

	const FVector HitPoint = bHit ? BallisticHit.ImpactPoint : TargetPoint;

	// 4. Debug Tracer (Cyan for miss, Red for impact)
	DrawDebugLine(World, TraceStart, HitPoint, bHit ? FColor::Red : FColor::Cyan, false, 1.2f, 0, 1.5f);

	if (bHit && BallisticHit.GetActor())
	{
		DrawDebugSphere(World, BallisticHit.ImpactPoint, WeaponConfig.BulletRadius * 2.0f, 10, FColor::Yellow, false, 1.2f);

		AActor* StruckActor = BallisticHit.GetActor();

		// Check for IZombieDamageableInterface
		if (StruckActor->Implements<UZombieDamageableInterface>())
		{
			FZombieDamageData DamageData;
			DamageData.BaseDamage = WeaponConfig.BaseDamage;
			DamageData.HitLocation = BallisticHit.ImpactPoint;
			DamageData.HitBoneName = BallisticHit.BoneName;
			DamageData.HitImpulse = BallisticDirection * 3000.0f;
			DamageData.DamageCauser = this;
			DamageData.InstigatedBy = OwningCharacter.IsValid() ? OwningCharacter->GetController() : nullptr;

			// Headshot verification:
			// 1. Skeletal meshes: checks if the hit bone is "head"
			// 2. Static meshes (e.g. test dummy cylinders): checks if the hit is in the upper 25% of the actor's height
			if (BallisticHit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase))
			{
				DamageData.bIsHeadshot = true;
			}
			else if (BallisticHit.BoneName.IsNone() && StruckActor)
			{
				FVector ActorOrigin, ActorBoxExtent;
				StruckActor->GetActorBounds(true, ActorOrigin, ActorBoxExtent);
				const float HeadshotThresholdZ = ActorOrigin.Z + (ActorBoxExtent.Z * 0.5f);
				DamageData.bIsHeadshot = (BallisticHit.ImpactPoint.Z >= HeadshotThresholdZ);
			}

			const float ActualDamage = IZombieDamageableInterface::Execute_TakeZombieDamage(StruckActor, DamageData);

			UE_LOG(LogTemp, Log, TEXT("[%s] Struck %s for %f dmg (Headshot: %s) | Ammo: %d/%d"),
				*WeaponConfig.WeaponName, *StruckActor->GetName(), ActualDamage,
				DamageData.bIsHeadshot ? TEXT("YES") : TEXT("NO"), CurrentMagAmmo, CurrentReserveAmmo);
		}

		OnWeaponFired.Broadcast(BallisticHit);
	}

	// 5. Play firing recoil montage on character
	if (FireMontage && OwningCharacter.IsValid())
	{
		OwningCharacter->PlayAnimMontage(FireMontage);
	}

	// If magazine is empty, stop automatic fire and trigger reload
	if (CurrentMagAmmo == 0)
	{
		StopFire();
		if (CurrentReserveAmmo > 0)
		{
			Reload();
		}
	}
}

void AWeaponBase::Reload()
{
	if (bIsReloading || CurrentMagAmmo >= WeaponConfig.MagCapacity || CurrentReserveAmmo <= 0)
	{
		return;
	}

	bIsReloading = true;
	StopFire();

	OnReloadStateChanged.Broadcast(true);

	if (ReloadMontage && OwningCharacter.IsValid())
	{
		OwningCharacter->PlayAnimMontage(ReloadMontage);
	}

	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AWeaponBase::FinishReload,
		WeaponConfig.ReloadDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[%s] Reload started (%f seconds)..."), *WeaponConfig.WeaponName, WeaponConfig.ReloadDuration);
}

void AWeaponBase::FinishReload()
{
	bIsReloading = false;

	const int32 AmmoNeeded = WeaponConfig.MagCapacity - CurrentMagAmmo;
	const int32 AmmoToLoad = FMath::Min(AmmoNeeded, CurrentReserveAmmo);

	CurrentMagAmmo += AmmoToLoad;
	CurrentReserveAmmo -= AmmoToLoad;

	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);
	OnReloadStateChanged.Broadcast(false);

	UE_LOG(LogTemp, Log, TEXT("[%s] Reload complete! Mag: %d, Reserve: %d"),
		*WeaponConfig.WeaponName, CurrentMagAmmo, CurrentReserveAmmo);
}

void AWeaponBase::CancelReload()
{
	if (bIsReloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		bIsReloading = false;
		OnReloadStateChanged.Broadcast(false);

		if (ReloadMontage && OwningCharacter.IsValid())
		{
			OwningCharacter->StopAnimMontage(ReloadMontage);
		}
	}
}

bool AWeaponBase::CanFire() const
{
	return !bIsReloading && CurrentMagAmmo > 0;
}

void AWeaponBase::AttachToCharacter(ACharacter* InCharacter, FName SocketName)
{
	if (!InCharacter)
	{
		return;
	}

	OwningCharacter = InCharacter;
	SetOwner(InCharacter);

	if (USkeletalMeshComponent* CharacterMesh = InCharacter->GetMesh())
	{
		AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	}
}

FVector AWeaponBase::GetMuzzleLocation() const
{
	// Priority: Skeletal mesh socket -> Static mesh socket -> Actor location
	if (WeaponSkeletalMesh && WeaponSkeletalMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponSkeletalMesh->GetSocketLocation(MuzzleSocketName);
	}

	if (WeaponStaticMesh && WeaponStaticMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponStaticMesh->GetSocketLocation(MuzzleSocketName);
	}

	// Fallback forward tip of the weapon
	return GetActorLocation() + (GetActorForwardVector() * 50.0f);
}

FVector AWeaponBase::GetMuzzleForwardVector() const
{
	if (WeaponSkeletalMesh && WeaponSkeletalMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponSkeletalMesh->GetSocketRotation(MuzzleSocketName).Vector();
	}

	if (WeaponStaticMesh && WeaponStaticMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponStaticMesh->GetSocketRotation(MuzzleSocketName).Vector();
	}

	return GetActorForwardVector();
}
