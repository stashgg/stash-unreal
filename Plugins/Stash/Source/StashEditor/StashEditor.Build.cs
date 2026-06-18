// Copyright Stash. All Rights Reserved.

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
