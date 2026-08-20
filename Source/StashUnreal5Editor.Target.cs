using UnrealBuildTool;
using System.Collections.Generic;

public class StashUnreal5EditorTarget : TargetRules
{
    public StashUnreal5EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.AddRange(new string[] { "StashUnreal5" });
    }
}
