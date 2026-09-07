// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

/**
 * Data structure representing a playable survival map in the main menu.
 */
USTRUCT(BlueprintType)
struct FZombieMapInfo
{
	GENERATED_BODY()

	/** Exact Level asset name to load via OpenLevel (e.g. Lvl_ThirdPerson, Industrial_UnfinishedBuilding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	FName LevelName = NAME_None;

	/** User-facing display title (e.g. "Facility Prototype"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	FText DisplayName;

	/** Lore or difficulty summary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	FText Description;

	/** Optional preview thumbnail for UI cards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
	TSoftObjectPtr<UTexture2D> Thumbnail;
};

/**
 * Main Menu HUD providing level selection and session control.
 */
UCLASS(Blueprintable, BlueprintType)
class ISPPV1_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	/** Launches a specific level safely by name. */
	UFUNCTION(BlueprintCallable, Category="Main Menu|Navigation")
	virtual void LaunchMap(FName InLevelName);

	/** Launches a level by its index in the PlayableMaps array. */
	UFUNCTION(BlueprintCallable, Category="Main Menu|Navigation")
	virtual void LaunchMapByIndex(int32 MapIndex);

	/** Returns a reference to the configured playable maps. */
	UFUNCTION(BlueprintPure, Category="Main Menu|Maps")
	const TArray<FZombieMapInfo>& GetPlayableMaps() const { return PlayableMaps; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ----------------------------------------------------------------------------------
	// Bound Widgets (Optional - for simple static layouts)
	// ----------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Main Menu|UI")
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Main Menu|UI")
	TObjectPtr<UButton> CustomMapButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category="Main Menu|UI")
	TObjectPtr<UButton> QuitButton;

	// ----------------------------------------------------------------------------------
	// Map Destination Configuration
	// ----------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|Maps")
	FName PrototypeLevelName = FName("Lvl_Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|Maps")
	FName CustomMapLevelName = NAME_None;

	/** Dynamic data-driven array of playable levels configured in Blueprint details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu|Maps")
	TArray<FZombieMapInfo> PlayableMaps;

protected:
	UFUNCTION()
	virtual void OnPlayClicked();

	UFUNCTION()
	virtual void OnCustomMapClicked();

	UFUNCTION()
	virtual void OnQuitClicked();
};
