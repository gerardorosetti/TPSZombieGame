// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/PlayerAnimInstance.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (PlayerCharacter)
	{
		MovementComponent = PlayerCharacter->GetCharacterMovement();
		CombatComponent = PlayerCharacter->GetCombatComponent();
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
		if (!PlayerCharacter)
		{
			return;
		}
		MovementComponent = PlayerCharacter->GetCharacterMovement();
		CombatComponent = PlayerCharacter->GetCombatComponent();
	}

	// 1. Locomotion Parameters
	const FVector Velocity = PlayerCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();

	if (MovementComponent)
	{
		bShouldMove = (GroundSpeed > 3.0f) && (MovementComponent->GetCurrentAcceleration().SizeSquared() > 0.0f);
		bIsFalling = MovementComponent->IsFalling();
	}

	// 2. Combat & Aiming State
	if (CombatComponent)
	{
		bIsAiming = CombatComponent->IsAiming();
		CurrentWeapon = CombatComponent->GetCurrentWeapon();
	}

	// 3. Aim Offset Calculation (Pitch and Yaw deltas relative to actor heading)
	const FRotator AimRotation = PlayerCharacter->GetBaseAimRotation();
	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, ActorRotation);

	// AO_Rifle uses a normalized [-1.0, 1.0] axis where:
	// -1.0 = looking fully down (-80 degrees)
	//  0.0 = looking horizontal (0 degrees, neutral center)
	// +1.0 = looking fully up (+80 degrees)
	const float TargetPitch = FMath::Clamp(DeltaRot.Pitch / 80.0f, -1.0f, 1.0f);
	AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, 15.0f);

	const float TargetYaw = FMath::Clamp(DeltaRot.Yaw / 80.0f, -1.0f, 1.0f);
	AimYaw = FMath::FInterpTo(AimYaw, TargetYaw, DeltaSeconds, 15.0f);
}
