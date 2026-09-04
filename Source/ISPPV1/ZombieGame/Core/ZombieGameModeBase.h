// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameModeBase.generated.h"

class AZombieWaveManager;

/**
 * Base Game Mode for the Zombie Survival TPS game.
 * 
 * Pedagogical Architecture:
 * - Central GameMode orchestrating player lifecycle, match state, and wave subsystem.
 * - Configures default classes: APlayerCharacter as DefaultPawnClass, AZombiePlayerState as PlayerStateClass.
 * - Manages the AZombieWaveManager lifecycle and binds player death to GameOver.
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

public:
	/** Returns the active wave manager instance. */
	UFUNCTION(BlueprintPure, Category="Zombie|Waves")
	AZombieWaveManager* GetWaveManager() const { return ActiveWaveManager; }

	/** Handles player death to trigger the Game Over sequence. */
	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor, AActor* KillerActor);
};
