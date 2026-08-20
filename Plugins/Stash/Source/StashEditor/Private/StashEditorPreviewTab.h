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

	/** ToolMenus owner name for the Window-menu entry; shared with the module's UnregisterOwner call. */
	static FName GetMenuOwnerName();

private:
	static TSharedRef<SDockTab> SpawnPreviewTab(const FSpawnTabArgs& Args);
};
