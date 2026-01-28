// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine 4 SDK Demo Project

using UnrealBuildTool;

public class StashUnreal4 : ModuleRules
{
    public StashUnreal4(ReadOnlyTargetRules Target) : base(Target)
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
