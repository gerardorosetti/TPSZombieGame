// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "UObject/ConstructorHelpers.h"
#include "InputCoreTypes.h"
#include "ZombieGame/Interfaces/ZombieDamageableInterface.h"

APlayerCharacter::APlayerCharacter()
{
	// 1. Configure character movement rotation
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

	// 2. Setup Camera Boom (Spring Arm) - Over-The-Shoulder Style
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 220.0f;                       // Tighter
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 45.0f);     // Right shoulder offset
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 25.0f);      // Eye/chest level pivot
	CameraBoom->bUsePawnControlRotation = true;                 // Rotate boom with mouse controller
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 25.0f;                         // Responsive, crisp camera motion

	// 3. Setup Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 85.0f;

	// 4. Setup Prototype Weapon Visual (Attached to hand_r socket)
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeLocation(FVector(2.0f, 6.0f, -1.0f));
	WeaponMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	WeaponMesh->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (MeshFinder.Succeeded())
	{
		WeaponMesh->SetStaticMesh(MeshFinder.Object);
	}
	// Note: Input actions and mapping contexts are configured in derived Blueprints
	// (e.g. BP_PlayerCharacter) to ensure clean separation between C++ code and content assets.
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	RegisterInputMappingContext();
}

void APlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	RegisterInputMappingContext();
}

void APlayerCharacter::RegisterInputMappingContext()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->RemoveMappingContext(DefaultMappingContext);
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
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

	// Always provide a fallback direct binding for Left Mouse Button so testing never fails
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &APlayerCharacter::FireTestHitscan);
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

	// 2. Muzzle start location (from weapon mesh if valid, or shoulder)
	const FVector MuzzleLoc = WeaponMesh ? WeaponMesh->GetComponentLocation() : (TraceStart + (FollowCamera->GetRightVector() * 20.0f) - FVector(0, 0, 20.0f));

	// 3. Draw visual debug tracer
	DrawDebugLine(
		World,
		MuzzleLoc,
		bHit ? HitResult.ImpactPoint : TraceEnd,
		bHit ? FColor::Red : FColor::Cyan,
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

		// 4. Check for IZombieDamageableInterface via Unreal reflection
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
