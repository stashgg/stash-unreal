// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MobileNativeCode : ModuleRules
{
	public MobileNativeCode(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
		);
				
		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects"
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "Launch" });
			// UPL file is in the plugin root directory, not in Source/MobileNativeCode
			string PluginRoot = System.IO.Path.GetDirectoryName(ModuleDirectory); // Go up from Source/MobileNativeCode to Source
			PluginRoot = System.IO.Path.GetDirectoryName(PluginRoot); // Go up from Source to plugin root
			string PluginPath = Utils.MakePathRelativeTo(PluginRoot, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "MobileNativeCode_UPL_Android.xml"));
		}
		
		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			// WebKit and SafariServices are system frameworks - automatically linked when imported
			// stash-native iOS SDK is referenced externally (not copied into plugin)
			// stash-native includes built-in Unreal compatibility (__has_feature(objc_arc) checks)
			
			// Get plugin root directory
			string PluginRoot = System.IO.Path.GetDirectoryName(ModuleDirectory); // Go up from Source/MobileNativeCode to Source
			PluginRoot = System.IO.Path.GetDirectoryName(PluginRoot); // Go up from Source to plugin root
			
			// Path to stash-native iOS SDK (external package)
			string StashNativeRoot = System.IO.Path.Combine(PluginRoot, "../stash-native-main/iOS/StashPay");
			string StashNativeInclude = System.IO.Path.Combine(StashNativeRoot, "Sources/StashPay/include");
			string StashNativeSource = System.IO.Path.Combine(StashNativeRoot, "Sources/StashPay");
			string StashPayCardMM = System.IO.Path.Combine(StashNativeSource, "StashPayCard.mm");
			
			// Add stash-native headers to include path
			PublicIncludePaths.Add(StashNativeInclude);
			
			PrivateIncludePaths.AddRange(
				new string[] {
					"Private/IOS/ObjC",
					StashNativeSource  // For compiling StashPayCard.mm
				}
			);
			
			// Add iOS frameworks needed for StashPay
			PublicFrameworks.AddRange(
				new string[]
				{
					"SafariServices", // For SFSafariViewController
					"WebKit"          // For WKWebView
				}
			);
			
			// Don't enable ARC at module level (conflicts with PCH)
			bEnableObjCAutomaticReferenceCounting = false;
			
			// UPL file is in the plugin root directory
			string PluginPath = Utils.MakePathRelativeTo(PluginRoot, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("IOSPlugin", System.IO.Path.Combine(PluginPath, "MobileNativeCode_UPL_iOS.xml"));
		}
	}
}
