using UnrealBuildTool;
using System.Collections.Generic;

public class StashUnreal4EditorTarget : TargetRules
{
    public StashUnreal4EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        ExtraModuleNames.AddRange(new string[] { "StashUnreal4" });
    }
}
