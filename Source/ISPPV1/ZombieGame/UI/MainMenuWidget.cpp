// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/UI/MainMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrototypeLevelName = FName("Lvl_Default");
	CustomMapLevelName = NAME_None;

	// Default pre-populated map for instant usability
	FZombieMapInfo FacilityMap;
	FacilityMap.LevelName = FName("Lvl_Default");
	FacilityMap.DisplayName = NSLOCTEXT("MainMenu", "MapPrototype", "Facility Proving Grounds");
	FacilityMap.Description = NSLOCTEXT("MainMenu", "MapPrototypeDesc", "Underground military research laboratory proving grounds.");
	PlayableMaps.Add(FacilityMap);
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
	}

	if (CustomMapButton)
	{
		CustomMapButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCustomMapClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::OnPlayClicked);
	}

	if (CustomMapButton)
	{
		CustomMapButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::OnCustomMapClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::OnQuitClicked);
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::LaunchMap(FName InLevelName)
{
	if (!InLevelName.IsNone())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
		RemoveFromParent();
		UGameplayStatics::OpenLevel(this, InLevelName);
	}
}

void UMainMenuWidget::LaunchMapByIndex(int32 MapIndex)
{
	if (PlayableMaps.IsValidIndex(MapIndex))
	{
		LaunchMap(PlayableMaps[MapIndex].LevelName);
	}
}

void UMainMenuWidget::OnPlayClicked()
{
	LaunchMap(PrototypeLevelName);
}

void UMainMenuWidget::OnCustomMapClicked()
{
	if (!CustomMapLevelName.IsNone())
	{
		LaunchMap(CustomMapLevelName);
	}
	else
	{
		LaunchMap(PrototypeLevelName);
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
