// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Combat/WeaponBase.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Combat/DroppedMagazine.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "CollisionShape.h"
#include "Animation/AnimMontage.h"
#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/UI/CombatHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Camera/CameraShakeBase.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// 3. Detachable Magazine Mesh (Attached to Weapon Body for Procedural Reload)
	MagazineStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineStaticMesh"));
	MagazineStaticMesh->SetupAttachment(WeaponStaticMesh);
	MagazineStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DroppedMagazineClass = ADroppedMagazine::StaticClass();
	MuzzleOffset = FVector::ZeroVector;
	AimingLocationOffset = FVector::ZeroVector;
	AimingRotationOffset = FRotator::ZeroRotator;
	AimInterpSpeed = 14.0f;

	ReloadDetachTime = 0.35f;
	ReloadTossTime = 0.85f;
	ReloadGrabNewTime = 1.05f;
	ReloadInsertTime = 2.05f;
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Ensure reload insert timing finishes ~0.15s before full reload duration concludes
	if (ReloadInsertTime >= WeaponConfig.ReloadDuration)
	{
		ReloadInsertTime = FMath::Max(ReloadGrabNewTime + 0.1f, WeaponConfig.ReloadDuration - 0.15f);
	}

	// Cache initial magazine relative transform for reload animation interpolation
	if (MagazineStaticMesh)
	{
		InitialMagLocation = MagazineStaticMesh->GetRelativeLocation();
		InitialMagRotation = MagazineStaticMesh->GetRelativeRotation();
	}

	// Cache initial weapon mesh relative transform for aim offset interpolation
	if (WeaponStaticMesh)
	{
		DefaultMeshLocation = WeaponStaticMesh->GetRelativeLocation();
		DefaultMeshRotation = WeaponStaticMesh->GetRelativeRotation();
	}

	// Initialize ammunition counters from configuration
	CurrentMagAmmo = WeaponConfig.MagCapacity;
	CurrentReserveAmmo = WeaponConfig.MaxReserveAmmo;

	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);
}

void AWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Dynamic Aim Alignment (Smooth transition between hipfire and aiming ADS offsets)
	if (WeaponStaticMesh)
	{
		const FVector TargetLoc = bIsAiming ? (DefaultMeshLocation + AimingLocationOffset) : DefaultMeshLocation;
		const FRotator TargetRot = bIsAiming ? (DefaultMeshRotation + AimingRotationOffset) : DefaultMeshRotation;

		const FVector NewLoc = FMath::VInterpTo(WeaponStaticMesh->GetRelativeLocation(), TargetLoc, DeltaTime, AimInterpSpeed);
		const FRotator NewRot = FMath::RInterpTo(WeaponStaticMesh->GetRelativeRotation(), TargetRot, DeltaTime, AimInterpSpeed);

		WeaponStaticMesh->SetRelativeLocation(NewLoc);
		WeaponStaticMesh->SetRelativeRotation(NewRot);
	}

	// 2. Procedural magazine reload animation (Hand-Synchronized with hand_l + Dropped Physics Magazine)
	if (bIsReloading && bEnableProceduralReload && MagazineStaticMesh)
	{
		const float Elapsed = GetWorldTimerManager().GetTimerElapsed(ReloadTimerHandle);

		// Phase 4: Fresh magazine inserted back into rifle well (~0.1s before animation concludes)
		if (Elapsed >= ReloadInsertTime && ReloadPhase < 4)
		{
			ReloadPhase = 4;
			if (MagInSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, MagInSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
			}
			MagazineStaticMesh->AttachToComponent(WeaponStaticMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			MagazineStaticMesh->SetRelativeLocation(InitialMagLocation);
			MagazineStaticMesh->SetRelativeRotation(InitialMagRotation);
			MagazineStaticMesh->SetVisibility(true);
		}
		// Phase 3: Fresh magazine retrieved from waist/hip (~0.2s after throw)
		else if (Elapsed >= ReloadGrabNewTime && ReloadPhase < 3)
		{
			ReloadPhase = 3;
			if (OwningCharacter.IsValid() && OwningCharacter->GetMesh())
			{
				MagazineStaticMesh->AttachToComponent(OwningCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ReloadHandSocket);
				MagazineStaticMesh->SetRelativeLocation(MagHandOffset);
				MagazineStaticMesh->SetRelativeRotation(MagHandRotation);
			}
			MagazineStaticMesh->SetVisibility(true);
		}
		// Phase 2: Empty magazine tossed away -> Spawn simulated physical magazine falling to ground!
		else if (Elapsed >= ReloadTossTime && ReloadPhase < 2)
		{
			ReloadPhase = 2;
			MagazineStaticMesh->SetVisibility(false);

			if (UWorld* World = GetWorld())
			{
				const FTransform MagWorldTransform = MagazineStaticMesh->GetComponentTransform();
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				TSubclassOf<ADroppedMagazine> SpawnClass = DroppedMagazineClass ? DroppedMagazineClass : TSubclassOf<ADroppedMagazine>(ADroppedMagazine::StaticClass());
				if (ADroppedMagazine* DroppedMag = World->SpawnActor<ADroppedMagazine>(SpawnClass, MagWorldTransform, SpawnParams))
				{
					FVector TossImpulse = (GetActorRightVector() * DroppedMagazineImpulse.Y) +
					                      (GetActorForwardVector() * DroppedMagazineImpulse.X) +
					                      FVector(0.0f, 0.0f, DroppedMagazineImpulse.Z);
					if (OwningCharacter.IsValid())
					{
						TossImpulse += OwningCharacter->GetVelocity() * 0.4f;
					}
					DroppedMag->InitializeDroppedMagazine(MagazineStaticMesh->GetStaticMesh(), TossImpulse);
				}
			}
		}
		// Phase 1: Left hand reaches rifle and grabs empty magazine
		else if (Elapsed >= ReloadDetachTime && ReloadPhase < 1)
		{
			ReloadPhase = 1;
			if (MagOutSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, MagOutSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
			}
			if (OwningCharacter.IsValid() && OwningCharacter->GetMesh())
			{
				MagazineStaticMesh->AttachToComponent(OwningCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ReloadHandSocket);
				MagazineStaticMesh->SetRelativeLocation(MagHandOffset);
				MagazineStaticMesh->SetRelativeRotation(MagHandRotation);
			}
			MagazineStaticMesh->SetVisibility(true);
		}
	}
}

void AWeaponBase::StartFire()
{
	if (!CanFire())
	{
		if (CurrentMagAmmo == 0)
		{
			if (DryFireSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetMuzzleLocation(), FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
			}

			if (CurrentReserveAmmo > 0)
			{
				Reload();
			}
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

	FVector TraceStart = GetMuzzleLocation();

	// 2. Gunshot audio, Camera Shake and Muzzle Flash
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, TraceStart, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SpatialAttenuation);
	}

	if (FireCameraShakeClass && OwningCharacter.IsValid())
	{
		if (APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController()))
		{
			PC->ClientStartCameraShake(FireCameraShakeClass);
		}
	}

	if (MuzzleFlashFX)
	{
		USceneComponent* AttachComp = WeaponSkeletalMesh ? Cast<USceneComponent>(WeaponSkeletalMesh) : Cast<USceneComponent>(WeaponStaticMesh);
		if (AttachComp)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlashFX,
				AttachComp,
				NAME_None,
				TraceStart,
				GetMuzzleForwardVector().Rotation(),
				EAttachLocation::KeepWorldPosition,
				true
			);
		}
		else if (World)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				MuzzleFlashFX,
				TraceStart,
				GetMuzzleForwardVector().Rotation()
			);
		}
	}

	// 3. Ballistics Pipeline: TPS Parallax-Compensated SphereTrace
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

	// 3. Point-Blank Close Quarters Contact Check:
	// If a zombie is hugging the player (between Camera/Chest and Muzzle tip),
	// ensure they take the damage so bullets never tunnel through enemies at zero distance!
	FHitResult CloseHit;
	bool bCloseHit = false;
	if (OwningCharacter.IsValid())
	{
		FCollisionQueryParams CloseQueryParams;
		CloseQueryParams.AddIgnoredActor(this);
		CloseQueryParams.AddIgnoredActor(OwningCharacter.Get());
		CloseQueryParams.bReturnPhysicalMaterial = true;

		const FVector ChestLoc = OwningCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
		bCloseHit = World->SweepSingleByChannel(
			CloseHit,
			ChestLoc,
			TraceStart + (GetMuzzleForwardVector() * 25.0f),
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(WeaponConfig.BulletRadius * 3.0f),
			CloseQueryParams
		);
	}

	// 4. Physical trace from Muzzle to Target Point with SphereTrace
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
	bool bHit = false;

	if (bCloseHit && CloseHit.GetActor() && CloseHit.GetActor() != OwningCharacter.Get())
	{
		BallisticHit = CloseHit;
		bHit = true;
	}
	else
	{
		bHit = World->SweepSingleByChannel(
			BallisticHit,
			TraceStart,
			BallisticEnd,
			FQuat::Identity,
			ECC_Visibility,
			BulletShape,
			BallisticQueryParams
		);
	}

	const FVector HitPoint = bHit ? BallisticHit.ImpactPoint : TargetPoint;

	// 5. Visual Tracer strictly from MuzzleLocation to HitPoint!
	if (bEnableDebugTraces)
	{
		DrawDebugLine(World, TraceStart, HitPoint, bHit ? FColor::Red : FColor::Cyan, false, 1.2f, 0, 1.5f);
	}

	if (TracerFX)
	{
		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			TracerFX,
			TraceStart,
			BallisticDirection.Rotation()
		);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TEXT("BeamEnd"), HitPoint);
		}
	}

	if (bHit && BallisticHit.GetActor())
	{
		if (bEnableDebugTraces)
		{
			DrawDebugSphere(World, BallisticHit.ImpactPoint, WeaponConfig.BulletRadius * 2.0f, 10, FColor::Yellow, false, 1.2f);
		}

		AActor* StruckActor = BallisticHit.GetActor();

		// Check for IZombieDamageableInterface (Living enemy or flesh target)
		if (StruckActor->Implements<UZombieDamageableInterface>())
		{
			FZombieDamageData DamageData;
			DamageData.BaseDamage = WeaponConfig.BaseDamage;

			// Global Insta-Kill match buff check
			if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
			{
				if (GM->IsInstaKillActive())
				{
					DamageData.BaseDamage = 100000.0f;
				}
			}

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

			// Spawn Niagara blood splash oriented along impact normal
			if (FleshImpactFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World,
					FleshImpactFX,
					BallisticHit.ImpactPoint,
					BallisticHit.ImpactNormal.Rotation()
				);
			}

			// Play 3D spatial flesh impact audio (headshot or body)
			USoundBase* FleshSoundToPlay = (DamageData.bIsHeadshot && HeadshotFleshImpactSound) ? HeadshotFleshImpactSound.Get() : FleshImpactSound.Get();
			if (FleshSoundToPlay)
			{
				UGameplayStatics::PlaySoundAtLocation(
					World,
					FleshSoundToPlay,
					BallisticHit.ImpactPoint,
					FRotator::ZeroRotator,
					1.0f,
					1.0f,
					0.0f,
					SpatialAttenuation
				);
			}

			// Broadcast hit event for hitmarkers and audio
			OnWeaponHitTarget.Broadcast(DamageData.bIsHeadshot);

			if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
			{
				if (UCombatHUDWidget* HUD = GM->GetActiveHUDWidget())
				{
					HUD->ShowHitmarker(DamageData.bIsHeadshot);
				}
			}

			ZOMBIE_LOG(Log, TEXT("[%s] Struck %s for %f dmg (Headshot: %s) | Ammo: %d/%d"),
				*WeaponConfig.WeaponName, *StruckActor->GetName(), ActualDamage,
				DamageData.bIsHeadshot ? TEXT("YES") : TEXT("NO"), CurrentMagAmmo, CurrentReserveAmmo);
		}
		else
		{
			// World Static / Environment Hit: sparks, dust and bullet hole decal
			if (WorldImpactFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World,
					WorldImpactFX,
					BallisticHit.ImpactPoint,
					BallisticHit.ImpactNormal.Rotation()
				);
			}

			if (WorldImpactSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					World,
					WorldImpactSound,
					BallisticHit.ImpactPoint,
					FRotator::ZeroRotator,
					1.0f,
					1.0f,
					0.0f,
					SpatialAttenuation
				);
			}

			if (BulletHoleDecal)
			{
				UGameplayStatics::SpawnDecalAtLocation(
					World,
					BulletHoleDecal,
					BulletHoleDecalSize,
					BallisticHit.ImpactPoint,
					(-BallisticHit.ImpactNormal).Rotation(),
					30.0f
				);
			}
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

void AWeaponBase::RefillAmmo(bool bRefillMag, bool bRefillReserve)
{
	if (bRefillMag)
	{
		CurrentMagAmmo = WeaponConfig.MagCapacity;
	}
	if (bRefillReserve)
	{
		CurrentReserveAmmo = WeaponConfig.MaxReserveAmmo;
	}

	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);
	ZOMBIE_LOG(Log, TEXT("[%s] Ammo refilled! Current: %d / Reserve: %d"),
		*WeaponConfig.WeaponName, CurrentMagAmmo, CurrentReserveAmmo);
}

void AWeaponBase::Reload()
{
	if (bIsReloading || CurrentMagAmmo >= WeaponConfig.MagCapacity || CurrentReserveAmmo <= 0)
	{
		return;
	}

	bIsReloading = true;
	ReloadPhase = 0;
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

	ZOMBIE_LOG(Log, TEXT("[%s] Reload started (%f seconds)..."), *WeaponConfig.WeaponName, WeaponConfig.ReloadDuration);
}

void AWeaponBase::FinishReload()
{
	bIsReloading = false;
	ReloadPhase = 0;

	if (MagazineStaticMesh)
	{
		MagazineStaticMesh->AttachToComponent(WeaponStaticMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MagazineStaticMesh->SetRelativeLocation(InitialMagLocation);
		MagazineStaticMesh->SetRelativeRotation(InitialMagRotation);
		MagazineStaticMesh->SetVisibility(true);
	}

	const int32 AmmoNeeded = WeaponConfig.MagCapacity - CurrentMagAmmo;
	const int32 AmmoToLoad = FMath::Min(AmmoNeeded, CurrentReserveAmmo);

	CurrentMagAmmo += AmmoToLoad;
	CurrentReserveAmmo -= AmmoToLoad;

	OnAmmoChanged.Broadcast(CurrentMagAmmo, CurrentReserveAmmo);
	OnReloadStateChanged.Broadcast(false);

	ZOMBIE_LOG(Log, TEXT("[%s] Reload complete! Mag: %d, Reserve: %d"),
		*WeaponConfig.WeaponName, CurrentMagAmmo, CurrentReserveAmmo);
}

void AWeaponBase::CancelReload()
{
	if (bIsReloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		bIsReloading = false;
		ReloadPhase = 0;

		if (MagazineStaticMesh)
		{
			MagazineStaticMesh->AttachToComponent(WeaponStaticMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			MagazineStaticMesh->SetRelativeLocation(InitialMagLocation);
			MagazineStaticMesh->SetRelativeRotation(InitialMagRotation);
			MagazineStaticMesh->SetVisibility(true);
		}

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
	const FVector RotatedOffset = GetActorRotation().RotateVector(MuzzleOffset);

	// Priority 1: Socket on Skeletal Mesh
	if (WeaponSkeletalMesh && WeaponSkeletalMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponSkeletalMesh->GetSocketLocation(MuzzleSocketName) + RotatedOffset;
	}

	// Priority 2: Socket on Static Mesh
	if (WeaponStaticMesh && WeaponStaticMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponStaticMesh->GetSocketLocation(MuzzleSocketName) + RotatedOffset;
	}

	// Priority 3: Compute forward-most physical tip from Static Mesh bounding box
	if (WeaponStaticMesh && WeaponStaticMesh->GetStaticMesh())
	{
		const FBoxSphereBounds MeshBounds = WeaponStaticMesh->GetStaticMesh()->GetBounds();
		const FVector MeshTip = WeaponStaticMesh->GetComponentLocation() + (WeaponStaticMesh->GetForwardVector() * MeshBounds.BoxExtent.X);
		return MeshTip + RotatedOffset;
	}

	// Priority 4: Fallback forward tip of the weapon actor
	return GetActorLocation() + (GetActorForwardVector() * 70.0f) + RotatedOffset;
}

FVector AWeaponBase::GetMuzzleForwardVector() const
{
	if (WeaponSkeletalMesh && WeaponSkeletalMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponSkeletalMesh->GetSocketRotation(MuzzleSocketName).Vector();
	}

	if (WeaponStaticMesh)
	{
		if (WeaponStaticMesh->DoesSocketExist(MuzzleSocketName))
		{
			return WeaponStaticMesh->GetSocketRotation(MuzzleSocketName).Vector();
		}
		return WeaponStaticMesh->GetForwardVector();
	}

	return GetActorForwardVector();
}
