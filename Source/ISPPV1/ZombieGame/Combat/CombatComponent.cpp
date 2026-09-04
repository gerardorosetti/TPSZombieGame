// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"

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
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void UCombatComponent::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
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
		OnAimStateChanged.Broadcast(bIsAiming);
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
			UE_LOG(LogTemp, Log, TEXT("[CombatComponent] Successfully equipped and attached weapon: %s to socket: %s"),
				*CurrentWeapon->GetName(), *WeaponAttachSocket.ToString());
		}
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
