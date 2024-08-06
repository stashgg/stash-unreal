# Stash Plugin for Unreal Engine

## About

This plugin enables seamless integration of Stash services into your Unreal Engine projects. The package is wrapping Stash API endpoints and is very lightweight with no external dependencies. 

To start with the SDK, you need to import the package from the releases tab and follow [getting started guide](https://docs.stash.gg/docs/configure-unity-project).

## Usage

To interact with Stash API and handle responses, the SDK offers the `StashAuth`, `StashLauncher`, and other classes, named according to the product you plan to use.
All classes are static, with no inheritance to the MonoBehaviour, no need to place them in the scene, and can be called only when needed.

### Import package manually

Download the latest release of the UnrealPlugin and SDK (make sure to use the .zip link)
1. Open or create a new project.
2. Create a Plugins folder in your project root folder (if one doesn't already exist).
3. Drag the unzipped Stash plugin into the project's Plugins folder
4. The plugin should be enabled and ready to use. If not, enable it.
5. Use our Unreal Examples to get started.

## Changelog

Package follows Semantic Versioning `(major.minor.patch)`. Any potential breaking changes will always cause a major version increment, non-breaking new features will cause a minor version increment, and bugfixes will cause a patch version increment.
A full version changelog is available in the [changelog](/CHANGELOG.md) file.

## Feedback and troubleshooting

If you run into any problems or have a feature request, open up a [new issue](https://github.com/stashgg/stash-unity/issues/new) in the repository. Please follow the issue/request template.
