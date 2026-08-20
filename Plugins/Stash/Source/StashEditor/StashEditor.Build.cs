// Copyright Stash. All Rights Reserved.
// Editor-only module: Blueprint graph UX for Stash (e.g. slider pins on modal config ratios).
// Keeps UnrealEd / GraphEditor dependencies out of the runtime Stash module.

using System.IO;
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
			"ToolMenus",
			"WorkspaceMenuStructure",
			"DeveloperSettings",
			"ImageWrapper",
			"HTTP",
		});

		// Editor preview uses CEF via the engine WebBrowser module when present.
		// UE 5.7+ ships WebBrowser under Engine/Source/Runtime (not always as a .uproject plugin).
		string WebBrowserModule = Path.Combine(EngineDirectory, "Source", "Runtime", "WebBrowser", "WebBrowser.Build.cs");
		string WebBrowserPlugin = Path.Combine(EngineDirectory, "Plugins", "Runtime", "WebBrowser", "WebBrowser.uplugin");
		if (File.Exists(WebBrowserModule) || File.Exists(WebBrowserPlugin))
		{
			PrivateDependencyModuleNames.Add("WebBrowser");
			PrivateDefinitions.Add("STASH_HAS_WEBBROWSER=1");
		}
		else
		{
			PrivateDefinitions.Add("STASH_HAS_WEBBROWSER=0");
		}
	}
}
