// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheCameraReferences();

	// Automatically spawn default weapon if assigned in Blueprint
	if (DefaultWeaponClass)
	{
		EquipWeapon(DefaultWeaponClass);
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ensure camera references are valid
	if (!CachedCamera.IsValid() || !CachedSpringArm.IsValid())
	{
		CacheCameraReferences();
	}

	// Smooth ADS (Aim Down Sights) zoom interpolation
	const float TargetFOV = (bIsAiming && CurrentWeapon) ? CurrentWeapon->GetAimFOV() : DefaultFOV;
	const float TargetArmLength = bIsAiming ? AimArmLength : DefaultArmLength;

	if (CachedCamera.IsValid())
	{
		const float NewFOV = FMath::FInterpTo(CachedCamera->FieldOfView, TargetFOV, DeltaTime, AimInterpSpeed);
		CachedCamera->SetFieldOfView(NewFOV);
	}

	if (CachedSpringArm.IsValid())
	{
		const float NewArmLength = FMath::FInterpTo(CachedSpringArm->TargetArmLength, TargetArmLength, DeltaTime, AimInterpSpeed);
		CachedSpringArm->TargetArmLength = NewArmLength;
	}
}

void UCombatComponent::StartFire()
{
	ActivateCombatStance();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatStanceTimerHandle);
		World->GetTimerManager().ClearTimer(InitialFireDelayTimerHandle);
	}

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (Char)
	{
		const float YawDiff = FMath::Abs(FRotator::NormalizeAxis(Char->GetControlRotation().Yaw - Char->GetActorRotation().Yaw));
		// If character is facing significantly away from camera (> 25 degrees), wait a brief moment for rotation to align and weapon to raise
		if (YawDiff > 25.0f && !bIsAiming)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(InitialFireDelayTimerHandle, this, &UCombatComponent::ExecuteDelayedFire, 0.12f, false);
				return;
			}
		}
	}

	ExecuteDelayedFire();
}

void UCombatComponent::ExecuteDelayedFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void UCombatComponent::StopFire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialFireDelayTimerHandle);
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}

	// Schedule transition out of combat stance after duration
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CombatStanceTimerHandle, this, &UCombatComponent::DeactivateCombatStance, CombatStanceDuration, false);
	}
}

void UCombatComponent::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

void UCombatComponent::SetAiming(bool bNewAiming)
{
	if (bIsAiming != bNewAiming)
	{
		bIsAiming = bNewAiming;

		if (CurrentWeapon)
		{
			CurrentWeapon->SetAiming(bIsAiming);
		}

		if (bIsAiming)
		{
			ActivateCombatStance();
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(CombatStanceTimerHandle);
			}
		}
		else
		{
			// Stopped aiming: if not firing, schedule returning to relaxed stance
			if (!CurrentWeapon || !CurrentWeapon->IsFiring())
			{
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().SetTimer(CombatStanceTimerHandle, this, &UCombatComponent::DeactivateCombatStance, 0.2f, false);
				}
			}
		}

		OnAimStateChanged.Broadcast(bIsAiming);
	}
}

void UCombatComponent::ActivateCombatStance()
{
	bWeaponRaised = true;

	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->bUseControllerDesiredRotation = true;
			MoveComp->bOrientRotationToMovement = false;
		}
	}
}

void UCombatComponent::DeactivateCombatStance()
{
	if (bIsAiming)
	{
		return;
	}

	if (CurrentWeapon && CurrentWeapon->IsFiring())
	{
		return;
	}

	bWeaponRaised = false;

	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->bUseControllerDesiredRotation = false;
			MoveComp->bOrientRotationToMovement = true;
		}
	}
}

void UCombatComponent::EquipWeapon(TSubclassOf<AWeaponBase> NewWeaponClass)
{
	UWorld* World = GetWorld();
	if (!World || !NewWeaponClass)
	{
		return;
	}

	// Destroy existing weapon if present
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = World->SpawnActor<AWeaponBase>(NewWeaponClass, SpawnParams);

	if (CurrentWeapon)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			CurrentWeapon->AttachToCharacter(OwnerCharacter, WeaponAttachSocket);
			CurrentWeapon->SetAiming(bIsAiming);
			ZOMBIE_LOG(Log, TEXT("[CombatComponent] Successfully equipped and attached weapon: %s to socket: %s"),
				*CurrentWeapon->GetName(), *WeaponAttachSocket.ToString());
		}
	}
}

void UCombatComponent::RefillAllWeaponsAmmo()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->RefillAmmo(true, true);
	}
}

void UCombatComponent::CacheCameraReferences()
{
	if (AActor* Owner = GetOwner())
	{
		if (!CachedCamera.IsValid())
		{
			CachedCamera = Owner->FindComponentByClass<UCameraComponent>();
		}
		if (!CachedSpringArm.IsValid())
		{
			CachedSpringArm = Owner->FindComponentByClass<USpringArmComponent>();
		}
	}
}
