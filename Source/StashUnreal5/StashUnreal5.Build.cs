// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine 4 SDK Demo Project

using UnrealBuildTool;

public class StashUnreal5 : ModuleRules
{
    public StashUnreal5(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore"
        });
    }
}
