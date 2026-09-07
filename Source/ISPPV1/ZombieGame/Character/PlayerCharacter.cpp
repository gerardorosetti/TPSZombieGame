// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/Combat/CombatComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "ZombieGame/Gameplay/InteractionComponent.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/UI/CombatHUDWidget.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

APlayerCharacter::APlayerCharacter()
{
	// 1. Configure character movement rotation
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
		MoveComp->JumpZVelocity = 500.0f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.0f;
	}

	// 2. Setup Camera Boom (Spring Arm) - Over-The-Shoulder Style
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 220.0f;
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

	// 4. Instantiate Modular Combat Component
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// 5. Setup AI Perception Stimuli Source
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComponent"));
	if (StimuliSourceComponent)
	{
		StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSourceComponent->RegisterForSense(UAISense_Damage::StaticClass());
		StimuliSourceComponent->bAutoRegister = true;
	}

	// 6. Instantiate Interaction Component for World Interactables
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

	// 7. Enable Survival Health Auto-Regeneration on Player
	if (HealthComponent)
	{
		HealthComponent->bEnableAutoRegen = true;
		HealthComponent->RegenDelay = 4.0f;
		HealthComponent->RegenRate = 50.0f;
	}
}

void APlayerCharacter::RestorePlayerControl()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		EnableInput(PC);
	}
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RegisterInputMappingContext();
	RestorePlayerControl();
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	RegisterInputMappingContext();
	RestorePlayerControl();

	if (StimuliSourceComponent)
	{
		StimuliSourceComponent->RegisterWithPerceptionSystem();
	}

	// Client / Standalone Safeguard: Ensure Combat HUD is active for local player
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			bool bHUDAlreadyCreated = false;
			if (UWorld* World = GetWorld())
			{
				if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
				{
					if (GM->GetActiveHUDWidget())
					{
						bHUDAlreadyCreated = true;
					}
				}
			}

			if (!bHUDAlreadyCreated && HUDWidgetClass && !HUDWidgetClass->HasAnyClassFlags(CLASS_Abstract))
			{
				UCombatHUDWidget* FallbackHUD = CreateWidget<UCombatHUDWidget>(PC, HUDWidgetClass);
				if (FallbackHUD)
				{
					FallbackHUD->AddToViewport(0);
					ZOMBIE_LOG(Log, TEXT("[PlayerCharacter] Mounted Combat HUD to viewport."));
				}
			}
		}
	}
}

void APlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	RegisterInputMappingContext();
	RestorePlayerControl();
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::StartFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopFire);
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacter::ReloadWeapon);
		}

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APlayerCharacter::StartAiming);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopAiming);
		}

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::Interact);
		}
	}

	// Fallback direct key binding for [E] ensuring out-of-the-box interaction without requiring IMC editing
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &APlayerCharacter::Interact);
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

void APlayerCharacter::StartFire()
{
	if (CombatComponent)
	{
		CombatComponent->StartFire();
	}
}

void APlayerCharacter::StopFire()
{
	if (CombatComponent)
	{
		CombatComponent->StopFire();
	}
}

void APlayerCharacter::ReloadWeapon()
{
	if (CombatComponent)
	{
		CombatComponent->Reload();
	}
}

void APlayerCharacter::StartAiming()
{
	// Smoothly orient character towards controller aim direction (no instant snap)
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // Smooth, responsive turn rate (degrees/sec)
	}

	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void APlayerCharacter::StopAiming()
{
	// Return smoothly to free exploration mode (orients to movement direction)
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	}

	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void APlayerCharacter::Interact()
{
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LowHealthAudioComponent && LowHealthAudioComponent->IsPlaying())
	{
		LowHealthAudioComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData)
{
	Super::HandleHealthChanged(CurrentHealth, MaxHealth, HealthDelta, DamageData);

	if (HealthDelta < 0.0f)
	{
		// 1. Play jarring damage camera shake to alert player
		if (DamageCameraShakeClass)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->ClientStartCameraShake(DamageCameraShakeClass);
			}
		}

		// 2. Play 2D hurt audio feedback
		if (HurtSound)
		{
			UGameplayStatics::PlaySound2D(this, HurtSound);
		}
	}

	// 3. Low Health Audio System (Danger heartbeat, enter and exit cues)
	if (MaxHealth > 0.0f)
	{
		const float HealthRatio = CurrentHealth / MaxHealth;
		const bool bShouldBeLowHealth = (HealthRatio <= LowHealthThreshold && CurrentHealth > 0.0f);

		if (bShouldBeLowHealth && !bIsLowHealth)
		{
			bIsLowHealth = true;

			// Play low health entry stinger (initial danger alert)
			if (LowHealthEnterSound)
			{
				UGameplayStatics::PlaySound2D(this, LowHealthEnterSound);
			}

			// Start looping heartbeat/tension audio
			if (LowHealthLoopSound)
			{
				if (!LowHealthAudioComponent)
				{
					LowHealthAudioComponent = UGameplayStatics::SpawnSound2D(this, LowHealthLoopSound, 1.0f, 1.0f, 0.0f, nullptr, true);
				}
				else
				{
					LowHealthAudioComponent->SetSound(LowHealthLoopSound);
					LowHealthAudioComponent->Play();
				}
			}
		}
		else if (!bShouldBeLowHealth && bIsLowHealth)
		{
			bIsLowHealth = false;

			// Stop looping danger heartbeat
			if (LowHealthAudioComponent && LowHealthAudioComponent->IsPlaying())
			{
				LowHealthAudioComponent->Stop();
			}

			// Play recovery / danger exit sound
			if (LowHealthExitSound && CurrentHealth > 0.0f)
			{
				UGameplayStatics::PlaySound2D(this, LowHealthExitSound);
			}
		}
	}
}

void APlayerCharacter::OnDeathStarted(AActor* Killer)
{
	// 1. Stop low health heartbeat audio immediately on death
	if (LowHealthAudioComponent && LowHealthAudioComponent->IsPlaying())
	{
		LowHealthAudioComponent->Stop();
	}
	bIsLowHealth = false;

	// 2. Play death sound
	if (DeathSound)
	{
		UGameplayStatics::PlaySound2D(this, DeathSound);
	}

	// 3. Stop active weapon firing
	if (CombatComponent)
	{
		CombatComponent->StopFire();
	}

	// 4. Disable Enhanced Input Mapping Context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->RemoveMappingContext(DefaultMappingContext);
			}
		}
	}

	// Preserve ragdoll corpse for Game Over view
	SetLifeSpan(0.0f);
}

