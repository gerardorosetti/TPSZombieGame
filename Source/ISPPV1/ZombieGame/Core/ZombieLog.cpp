// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Core/ZombieLog.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogZombie);

// Disabled by default for maximum runtime performance
bool FZombieDebugConfig::bEnableConsoleLogs = false;

static FAutoConsoleVariableRef CVarZombieDebugLogs(
	TEXT("zombie.DebugLogs"),
	FZombieDebugConfig::bEnableConsoleLogs,
	TEXT("Enables or disables gameplay debug console logs across the survival shooter framework.\n0 = Disabled (optimal performance)\n1 = Enabled (verbose diagnostics)"),
	ECVF_Default
);
