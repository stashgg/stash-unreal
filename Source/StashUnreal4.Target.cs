using UnrealBuildTool;
using System.Collections.Generic;

public class StashUnreal4Target : TargetRules
{
    public StashUnreal4Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        ExtraModuleNames.AddRange(new string[] { "StashUnreal4" });
    }
}
