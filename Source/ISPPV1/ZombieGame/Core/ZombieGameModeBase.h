// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameModeBase.generated.h"

class AZombieWaveManager;
class UCombatHUDWidget;
class APlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInstaKillStateChangedSignature, bool, bIsActive, float, Duration);

/**
 * Base Game Mode for the survival shooter game.
 * Orchestrates player lifecycle, match state, wave subsystem, and global power-up events.
 */
UCLASS()
class ISPPV1_API AZombieGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZombieGameModeBase();

protected:
	virtual void BeginPlay() override;

	/** Wave manager class to spawn if none exists in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|Waves")
	TSubclassOf<AZombieWaveManager> WaveManagerClass;

	/** Active wave manager instance orchestrating round generation. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|Waves")
	TObjectPtr<AZombieWaveManager> ActiveWaveManager;

	/** Combat HUD widget class instantiated and added to the player's viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zombie|UI")
	TSubclassOf<UCombatHUDWidget> HUDWidgetClass;

	/** Active HUD widget instance displayed in the viewport. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|UI")
	TObjectPtr<UCombatHUDWidget> ActiveHUDWidget;

	// ----------------------------------------------------------------------------------
	// Power-Up Match State (Insta-Kill)
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|PowerUps")
	bool bIsInstaKillActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Zombie|PowerUps")
	float InstaKillDuration = 30.0f;

	FTimerHandle InstaKillTimerHandle;

public:
	/** Broadcast when Insta-Kill is activated or expires. */
	UPROPERTY(BlueprintAssignable, Category="Zombie|PowerUps")
	FOnInstaKillStateChangedSignature OnInstaKillStateChanged;

	/** Returns the active wave manager instance. */
	UFUNCTION(BlueprintPure, Category="Zombie|Waves")
	AZombieWaveManager* GetWaveManager() const { return ActiveWaveManager; }

	/** Returns the active combat HUD widget. */
	UFUNCTION(BlueprintPure, Category="Zombie|UI")
	UCombatHUDWidget* GetActiveHUDWidget() const { return ActiveHUDWidget; }

	/** Activates global Insta-Kill buff across all weapons. */
	UFUNCTION(BlueprintCallable, Category="Zombie|PowerUps")
	void ActivateInstaKill(float Duration = 30.0f);

	/** Deactivates Insta-Kill buff. */
	UFUNCTION(BlueprintCallable, Category="Zombie|PowerUps")
	void DeactivateInstaKill();

	/** Returns true if Insta-Kill is currently active. */
	UFUNCTION(BlueprintPure, Category="Zombie|PowerUps")
	FORCEINLINE bool IsInstaKillActive() const { return bIsInstaKillActive; }

	/** Returns remaining seconds of Insta-Kill bonus (0.0 if inactive). */
	UFUNCTION(BlueprintPure, Category="Zombie|PowerUps")
	float GetInstaKillTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category="Zombie|PowerUps")
	FORCEINLINE float GetInstaKillDuration() const { return InstaKillDuration; }

	/** Detonates a nuke: instantly destroys all active living zombies and awards 400 pts bonus. */
	UFUNCTION(BlueprintCallable, Category="Zombie|PowerUps")
	void TriggerNuke(APlayerCharacter* InstigatorPlayer = nullptr);

	/** Handles player death to trigger the Game Over sequence. */
	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor, AActor* KillerActor);
};
