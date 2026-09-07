// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/UI/CombatHUDWidget.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "ZombieGame/Combat/CombatComponent.h"
#include "ZombieGame/Combat/WeaponBase.h"
#include "ZombieGame/Core/PlayerStateBase.h"
#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Gameplay/InteractionComponent.h"
#include "ZombieGame/Gameplay/ZombieWaveManager.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"

UCombatHUDWidget::UCombatHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize default visibility states
	if (HitmarkerImage)
	{
		HitmarkerImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (PointsFlyoutText)
	{
		PointsFlyoutText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (InteractionPromptContainer)
	{
		InteractionPromptContainer->SetVisibility(ESlateVisibility::Hidden);
	}

	if (PowerUpContainer)
	{
		PowerUpContainer->SetVisibility(ESlateVisibility::Hidden);
	}

	if (LowAmmoWarningText)
	{
		LowAmmoWarningText->SetVisibility(ESlateVisibility::Hidden);
	}

	// Initialize trauma overlays to completely transparent
	if (DamageVignetteImage)
	{
		FLinearColor InitColor = DamageVignetteColor;
		InitColor.A = 0.0f;
		DamageVignetteImage->SetColorAndOpacity(InitColor);
	}

	if (BloodSplatterImage)
	{
		FLinearColor InitColor = BloodSplatterColor;
		InitColor.A = 0.0f;
		BloodSplatterImage->SetColorAndOpacity(InitColor);
	}

	if (InstaKillTimerBar)
	{
		InstaKillTimerBar->SetFillColorAndOpacity(InstaKillColor);
	}

	if (DoublePointsTimerBar)
	{
		DoublePointsTimerBar->SetFillColorAndOpacity(DoublePointsColor);
	}

	UpdatePowerUpUIState();

	BindToPlayerSystems();
}

void UCombatHUDWidget::BindToPlayerSystems()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		return;
	}

	CachedPlayer = Player;

	// 1. Health & Survival Damage Overlay
	if (UZombieHealthComponent* HC = Player->GetHealthComponent())
	{
		HC->OnHealthChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleHealthChanged);
		HC->OnHealthChanged.AddDynamic(this, &UCombatHUDWidget::HandleHealthChanged);
		TargetDamageOpacity = FMath::Clamp(1.0f - HC->GetHealthPercent(), 0.0f, 1.0f);
	}

	// 2. Combat & Weapons
	if (UCombatComponent* CombatComp = Player->GetCombatComponent())
	{
		CombatComp->OnAimStateChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleAimStateChanged);
		CombatComp->OnAimStateChanged.AddDynamic(this, &UCombatHUDWidget::HandleAimStateChanged);

		if (AWeaponBase* Weapon = CombatComp->GetCurrentWeapon())
		{
			CachedWeapon = Weapon;

			Weapon->OnAmmoChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleAmmoChanged);
			Weapon->OnAmmoChanged.AddDynamic(this, &UCombatHUDWidget::HandleAmmoChanged);
			HandleAmmoChanged(Weapon->GetCurrentMagAmmo(), Weapon->GetCurrentReserveAmmo());

			Weapon->OnWeaponHitTarget.RemoveDynamic(this, &UCombatHUDWidget::ShowHitmarker);
			Weapon->OnWeaponHitTarget.AddDynamic(this, &UCombatHUDWidget::ShowHitmarker);
		}
	}

	// 3. World Interaction
	if (UInteractionComponent* InterComp = Player->FindComponentByClass<UInteractionComponent>())
	{
		InterComp->OnInteractableFound.RemoveDynamic(this, &UCombatHUDWidget::HandleInteractableFound);
		InterComp->OnInteractableFound.AddDynamic(this, &UCombatHUDWidget::HandleInteractableFound);

		InterComp->OnInteractableLost.RemoveDynamic(this, &UCombatHUDWidget::HandleInteractableLost);
		InterComp->OnInteractableLost.AddDynamic(this, &UCombatHUDWidget::HandleInteractableLost);
	}

	// 4. Points & Economy
	if (APlayerStateBase* PS = Player->GetPlayerState<APlayerStateBase>())
	{
		PS->OnPointsChanged.RemoveDynamic(this, &UCombatHUDWidget::HandlePointsChanged);
		PS->OnPointsChanged.AddDynamic(this, &UCombatHUDWidget::HandlePointsChanged);
		HandlePointsChanged(PS->GetCurrentPoints(), 0);

		PS->OnDoublePointsStateChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleDoublePointsChanged);
		PS->OnDoublePointsStateChanged.AddDynamic(this, &UCombatHUDWidget::HandleDoublePointsChanged);
	}

	// 5. Game Mode, Waves, and Global Power-Ups
	if (UWorld* World = GetWorld())
	{
		if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
		{
			if (AZombieWaveManager* WM = GM->GetWaveManager())
			{
				WM->OnWaveStarted.RemoveDynamic(this, &UCombatHUDWidget::HandleWaveStarted);
				WM->OnWaveStarted.AddDynamic(this, &UCombatHUDWidget::HandleWaveStarted);

				WM->OnWaveStateChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleWaveStateChanged);
				WM->OnWaveStateChanged.AddDynamic(this, &UCombatHUDWidget::HandleWaveStateChanged);

				WM->OnZombiesRemainingChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleZombiesRemainingChanged);
				WM->OnZombiesRemainingChanged.AddDynamic(this, &UCombatHUDWidget::HandleZombiesRemainingChanged);

				HandleWaveStarted(WM->GetCurrentWaveNumber());
				HandleWaveStateChanged(WM->GetCurrentWaveState());
				HandleZombiesRemainingChanged(WM->GetRemainingZombiesCount(), WM->GetTotalZombiesForWave());
			}

			GM->OnInstaKillStateChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleInstaKillChanged);
			GM->OnInstaKillStateChanged.AddDynamic(this, &UCombatHUDWidget::HandleInstaKillChanged);
		}
	}
}

void UCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Dynamic Late-Binding Safeguard
	if (!CachedPlayer.IsValid())
	{
		BindToPlayerSystems();
	}
	else
	{
		// Dynamic Weapon Sync (Detects newly equipped or switched weapons)
		if (UCombatComponent* CombatComp = CachedPlayer->GetCombatComponent())
		{
			AWeaponBase* CurrentWeapon = CombatComp->GetCurrentWeapon();
			if (CurrentWeapon && CurrentWeapon != CachedWeapon.Get())
			{
				CachedWeapon = CurrentWeapon;

				CurrentWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatHUDWidget::HandleAmmoChanged);
				CurrentWeapon->OnAmmoChanged.AddDynamic(this, &UCombatHUDWidget::HandleAmmoChanged);
				HandleAmmoChanged(CurrentWeapon->GetCurrentMagAmmo(), CurrentWeapon->GetCurrentReserveAmmo());

				CurrentWeapon->OnWeaponHitTarget.RemoveDynamic(this, &UCombatHUDWidget::ShowHitmarker);
				CurrentWeapon->OnWeaponHitTarget.AddDynamic(this, &UCombatHUDWidget::ShowHitmarker);
			}
		}
	}

	RunningTime += InDeltaTime;

	// 1. Progressive Damage & Blood Vignette Smoothing
	CurrentDamageOpacity = FMath::FInterpTo(CurrentDamageOpacity, TargetDamageOpacity, InDeltaTime, DamageFadeSpeed);

	float DisplayOpacity = CurrentDamageOpacity;

	// Critical health heartbeat throbbing (< 30% HP)
	if (CurrentDamageOpacity >= 0.7f)
	{
		const float HeartbeatPulse = 0.8f + (0.2f * FMath::Sin(RunningTime * 10.0f));
		DisplayOpacity = FMath::Clamp(CurrentDamageOpacity * HeartbeatPulse, 0.0f, 1.0f);
	}

	if (DamageVignetteImage)
	{
		FLinearColor Tint = DamageVignetteColor;
		Tint.A = DisplayOpacity;
		DamageVignetteImage->SetColorAndOpacity(Tint);
	}

	if (BloodSplatterImage)
	{
		FLinearColor Tint = BloodSplatterColor;
		// Blood splatters kick in strongly below 60% HP
		const float BloodAlpha = FMath::Clamp((DisplayOpacity - 0.4f) / 0.6f, 0.0f, 1.0f);
		Tint.A = BloodAlpha;
		BloodSplatterImage->SetColorAndOpacity(Tint);
	}

	// 2. Hitmarker Fadeout
	if (HitmarkerTimer > 0.0f)
	{
		HitmarkerTimer -= InDeltaTime;
		if (HitmarkerTimer <= 0.0f && HitmarkerImage)
		{
			HitmarkerImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 3. Points Flyout Fadeout
	if (PointsFlyoutTimer > 0.0f)
	{
		PointsFlyoutTimer -= InDeltaTime;
		if (PointsFlyoutTimer <= 0.0f)
		{
			CurrentFlyoutAccumulatedPoints = 0;
			if (PointsFlyoutText)
			{
				PointsFlyoutText->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	// 4. Power-Up Active Timers Tick (Driven authoritatively by subsystem timers)
	if (bIsInstaKillActive)
	{
		if (const UWorld* World = GetWorld())
		{
			if (AZombieGameModeBase* GM = Cast<AZombieGameModeBase>(World->GetAuthGameMode()))
			{
				InstaKillTimeRemaining = GM->GetInstaKillTimeRemaining();
				InstaKillDuration = GM->GetInstaKillDuration();
			}
			else
			{
				InstaKillTimeRemaining = FMath::Max(0.0f, InstaKillTimeRemaining - InDeltaTime);
			}
		}

		const float Percent = InstaKillDuration > 0.0f ? (InstaKillTimeRemaining / InstaKillDuration) : 0.0f;
		const FText TimeText = FText::FromString(FString::Printf(TEXT("%.1fs"), InstaKillTimeRemaining));

		if (InstaKillTimerBar)
		{
			InstaKillTimerBar->SetPercent(Percent);
		}
		if (InstaKillTimerText)
		{
			InstaKillTimerText->SetText(TimeText);
		}

		if (!bIsDoublePointsActive)
		{
			if (PowerUpTimerBar)
			{
				PowerUpTimerBar->SetPercent(Percent);
			}
			if (PowerUpTimerText)
			{
				PowerUpTimerText->SetText(TimeText);
			}
		}

		if (InstaKillTimeRemaining <= 0.0f)
		{
			bIsInstaKillActive = false;
			UpdatePowerUpUIState();
		}
	}

	if (bIsDoublePointsActive)
	{
		if (CachedPlayer.IsValid())
		{
			if (APlayerStateBase* PS = CachedPlayer->GetPlayerState<APlayerStateBase>())
			{
				DoublePointsTimeRemaining = PS->GetDoublePointsTimeRemaining();
				DoublePointsDuration = PS->GetDoublePointsDuration();
			}
			else
			{
				DoublePointsTimeRemaining = FMath::Max(0.0f, DoublePointsTimeRemaining - InDeltaTime);
			}
		}

		const float Percent = DoublePointsDuration > 0.0f ? (DoublePointsTimeRemaining / DoublePointsDuration) : 0.0f;
		const FText TimeText = FText::FromString(FString::Printf(TEXT("%.1fs"), DoublePointsTimeRemaining));

		if (DoublePointsTimerBar)
		{
			DoublePointsTimerBar->SetPercent(Percent);
		}
		if (DoublePointsTimerText)
		{
			DoublePointsTimerText->SetText(TimeText);
		}

		if (!bIsInstaKillActive)
		{
			if (PowerUpTimerBar)
			{
				PowerUpTimerBar->SetPercent(Percent);
			}
			if (PowerUpTimerText)
			{
				PowerUpTimerText->SetText(TimeText);
			}
		}

		if (DoublePointsTimeRemaining <= 0.0f)
		{
			bIsDoublePointsActive = false;
			UpdatePowerUpUIState();
		}
	}
}

void UCombatHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth, float HealthDelta, const FZombieDamageData& DamageData)
{
	if (MaxHealth > 0.0f)
	{
		const float HealthRatio = CurrentHealth / MaxHealth;
		TargetDamageOpacity = FMath::Clamp(1.0f - HealthRatio, 0.0f, 1.0f);
	}
}

void UCombatHUDWidget::HandleAmmoChanged(int32 CurrentMag, int32 CurrentReserve)
{
	if (CurrentMagText)
	{
		CurrentMagText->SetText(FText::AsNumber(CurrentMag));
	}

	if (ReserveAmmoText)
	{
		ReserveAmmoText->SetText(FText::AsNumber(CurrentReserve));
	}

	if (LowAmmoWarningText)
	{
		const bool bLowAmmo = (CurrentMag <= 5);
		LowAmmoWarningText->SetVisibility(bLowAmmo ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UCombatHUDWidget::HandleAimStateChanged(bool bIsAiming)
{
	if (CrosshairContainer)
	{
		// Tighten crosshair visibility or scale during precision ADS
		CrosshairContainer->SetRenderScale(bIsAiming ? FVector2D(0.4f, 0.4f) : FVector2D(1.0f, 1.0f));
	}

	if (ReticleImage)
	{
		// Scale and highlight central reticle dot when aiming
		ReticleImage->SetRenderScale(bIsAiming ? FVector2D(0.6f, 0.6f) : FVector2D(1.0f, 1.0f));
		FLinearColor Color = ReticleImage->GetColorAndOpacity();
		Color.A = bIsAiming ? 1.0f : 0.65f;
		ReticleImage->SetColorAndOpacity(Color);
	}
}

void UCombatHUDWidget::HandlePointsChanged(int32 NewPoints, int32 DeltaPoints)
{
	if (CurrentPointsText)
	{
		CurrentPointsText->SetText(FText::AsNumber(NewPoints));
	}

	if (DeltaPoints != 0 && PointsFlyoutText)
	{
		if (DeltaPoints < 0)
		{
			// Point deduction (e.g. door or weapon purchase)
			CurrentFlyoutAccumulatedPoints = 0;
			PointsFlyoutText->SetText(FText::FromString(FString::Printf(TEXT("%d"), DeltaPoints)));
			PointsFlyoutText->SetColorAndOpacity(FSlateColor(DeductionFlyoutColor));
		}
		else
		{
			// Point gain: accumulate if active within the timer window, otherwise reset baseline
			if (bAccumulatePointsFlyout && PointsFlyoutTimer > 0.0f)
			{
				CurrentFlyoutAccumulatedPoints += DeltaPoints;
			}
			else
			{
				CurrentFlyoutAccumulatedPoints = DeltaPoints;
			}

			PointsFlyoutText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), CurrentFlyoutAccumulatedPoints)));

			// Critical/Headshot bonus threshold (e.g. 100+ points in single event) displays golden yellow
			const bool bIsCritical = (DeltaPoints >= 100);
			if (bIsCritical)
			{
				PointsFlyoutText->SetColorAndOpacity(FSlateColor(HeadshotFlyoutColor));
			}
			else if (PointsFlyoutTimer <= 0.0f)
			{
				PointsFlyoutText->SetColorAndOpacity(FSlateColor(PointsFlyoutColor));
			}
		}

		PointsFlyoutText->SetVisibility(ESlateVisibility::Visible);
		PointsFlyoutTimer = PointsFlyoutDuration;
	}
}

void UCombatHUDWidget::HandleWaveStarted(int32 WaveNumber)
{
	if (RoundNumberText)
	{
		RoundNumberText->SetText(FText::FromString(FString::Printf(TEXT("ROUND %d"), WaveNumber)));
	}
}

void UCombatHUDWidget::HandleWaveStateChanged(EWaveState NewState)
{
	if (RoundStatusText)
	{
		switch (NewState)
		{
		case EWaveState::Intermission:
			RoundStatusText->SetText(FText::FromString(TEXT("PREPARE...")));
			break;
		case EWaveState::WaveActive:
			RoundStatusText->SetText(FText::FromString(TEXT("SURVIVE")));
			break;
		case EWaveState::WaveCompleted:
			RoundStatusText->SetText(FText::FromString(TEXT("ROUND CLEARED!")));
			break;
		case EWaveState::GameOver:
			RoundStatusText->SetText(FText::FromString(TEXT("DEFEAT")));
			break;
		default:
			RoundStatusText->SetText(FText::GetEmpty());
			break;
		}
	}
}

void UCombatHUDWidget::HandleZombiesRemainingChanged(int32 RemainingCount, int32 TotalWaveCount)
{
	if (ZombiesRemainingText)
	{
		ZombiesRemainingText->SetText(FText::FromString(FString::Printf(TEXT("Enemies: %d"), RemainingCount)));
	}
}

void UCombatHUDWidget::HandleInteractableFound(TScriptInterface<IInteractableInterface> Interactable, const FText& Prompt, int32 Cost)
{
	if (InteractionPromptContainer)
	{
		InteractionPromptContainer->SetVisibility(ESlateVisibility::Visible);
	}

	if (InteractionPromptText)
	{
		InteractionPromptText->SetText(Prompt);
	}

	if (InteractionCostText)
	{
		if (Cost > 0)
		{
			InteractionCostText->SetText(FText::FromString(FString::Printf(TEXT("[Cost: %d Points]"), Cost)));
			InteractionCostText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			InteractionCostText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCombatHUDWidget::HandleInteractableLost()
{
	if (InteractionPromptContainer)
	{
		InteractionPromptContainer->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UCombatHUDWidget::HandleInstaKillChanged(bool bIsActive, float Duration)
{
	bIsInstaKillActive = bIsActive;
	if (bIsActive)
	{
		InstaKillDuration = Duration;
		InstaKillTimeRemaining = Duration;

		if (InstaKillTimerBar)
		{
			InstaKillTimerBar->SetPercent(1.0f);
		}
		if (InstaKillTimerText)
		{
			InstaKillTimerText->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Duration)));
		}
		if (!bIsDoublePointsActive)
		{
			if (PowerUpTimerBar)
			{
				PowerUpTimerBar->SetPercent(1.0f);
			}
			if (PowerUpTimerText)
			{
				PowerUpTimerText->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Duration)));
			}
		}
	}
	else
	{
		InstaKillTimeRemaining = 0.0f;
	}

	UpdatePowerUpUIState();
}

void UCombatHUDWidget::HandleDoublePointsChanged(bool bIsActive, float Duration)
{
	bIsDoublePointsActive = bIsActive;
	if (bIsActive)
	{
		DoublePointsDuration = Duration;
		DoublePointsTimeRemaining = Duration;

		if (DoublePointsTimerBar)
		{
			DoublePointsTimerBar->SetPercent(1.0f);
		}
		if (DoublePointsTimerText)
		{
			DoublePointsTimerText->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Duration)));
		}
		if (!bIsInstaKillActive)
		{
			if (PowerUpTimerBar)
			{
				PowerUpTimerBar->SetPercent(1.0f);
			}
			if (PowerUpTimerText)
			{
				PowerUpTimerText->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Duration)));
			}
		}
	}
	else
	{
		DoublePointsTimeRemaining = 0.0f;
	}

	UpdatePowerUpUIState();
}

void UCombatHUDWidget::UpdatePowerUpUIState()
{
	const bool bAnyActive = bIsInstaKillActive || bIsDoublePointsActive;

	if (PowerUpContainer)
	{
		PowerUpContainer->SetVisibility(bAnyActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (!bAnyActive)
	{
		if (InstaKillRow)
		{
			InstaKillRow->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (DoublePointsRow)
		{
			DoublePointsRow->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	// 1. Dedicated row visibility
	if (InstaKillRow)
	{
		InstaKillRow->SetVisibility(bIsInstaKillActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (DoublePointsRow)
	{
		DoublePointsRow->SetVisibility(bIsDoublePointsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 2. Header text and fallback single-row styling
	if (bIsInstaKillActive && bIsDoublePointsActive)
	{
		if (PowerUpNameText)
		{
			PowerUpNameText->SetText(FText::FromString(TEXT("INSTA-KILL  •  DOUBLE POINTS")));
			PowerUpNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.9f, 0.4f, 1.0f)));
		}
	}
	else if (bIsInstaKillActive)
	{
		if (PowerUpNameText)
		{
			PowerUpNameText->SetText(FText::FromString(TEXT("INSTA-KILL")));
			PowerUpNameText->SetColorAndOpacity(FSlateColor(InstaKillColor));
		}
		if (PowerUpTimerBar)
		{
			PowerUpTimerBar->SetFillColorAndOpacity(InstaKillColor);
		}
	}
	else if (bIsDoublePointsActive)
	{
		if (PowerUpNameText)
		{
			PowerUpNameText->SetText(FText::FromString(TEXT("DOUBLE POINTS")));
			PowerUpNameText->SetColorAndOpacity(FSlateColor(DoublePointsColor));
		}
		if (PowerUpTimerBar)
		{
			PowerUpTimerBar->SetFillColorAndOpacity(DoublePointsColor);
		}
	}
}

void UCombatHUDWidget::ShowHitmarker(bool bIsHeadshot)
{
	if (!bEnableHitmarkers)
	{
		return;
	}

	if (HitmarkerImage)
	{
		const FLinearColor HitColor = bIsHeadshot ? FLinearColor(1.0f, 0.1f, 0.1f, 1.0f) : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		HitmarkerImage->SetColorAndOpacity(HitColor);
		HitmarkerImage->SetVisibility(ESlateVisibility::Visible);
		HitmarkerTimer = HitmarkerDuration;
	}
}
