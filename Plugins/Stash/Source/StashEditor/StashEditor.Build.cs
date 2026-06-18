// Copyright Stash. All Rights Reserved.
// Editor-only module: Blueprint graph UX for Stash (e.g. slider pins on modal config ratios).
// Keeps UnrealEd / GraphEditor dependencies out of the runtime Stash module.

using UnrealBuildTool;

public class StashEditor : ModuleRules
{
	public StashEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Stash",
			"UnrealEd",
			"BlueprintGraph",
			"GraphEditor",
			"Kismet",
			"Slate",
			"SlateCore",
		});
	}
}
