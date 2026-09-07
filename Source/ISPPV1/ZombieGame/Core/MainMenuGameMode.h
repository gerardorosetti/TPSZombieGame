// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class UMainMenuWidget;
class USoundBase;

/**
 * Game Mode dedicated to the main menu frontend.
 * Configures UI-only input mode, cursor visibility, widget lifecycle, and background audio.
 */
UCLASS()
class ISPPV1_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

protected:
	virtual void BeginPlay() override;

	/** Main menu widget class instantiated in the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	/** Active main menu widget instance. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Main Menu|UI")
	TObjectPtr<UMainMenuWidget> ActiveMainMenuWidget;

	/** Optional background music played while in the main menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|Audio")
	TObjectPtr<USoundBase> MenuMusic;

	/** Optional tag to target a specific CameraActor or CineCameraActor in the level. If None, auto-selects the first CameraActor found. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|Camera")
	FName MenuCameraTag = NAME_None;
};
