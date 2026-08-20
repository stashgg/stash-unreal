# iOS app icon source art

Branded Stash sample-app icons, preserved from the pre-merge `main`.

These are **source art only** - nothing in the build consumes them today.
UE 5.7 builds the iOS app icon from
`Build/IOS/Resources/Assets.xcassets/AppIcon.appiconset/` (see
`IOSToolChain.cs`); the legacy `Build/IOS/Resources/Graphics/Icon*.png` path
these came from is only read for `LaunchScreenIOS.png` (`UEDeployIOS.cs`).

To actually ship them, generate an `AppIcon.appiconset` from `Icon1024.png`
under `Build/IOS/Resources/Assets.xcassets/`. Note that `Build/` is gitignored
on this branch, so that would need either `git add -f` or a narrow un-ignore
rule.
