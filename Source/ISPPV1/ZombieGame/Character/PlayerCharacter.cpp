// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

APlayerCharacter::APlayerCharacter()
{
	// Configure character movement rotation
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		MoveComp->JumpZVelocity = 500.0f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.0f;
	}

	// 1. Setup Camera Boom (Spring Arm) positioned over the right shoulder
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 280.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 65.0f, 65.0f); // Over-the-shoulder offset
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;

	// 2. Setup Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind actions via Enhanced Input
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::FireTestHitscan);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find forward direction based on camera yaw
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayerCharacter::FireTestHitscan()
{
	UWorld* World = GetWorld();
	if (!World || !FollowCamera)
	{
		return;
	}

	// 1. Raycast parameters from center of camera
	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceDirection = FollowCamera->GetForwardVector();
	const FVector TraceEnd = TraceStart + (TraceDirection * 10000.0f); // 100 meters range

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	// 2. Draw visual debug tracer
	DrawDebugLine(
		World,
		TraceStart + (FollowCamera->GetRightVector() * 20.0f) - FVector(0, 0, 20.0f), // approximate muzzle
		bHit ? HitResult.ImpactPoint : TraceEnd,
		bHit ? FColor::Red : FColor::Green,
		false,
		1.5f,
		0,
		1.5f
	);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// Draw impact point
		DrawDebugSphere(World, HitResult.ImpactPoint, 10.0f, 12, FColor::Yellow, false, 1.5f);

		// 3. Check for IZombieDamageableInterface via Unreal reflection
		if (HitActor->Implements<UZombieDamageableInterface>())
		{
			FZombieDamageData DamageData;
			DamageData.BaseDamage = 35.0f;
			DamageData.HitLocation = HitResult.ImpactPoint;
			DamageData.HitBoneName = HitResult.BoneName;
			DamageData.HitImpulse = TraceDirection * 2500.0f;
			DamageData.DamageCauser = this;
			DamageData.InstigatedBy = GetController();

			// Detect headshot
			DamageData.bIsHeadshot = HitResult.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase);

			const float ActualDamage = IZombieDamageableInterface::Execute_TakeZombieDamage(HitActor, DamageData);

			UE_LOG(LogTemp, Log, TEXT("[Player Hitscan] Struck %s! Applied %f damage (Headshot: %s)"),
				*HitActor->GetName(), ActualDamage, DamageData.bIsHeadshot ? TEXT("YES") : TEXT("NO"));
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Player Hitscan] Struck non-damageable actor: %s"), *HitActor->GetName());
		}
	}
}
