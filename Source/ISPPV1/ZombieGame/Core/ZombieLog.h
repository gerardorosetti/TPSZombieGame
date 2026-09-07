// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

ISPPV1_API DECLARE_LOG_CATEGORY_EXTERN(LogZombie, Log, All);

/**
 * Centralized debug configuration for gameplay logging.
 * Controls verbose console output across all combat, AI, wave, and economy subsystems.
 */
struct ISPPV1_API FZombieDebugConfig
{
	/** Global runtime toggle for gameplay console logging. Defaults to false for maximum performance. */
	static bool bEnableConsoleLogs;
};

/**
 * High-performance logging macro that skips string evaluation and console output when debug logs are disabled.
 */
#define ZOMBIE_LOG(Verbosity, Format, ...) \
	do { \
		if (FZombieDebugConfig::bEnableConsoleLogs) \
		{ \
			UE_LOG(LogZombie, Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while (0)
