// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * Game Over HUD presenting match summary analytics and navigation controls.
 */
UCLASS(Blueprintable, BlueprintType)
class ISPPV1_API UGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameOverWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ----------------------------------------------------------------------------------
	// Bound Widgets
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UTextBlock> RoundsSurvivedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UTextBlock> TotalKillsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UTextBlock> TotalHeadshotsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UTextBlock> TotalScoreText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Game Over|UI")
	TObjectPtr<UButton> QuitButton;

	// ----------------------------------------------------------------------------------
	// Level Navigation Configuration
	// ----------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Over|Navigation")
	FName MainMenuLevelName = FName("Lvl_MainMenu");

public:
	/** Populates match analytics on the HUD display. */
	UFUNCTION(BlueprintCallable, Category="Game Over")
	void SetupGameOverStats(int32 RoundsSurvived, int32 TotalKills, int32 TotalHeadshots, int32 TotalScore);

protected:
	UFUNCTION()
	virtual void OnRestartClicked();

	UFUNCTION()
	virtual void OnMainMenuClicked();

	UFUNCTION()
	virtual void OnQuitClicked();
};
