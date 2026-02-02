// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Build Configuration

using System.IO;
using UnrealBuildTool;

public class Stash : ModuleRules
{
	private string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "ThirdParty/")); }
	}

	public Stash(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });
		PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Engine",
			"Core",
			"CoreUObject",
		});

		LoadLib(Target);
	}

	public void LoadLib(ReadOnlyTargetRules Target)
	{
		// Android platform configuration
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Required for JNI
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Launch"
			});

			PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private", "Android") });

			// Android build configuration via UPL
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "Stash_UPL_Android.xml"));
		}

		// iOS platform configuration
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			// Required compiler definitions
			PublicDefinitions.Add("TARGET_TV_OS=0");
			PublicDefinitions.Add("BUCK=1");

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Launch"
			});

			// iOS build configuration via UPL
			AdditionalPropertiesForReceipt.Add("IOSPlugin", Path.Combine(ModuleDirectory, "Stash_UPL_iOS.xml"));

			PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private", "IOS") });

			// Required Apple frameworks for StashPay SDK
			PublicFrameworks.AddRange(new string[]
			{
				"UIKit",
				"Foundation",
				"CoreGraphics",
				"WebKit",
				"SafariServices",
				"QuartzCore",
			});

			// Add pre-built StashPay XCFramework
			string XCFrameworkPath = Path.Combine(ThirdPartyPath, "iOS", "StashPay.xcframework");
			if (Directory.Exists(XCFrameworkPath))
			{
				// For UE4.27+, use PublicAdditionalFrameworks with the ios-arm64 framework inside the xcframework
				string FrameworkPath = Path.Combine(XCFrameworkPath, "ios-arm64", "StashPay.framework");
				PublicAdditionalFrameworks.Add(new Framework("StashPay", FrameworkPath, null, true));
				
				// Add framework headers to include path for #import <StashPay/StashPayCard.h>
				string HeaderPath = Path.Combine(FrameworkPath, "Headers");
				PublicIncludePaths.Add(HeaderPath);
			}
		}
	}
}
