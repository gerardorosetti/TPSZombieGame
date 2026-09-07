// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZombieGame/Interfaces/ZombieDamageableInterface.h"
#include "ZombieGame/Interfaces/InteractableInterface.h"
#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "CombatHUDWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UCanvasPanel;
class APlayerCharacter;
class UZombieHealthComponent;
class UCombatComponent;
class APlayerStateBase;
class AZombieWaveManager;

/**
 * Unified combat HUD managing real-time weapon ammo, score display, wave counters, and damage overlays.
 */
UCLASS(Blueprintable, BlueprintType)
class ISPPV1_API UCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCombatHUDWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ----------------------------------------------------------------------------------
	// Bound UMG Widgets (Optional bindings for Blueprint designer flexibility)
	// ----------------------------------------------------------------------------------

	/** Full-screen radial red gradient vignette darkening the periphery as health drops. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Damage")
	TObjectPtr<UImage> DamageVignetteImage;

	/** Full-screen blood splatters framing screen borders when severely wounded. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Damage")
	TObjectPtr<UImage> BloodSplatterImage;

	/** Central hitmarker cross tick shown on bullet impact. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Crosshair")
	TObjectPtr<UImage> HitmarkerImage;

	/** Reticle elements for dynamic spread bloom. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Crosshair")
	TObjectPtr<UWidget> CrosshairContainer;

	/** Center reticle image for hipfire / aiming visual feedback. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Crosshair")
	TObjectPtr<UImage> ReticleImage;

	// Ammo Displays
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Ammo")
	TObjectPtr<UTextBlock> CurrentMagText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Ammo")
	TObjectPtr<UTextBlock> ReserveAmmoText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Ammo")
	TObjectPtr<UTextBlock> LowAmmoWarningText;

	// Economy Displays
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Economy")
	TObjectPtr<UTextBlock> CurrentPointsText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Economy")
	TObjectPtr<UTextBlock> PointsFlyoutText;

	// Wave / Round Displays
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Waves")
	TObjectPtr<UTextBlock> RoundNumberText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Waves")
	TObjectPtr<UTextBlock> ZombiesRemainingText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Waves")
	TObjectPtr<UTextBlock> RoundStatusText;

	// Interaction Prompt
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Interaction")
	TObjectPtr<UWidget> InteractionPromptContainer;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Interaction")
	TObjectPtr<UTextBlock> InteractionPromptText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|Interaction")
	TObjectPtr<UTextBlock> InteractionCostText;

	// Power-Up Active Buffs
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UWidget> PowerUpContainer;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UTextBlock> PowerUpNameText;

	// Dedicated Insta-Kill Widgets
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UWidget> InstaKillRow;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UProgressBar> InstaKillTimerBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UTextBlock> InstaKillTimerText;

	// Dedicated Double Points Widgets
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UWidget> DoublePointsRow;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UProgressBar> DoublePointsTimerBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UTextBlock> DoublePointsTimerText;

	// Shared Fallback Widgets (Maintains compatibility with single-row UMG layout)
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UProgressBar> PowerUpTimerBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="HUD|PowerUp")
	TObjectPtr<UTextBlock> PowerUpTimerText;

	/** Color tint for Insta-Kill timer bar and UI elements. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|PowerUp")
	FLinearColor InstaKillColor = FLinearColor(0.85f, 0.05f, 0.05f, 1.0f);

	/** Color tint for Double Points timer bar and UI elements. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|PowerUp")
	FLinearColor DoublePointsColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);

	// ----------------------------------------------------------------------------------
	// Configuration
	// ----------------------------------------------------------------------------------

	/** Base color tint for non-critical damage vignette. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Damage")
	FLinearColor DamageVignetteColor = FLinearColor(0.85f, 0.05f, 0.05f, 1.0f);

	/** Base color tint for blood splatter overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Damage")
	FLinearColor BloodSplatterColor = FLinearColor(0.6f, 0.02f, 0.02f, 1.0f);

	/** Speed of health recovery damage fade out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Damage")
	float DamageFadeSpeed = 4.0f;

	/** Sound played when bullet strikes body. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Audio")
	TObjectPtr<class USoundBase> HitmarkerSound;

	/** Sound played on critical headshot elimination / bullet impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Audio")
	TObjectPtr<class USoundBase> HeadshotHitmarkerSound;

	/** If true, shows 2D UI hitmarker on bullet hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Crosshair")
	bool bEnableHitmarkers = true;

	/** Duration in seconds that hitmarker stays on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Crosshair")
	float HitmarkerDuration = 0.15f;

	/** Duration in seconds that point delta flyout stays on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Economy")
	float PointsFlyoutDuration = 1.2f;

	/** If true, rapid successive point gains accumulate into a single running total during the flyout duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Economy")
	bool bAccumulatePointsFlyout = true;

	/** Color tint for standard point gains. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Economy")
	FLinearColor PointsFlyoutColor = FLinearColor(0.25f, 1.0f, 0.25f, 1.0f);

	/** Color tint for critical and headshot point gains. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Economy")
	FLinearColor HeadshotFlyoutColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);

	/** Color tint for point deductions (purchases). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Economy")
	FLinearColor DeductionFlyoutColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

public:
	// ----------------------------------------------------------------------------------
	// Public UI Triggers
	// ----------------------------------------------------------------------------------

	/** Triggers visual hit confirmation on screen. */
	UFUNCTION(BlueprintCallable, Category="HUD|Feedback")
	void ShowHitmarker(bool bIsHeadshot);

	/** Manually binds HUD listeners to local player systems. */
	UFUNCTION(BlueprintCallable, Category="HUD|Setup")
	void BindToPlayerSystems();

protected:
	// ----------------------------------------------------------------------------------
	// Subsystem Event Handlers
	// ----------------------------------------------------------------------------------

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData);

	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentMag, int32 CurrentReserve);

	UFUNCTION()
	void HandleAimStateChanged(bool bIsAiming);

	UFUNCTION()
	void HandlePointsChanged(int32 NewPoints, int32 DeltaPoints);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber);

	UFUNCTION()
	void HandleWaveStateChanged(EWaveState NewState);

	UFUNCTION()
	void HandleZombiesRemainingChanged(int32 RemainingCount, int32 TotalWaveCount);

	UFUNCTION()
	void HandleInteractableFound(TScriptInterface<IInteractableInterface> Interactable, const FText& Prompt, int32 Cost);

	UFUNCTION()
	void HandleInteractableLost();

	UFUNCTION()
	void HandleInstaKillChanged(bool bIsActive, float Duration);

	UFUNCTION()
	void HandleDoublePointsChanged(bool bIsActive, float Duration);

	/** Updates visibility, text, and row layouts depending on active power-up combinations. */
	void UpdatePowerUpUIState();

	// ----------------------------------------------------------------------------------
	// Runtime Animation State
	// ----------------------------------------------------------------------------------

	float TargetDamageOpacity = 0.0f;
	float CurrentDamageOpacity = 0.0f;
	float RunningTime = 0.0f;
	float HitmarkerTimer = 0.0f;
	float PointsFlyoutTimer = 0.0f;
	int32 CurrentFlyoutAccumulatedPoints = 0;

	bool bIsInstaKillActive = false;
	float InstaKillDuration = 30.0f;
	float InstaKillTimeRemaining = 0.0f;

	bool bIsDoublePointsActive = false;
	float DoublePointsDuration = 30.0f;
	float DoublePointsTimeRemaining = 0.0f;

	TWeakObjectPtr<APlayerCharacter> CachedPlayer;
	TWeakObjectPtr<class AWeaponBase> CachedWeapon;
};
