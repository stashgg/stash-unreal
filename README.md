# Stash Unreal

Unreal Engine project integrating Stash Pay checkout functionality.

## Setup

After cloning this repository, initialize the Git submodules:

```bash
git submodule update --init --recursive
```

This will fetch the Stash Native SDK from `Plugins/stash-native-main/`.

## Requirements

- Unreal Engine
- iOS/Android development tools (for mobile builds)

## Project Structure

- `Source/` - C++ source code
- `Content/` - Unreal assets and blueprints
- `Plugins/stash-native-main/` - Stash Native SDK (Git submodule)
- `Config/` - Project configuration files

## Stash Integration

This project uses the [Stash Native SDK](https://github.com/stashgg/stash-native) for payment integration on iOS and Android platforms.
