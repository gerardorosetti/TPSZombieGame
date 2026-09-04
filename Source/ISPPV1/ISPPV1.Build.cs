// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ISPPV1 : ModuleRules
{
	public ISPPV1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ISPPV1",
			"ISPPV1/Zombies",
			"ISPPV1/Zombies/AI",
			"ISPPV1/Zombies/Character",
			"ISPPV1/Zombies/Combat",
			"ISPPV1/Zombies/Core",
			"ISPPV1/Zombies/Gameplay",
			"ISPPV1/Zombies/Interfaces",
			"ISPPV1/Zombies/UI",
			"ISPPV1/Variant_Combat",
			"ISPPV1/Variant_Combat/AI",
			"ISPPV1/Variant_Combat/Animation",
			"ISPPV1/Variant_Combat/Gameplay",
			"ISPPV1/Variant_Combat/Interfaces",
			"ISPPV1/Variant_Combat/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
