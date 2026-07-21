// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Build Configuration (Stash Native 2.3.0)
//
// Runtime module only — editor-only UX (Blueprint slider pins) lives in the sibling
// StashEditor module (see Plugins/Stash/Stash.uplugin).

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
		// Small module with split platform .cpp files; unity + adaptive git builds can omit unchanged subfolder sources.
		bUseUnity = false;

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
			// Required for JNI; ImageWrapper + RenderCore for checkout viewport capture (PNG)
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Launch",
				"ImageWrapper",
				"RHI",
				"RenderCore",
			});

			PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private", "Android") });

			// Android build configuration via UPL
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "Stash_UPL_Android.xml"));
		}

		// iOS platform configuration
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			// Preprocessor flags expected by the prebuilt StashNative xcframework consumer build.
			// Carried forward from the original Stash Unreal integration — not referenced in plugin
			// sources, but keep until an iOS device package build confirms they can be removed.
			// TARGET_TV_OS=0 — iOS/iPadOS only (this plugin does not target tvOS).
			// BUCK=1 — legacy native-SDK packaging flag; retained for xcframework compatibility.
			PublicDefinitions.Add("TARGET_TV_OS=0");
			PublicDefinitions.Add("BUCK=1");

			// StashNative ObjC bridge sources are written ARC-style; enable ARC for this module's ObjC/ObjC++.
			bEnableObjCAutomaticReferenceCounting = true;

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Launch"
			});

			// iOS build configuration via UPL
			AdditionalPropertiesForReceipt.Add("IOSPlugin", Path.Combine(ModuleDirectory, "Stash_UPL_iOS.xml"));

			PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private", "IOS") });

			// Required Apple frameworks for StashNative SDK
			PublicFrameworks.AddRange(new string[]
			{
				"UIKit",
				"Foundation",
				"CoreGraphics",
				"WebKit",
				"SafariServices",
				"QuartzCore",
			});

			// Pre-built StashNative XCFramework (device slice only).
			// The bundle also ships ios-arm64_x86_64-simulator, but UE iOS packaging here targets
			// physical devices. Wiring the simulator slice would require selecting that path when
			// Target.Platform is IOS and the build is for the Xcode simulator — not done today.
			string XCFrameworkPath = Path.Combine(ThirdPartyPath, "iOS", "StashNative.xcframework");
			if (Directory.Exists(XCFrameworkPath))
			{
				string FrameworkPath = Path.Combine(XCFrameworkPath, "ios-arm64", "StashNative.framework");
				PublicAdditionalFrameworks.Add(new Framework("StashNative", FrameworkPath, null, true));

				string HeaderPath = Path.Combine(FrameworkPath, "Headers");
				PublicIncludePaths.Add(HeaderPath);
			}
		}
	}
}
