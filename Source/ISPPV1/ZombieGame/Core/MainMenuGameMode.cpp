// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/MainMenuGameMode.h"
#include "ZombieGame/Core/ZombieLog.h"
#include "ZombieGame/UI/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Sound/SoundBase.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	MainMenuWidgetClass = nullptr;
	MenuMusic = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		// Automatically bind any CameraActor / CineCameraActor in the level as the view target
		AActor* TargetCamera = nullptr;
		if (!MenuCameraTag.IsNone())
		{
			TArray<AActor*> TaggedActors;
			UGameplayStatics::GetAllActorsWithTag(this, MenuCameraTag, TaggedActors);
			if (TaggedActors.Num() > 0 && TaggedActors[0])
			{
				TargetCamera = TaggedActors[0];
			}
		}

		if (!TargetCamera)
		{
			TArray<AActor*> FoundCameras;
			UGameplayStatics::GetAllActorsOfClass(this, ACameraActor::StaticClass(), FoundCameras);
			if (FoundCameras.Num() > 0 && FoundCameras[0])
			{
				TargetCamera = FoundCameras[0];
			}
		}

		if (TargetCamera)
		{
			PC->SetViewTargetWithBlend(TargetCamera, 0.0f);
			ZOMBIE_LOG(Log, TEXT("[MainMenuGameMode] Auto-assigned view target to camera '%s'."), *TargetCamera->GetName());
		}

		if (MainMenuWidgetClass && !ActiveMainMenuWidget)
		{
			ActiveMainMenuWidget = CreateWidget<UMainMenuWidget>(PC, MainMenuWidgetClass);
			if (ActiveMainMenuWidget)
			{
				ActiveMainMenuWidget->AddToViewport(0);
				ZOMBIE_LOG(Log, TEXT("[MainMenuGameMode] Main Menu HUD mounted successfully."));
			}
		}
	}

	if (MenuMusic)
	{
		UGameplayStatics::PlaySound2D(this, MenuMusic);
	}
}
