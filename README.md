# Stash SDK for Unreal Engine

> **Warning:**  
> This is a preview version currently under development. Features, APIs, and workflow may change, and the integration process or plugin behavior is subject to updates.
>  


Seamlessly integrate the Stash Pay native in-app purchase dialog with Unreal Engine using the MobileNativeCode plugin and the Stash Pay Native SDK.

You can either explore the included Unreal Engine 5.0+ sample project in this repository, or follow the "Setup in Clean Unreal Project" guide below to add Stash Pay integration to your own Unreal project.

**Engine Support**: UE 4.21+ and UE 5.0+

## Demo

[Watch iOS Demo Video](.github/video/video_ios.webm)

*Click to view: Stash Pay checkout integration running on iOS in Unreal Engine*

## Architecture

Integration uses a layered architecture to integrate native Stash Pay dialog functionality:

```
Unreal Engine (C++/Blueprints)
         ↓
MobileNativeCode Plugin (Wrapper)
         ↓
Stash Native SDK (iOS/Android)
```

**Components:**

1. **[MobileNativeCode](https://github.com/Sovahero/PluginMobileNativeCode)** - Base plugin providing native mobile functionality access to Stash SDK for Unreal Engine projects. Includes wrapper for Stash native SDK.
2. **[Stash Native SDK](https://github.com/stashgg/stash-native)** - Native iOS/Android implementation of the Stash Pay checkout dialog.

## Quick Setup (This Sample Project)

This sample project is UE 5.7. After cloning this repository, initialize the Git submodules:

```bash
git submodule update --init --recursive
```

This command will download the Stash Native SDK into the `Plugins/stash-native-main/` directory. Once complete, you can build for iOS or Android, provided you have a properly configured Unreal Engine iOS or Android development environment.

## Setup in Clean Unreal Project

Follow these steps to integrate Stash Pay into your own Unreal Engine (4.21+ / 5.0+) project:

### 1. Add MobileNativeCode Plugin

Copy the `Plugins/MobileNativeCode/` folder from this repository to your project's `Plugins/` directory (create it if needed). This includes all necessary wrappers for native Stash Pay SDK.

### 2. Add Stash Native SDK

Add the Stash Native SDK as a Git submodule / folder in your project:

```bash
cd YourProject
git submodule add https://github.com/stashgg/stash-native.git Plugins/stash-native-main
```

> **Keep the folder structure as shown in this sample project.**  
> The MobileNativeCode plugin wrappers are configured to call into the Stash SDK located at `Plugins/stash-native-main`. If you change the folder name or location, you will need to update wrapper references accordingly in your C++ and Blueprint integration code.


### 3. Enable Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins → Installed → Mobile → MobileNativeCode**
3. Enable the plugin and restart the editor

### 4. Verify Setup

1. Open the Level Blueprint
2. You should be able to call MobileNativeCode Blueprint functions
3. Package for Android or iOS to test native integration

## Requirements

- Unreal Engine 4.21+ or 5.0+
- Visual Studio (for Windows/Android development)
- Xcode with iOS SDK (for iOS development)
- Android SDK (for Android builds)

## Key Files

**Wrapper Implementation:**
- `Plugins/MobileNativeCode/Source/MobileNativeCode/Private/IOS/ObjC/StashPayCardWrapper.mm`
- `Plugins/MobileNativeCode/Source/MobileNativeCode/Private/Android/Java/` (Android wrappers)

**Native SDK:**
- `Plugins/stash-native-main/iOS/` - iOS implementation
- `Plugins/stash-native-main/Android/` - Android implementation

## Documentation

- [MobileNativeCode Plugin](https://github.com/Sovahero/PluginMobileNativeCode) - Base plugin documentation
- [Stash Native SDK](https://github.com/stashgg/stash-native) - Native SDK documentation
- [Stash Pay Docs](https://docs.stash.gg) - Official Stash Pay documentation

## Troubleshooting

If you encounter issues specifically with the **MobileNativeCode plugin** (e.g., plugin not loading, native calls failing, Android/iOS compilation errors), please refer to the [MobileNativeCode repository README](https://github.com/Sovahero/PluginMobileNativeCode) for detailed setup instructions and troubleshooting guidance.

## Support

For Stash Pay integration issues, contact: developers@stash.gg
