// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/ZombieGameModeBase.h"
#include "ZombieGame/Character/PlayerCharacter.h"

AZombieGameModeBase::AZombieGameModeBase()
{
	// Set default player pawn class to our C++ APlayerCharacter
	DefaultPawnClass = APlayerCharacter::StaticClass();
}
