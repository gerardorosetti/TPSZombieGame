// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/UI/GameOverWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UGameOverWidget::UGameOverWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnQuitClicked);
	}
}

void UGameOverWidget::NativeDestruct()
{
	if (RestartButton)
	{
		RestartButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnMainMenuClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnQuitClicked);
	}

	Super::NativeDestruct();
}

void UGameOverWidget::SetupGameOverStats(int32 RoundsSurvived, int32 TotalKills, int32 TotalHeadshots, int32 TotalScore)
{
	if (RoundsSurvivedText)
	{
		RoundsSurvivedText->SetText(FText::AsNumber(RoundsSurvived));
	}

	if (TotalKillsText)
	{
		TotalKillsText->SetText(FText::AsNumber(TotalKills));
	}

	if (TotalHeadshotsText)
	{
		TotalHeadshotsText->SetText(FText::AsNumber(TotalHeadshots));
	}

	if (TotalScoreText)
	{
		TotalScoreText->SetText(FText::AsNumber(TotalScore));
	}
}

void UGameOverWidget::OnRestartClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
	RemoveFromParent();
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void UGameOverWidget::OnMainMenuClicked()
{
	if (!MainMenuLevelName.IsNone())
	{
		RemoveFromParent();
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
	}
}

void UGameOverWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
