// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Docking/SDockTab.h"

class SStashPreviewPanel;

class FStashEditorPreviewTab
{
public:
	static void RegisterTabSpawner();
	static void UnregisterTabSpawner();
	static void RegisterMenuEntry();
	static void OpenPreviewTab();

private:
	static TSharedRef<SDockTab> SpawnPreviewTab(const FSpawnTabArgs& Args);
};
