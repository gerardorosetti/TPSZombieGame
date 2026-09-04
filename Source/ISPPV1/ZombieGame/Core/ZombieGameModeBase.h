// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameModeBase.generated.h"

/**
 * Base Game Mode for the Zombie TPS game.
 * 
 * Sets the default player pawn to APlayerCharacter and manages
 * match state, rounds, and game over rules.
 */
UCLASS()
class ISPPV1_API AZombieGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZombieGameModeBase();
};
