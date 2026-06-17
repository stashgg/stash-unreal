// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK Demo Project

using System.IO;
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
            "InputCore",
            "Stash"
        });

        // Sample-only: example keep-alive notification icons (StashUnrealSample/Android/res/drawable/)
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            AdditionalPropertiesForReceipt.Add(
                "AndroidPlugin",
                Path.Combine(ModuleDirectory, "StashUnreal5_UPL_Android.xml"));
        }
    }
}
